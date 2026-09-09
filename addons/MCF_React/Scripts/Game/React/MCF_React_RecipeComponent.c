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
//! follow-up once a Tick Manager exists to schedule them. Step encoding
//! and execution live in MCF_React_StepRunner, shared with
//! MCF_React_SequencePlaybackComponent.
//!
//! NOTE on OnPostInit: EOnInit only fires if EntityEvent.INIT is in the
//! entity's event mask, set from OnPostInit. Without it this component
//! never subscribes to its trigger event on a placed entity.

class MCF_React_Step
{
	string m_sValue;

	void MCF_React_Step(string value)
	{
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

		if (m_sTriggerEvent.IsEmpty())
		{
			MCF_Core_Log.Debug("Recipe init but no trigger event set -- inactive");
			return;
		}

		m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
		m_TriggerInvoker.Insert(OnTrigger);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_React_RecipeComponent (trigger event)");

		int stepCount = 0;
		if (m_aSteps)
			stepCount = m_aSteps.Count();

		MCF_Core_Log.Debug("Recipe init, listening for " + m_sTriggerEvent + " with " + stepCount.ToString() + " steps");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTrigger);
	}

	protected void OnTrigger(Managed payload)
	{
		MCF_Core_Log.Debug("Recipe TRIGGERED by " + m_sTriggerEvent);
		RunSteps();
	}

	//! Runs every configured step in order, immediately (no delays yet).
	void RunSteps()
	{
		if (!m_aSteps)
			return;

		foreach (string rawStep : m_aSteps)
		{
			MCF_Core_Log.Debug("Recipe running step: " + rawStep);
			MCF_React_StepRunner.RunStep(rawStep);
		}
	}
}
