//! Bridges MCF's Event Bus to Arma Reforger's native AI waypoint system.
//! On m_sTriggerEvent, looks up a tagged SCR_AIGroup and a tagged
//! AIWaypoint (both placed and tagged by the mission maker via
//! MCF_Core_ObjectIdentityComponent, native BI entities otherwise
//! untouched by MCF) and calls the confirmed native
//! SCR_AIGroup.AddWaypoint() -- this is the real AI movement/animation
//! system (including SCR_AIAnimationWaypoint for animations), not
//! MCF_AI_SimpleMoverComponent's straight-line fallback.
//!
//! Deriving a group automatically from a character entity was
//! considered but not attempted -- the exact AIAgent-to-group lookup
//! wasn't confirmed, and tagging the group entity directly sidesteps
//! that uncertainty entirely.

[ComponentEditorProps(category: "MCF/AI", description: "On a trigger event, adds a tagged native AIWaypoint to a tagged native SCR_AIGroup.")]
class MCF_AI_EventToWaypointComponentClass : ScriptComponentClass
{
}

class MCF_AI_EventToWaypointComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that triggers adding the waypoint to the group.")]
	protected string m_sTriggerEvent;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Tag (via MCF_Core_ObjectIdentityComponent) of the native SCR_AIGroup entity to command.")]
	protected string m_sGroupTag;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Tag (via MCF_Core_ObjectIdentityComponent) of the native AIWaypoint entity to add (e.g. a placed SCR_AIAnimationWaypoint).")]
	protected string m_sWaypointTag;

	protected ScriptInvoker m_TriggerInvoker;

	override void EOnInit(IEntity owner)
	{
		if (m_sTriggerEvent.IsEmpty())
			return;

		m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
		m_TriggerInvoker.Insert(OnTrigger);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_AI_EventToWaypointComponent (trigger event)");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTrigger);
	}

	protected void OnTrigger(Managed payload)
	{
		SendGroupToWaypoint();
	}

	//! Looks up the tagged group and waypoint and adds the waypoint to
	//! the group. Does nothing (silently) if either tag doesn't resolve --
	//! a missing tag is a mission-maker configuration issue, already
	//! reported by the validation pass for the trigger event itself.
	void SendGroupToWaypoint()
	{
		IEntity groupEntity = MCF_Core_TagRegistry.GetInstance().GetByTag(m_sGroupTag);
		IEntity waypointEntity = MCF_Core_TagRegistry.GetInstance().GetByTag(m_sWaypointTag);
		if (!groupEntity || !waypointEntity)
			return;

		SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!group || !waypoint)
			return;

		group.AddWaypoint(waypoint);
	}
}
