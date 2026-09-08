//! Scripted AI Reactions catalog (ARCHITECTURE.md 5.12). A "recipe" is a
//! trigger event tied to a fixed sequence of steps, each reusing an
//! existing building block (a waypoint animation request, a text line, a
//! generic Event Bus publish) rather than any new behavior logic.
//!
//! Deliberately not a node-graph editor -- see the scope decision in
//! ARCHITECTURE.md 5.12. A recipe here is one component with an ordered
//! step list; a mission maker assembles steps in the Editor Attributes
//! panel, no script writing needed.
//!
//! Step execution is sequential and immediate (no waiting between steps
//! yet) -- delays between steps (e.g. "wait for evacuation") are a
//! follow-up once a Tick Manager exists to schedule them.

enum EMCF_ReactionStepType
{
	PUBLISH_EVENT,
	PLAY_TEXT_LINE,
	REQUEST_ANIMATION
}

class MCF_React_Step
{
	EMCF_ReactionStepType m_eType;
	string m_sValue; // event name, text line, or animation name depending on m_eType

	void MCF_React_Step(EMCF_ReactionStepType type, string value)
	{
		m_eType = type;
		m_sValue = value;
	}
}

[ComponentEditorProps(category: "MCF/React", description: "Reusable behavior recipe -- a trigger event tied to an ordered sequence of existing building blocks.")]
class MCF_React_RecipeComponentClass : ScriptComponentClass
{
}

class MCF_React_RecipeComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that triggers this recipe.")]
	protected string m_sTriggerEvent;

	[Attribute(desc: "Step values in order, formatted as \"TYPE:value\" -- TYPE is PUBLISH_EVENT, PLAY_TEXT_LINE, or REQUEST_ANIMATION.")]
	protected ref array<string> m_aSteps;

	protected ScriptInvoker m_TriggerInvoker;

	override void EOnInit(IEntity owner)
	{
		if (m_sTriggerEvent.IsEmpty())
			return;

		m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
		m_TriggerInvoker.Insert(OnTrigger);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_React_RecipeComponent (trigger event)");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTrigger);
	}

	protected void OnTrigger(Managed payload)
	{
		RunSteps();
	}

	//! Runs every configured step in order, immediately (no delays yet).
	void RunSteps()
	{
		if (!m_aSteps)
			return;

		foreach (string rawStep : m_aSteps)
			RunStep(rawStep);
	}

	protected void RunStep(string rawStep)
	{
		array<string> parts = new array<string>();
		rawStep.Split(":", parts, false);
		if (parts.Count() < 2)
			return;

		string typeStr = parts[0];
		string value = parts[1];

		if (typeStr == "PUBLISH_EVENT")
			MCF_Core_EventManager.GetInstance().Publish(value, this);
		else if (typeStr == "PLAY_TEXT_LINE")
			MCF_Voice_LineQueueManager.GetInstance().Enqueue(value, 0);
		else if (typeStr == "REQUEST_ANIMATION")
			MCF_Core_EventManager.GetInstance().Publish("MCF_AI_WaypointAnimationRequested", this);
	}
}
