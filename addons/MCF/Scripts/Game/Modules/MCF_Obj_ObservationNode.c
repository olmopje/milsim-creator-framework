//! POI/Observation node (ARCHITECTURE.md 4.2). Reports activity to a
//! shared listener via the Event Bus instead of a hard reference, so
//! multiple observation points can feed the same branching logic (e.g. a
//! Logic Node in OR mode watching several POIs at once).

[ComponentEditorProps(category: "MCF/Objective", description: "Reports activity to a shared Event Bus listener.")]
class MCF_Obj_ObservationNodeClass : ScriptComponentClass
{
}

class MCF_Obj_ObservationNode : ScriptComponent
{
	[Attribute(defvalue: "MCF_Obj_ObservationReported", uiwidget: UIWidgets.EditBox, desc: "Event name published on the Event Bus when this POI reports activity.")]
	protected string m_sReportEvent;

	protected int m_iReportCount;

	override void EOnInit(IEntity owner)
	{
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sReportEvent);
	}

	//! Publishes m_sReportEvent with this node as payload, so the listener
	//! can identify which POI reported (e.g. via its MCF_Core_ObjectIdentityComponent tag).
	void Report()
	{
		m_iReportCount++;
		MCF_Core_EventManager.GetInstance().Publish(m_sReportEvent, this);
	}

	int GetReportCount()
	{
		return m_iReportCount;
	}
}
