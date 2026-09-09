//! Base class for any component that wants to be told about every newly
//! spawned controllable entity.
//!
//! WHY THIS EXISTS. MCF_Core_AutoWatcherRegistry used to hold three typed
//! arrays, one per detection component, which meant Core named three classes
//! that live in the Objectives module. That is the wrong direction for a
//! dependency: Core has to be installable on its own. Core now names only this
//! base class, which it owns, and the detection components extend it.
//!
//! The fan-out itself is unchanged -- same registry, same order, same call per
//! spawn. Only the type Core knows about has moved.
//!
//! HOW TO USE IT. Extend this instead of ScriptComponent, extend
//! MCF_Core_ControllableWatcherComponentClass instead of ScriptComponentClass,
//! override OnControllableSpawned(), and call MCF_RegisterAsWatcher() from
//! EOnInit and MCF_UnregisterAsWatcher() from EOnDeactivate. Forgetting the
//! registration is silent: the component ticks and never sees anybody.

class MCF_Core_ControllableWatcherComponentClass : ScriptComponentClass
{
}

class MCF_Core_ControllableWatcherComponent : ScriptComponent
{
	//! Called once per newly spawned controllable entity. The default does
	//! nothing; a detection component overrides it to start watching entity.
	void OnControllableSpawned(IEntity entity)
	{
	}

	//! Start receiving OnControllableSpawned. Call from EOnInit.
	protected void MCF_RegisterAsWatcher()
	{
		MCF_Core_AutoWatcherRegistry.GetInstance().RegisterWatcher(this);
	}

	//! Stop receiving it. Call from EOnDeactivate -- the registry holds a
	//! reference, so a component that never unregisters keeps its entity
	//! alive and gets called after it should be gone.
	protected void MCF_UnregisterAsWatcher()
	{
		MCF_Core_AutoWatcherRegistry.GetInstance().UnregisterWatcher(this);
	}
}
