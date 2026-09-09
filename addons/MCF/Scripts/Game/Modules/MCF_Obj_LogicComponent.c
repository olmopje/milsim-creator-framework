//! Generic boolean/counting node that other nodes plug into
//! (ARCHITECTURE.md 4.3). Listens to a set of input events and fires
//! m_sOutputEvent once its condition is met.
//!
//! Supported modes (set via m_sMode, case-insensitive):
//!   "OR"      -- fires as soon as any one input event (from m_aInputEvents)
//!                occurs
//!   "COUNTER" -- fires once the input events (from m_aInputEvents) have
//!                occurred m_iRequiredCount times in total
//!   "AND"     -- fires once every configured AND input slot (m_sAndInput1-4)
//!                has fired at least once. Uses 4 fixed named slots instead
//!                of the dynamic m_aInputEvents list, because each slot
//!                needs its own distinct callback to know which input
//!                fired -- Enforce Script has no closures to generate that
//!                dynamically from an arbitrary-length array. Leave a slot
//!                empty if you need fewer than 4 inputs.

[ComponentEditorProps(category: "MCF/Objective", description: "Generic OR/AND/Counter logic node.")]
class MCF_Obj_LogicComponentClass : ScriptComponentClass
{
}

class MCF_Obj_LogicComponent : ScriptComponent
{
	[Attribute(defvalue: "OR", uiwidget: UIWidgets.EditBox, desc: "Logic mode: \"OR\", \"AND\", or \"COUNTER\".")]
	protected string m_sMode;

	[Attribute(desc: "Event names this node listens to (OR/COUNTER modes).")]
	protected ref array<string> m_aInputEvents;

	[Attribute(defvalue: "MCF_Obj_LogicFired", uiwidget: UIWidgets.EditBox, desc: "Event name published when this node's condition is met.")]
	protected string m_sOutputEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Number of input occurrences required before firing (COUNTER mode only).")]
	protected int m_iRequiredCount;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "AND mode input slot 1. Leave empty if unused.")]
	protected string m_sAndInput1;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "AND mode input slot 2. Leave empty if unused.")]
	protected string m_sAndInput2;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "AND mode input slot 3. Leave empty if unused.")]
	protected string m_sAndInput3;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "AND mode input slot 4. Leave empty if unused.")]
	protected string m_sAndInput4;

	protected int m_iFireCount;
	protected bool m_bTriggered;
	protected bool m_bAndInput1Fired;
	protected bool m_bAndInput2Fired;
	protected bool m_bAndInput3Fired;
	protected bool m_bAndInput4Fired;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		// Reaction and logic nodes are server-side, like the detection nodes
		// that feed them. On a client these would subscribe to events that
		// never fire there -- harmless today, but it leaves the client holding
		// framework state it should not have, and if anything ever did publish
		// locally the two machines would diverge.
		if (!Replication.IsServer())
			return;

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sOutputEvent);

		if (m_sMode == "AND")
		{
			if (!m_sAndInput1.IsEmpty())
			{
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput1).Insert(OnAndInput1Fired);
				MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sAndInput1, string.Format("MCF_Obj_LogicComponent (output '%1', AND input 1)", m_sOutputEvent));
			}
			if (!m_sAndInput2.IsEmpty())
			{
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput2).Insert(OnAndInput2Fired);
				MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sAndInput2, string.Format("MCF_Obj_LogicComponent (output '%1', AND input 2)", m_sOutputEvent));
			}
			if (!m_sAndInput3.IsEmpty())
			{
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput3).Insert(OnAndInput3Fired);
				MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sAndInput3, string.Format("MCF_Obj_LogicComponent (output '%1', AND input 3)", m_sOutputEvent));
			}
			if (!m_sAndInput4.IsEmpty())
			{
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput4).Insert(OnAndInput4Fired);
				MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sAndInput4, string.Format("MCF_Obj_LogicComponent (output '%1', AND input 4)", m_sOutputEvent));
			}
			return;
		}

		if (!m_aInputEvents)
			return;

		foreach (string eventName : m_aInputEvents)
		{
			MCF_Core_EventManager.GetInstance().GetInvoker(eventName).Insert(OnInputFired);
			MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(eventName, string.Format("MCF_Obj_LogicComponent (output '%1', input event)", m_sOutputEvent));
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_sMode == "AND")
		{
			if (!m_sAndInput1.IsEmpty())
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput1).Remove(OnAndInput1Fired);
			if (!m_sAndInput2.IsEmpty())
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput2).Remove(OnAndInput2Fired);
			if (!m_sAndInput3.IsEmpty())
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput3).Remove(OnAndInput3Fired);
			if (!m_sAndInput4.IsEmpty())
				MCF_Core_EventManager.GetInstance().GetInvoker(m_sAndInput4).Remove(OnAndInput4Fired);
			return;
		}

		if (!m_aInputEvents)
			return;

		foreach (string eventName : m_aInputEvents)
			MCF_Core_EventManager.GetInstance().GetInvoker(eventName).Remove(OnInputFired);
	}

	protected void OnAndInput1Fired(Managed payload)
	{
		m_bAndInput1Fired = true;
		CheckAndCondition();
	}

	protected void OnAndInput2Fired(Managed payload)
	{
		m_bAndInput2Fired = true;
		CheckAndCondition();
	}

	protected void OnAndInput3Fired(Managed payload)
	{
		m_bAndInput3Fired = true;
		CheckAndCondition();
	}

	protected void OnAndInput4Fired(Managed payload)
	{
		m_bAndInput4Fired = true;
		CheckAndCondition();
	}

	protected void CheckAndCondition()
	{
		MCF_Core_Log.Debug("Logic AND state: 1=" + m_bAndInput1Fired.ToString() + " 2=" + m_bAndInput2Fired.ToString() + " 3=" + m_bAndInput3Fired.ToString() + " 4=" + m_bAndInput4Fired.ToString());
		if (m_bTriggered)
			return;

		if (!m_sAndInput1.IsEmpty() && !m_bAndInput1Fired)
			return;
		if (!m_sAndInput2.IsEmpty() && !m_bAndInput2Fired)
			return;
		if (!m_sAndInput3.IsEmpty() && !m_bAndInput3Fired)
			return;
		if (!m_sAndInput4.IsEmpty() && !m_bAndInput4Fired)
			return;

		m_bTriggered = true;
		MCF_Core_Log.Debug("Logic AND satisfied, publishing " + m_sOutputEvent);
		MCF_Core_EventManager.GetInstance().Publish(m_sOutputEvent, this);
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
			MCF_Core_Log.Debug("Logic " + m_sMode + " satisfied, publishing " + m_sOutputEvent);
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
