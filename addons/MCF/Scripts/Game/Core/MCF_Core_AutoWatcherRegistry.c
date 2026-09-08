//! Registry that MCF_Obj_ProximityTriggerComponent and
//! MCF_Obj_SpottedByPlayerComponent register themselves into on init, so
//! MCF_Core_GameModeComponent can automatically register every newly
//! spawned controllable entity as a watcher/watched-target on all of
//! them, instead of a mission maker having to wire that up by hand.
//!
//! Registers ALL controllable entities (per the confirmed native
//! BaseGameMode.OnControllableSpawned behavior), not only actual players --
//! AI-controlled entities pass through the same hook. This means AI can
//! currently also count as a "watcher"/proximity source, which is a
//! known simplification, not a confirmed player-only filter.

class MCF_Core_AutoWatcherRegistry
{
	private static ref MCF_Core_AutoWatcherRegistry s_Instance;

	protected ref array<MCF_Obj_ProximityTriggerComponent> m_aProximityTriggers;
	protected ref array<MCF_Obj_SpottedByPlayerComponent> m_aSpottedTriggers;

	void MCF_Core_AutoWatcherRegistry()
	{
		m_aProximityTriggers = new array<MCF_Obj_ProximityTriggerComponent>();
		m_aSpottedTriggers = new array<MCF_Obj_SpottedByPlayerComponent>();
	}

	static MCF_Core_AutoWatcherRegistry GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_AutoWatcherRegistry();
		return s_Instance;
	}

	void RegisterProximityTrigger(MCF_Obj_ProximityTriggerComponent trigger)
	{
		if (trigger && m_aProximityTriggers.Find(trigger) == -1)
			m_aProximityTriggers.Insert(trigger);
	}

	void UnregisterProximityTrigger(MCF_Obj_ProximityTriggerComponent trigger)
	{
		int index = m_aProximityTriggers.Find(trigger);
		if (index != -1)
			m_aProximityTriggers.Remove(index);
	}

	void RegisterSpottedTrigger(MCF_Obj_SpottedByPlayerComponent trigger)
	{
		if (trigger && m_aSpottedTriggers.Find(trigger) == -1)
			m_aSpottedTriggers.Insert(trigger);
	}

	void UnregisterSpottedTrigger(MCF_Obj_SpottedByPlayerComponent trigger)
	{
		int index = m_aSpottedTriggers.Find(trigger);
		if (index != -1)
			m_aSpottedTriggers.Remove(index);
	}

	//! Call from MCF_Core_GameModeComponent.OnControllableSpawned() with
	//! the newly spawned entity. Registers it as a watcher/watched-target
	//! on every currently active trigger of both kinds.
	void NotifyNewControllable(IEntity entity)
	{
		if (!entity)
			return;

		foreach (MCF_Obj_ProximityTriggerComponent proximityTrigger : m_aProximityTriggers)
		{
			if (proximityTrigger)
				proximityTrigger.RegisterWatchedEntity(entity);
		}

		foreach (MCF_Obj_SpottedByPlayerComponent spottedTrigger : m_aSpottedTriggers)
		{
			if (spottedTrigger)
				spottedTrigger.RegisterWatcher(entity);
		}
	}
}
