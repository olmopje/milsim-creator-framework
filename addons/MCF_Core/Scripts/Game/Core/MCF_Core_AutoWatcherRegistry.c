//! Registry that detection components register themselves into on init, so
//! MCF_Core_GameModeComponent can automatically hand every newly spawned
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
//! failure and the missing INIT event mask.
//!
//! Since 2026-09-10 the registry holds one array of
//! MCF_Core_ControllableWatcherComponent rather than one typed array per
//! detection component, so that Core no longer names classes belonging to the
//! Objectives module. Any new detection component must still extend that base
//! class AND register itself, or it is dead on arrival exactly as before.

class MCF_Core_AutoWatcherRegistry
{
	private static ref MCF_Core_AutoWatcherRegistry s_Instance;

	protected ref array<MCF_Core_ControllableWatcherComponent> m_aWatchers;

	void MCF_Core_AutoWatcherRegistry()
	{
		m_aWatchers = new array<MCF_Core_ControllableWatcherComponent>();
	}

	static MCF_Core_AutoWatcherRegistry GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_AutoWatcherRegistry();
		return s_Instance;
	}

	void RegisterWatcher(MCF_Core_ControllableWatcherComponent watcher)
	{
		if (watcher && m_aWatchers.Find(watcher) == -1)
			m_aWatchers.Insert(watcher);
	}

	void UnregisterWatcher(MCF_Core_ControllableWatcherComponent watcher)
	{
		int index = m_aWatchers.Find(watcher);
		if (index != -1)
			m_aWatchers.Remove(index);
	}

	//! Call from MCF_Core_GameModeComponent.OnControllableSpawned() with
	//! the newly spawned entity. Offers it to every registered watcher, which
	//! decides for itself what to do with it.
	void NotifyNewControllable(IEntity entity)
	{
		if (!entity)
			return;

		MCF_Core_Log.Debug("Controllable spawned -- notifying " + m_aWatchers.Count().ToString() + " registered watcher(s)");

		foreach (MCF_Core_ControllableWatcherComponent watcher : m_aWatchers)
		{
			if (watcher)
				watcher.OnControllableSpawned(entity);
		}
	}
}
