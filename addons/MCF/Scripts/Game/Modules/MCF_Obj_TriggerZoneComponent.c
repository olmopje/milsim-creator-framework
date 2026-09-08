// MCF_Obj_TriggerZoneComponent
//
// Phase 0 deliverable (ARCHITECTURE.md section 9): the first GM-placeable
// entity that proves out the full pipeline -- placement, live attributes,
// Event Bus publish, save/load. Deliberately minimal: manual Activate()
// call for now, not yet collision-volume-driven auto-detection (that is a
// separate, bigger piece -- see open question at the bottom of this file).
//
// Namespace: MCF_Obj_ (narrative/objective nodes, ARCHITECTURE.md 3.1).

[ComponentEditorProps(category: "MCF/Objective", description: "Fires an Event Bus event when activated. Phase 0 proof-of-concept node.")]
class MCF_Obj_TriggerZoneComponentClass : ScriptComponentClass
{
}

class MCF_Obj_TriggerZoneComponent : ScriptComponent
{
	[Attribute(defvalue: "MCF_Obj_ZoneActivated", uiwidget: UIWidgets.EditBox, desc: "Event name published on the Event Bus when this zone activates. Follow the Module_Action naming contract (ARCHITECTURE.md 3.1).")]
	protected string m_sEventName;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "If true, this zone only fires once -- repeated Activate() calls after the first do nothing.")]
	protected bool m_bTriggerOnce;

	protected bool m_bHasTriggered;

	//! Manually invoked for now (e.g. from another script, or a future
	//! UserAction/collision-volume hookup). Publishes m_sEventName on the
	//! Event Bus so any module can react without a hard reference to this
	//! entity.
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
