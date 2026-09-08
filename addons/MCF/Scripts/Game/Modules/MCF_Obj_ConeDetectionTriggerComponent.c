//! Line-of-sight-style trigger (reusable detection building block). This
//! is NOT true line of sight -- there is no confirmed raycast/occlusion
//! API (see docs/architecture/PROJECT_STATUS.md), so this approximates it
//! with the same distance+angle-cone check used by
//! MCF_AI_ComplianceComponent.IsBeingAimedAt(): fires when a watched
//! entity is within range AND within a forward-facing cone of this
//! trigger's facing direction. It will fire even through a wall.
//!
//! Self-drives via MCF_Core_TickCritical, same registration pattern as
//! MCF_Obj_ProximityTriggerComponent.

[ComponentEditorProps(category: "MCF/Objective", description: "Fires when a registered entity is within range and within a facing cone (distance+angle approximation, not true LOS).")]
class MCF_Obj_ConeDetectionTriggerComponentClass : ScriptComponentClass
{
}

class MCF_Obj_ConeDetectionTriggerComponent : ScriptComponent
{
	[Attribute(defvalue: "50", uiwidget: UIWidgets.EditBox, desc: "Detection radius in metres.")]
	protected float m_fRadius;

	[Attribute(defvalue: "45", uiwidget: UIWidgets.EditBox, desc: "Half-angle of the detection cone in degrees, measured from this entity's forward direction.")]
	protected float m_fHalfAngleDegrees;

	[Attribute(defvalue: "MCF_Obj_ConeDetected", uiwidget: UIWidgets.EditBox, desc: "Event name published when a watched entity enters the cone.")]
	protected string m_sTriggeredEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "If true, only fires once.")]
	protected bool m_bTriggerOnce;

	protected ref array<IEntity> m_aWatchedEntities;
	protected bool m_bHasTriggered;
	protected IEntity m_Owner;
	protected ScriptInvoker m_TickInvoker;

	override void EOnInit(IEntity owner)
	{
		m_Owner = owner;
		m_aWatchedEntities = new array<IEntity>();

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sTriggeredEvent);

		m_TickInvoker = MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Core_TickCritical");
		m_TickInvoker.Insert(OnTickCritical);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TickInvoker)
			m_TickInvoker.Remove(OnTickCritical);
	}

	void RegisterWatchedEntity(IEntity entity)
	{
		if (entity && m_aWatchedEntities.Find(entity) == -1)
			m_aWatchedEntities.Insert(entity);
	}

	void UnregisterWatchedEntity(IEntity entity)
	{
		int index = m_aWatchedEntities.Find(entity);
		if (index != -1)
			m_aWatchedEntities.Remove(index);
	}

	protected void OnTickCritical(Managed payload)
	{
		if ((m_bTriggerOnce && m_bHasTriggered) || !m_Owner)
			return;

		vector ownPosition = m_Owner.GetOrigin();
		vector forward = m_Owner.GetTransformAxis(2);

		foreach (IEntity watched : m_aWatchedEntities)
		{
			if (!watched)
				continue;

			vector toWatched = watched.GetOrigin() - ownPosition;
			float distance = toWatched.Length();
			if (distance > m_fRadius)
				continue;

			toWatched.Normalize();
			float angleDegrees = Math.Acos(Math.Clamp(vector.Dot(forward, toWatched), -1, 1)) * Math.RAD2DEG;

			if (angleDegrees <= m_fHalfAngleDegrees)
			{
				m_bHasTriggered = true;
				MCF_Core_EventManager.GetInstance().Publish(m_sTriggeredEvent, this);
				return;
			}
		}
	}
}
