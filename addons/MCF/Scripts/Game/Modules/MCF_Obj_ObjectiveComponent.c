//! Narrative task node (ARCHITECTURE.md 4.1). Publishes Objective_Complete
//! or Objective_Fail on the Event Bus; other nodes listen for those events
//! instead of holding a hard reference to this one.
//!
//! Intel gate: if m_sIntelGateEvent is set, the objective stays locked
//! (IsUnlocked() returns false) until that event fires on the Event Bus.
//! Leave empty for an objective that is unlocked from the start.
//!
//! A general-purpose condition slot (arbitrary boolean checks beyond the
//! intel gate) is not yet implemented -- this covers the intel-gate case
//! from the spec, broader conditions are a follow-up.

[ComponentEditorProps(category: "MCF/Objective", description: "Narrative task node -- publishes Objective_Complete/Objective_Fail on the Event Bus.")]
class MCF_Obj_ObjectiveComponentClass : ScriptComponentClass
{
}

class MCF_Obj_ObjectiveComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Objective title shown to players.")]
	protected string m_sTitle;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Objective description shown to players.")]
	protected string m_sDescription;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "If true, this objective shows a marker on the map.")]
	protected bool m_bVisibleOnMap;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that unlocks this objective (intel gate). Leave empty for an objective that starts unlocked.")]
	protected string m_sIntelGateEvent;

	protected bool m_bUnlocked;
	protected bool m_bComplete;
	protected bool m_bFailed;

	protected ScriptInvoker m_UnlockInvoker;

	override void EOnInit(IEntity owner)
	{
		m_bUnlocked = m_sIntelGateEvent.IsEmpty();

		if (!m_bUnlocked)
		{
			m_UnlockInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sIntelGateEvent);
			m_UnlockInvoker.Insert(OnIntelGateEvent);
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_UnlockInvoker)
			m_UnlockInvoker.Remove(OnIntelGateEvent);
	}

	protected void OnIntelGateEvent(Managed payload)
	{
		m_bUnlocked = true;
	}

	bool IsUnlocked()
	{
		return m_bUnlocked;
	}

	string GetTitle()
	{
		return m_sTitle;
	}

	string GetDescription()
	{
		return m_sDescription;
	}

	bool IsVisibleOnMap()
	{
		return m_bVisibleOnMap && m_bUnlocked;
	}

	//! Marks the objective complete and publishes "Objective_Complete".
	//! Does nothing if already complete or failed.
	void Complete()
	{
		if (m_bComplete || m_bFailed)
			return;

		m_bComplete = true;
		MCF_Core_EventManager.GetInstance().Publish("Objective_Complete", this);
	}

	//! Marks the objective failed and publishes "Objective_Fail".
	//! Does nothing if already complete or failed.
	void Fail()
	{
		if (m_bComplete || m_bFailed)
			return;

		m_bFailed = true;
		MCF_Core_EventManager.GetInstance().Publish("Objective_Fail", this);
	}

	bool IsComplete()
	{
		return m_bComplete;
	}

	bool IsFailed()
	{
		return m_bFailed;
	}
}
