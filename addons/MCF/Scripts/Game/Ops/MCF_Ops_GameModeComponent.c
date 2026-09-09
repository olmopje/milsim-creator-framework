//! The operations module's end of the game mode: the task store, the intel
//! store, and getting each player exactly the taskings and intel they are
//! entitled to see.
//!
//! WHY THIS IS A SEPARATE COMPONENT. All of this lived in
//! MCF_Core_GameModeComponent until 2026-09-10, which meant Core named
//! MCF_Task_Store, MCF_Intel_Store, MCF_Task and MCF_Intel_Record --
//! a hard dependency from the layer that must be installable alone onto a
//! module that is meant to be optional. It now listens for Core's lifecycle
//! events instead and Core knows nothing about it.
//!
//! Put this on the game mode entity alongside MCF_Core_GameModeComponent. If
//! the Ops module is not installed the entry is dropped at prefab load with
//! one error line and everything else on the game mode still works.
//!
//! ORDERING, THE PART THAT BIT US. MCF_Core_PlayerRegistered can arrive before
//! MCF_Core_PersistentStoreReady. On a listen server the host's own player
//! registers first -- observed at 16:20:29.666 against a game mode start of
//! 16:20:29.763 -- so this is the normal case, not an occasional race. Hence
//! m_bTaskStoreReady: registrations before the store is up are deferred, and
//! everyone already connected is caught up once it is. Verified: the host
//! receives its task instead of an empty list.

[ComponentEditorProps(category: "MCF/Ops", description: "Loads the task and intel stores at mission start and keeps every player's board up to date with what they are entitled to see.")]
class MCF_Ops_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_Ops_GameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Development aid: if the task store is empty at mission start, create a set of sample tasks that exercise every visibility rule. There is no way to author a task in game yet, so this exists to test the task system. Turn off for anything real.")]
	protected bool m_bCreateSampleTask;

	[Attribute(defvalue: "US", uiwidget: UIWidgets.EditBox, desc: "Faction key the faction-scoped sample task is addressed to. Only used when sample tasks are enabled.")]
	protected string m_sSampleFactionKey;

	//! False until the task store has been loaded on this server. While false,
	//! a player registration sends nothing -- there is nothing to send yet, and
	//! sending an empty list is how the host ended up with no tasks at all.
	protected bool m_bTaskStoreReady;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		MCF_Core_EventManager events = MCF_Core_EventManager.GetInstance();
		events.GetInvoker(MCF_Core_GameModeComponent.EVENT_STORE_READY).Insert(OnPersistentStoreReady);
		events.GetInvoker(MCF_Core_GameModeComponent.EVENT_PLAYER_REGISTERED).Insert(OnCorePlayerRegistered);
		events.GetInvoker(MCF_Core_GameModeComponent.EVENT_FACTION_CHANGED).Insert(OnCoreFactionChanged);

		MCF_Core_ValidationRegistry registry = MCF_Core_ValidationRegistry.GetInstance();
		registry.RegisterConsumer(MCF_Core_GameModeComponent.EVENT_STORE_READY, "MCF_Ops_GameModeComponent (load the task and intel stores)");
		registry.RegisterConsumer(MCF_Core_GameModeComponent.EVENT_PLAYER_REGISTERED, "MCF_Ops_GameModeComponent (send a joining player their board)");
		registry.RegisterConsumer(MCF_Core_GameModeComponent.EVENT_FACTION_CHANGED, "MCF_Ops_GameModeComponent (re-send a player's board after they pick a side)");
	}

	override void OnDelete(IEntity owner)
	{
		MCF_Core_EventManager events = MCF_Core_EventManager.GetInstance();
		events.GetInvoker(MCF_Core_GameModeComponent.EVENT_STORE_READY).Remove(OnPersistentStoreReady);
		events.GetInvoker(MCF_Core_GameModeComponent.EVENT_PLAYER_REGISTERED).Remove(OnCorePlayerRegistered);
		events.GetInvoker(MCF_Core_GameModeComponent.EVENT_FACTION_CHANGED).Remove(OnCoreFactionChanged);

		super.OnDelete(owner);
	}

	//! Server only -- Core publishes this inside its own IsServer guard.
	protected void OnPersistentStoreReady(Managed payload)
	{
		m_bTaskStoreReady = false;

		MCF_Task_Store.GetInstance().Load();
		MCF_Intel_Store.GetInstance().Load();

		if (m_bCreateSampleTask)
			CreateSampleTasks();

		// The store is only now safe to read from. Anyone who registered
		// before this point was skipped and is caught up here.
		m_bTaskStoreReady = true;
		SendTasksToConnectedPlayers();
	}

	protected void OnCorePlayerRegistered(Managed payload)
	{
		MCF_Core_PlayerPayload player = MCF_Core_PlayerPayload.Cast(payload);
		if (!player)
			return;

		// May arrive before the store is up on a listen server. Deferred to the
		// catch-up pass at the end of OnPersistentStoreReady.
		if (!m_bTaskStoreReady)
		{
			MCF_Core_Log.Debug("player " + player.m_iPlayerId.ToString() + " registered before the task store was ready -- deferring their tasks");
			return;
		}

		SendTasksToPlayer(player.m_iPlayerId);
	}

	protected void OnCoreFactionChanged(Managed payload)
	{
		MCF_Core_PlayerPayload player = MCF_Core_PlayerPayload.Cast(payload);
		if (!player)
			return;

		if (!m_bTaskStoreReady)
			return;

		MCF_Core_Log.Debug("resending tasks to player " + player.m_iPlayerId.ToString() + " after a faction change");
		SendTasksToPlayer(player.m_iPlayerId);
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

	//! Development aid only. Nothing in game can author a task yet, so without
	//! this there is no way to see the task system do anything.
	//!
	//! The four tasks are a visibility fixture, one per rule in
	//! MCF_Task_Store.IsVisibleTo, so that a Peer Tool session with three
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
		MCF_Task_Store taskStore = MCF_Task_Store.GetInstance();
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
	//! The filtering happens on the server (MCF_Task_Store.IsVisibleTo) so
	//! that a client is never sent a plan it has no business holding.
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

		string factionKey = MCF_Core_FactionHelper.GetPlayerFactionKey(playerId);

		SendIntelToPlayer(controller, playerId, factionKey);

		array<MCF_Task> visible = {};
		MCF_Task_Store.GetInstance().GetTasksVisibleTo(playerId, factionKey, visible);

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
		MCF_Intel_Store.GetInstance().GetVisibleTo(playerId, factionKey, visible);

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
}
