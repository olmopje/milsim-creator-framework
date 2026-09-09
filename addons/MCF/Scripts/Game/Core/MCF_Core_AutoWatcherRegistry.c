//! Registry that the detection components register themselves into on init,
//! so MCF_Core_GameModeComponent can automatically hand every newly spawned
//! controllable entity to all of them, instead of a mission maker having to
//! wire that up by hand.
//!
//! Registers ALL controllable entities (per the confirmed native
//! BaseGameMode.OnControllableSpawned behavior), not only actual players --
//! AI-controlled entities pass through the same hook. This means AI can
//! currently also count as a "watcher"/proximity source, which is a
//! known simplification, not a confirmed player-only filter. Making that a
//! mission setting is noted in docs/research/multiplayer-and-audience.md.
//!
//! Cone Detection was missing here until 2026-09-09: it had a
//! RegisterWatchedEntity() API and ticked every frame, but nothing ever gave
//! it anything to watch, so it could never fire. It compiled, initialised and
//! logged nothing -- the same shape of silent defect as the m_Flags parse
//! failure and the missing INIT event mask. Any new detection component must
//! be added here as well, or it is dead on arrival.

class MCF_Core_AutoWatcherRegistry
{
	private static ref MCF_Core_AutoWatcherRegistry s_Instance;

	protected ref array<MCF_Obj_ProximityTriggerComponent> m_aProximityTriggers;
	protected ref array<MCF_Obj_SpottedByPlayerComponent> m_aSpottedTriggers;
	protected ref array<MCF_Obj_ConeDetectionTriggerComponent> m_aConeTriggers;

	void MCF_Core_AutoWatcherRegistry()
	{
		m_aProximityTriggers = new array<MCF_Obj_ProximityTriggerComponent>();
		m_aSpottedTriggers = new array<MCF_Obj_SpottedByPlayerComponent>();
		m_aConeTriggers = new array<MCF_Obj_ConeDetectionTriggerComponent>();
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

	void RegisterConeTrigger(MCF_Obj_ConeDetectionTriggerComponent trigger)
	{
		if (trigger && m_aConeTriggers.Find(trigger) == -1)
			m_aConeTriggers.Insert(trigger);
	}

	void UnregisterConeTrigger(MCF_Obj_ConeDetectionTriggerComponent trigger)
	{
		int index = m_aConeTriggers.Find(trigger);
		if (index != -1)
			m_aConeTriggers.Remove(index);
	}

	//! Call from MCF_Core_GameModeComponent.OnControllableSpawned() with
	//! the newly spawned entity. Registers it as a watcher/watched-target
	//! on every currently active trigger.
	void NotifyNewControllable(IEntity entity)
	{
		if (!entity)
			return;

		MCF_Core_Log.Debug("Controllable spawned -- notifying " + m_aProximityTriggers.Count().ToString() + " proximity, " + m_aConeTriggers.Count().ToString() + " cone and " + m_aSpottedTriggers.Count().ToString() + " spotted triggers");

		foreach (MCF_Obj_ProximityTriggerComponent proximityTrigger : m_aProximityTriggers)
		{
			if (proximityTrigger)
				proximityTrigger.RegisterWatchedEntity(entity);
		}

		foreach (MCF_Obj_ConeDetectionTriggerComponent coneTrigger : m_aConeTriggers)
		{
			if (coneTrigger)
				coneTrigger.RegisterWatchedEntity(entity);
		}

		foreach (MCF_Obj_SpottedByPlayerComponent spottedTrigger : m_aSpottedTriggers)
		{
			if (spottedTrigger)
				spottedTrigger.RegisterWatcher(entity);
		}
	}
}
