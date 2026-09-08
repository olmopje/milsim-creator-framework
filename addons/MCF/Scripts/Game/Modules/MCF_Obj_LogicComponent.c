//! Generic boolean/counting node that other nodes plug into
//! (ARCHITECTURE.md 4.3). Listens to a set of input events and fires
//! m_sOutputEvent once its condition is met.
//!
//! Supported modes (set via m_sMode, case-insensitive):
//!   "OR"      -- fires as soon as any one input event occurs
//!   "COUNTER" -- fires once the input events have occurred
//!                m_iRequiredCount times in total
//!
//! AND mode (fires only once every listed input has occurred at least
//! once) is not implemented yet. It needs to track completion per input
//! event individually, which requires a distinct callback per input --
//! Enforce Script has no closures to generate those dynamically from an
//! array, so it needs a different design (e.g. a small fixed number of
//! named input slots) rather than an arbitrary-length array. Follow-up.

[ComponentEditorProps(category: "MCF/Objective", description: "Generic OR/Counter logic node.")]
class MCF_Obj_LogicComponentClass : ScriptComponentClass
{
}

class MCF_Obj_LogicComponent : ScriptComponent
{
	[Attribute(defvalue: "OR", uiwidget: UIWidgets.EditBox, desc: "Logic mode: \"OR\" or \"COUNTER\".")]
	protected string m_sMode;

	[Attribute(desc: "Event names this node listens to.")]
	protected ref array<string> m_aInputEvents;

	[Attribute(defvalue: "MCF_Obj_LogicFired", uiwidget: UIWidgets.EditBox, desc: "Event name published when this node's condition is met.")]
	protected string m_sOutputEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Number of input occurrences required before firing (COUNTER mode only).")]
	protected int m_iRequiredCount;

	protected int m_iFireCount;
	protected bool m_bTriggered;

	override void EOnInit(IEntity owner)
	{
		if (!m_aInputEvents)
			return;

		foreach (string eventName : m_aInputEvents)
			MCF_Core_EventManager.GetInstance().GetInvoker(eventName).Insert(OnInputFired);
	}

	override void OnDelete(IEntity owner)
	{
		if (!m_aInputEvents)
			return;

		foreach (string eventName : m_aInputEvents)
			MCF_Core_EventManager.GetInstance().GetInvoker(eventName).Remove(OnInputFired);
	}

	protected void OnInputFired(Managed payload)
	{
		if (m_bTriggered)
			return;

		m_iFireCount++;

		bool conditionMet = false;
		if (m_sMode == "COUNTER")
			conditionMet = m_iFireCount >= m_iRequiredCount;
		else
			conditionMet = true; // default / "OR"

		if (conditionMet)
		{
			m_bTriggered = true;
			MCF_Core_EventManager.GetInstance().Publish(m_sOutputEvent, this);
		}
	}

	int GetFireCount()
	{
		return m_iFireCount;
	}

	bool IsTriggered()
	{
		return m_bTriggered;
	}
}
