//! Narrative task node (ARCHITECTURE.md 4.1). Publishes Objective_Complete
//! or Objective_Fail on the Event Bus; other nodes listen for those events
//! instead of holding a hard reference to this one.
//!
//! Intel gate: if m_sIntelGateEvent is set, the objective stays locked
//! (IsUnlocked() returns false) until that event fires on the Event Bus.
//! Leave empty for an objective that is unlocked from the start.
//!
//! Driven by events, like everything else in MCF: set m_sCompleteEvent and
//! m_sFailEvent to wire this objective to whatever should finish it. Before
//! these existed, Complete() and Fail() were only reachable from script and
//! nothing called them, so a mission maker had no way to finish an objective
//! at all.
//!
//! Player-facing state changes go through MCF_Voice_LineQueueManager, so they
//! surface on screen via MCF_UI_LineDisplayComponent. This is deliberately
//! NOT the vanilla task system -- see docs/research/objective-task-system.md
//! for what integrating SCR_TaskSystem would involve and why it is a separate
//! piece of work. Until then an objective is announced, not tracked in the
//! player's task list, and m_bVisibleOnMap has no effect yet.

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

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "If true, this objective shows a marker on the map. Not implemented yet -- see the file header.")]
	protected bool m_bVisibleOnMap;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that unlocks this objective (intel gate). Leave empty for an objective that starts unlocked.")]
	protected string m_sIntelGateEvent;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that completes this objective. Leave empty to complete it only from script.")]
	protected string m_sCompleteEvent;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that fails this objective. Leave empty to fail it only from script.")]
	protected string m_sFailEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Announce unlock, completion and failure on screen.")]
	protected bool m_bAnnounce;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EAudience), desc: "Who sees this objective's announcements. GROUP is not implemented yet and falls back to everyone.")]
	protected MCF_EAudience m_eAudience;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Faction key, used only when Audience is FACTION. Leave empty to announce to everyone.")]
	protected string m_sAudienceFactionKey;

	protected bool m_bUnlocked;
	protected bool m_bComplete;
	protected bool m_bFailed;

	protected ScriptInvoker m_UnlockInvoker;
	protected ScriptInvoker m_CompleteInvoker;
	protected ScriptInvoker m_FailInvoker;

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

		m_bUnlocked = m_sIntelGateEvent.IsEmpty();

		if (!m_bUnlocked)
		{
			m_UnlockInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sIntelGateEvent);
			m_UnlockInvoker.Insert(OnIntelGateEvent);
			MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sIntelGateEvent, string.Format("MCF_Obj_ObjectiveComponent on '%1' (intel gate)", m_sTitle));
		}

		if (!m_sCompleteEvent.IsEmpty())
		{
			m_CompleteInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sCompleteEvent);
			m_CompleteInvoker.Insert(OnCompleteEvent);
			MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sCompleteEvent, string.Format("MCF_Obj_ObjectiveComponent on '%1' (complete)", m_sTitle));
		}

		if (!m_sFailEvent.IsEmpty())
		{
			m_FailInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sFailEvent);
			m_FailInvoker.Insert(OnFailEvent);
			MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sFailEvent, string.Format("MCF_Obj_ObjectiveComponent on '%1' (fail)", m_sTitle));
		}

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("Objective_Complete");
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("Objective_Fail");

		MCF_Core_Log.Debug("Objective '" + m_sTitle + "' init, unlocked=" + m_bUnlocked.ToString() + " complete=" + m_sCompleteEvent + " fail=" + m_sFailEvent);

		if (m_bUnlocked)
			Announce("New objective: ");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_UnlockInvoker)
			m_UnlockInvoker.Remove(OnIntelGateEvent);

		if (m_CompleteInvoker)
			m_CompleteInvoker.Remove(OnCompleteEvent);

		if (m_FailInvoker)
			m_FailInvoker.Remove(OnFailEvent);
	}

	protected void OnIntelGateEvent(Managed payload)
	{
		if (m_bUnlocked)
			return;

		m_bUnlocked = true;
		MCF_Core_Log.Debug("Objective '" + m_sTitle + "' unlocked by " + m_sIntelGateEvent);
		Announce("New objective: ");
	}

	protected void OnCompleteEvent(Managed payload)
	{
		Complete();
	}

	protected void OnFailEvent(Managed payload)
	{
		Fail();
	}

	//! Puts a line on screen through the shared line queue, if announcing is
	//! enabled and this objective has a title worth showing.
	protected void Announce(string prefix)
	{
		if (!m_bAnnounce || m_sTitle.IsEmpty())
			return;

		MCF_Voice_LineQueueManager.GetInstance().Enqueue(prefix + m_sTitle, 0, m_eAudience, m_sAudienceFactionKey);
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
	//! Does nothing if already resolved, or if still locked behind its intel
	//! gate -- a gate that could be completed through would not be a gate.
	void Complete()
	{
		if (m_bComplete || m_bFailed)
			return;

		if (!m_bUnlocked)
		{
			MCF_Core_Log.Debug("Objective '" + m_sTitle + "' complete event ignored -- still locked");
			return;
		}

		m_bComplete = true;
		MCF_Core_Log.Debug("Objective '" + m_sTitle + "' COMPLETE");
		Announce("Objective complete: ");
		MCF_Core_EventManager.GetInstance().Publish("Objective_Complete", this);
	}

	//! Marks the objective failed and publishes "Objective_Fail".
	//! Does nothing if already complete or failed.
	void Fail()
	{
		if (m_bComplete || m_bFailed)
			return;

		m_bFailed = true;
		MCF_Core_Log.Debug("Objective '" + m_sTitle + "' FAILED");
		Announce("Objective failed: ");
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
