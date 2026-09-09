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
//! WHAT THIS COMPONENT NO LONGER DOES, AND WHY. Until 2026-09-10 it also
//! booted the dialogue library, the hostility manager, the task store and the
//! intel store by name, and pushed tasks and intel to each player itself. That
//! made Core depend on three modules that are meant to be optional. It now
//! publishes four lifecycle events instead, and each module's own game-mode
//! component listens for the ones it cares about:
//!
//!   MCF_Core_PersistentStoreReady   server only, after $profile: is loaded.
//!                                   Where a module loads its own slice.
//!   MCF_Core_MissionStart           every machine, after the above.
//!   MCF_Core_PlayerRegistered       server only, payload MCF_Core_PlayerPayload.
//!   MCF_Core_PlayerFactionChanged   server only, same payload, faction filled in.
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
//! so this is the normal case, not an occasional race. A module that answers
//! MCF_Core_PlayerRegistered must therefore cope with being asked before its
//! own store is up, and catch those players up afterwards. MCF_Ops_GameModeComponent
//! is the worked example.

//! Payload for the two player lifecycle events above.
//!
//! m_sFactionKey is empty on MCF_Core_PlayerRegistered: faction is chosen at
//! spawn, well after a player registers. That is not a bug to work around, it
//! is why MCF_Core_PlayerFactionChanged exists.
class MCF_Core_PlayerPayload : Managed
{
	int m_iPlayerId;
	string m_sFactionKey;

	void MCF_Core_PlayerPayload(int playerId, string factionKey)
	{
		m_iPlayerId = playerId;
		m_sFactionKey = factionKey;
	}
}

[ComponentEditorProps(category: "MCF/Core", description: "Starts MCF at mission start: resets per-mission state, loads persistent data, validates event wiring, publishes the mission and player lifecycle events the modules listen for, and auto-registers spawned entities with the detection triggers.")]
class MCF_Core_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_Core_GameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Start the AAR/Debrief manager listening automatically.")]
	protected bool m_bEnableAAR;

	static const string EVENT_STORE_READY = "MCF_Core_PersistentStoreReady";
	static const string EVENT_MISSION_START = "MCF_Core_MissionStart";
	static const string EVENT_PLAYER_REGISTERED = "MCF_Core_PlayerRegistered";
	static const string EVENT_FACTION_CHANGED = "MCF_Core_PlayerFactionChanged";

	override void OnGameModeStart()
	{
		MCF_Core_Log.Debug("GameMode start -- resetting per-mission state");

		// Static singletons outlive the editor session; clear anything that
		// holds per-mission state before the mission actually begins.
		MCF_Voice_LineQueueManager.GetInstance().Reset();

		// Declared before validation runs, so that a module listening for one
		// of these is not reported as listening to an event nothing publishes.
		DeclarePublishedEvents();

		// Server state. Without this guard every client loads and rewrites the
		// store in its own profile. Confirmed on a Peer Tool session.
		if (Replication.IsServer())
			LoadPersistentState();

		if (m_bEnableAAR)
			MCF_AAR_DebriefManager.GetInstance().StartListening();

		MCF_Core_EventManager.GetInstance().Publish(EVENT_MISSION_START, null);

		if (Replication.IsServer())
			SubscribeToFactionChanges();
		if (Replication.IsServer())
			RunEventValidation();
	}

	protected void DeclarePublishedEvents()
	{
		MCF_Core_ValidationRegistry registry = MCF_Core_ValidationRegistry.GetInstance();
		registry.RegisterPublisher(EVENT_STORE_READY);
		registry.RegisterPublisher(EVENT_MISSION_START);
		registry.RegisterPublisher(EVENT_PLAYER_REGISTERED);
		registry.RegisterPublisher(EVENT_FACTION_CHANGED);
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

	//! Brings the cross-restart store up, then tells the modules it is safe to
	//! read from. The run counter is the proof that data written by one server
	//! session is readable by the next -- the thing the whole persistent-server
	//! design depends on.
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

		MCF_Core_EventManager.GetInstance().Publish(EVENT_STORE_READY, null);
	}

	//! Server-only per the base class. Fires after the player's identity is
	//! known, which is why the modules are told here rather than on connect.
	override void OnPlayerRegistered(int playerId)
	{
		if (!Replication.IsServer())
			return;

		MCF_Core_EventManager.GetInstance().Publish(EVENT_PLAYER_REGISTERED, new MCF_Core_PlayerPayload(playerId, string.Empty));
	}

	//! A player's faction is not known when they register -- it is chosen at
	//! spawn, well afterwards. That means anything faction-scoped can never be
	//! delivered by the join-time push: at that moment the player has no
	//! faction and a faction filter rightly withholds everything.
	//!
	//! This was invisible for a long time because a hosted server hides it.
	//! On a listen server the "client mirror" and the authoritative store are
	//! the same object, so the host's board found the faction task sitting
	//! there anyway and displayed it once the host picked a side. A real
	//! client, which only ever holds what the server sent it, would have been
	//! given nothing and shown nothing -- with no error anywhere.
	//!
	//! So: re-announce when a player's faction changes. Server-only invoker,
	//! confirmed in SCR_FactionManager.GetOnPlayerFactionChanged_S.
	protected void SubscribeToFactionChanges()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
		{
			MCF_Core_Log.Warn("no SCR_FactionManager -- faction-scoped content will not be re-sent on faction change");
			return;
		}

		factionManager.GetOnPlayerFactionChanged_S().Insert(OnPlayerFactionChanged);
	}

	//! Server side. Their entitlement just changed, so anything faction-scoped
	//! they hold must be reconsidered.
	protected void OnPlayerFactionChanged(int playerId, SCR_PlayerFactionAffiliationComponent playerComponent, Faction faction)
	{
		string key = string.Empty;
		if (faction)
			key = faction.GetFactionKey();

		string logged = key;
		if (logged.IsEmpty())
			logged = "none";

		MCF_Core_Log.Debug("player " + playerId.ToString() + " changed faction to " + logged);
		MCF_Core_EventManager.GetInstance().Publish(EVENT_FACTION_CHANGED, new MCF_Core_PlayerPayload(playerId, key));
	}

	override void OnControllableSpawned(IEntity entity)
	{
		// The watcher registry feeds server-side detection only.
		if (!Replication.IsServer())
			return;

		MCF_Core_AutoWatcherRegistry.GetInstance().NotifyNewControllable(entity);
	}
}
