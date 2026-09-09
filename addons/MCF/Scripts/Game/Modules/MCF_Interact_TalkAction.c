//! The "talk to this person" prompt.
//!
//! TWO BEHAVIOURS, ONE PROMPT. If the entity has an MCF_Dialogue_Component it
//! opens a conversation. If it only has an MCF_Interact_HintComponent it does
//! what it always did: fires one line into the text queue. Dialogue wins when
//! both are present.
//!
//! Kept as one action rather than two, because from a player's side there is
//! only one thing here -- a person you can speak to. Which of the two a
//! mission maker wired up is not a distinction worth putting on their screen,
//! and two prompts on one civilian would be a bug that looks like a feature.
//!
//! LOCAL EFFECT ONLY. Opening a screen and firing a line are both local. The
//! conversation itself is server-decided, but that happens over RPCs from
//! inside the menu, not through the action's own replication -- which means
//! the prompt does not need to travel and cannot be replayed.

class MCF_Interact_TalkAction : ScriptedUserAction
{
	protected MCF_Dialogue_Component m_Dialogue;
	protected MCF_Interact_HintComponent m_Hint;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Dialogue = MCF_Dialogue_Component.Cast(pOwnerEntity.FindComponent(MCF_Dialogue_Component));
		m_Hint = MCF_Interact_HintComponent.Cast(pOwnerEntity.FindComponent(MCF_Interact_HintComponent));

		// Logged because "the prompt does not appear" has two entirely
		// different causes -- never registered on the entity, or registered
		// and the UI declining to show it -- and without this there is no way
		// to tell them apart.
		if (m_Dialogue)
			MCF_Core_Log.Debug("talk action registered on '" + m_Dialogue.GetSpeakerName() + "' (conversation)");
		else if (m_Hint)
			MCF_Core_Log.Debug("talk action registered (single line)");
		else
			MCF_Core_Log.Warn("MCF_Interact_TalkAction sits on an entity with neither a dialogue nor a hint component -- it will never show");
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//! Hidden rather than greyed when there is nothing to say. An empty prompt
	//! on a civilian is worse than no prompt: it promises a conversation.
	override bool CanBeShownScript(IEntity user)
	{
		if (m_Dialogue && m_Dialogue.HasConversation())
			return true;

		return m_Hint != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		if (m_Dialogue && m_Dialogue.HasConversation())
		{
			outName = m_Dialogue.GetActionVerb() + ": " + m_Dialogue.GetSpeakerName();
			return true;
		}

		outName = "Talk";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_Dialogue && m_Dialogue.HasConversation())
		{
			// The entity is named to the server by its replicated component,
			// not by any index of our own. HARD-WON, twice: FindItemId takes
			// the COMPONENT and never the IEntity, and the component has to be
			// SCR_EditableEntityComponent -- RplComponent is an engine class
			// with no GetOwner() exposed to script, so the server has no way
			// back from it to the person being talked to.
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(pOwnerEntity.FindComponent(SCR_EditableEntityComponent));
			if (!editable)
			{
				MCF_Core_Log.Warn("cannot talk to '" + m_Dialogue.GetSpeakerName() + "' -- no SCR_EditableEntityComponent on it, so the server cannot be told who is meant");
				return;
			}

			MCF_Dialogue_Menu.OpenFor(Replication.FindItemId(editable), m_Dialogue.GetSpeakerName());
			return;
		}

		if (m_Hint)
			m_Hint.Interact();
	}
}
