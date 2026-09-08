//! Waypoint-style node that triggers an animation on arrival
//! (ARCHITECTURE.md 5.4). Arrival is currently a manual OnArrival() call --
//! actual AI waypoint system integration (auto-calling this when an AI
//! group reaches the node) is a follow-up, same pattern as the Trigger
//! Zone's manual Activate().
//!
//! Actually playing m_sAnimationName on a character is not wired up yet
//! either -- this component only publishes which animation was requested,
//! for something else to act on. Confirming the CharacterAnimationComponent
//! API needed for that is separate work.

[ComponentEditorProps(category: "MCF/AI", description: "Requests an animation on arrival (event-only for now, does not play it directly).")]
class MCF_AI_WaypointAnimationComponentClass : ScriptComponentClass
{
}

class MCF_AI_WaypointAnimationComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Animation name to request on arrival. Not yet wired to actually play -- see file header.")]
	protected string m_sAnimationName;

	[Attribute(defvalue: "MCF_AI_WaypointAnimationRequested", uiwidget: UIWidgets.EditBox, desc: "Event name published on arrival.")]
	protected string m_sEventName;

	override void EOnInit(IEntity owner)
	{
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sEventName);
	}

	//! Call when the AI (or player, for testing) reaches this waypoint.
	void OnArrival()
	{
		MCF_Core_EventManager.GetInstance().Publish(m_sEventName, this);
	}

	string GetAnimationName()
	{
		return m_sAnimationName;
	}
}
