//! Waypoint-style node that requests an animation on arrival
//! (ARCHITECTURE.md 5.4). Arrival is currently a manual OnArrival() call --
//! actual AI waypoint system integration (auto-calling this when an AI
//! group reaches the node) is a follow-up.
//!
//! IMPORTANT for anything with a real AI group (SCR_AIGroup): use the
//! native SCR_AIAnimationWaypoint instead of this component --
//! confirmed to exist via SCR_AIGroup.AddWaypoint(waypoint), the same
//! way vanilla AI patrols/waypoints work. This component is the
//! lightweight fallback for entities NOT in an AI group (e.g. a
//! standalone civilian), where the native waypoint system doesn't apply.
//!
//! A speculative CharacterControllerComponent.PlayGesture()/CanPlayGesture()
//! call was tried here and did not compile ("Undefined function") --
//! removed rather than guessed further. See
//! docs/architecture/PROJECT_STATUS.md for what was confirmed vs not.

[ComponentEditorProps(category: "MCF/AI", description: "Requests an animation on arrival (event-only). For real AI groups, prefer the native SCR_AIAnimationWaypoint instead.")]
class MCF_AI_WaypointAnimationComponentClass : ScriptComponentClass
{
}

class MCF_AI_WaypointAnimationComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Animation name to request on arrival. Not wired to actually play it -- see file header.")]
	protected string m_sAnimationName;

	[Attribute(defvalue: "MCF_AI_WaypointAnimationRequested", uiwidget: UIWidgets.EditBox, desc: "Event name published on arrival.")]
	protected string m_sEventName;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

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
