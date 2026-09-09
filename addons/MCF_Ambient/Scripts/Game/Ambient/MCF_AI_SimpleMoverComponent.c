//! Self-built movement -- moves an entity toward a target position at a
//! fixed speed by directly setting its origin each frame. Not real AI
//! pathfinding (it walks in a straight line and ignores obstacles); this
//! exists because no AI waypoint/pathfinding API was confirmed without
//! guessing (see docs/architecture/PROJECT_STATUS.md). It is the concrete
//! piece that makes something actually move, closing part of the
//! "publishes an event but nothing visibly happens" gap.

[ComponentEditorProps(category: "MCF/AI", description: "Moves the entity toward a target position in a straight line at a fixed speed.")]
class MCF_AI_SimpleMoverComponentClass : ScriptComponentClass
{
}

class MCF_AI_SimpleMoverComponent : ScriptComponent
{
	[Attribute(defvalue: "2", uiwidget: UIWidgets.EditBox, desc: "Movement speed in metres per second.")]
	protected float m_fMoveSpeed;

	[Attribute(defvalue: "0.3", uiwidget: UIWidgets.EditBox, desc: "Distance (metres) counted as \"arrived\" at the target.")]
	protected float m_fArrivalDistance;

	protected vector m_vTarget;
	protected bool m_bMoving;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bMoving)
			return;

		vector current = owner.GetOrigin();
		float distance = vector.Distance(current, m_vTarget);

		if (distance <= m_fArrivalDistance)
		{
			m_bMoving = false;
			MCF_Core_EventManager.GetInstance().Publish("MCF_AI_MoveArrived", this);
			return;
		}

		vector direction = m_vTarget - current;
		direction.Normalize();
		owner.SetOrigin(current + direction * (m_fMoveSpeed * timeSlice));
	}

	//! Starts moving the owning entity toward target in a straight line.
	void MoveTo(vector target)
	{
		m_vTarget = target;
		m_bMoving = true;
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}

	void Stop()
	{
		m_bMoving = false;
	}

	bool IsMoving()
	{
		return m_bMoving;
	}
}
