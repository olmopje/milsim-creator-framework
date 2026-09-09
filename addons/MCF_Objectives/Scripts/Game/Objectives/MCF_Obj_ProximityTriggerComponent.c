//! Proximity trigger (reusable detection building block). Fires
//! m_sTriggeredEvent when any registered entity comes within m_fRadius of
//! this trigger's position. Self-drives via MCF_Core_TickCritical.
//!
//! Entities to watch can be registered manually via RegisterWatchedEntity(),
//! or automatically -- this component registers itself with
//! MCF_Core_AutoWatcherRegistry, which MCF_Core_GameModeComponent uses to
//! auto-add every newly spawned controllable entity (see that file for
//! the "not player-only yet" caveat).
//!
//! NOTE on OnPostInit: EOnInit only fires if EntityEvent.INIT is in the
//! entity's event mask, and that mask has to be set from OnPostInit. Without
//! it EOnInit never runs on a plain placed entity, so the component never
//! registers its tick and silently does nothing. Confirmed against vanilla
//! SCR_BaseAreaMeshComponent, which sets the mask the same way.

[ComponentEditorProps(category: "MCF/Objective", description: "Fires an event when a registered entity comes within range.")]
class MCF_Obj_ProximityTriggerComponentClass : MCF_Core_ControllableWatcherComponentClass
{
}

class MCF_Obj_ProximityTriggerComponent : MCF_Core_ControllableWatcherComponent
{
	[Attribute(defvalue: "50", uiwidget: UIWidgets.EditBox, desc: "Detection radius in metres.")]
	protected float m_fRadius;

	[Attribute(defvalue: "MCF_Obj_ProximityDetected", uiwidget: UIWidgets.EditBox, desc: "Event name published when a watched entity enters range.")]
	protected string m_sTriggeredEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "If true, only fires once. If false, fires again each time an entity re-enters after leaving range.")]
	protected bool m_bTriggerOnce;

	protected ref array<IEntity> m_aWatchedEntities;
	protected ref array<bool> m_aWasInRange;
	protected bool m_bHasTriggered;
	protected IEntity m_Owner;
	protected ScriptInvoker m_TickInvoker;

	//! \return Detection radius in metres.
	float GetRadius()
	{
		return m_fRadius;
	}

	//! Sets the detection radius in metres.
	//! \param radius New detection radius in metres.
	void SetRadius(float radius)
	{
		m_fRadius = radius;
	}

	//! \return True if the trigger only fires once.
	bool GetTriggerOnce()
	{
		return m_bTriggerOnce;
	}

	//! Sets whether the trigger only fires once.
	//! \param triggerOnce New trigger-once state.
	void SetTriggerOnce(bool triggerOnce)
	{
		m_bTriggerOnce = triggerOnce;
	}

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_Owner = owner;
		m_aWatchedEntities = new array<IEntity>();
		m_aWasInRange = new array<bool>();

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sTriggeredEvent);

		// Detection is authoritative and runs on the server only. Without
		// this guard every client evaluates its own copy against its own
		// view of the world, publishes its own events and keeps its own
		// trigger-once state -- so "fires once" would mean once per machine,
		// and clients could disagree about whether it fired at all.
		// See docs/research/multiplayer-and-audience.md.
		if (!Replication.IsServer())
			return;

		m_TickInvoker = MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Core_TickCritical");
		m_TickInvoker.Insert(OnTickCritical);

		MCF_RegisterAsWatcher();

		MCF_Core_Log.Debug("ProximityTrigger init, radius=" + m_fRadius.ToString() + " event=" + m_sTriggeredEvent);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TickInvoker)
			m_TickInvoker.Remove(OnTickCritical);

		MCF_UnregisterAsWatcher();
	}

	//! Every newly spawned controllable becomes something this trigger
	//! checks against. Called by MCF_Core_AutoWatcherRegistry.
	override void OnControllableSpawned(IEntity entity)
	{
		RegisterWatchedEntity(entity);
	}

	//! Adds entity to the list this trigger checks against each tick.
	void RegisterWatchedEntity(IEntity entity)
	{
		if (entity && m_aWatchedEntities.Find(entity) == -1)
		{
			m_aWatchedEntities.Insert(entity);
			m_aWasInRange.Insert(false);
			MCF_Core_Log.Debug("ProximityTrigger now watching " + m_aWatchedEntities.Count().ToString() + " entities");
		}
	}

	void UnregisterWatchedEntity(IEntity entity)
	{
		int index = m_aWatchedEntities.Find(entity);
		if (index != -1)
		{
			m_aWatchedEntities.Remove(index);
			m_aWasInRange.Remove(index);
		}
	}

	protected void OnTickCritical(Managed payload)
	{
		if (m_bTriggerOnce && m_bHasTriggered)
			return;

		if (!m_Owner)
			return;

		vector ownPosition = m_Owner.GetOrigin();

		for (int i = 0; i < m_aWatchedEntities.Count(); i++)
		{
			IEntity watched = m_aWatchedEntities[i];
			if (!watched)
				continue;

			bool inRange = vector.Distance(ownPosition, watched.GetOrigin()) <= m_fRadius;
			bool wasInRange = m_aWasInRange[i];

			if (inRange && !wasInRange)
				Fire();

			m_aWasInRange[i] = inRange;
		}
	}

	protected void Fire()
	{
		m_bHasTriggered = true;
		MCF_Core_Log.Debug("ProximityTrigger FIRED, publishing " + m_sTriggeredEvent);
		MCF_Core_EventManager.GetInstance().Publish(m_sTriggeredEvent, this);
	}
}
