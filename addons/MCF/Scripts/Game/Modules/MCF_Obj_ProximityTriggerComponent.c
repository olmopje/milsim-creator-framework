//! Proximity trigger (reusable detection building block). Fires
//! m_sTriggeredEvent when any registered entity comes within m_fRadius of
//! this trigger's position. Self-drives via MCF_Core_TickCritical.
//!
//! Entities to watch must be registered manually via RegisterWatchedEntity()
//! -- there is no "get all entities in radius" query confirmed, so this
//! checks a known list rather than scanning the world. For watching
//! players specifically, register each player's controlled entity as they
//! join (not yet automated -- a future GameMode hook could do this).

[ComponentEditorProps(category: "MCF/Objective", description: "Fires an event when a registered entity comes within range.")]
class MCF_Obj_ProximityTriggerComponentClass : ScriptComponentClass
{
}

class MCF_Obj_ProximityTriggerComponent : ScriptComponent
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

	override void EOnInit(IEntity owner)
	{
		m_Owner = owner;
		m_aWatchedEntities = new array<IEntity>();
		m_aWasInRange = new array<bool>();

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sTriggeredEvent);

		m_TickInvoker = MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Core_TickCritical");
		m_TickInvoker.Insert(OnTickCritical);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TickInvoker)
			m_TickInvoker.Remove(OnTickCritical);
	}

	//! Adds entity to the list this trigger checks against each tick.
	void RegisterWatchedEntity(IEntity entity)
	{
		if (entity && m_aWatchedEntities.Find(entity) == -1)
		{
			m_aWatchedEntities.Insert(entity);
			m_aWasInRange.Insert(false);
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
		MCF_Core_EventManager.GetInstance().Publish(m_sTriggeredEvent, this);
	}
}
