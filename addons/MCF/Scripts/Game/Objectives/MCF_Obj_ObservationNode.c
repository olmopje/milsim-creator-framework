//! POI/Observation node (ARCHITECTURE.md 4.2). Reports activity to a
//! shared listener via the Event Bus instead of a hard reference, so
//! multiple observation points can feed the same branching logic (e.g. a
//! Logic Node in OR mode watching several POIs at once).
//!
//! Set m_sTriggerEvent to have this node report from the Event Bus. Without
//! it, Report() is only reachable from script and nothing in the framework
//! calls it, so the node could be placed but never fire.

[ComponentEditorProps(category: "MCF/Objective", description: "Reports activity to a shared Event Bus listener.")]
class MCF_Obj_ObservationNodeClass : ScriptComponentClass
{
}

class MCF_Obj_ObservationNode : ScriptComponent
{
	[Attribute(defvalue: "MCF_Obj_ObservationReported", uiwidget: UIWidgets.EditBox, desc: "Event name published on the Event Bus when this POI reports activity.")]
	protected string m_sReportEvent;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that makes this POI report. Leave empty to report only from script.")]
	protected string m_sTriggerEvent;

	protected int m_iReportCount;
	protected ScriptInvoker m_TriggerInvoker;

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

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sReportEvent);

		if (!m_sTriggerEvent.IsEmpty())
		{
			m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
			m_TriggerInvoker.Insert(OnTriggerEvent);
			MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_Obj_ObservationNode (report)");
		}

		MCF_Core_Log.Debug("ObservationNode init, triggered by '" + m_sTriggerEvent + "' publishes '" + m_sReportEvent + "'");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTriggerEvent);
	}

	protected void OnTriggerEvent(Managed payload)
	{
		Report();
	}

	//! Publishes m_sReportEvent with this node as payload, so the listener
	//! can identify which POI reported (e.g. via its MCF_Core_ObjectIdentityComponent tag).
	void Report()
	{
		m_iReportCount++;
		MCF_Core_Log.Debug("ObservationNode REPORTED (" + m_iReportCount.ToString() + "x), publishing " + m_sReportEvent);
		MCF_Core_EventManager.GetInstance().Publish(m_sReportEvent, this);
	}

	int GetReportCount()
	{
		return m_iReportCount;
	}
}
