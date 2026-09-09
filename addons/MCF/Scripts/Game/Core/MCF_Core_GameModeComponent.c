//! MCF's own game-mode-extending component. Add this to your GameMode
//! entity to have MCF start itself at mission start instead of needing a
//! manual kickoff call.
//!
//! Extends SCR_BaseGameModeComponent, which every vanilla game mode
//! (Conflict, Combat Ops, etc.) already supports adding components to,
//! confirmed via OnGameModeStart() -- "Called on every machine when game
//! mode starts."
//!
//! Also overrides the confirmed native OnControllableSpawned(IEntity) hook
//! to auto-register every newly spawned controllable entity with
//! MCF_Core_AutoWatcherRegistry, so the detection triggers work without a
//! mission maker having to register watchers by hand. Note this fires for
//! AI-controlled entities too, not only players.
//!
//! Two things to know about OnGameModeStart:
//!
//!   1. It fires on EVERY machine, clients included. Anything that is server
//!      state must be guarded with Replication.IsServer().
//!   2. It runs after every entity has finished EOnInit, which is why the
//!      event validation pass belongs here -- by then every node has declared
//!      what it publishes and what it listens for.
//!
//! MCF's static managers survive the World Editor -> play mode transition, so
//! anything holding per-mission state is reset here. A stuck line queue once
//! poisoned a whole play session exactly that way.
//!
//! Ordering: OnPlayerRegistered is NOT guaranteed to fire after
//! OnGameModeStart. On a listen server the host's own player registers first
//! -- observed at 16:20:29.666 against a game mode start of 16:20:29.763 --
//! so this is the normal case, not an occasional race. Hence
//! m_bTaskStoreReady below: registrations before the store is up are deferred,
//! and everyone already connected is caught up once it is. Verified: the host
//! now receives its task instead of an empty list.

[ComponentEditorProps(category: "MCF/Core", description: "Starts MCF at mission start: resets per-mission state, loads persistent data, validates event wiring, and auto-registers spawned entities with the detection triggers.")]
class MCF_Core_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_Core_GameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Start the AAR/Debrief manager listening automatically.")]
	protected bool m_bEnableAAR;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enable automatic Hostility decay.")]
	protected bool m_bEnableHostilityDecay;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Hostility decay rate per second, if enabled above.")]
	protected float m_fHostilityDecayRate;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Development aid: if the task store is empty at mission start, create a set of sample tasks that exercise every visibility rule. There is no way to author a task in game yet, so this exists to test the task system. Turn off for anything real.")]
	protected bool m_bCreateSampleTask;

	[Attribute(defvalue: "US", uiwidget: UIWidgets.EditBox, desc: "Faction key the faction-scoped sample task is addressed to. Only used when sample tasks are enabled.")]
	protected string m_sSampleFactionKey;

	//! False until the task store has been loaded on this server. While false,
	//! OnPlayerRegistered sends nothing -- there is nothing to send yet, and
	//! sending an empty list is how the host ended up with no tasks at all.
	protected bool m_bTaskStoreReady;

	override void OnGameModeStart()
	{
		MCF_Core_Log.Debug("GameMode start -- resetting per-mission state");

		// Static singletons outlive the editor session; clear anything that
		// holds per-mission state before the mission actually begins.
		MCF_Voice_LineQueueManager.GetInstance().Reset();
		m_bTaskStoreReady = false;

		// Server state. Without this guard every client loads and rewrites the
		// store in its own profile. Confirmed on a Peer Tool session.
		if (Replication.IsServer())
			LoadPersistentState();

		if (m_bEnableAAR)
			MCF_AAR_DebriefManager.GetInstance().StartListening();

		if (m_bEnableHostilityDecay)
			MCF_Hostility_Manager.GetInstance().StartAutoDecay(m_fHostilityDecayRate);

		if (Replication.IsServer())
			SubscribeToFactionChanges();
		if (Replication.IsServer())
			RunEventValidation();
	}

	//! Reports every event some node listens for that nothing publishes --
	//! almost always a typo in an event name, which otherwise just means the
	//! downstream node silently never fires.
	protected void RunEventValidation()
	{
		int warnings = MCF_Core_ValidationRegistry.GetInstance().RunValidation();
		if (warnings == 0)
			MCF_Core_Log.Debug("Event validation passed -- every consumed event has a publisher");
		else
			MCF_Core_Log.Warn("Event validation found " + warnings.ToString() + " event name(s) nothing publishes -- see the warnings above");
	}

	//! Brings the cross-restart store up, then the tasks stored in it. The run
	//! counter is the proof that data written by one server session is
	//! readable by the next -- the thing the whole persistent-server design
	//! depends on.
	protected void LoadPersistentState()
	{
		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();
		store.Load();

		int previousRuns = store.GetInt("serverRunCount", 0);
		int thisRun = previousRuns + 1;

		store.SetInt("serverRunCount", thisRun);
		store.Save();

		if (previousRuns == 0)
			MCF_Core_Log.Debug("PersistentStore: first recorded server run");
		else
			MCF_Core_Log.Debug("PersistentStore: this server has started " + thisRun.ToString() + " times -- previous runs survived restart");

		MCF_Core_TaskStore.GetInstance().Load();
		MCF_Core_IntelStore.GetInstance().Load();

		// Conversations a Game Master wrote in an earlier session. Loaded here
		// beside the intel store because they are the same kind of thing: text
		// authored during play, held by the server, and expected to still be
		// there tomorrow.
		MCF_Dialogue_Library.GetInstance().LoadRuntime();

		if (m_bCreateSampleTask)
			CreateSampleTasks();

		// The store is only now safe to read from. Anyone who registered
		// before this point was skipped and is caught up here.
		m_bTaskStoreReady = true;
		SendTasksToConnectedPlayers();
	}

	//! Development aid only. Nothing in game can author a task yet, so without
	//! this there is no way to see the task system do anything.
	//!
	//! The four tasks are a visibility fixture, one per rule in
	//! MCF_Core_TaskStore.IsVisibleTo, so that a Peer Tool session with three
	//! players shows each rule doing something different rather than everyone
	//! receiving the same list:
	//!
	//!   sample-published  PUBLISHED, unassigned   -> every player
	//!   sample-draft      DRAFT, author = host    -> the host alone
	//!   sample-player     ASSIGNED to player 2    -> player 2 alone
	//!   sample-faction    ASSIGNED to a faction   -> that faction's players
	//!
	//! Author is left at 0 on the assigned two so that the assignee rule is
	//! what is being tested -- an author always sees their own task, which
	//! would mask the result. Player id 1 is the host on a listen server.
	protected void CreateSampleTasks()
	{
		MCF_Core_TaskStore taskStore = MCF_Core_TaskStore.GetInstance();
		if (taskStore.Count() > 0)
			return;

		MCF_Task published = taskStore.CreateTask("Recon the north approach", 0);
		if (published)
		{
			published.m_sSituation = "Enemy patrols reported along the ridge since first light.";
			published.m_sMission = "2nd squad confirms enemy strength before dawn.";
			published.m_sExecution = "Move by the treeline, observe, do not engage.";
			published.m_sCommandSignal = "Report on company net, callsign BRAVO.";
			taskStore.SetState(published.m_sId, MCF_ETaskState.PUBLISHED);
		}

		MCF_Task draft = taskStore.CreateTask("DRAFT: withdrawal contingency", 1);
		if (draft)
			draft.m_sSituation = "Not yet published -- only the author should see this.";

		MCF_Task forPlayer = taskStore.CreateTask("Personal: escort the medic", 0);
		if (forPlayer)
		{
			forPlayer.m_sMission = "Stay with the medic on the move to the casualty point.";
			taskStore.AssignTask(forPlayer.m_sId, MCF_ETaskAssignee.PLAYER, "2");
		}

		MCF_Task forFaction = taskStore.CreateTask("Faction: hold the crossing", 0);
		if (forFaction)
		{
			forFaction.m_sMission = "All friendly elements hold the river crossing until relieved.";
			taskStore.AssignTask(forFaction.m_sId, MCF_ETaskAssignee.FACTION, m_sSampleFactionKey);
		}
	}

	//! Server-only per the base class. Fires after the player's identity is
	//! known, which is why the task push happens here rather than in
	//! OnPlayerConnected.
	override void OnPlayerRegistered(int playerId)
	{
		if (!Replication.IsServer())
			return;

		// May fire before OnGameModeStart on a listen server. Deferred to the
		// catch-up pass at the end of LoadPersistentState.
		if (!m_bTaskStoreReady)
		{
			MCF_Core_Log.Debug("player " + playerId.ToString() + " registered before the task store was ready -- deferring their tasks");
			return;
		}

		SendTasksToPlayer(playerId);
	}

	//! Public entry point for anything that changed a task and needs every
	//! player's view of the board brought back up to date -- the accept and
	//! issue actions on the task board, for one.
	//!
	//! Deliberately re-sends each player their whole visible set rather than
	//! the one task that changed. A delta cannot express "you may no longer
	//! see this" without naming the task, which is exactly what the server is
	//! trying not to tell them.
	void RefreshTasksForAllPlayers()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bTaskStoreReady)
			return;

		SendTasksToConnectedPlayers();
	}


	//! A player's faction is not known when they register -- it is chosen at
	//! spawn, well afterwards. That means a faction-scoped task can never be
	//! delivered by the join-time push: at that moment GetPlayerFactionKey
	//! returns empty and IsVisibleTo rightly withholds it.
	//!
	//! This was invisible for a long time because a hosted server hides it.
	//! On a listen server the "client mirror" and the authoritative store are
	//! the same object, so the host's board found the faction task sitting
	//! there anyway and displayed it once the host picked a side. A real
	//! client, which only ever holds what the server sent it, would have been
	//! given nothing and shown nothing -- with no error anywhere.
	//!
	//! So: re-push when a player's faction changes. Server-only invoker,
	//! confirmed in SCR_FactionManager.GetOnPlayerFactionChanged_S.
	protected void SubscribeToFactionChanges()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
		{
			MCF_Core_Log.Warn("no SCR_FactionManager -- faction-scoped tasks will not be delivered on faction change");
			return;
		}

		factionManager.GetOnPlayerFactionChanged_S().Insert(OnPlayerFactionChanged);
	}

	//! Server side. Their entitlement just changed, so their board must too.
	protected void OnPlayerFactionChanged(int playerId, SCR_PlayerFactionAffiliationComponent playerComponent, Faction faction)
	{
		if (!m_bTaskStoreReady)
			return;

		string key = "none";
		if (faction)
			key = faction.GetFactionKey();

		MCF_Core_Log.Debug("player " + playerId.ToString() + " changed faction to " + key + " -- resending their tasks");
		SendTasksToPlayer(playerId);
	}


	//! Catch-up pass for players who were already connected when the task
	//! store came up. On a listen server this is always at least the host.
	protected void SendTasksToConnectedPlayers()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		if (playerIds.IsEmpty())
		{
			MCF_Core_Log.Debug("TaskStore ready -- no players connected yet");
			return;
		}

		MCF_Core_Log.Debug("TaskStore ready -- catching up " + playerIds.Count().ToString() + " already-connected player(s)");

		foreach (int playerId : playerIds)
			SendTasksToPlayer(playerId);
	}

	//! Sends a player every task they are entitled to see, and nothing else.
	//! The filtering happens on the server (MCF_Core_TaskStore.IsVisibleTo) so
	//! that a client is never sent a plan it has no business holding.
	//!
	//! Only fires on join today. Nothing authors tasks at runtime yet, so
	//! there are no mid-session changes to push; once there are, this needs a
	//! counterpart that re-sends on change. Faction-scoped tasks need that
	//! counterpart even sooner -- see GetPlayerFactionKey below.
	protected void SendTasksToPlayer(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		if (!controller)
		{
			MCF_Core_Log.Warn("no player controller for player " + playerId.ToString() + " -- cannot send tasks");
			return;
		}

		// Clear before sending. A resend can add and update, but it cannot
		// take a task away -- and tasks do get taken away: the moment one
		// player accepts a board task, everyone else loses sight of it.
		controller.MCF_ClearTasks();

		string factionKey = GetPlayerFactionKey(playerId);

		SendIntelToPlayer(controller, playerId, factionKey);

		array<MCF_Task> visible = {};
		MCF_Core_TaskStore.GetInstance().GetTasksVisibleTo(playerId, factionKey, visible);

		// Log which tasks, not just how many. A count alone cannot tell a
		// working filter from a broken one -- everyone getting "1 task" looks
		// identical whether the filtering ran or not.
		string ids = "";
		foreach (int i, MCF_Task task : visible)
		{
			controller.MCF_SendTask(task);

			if (i > 0)
				ids = ids + ", ";
			ids = ids + task.m_sId;
		}

		if (ids.IsEmpty())
			ids = "none";

		string faction = factionKey;
		if (faction.IsEmpty())
			faction = "no faction yet";

		MCF_Core_Log.Debug("sent " + visible.Count().ToString() + " task(s) to player " + playerId.ToString() + " (" + faction + "): " + ids);
	}

	//! Sends a player the intel they are entitled to see.
	//!
	//! Rides along with the task push rather than having its own trigger: the
	//! two always change for the same reasons -- someone joined, someone
	//! picked a faction, something was logged -- and two separate refresh
	//! paths would drift out of step the first time only one of them was
	//! called.
	protected void SendIntelToPlayer(notnull SCR_PlayerController controller, int playerId, string factionKey)
	{
		controller.MCF_ClearIntel();

		array<MCF_Intel_Record> visible = {};
		MCF_Core_IntelStore.GetInstance().GetVisibleTo(playerId, factionKey, visible);

		foreach (MCF_Intel_Record record : visible)
			controller.MCF_SendIntel(record);

		// Log the faction alongside the count for the same reason the task
		// push does: "2 records" reads identically whether the faction filter
		// ran or silently passed everything through.
		string intelFaction = factionKey;
		if (intelFaction.IsEmpty())
			intelFaction = "no faction yet";

		MCF_Core_Log.Debug("sent " + visible.Count().ToString() + " intel record(s) to player " + playerId.ToString() + " (" + intelFaction + ")");
	}


	//! \return The player's faction key, or empty if they have not picked one
	//! yet.
	//!
	//! At registration time this is normally empty: faction is chosen at
	//! spawn, which happens well after the player registers. A faction-scoped
	//! task therefore cannot be delivered on join, and needs a re-send once
	//! the player picks a side.
	protected string GetPlayerFactionKey(int playerId)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return string.Empty;

		Faction faction = factionManager.GetPlayerFaction(playerId);
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	override void OnControllableSpawned(IEntity entity)
	{
		// The watcher registry feeds server-side detection only.
		if (!Replication.IsServer())
			return;

		MCF_Core_AutoWatcherRegistry.GetInstance().NotifyNewControllable(entity);
	}
}
