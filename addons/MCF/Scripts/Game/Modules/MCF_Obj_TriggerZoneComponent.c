//! Fires an Event Bus event when activated. The Phase 0 proof-of-concept
//! node for the narrative/objective layer (ARCHITECTURE.md 4.1, 9).
//!
//! Activation is currently manual (called from script). Collision-volume-
//! driven auto-detection is a separate follow-up piece.

[ComponentEditorProps(category: "MCF/Objective", description: "Fires an Event Bus event when activated.")]
class MCF_Obj_TriggerZoneComponentClass : ScriptComponentClass
{
}

class MCF_Obj_TriggerZoneComponent : ScriptComponent
{
	[Attribute(defvalue: "MCF_Obj_ZoneActivated", uiwidget: UIWidgets.EditBox, desc: "Event name published on the Event Bus when this zone activates. Follows the Module_Action naming contract (ARCHITECTURE.md 3.1).")]
	protected string m_sEventName;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "If true, this zone only fires once -- repeated Activate() calls after the first do nothing.")]
	protected bool m_bTriggerOnce;

	protected bool m_bHasTriggered;

	//! Publishes m_sEventName on the Event Bus so any module can react
	//! without a hard reference to this entity.
	void Activate()
	{
		if (m_bTriggerOnce && m_bHasTriggered)
			return;

		m_bHasTriggered = true;
		MCF_Core_EventManager.GetInstance().Publish(m_sEventName, this);
	}

	bool HasTriggered()
	{
		return m_bHasTriggered;
	}
}
