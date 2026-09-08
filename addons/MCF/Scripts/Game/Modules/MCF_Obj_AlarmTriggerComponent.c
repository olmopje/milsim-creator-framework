//! Alarm trigger (reusable detection building block) -- a simple relay:
//! listens for m_sSourceEvent and republishes m_sTriggeredEvent. Useful
//! for chaining detection nodes together (e.g. a Proximity Trigger feeds
//! into an Alarm node that other Recipes/Logic nodes listen to) with a
//! clearer, purpose-named event name than daisy-chaining raw event names
//! everywhere.

[ComponentEditorProps(category: "MCF/Objective", description: "Listens for a source event and republishes it under a new name -- for chaining detection nodes together.")]
class MCF_Obj_AlarmTriggerComponentClass : ScriptComponentClass
{
}

class MCF_Obj_AlarmTriggerComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that triggers this alarm.")]
	protected string m_sSourceEvent;

	[Attribute(defvalue: "MCF_Obj_AlarmRaised", uiwidget: UIWidgets.EditBox, desc: "Event name published when the alarm is raised.")]
	protected string m_sTriggeredEvent;

	protected ScriptInvoker m_SourceInvoker;

	override void EOnInit(IEntity owner)
	{
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sTriggeredEvent);

		if (m_sSourceEvent.IsEmpty())
			return;

		m_SourceInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sSourceEvent);
		m_SourceInvoker.Insert(OnSourceEvent);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sSourceEvent, "MCF_Obj_AlarmTriggerComponent (source event)");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_SourceInvoker)
			m_SourceInvoker.Remove(OnSourceEvent);
	}

	protected void OnSourceEvent(Managed payload)
	{
		MCF_Core_EventManager.GetInstance().Publish(m_sTriggeredEvent, this);
	}
}
