//! Game Master attributes for talking to people: which conversation somebody
//! is running, and how they feel about you.
//!
//! THE CONSTRAINT THAT SHAPES ALL OF THIS. SCR_BaseEditorAttributeVar packs
//! everything into one vector and replicates twelve bytes. It can carry a
//! number and it can never carry a sentence -- CreateInt, CreateFloat,
//! CreateBool, CreateVector, and nothing else. A custom attribute class does
//! not escape that; it is the transport, not the class.
//!
//! So a Game Master cannot type a conversation in. What they can do is pick
//! one, because a pick is a number: the attribute below is an index into
//! Configs/Dialogue/MCF_Conversations.conf. The words live in the library
//! where text belongs, and the Game Master decides who says them.
//!
//! WHY THIS BEATS THE ASSIGN NODE for Game Master work. The node was built for
//! mission makers, who work in the World Editor where prefab attributes are
//! editable and a radius is convenient. In a live session a Game Master has
//! the person selected already, and wants that person to talk -- not a radius
//! around a marker. Both routes end at the same MCF_Dialogue_Component.
//!
//! EVERY ATTRIBUTE HERE OPTS OUT BY RETURNING NULL. Editor attributes are
//! global: the list is offered to every entity, and an attribute that does not
//! apply must say so by returning null from ReadVariable, or it appears on
//! every rock in the world.

//! Which library conversation this person runs. 0 means none -- they are
//! silent, which is what every character starts as.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_DialogueConversationEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_Dialogue_Component dialogue = GetDialogue(item);
		if (!dialogue)
			return null;

		array<string> ids = {};
		MCF_Dialogue_Library.GetInstance().GetIds(ids);

		// One-based, so that 0 can mean "nobody, say nothing". A slider whose
		// zero position is a real conversation would make silence unreachable.
		int index = ids.Find(dialogue.GetConversationId()) + 1;

		// CreateFloat and not CreateInt, even though this is a whole number.
		// The slider layout this attribute uses writes a float, and GetInt()
		// on a float-backed var reads 0 -- so an Int here round-trips as
		// "no conversation" no matter what the Game Master picks, and the
		// slider appears to snap back on its own. The radius attribute next
		// to it has always used CreateFloat for the same reason.
		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_Dialogue_Component dialogue = GetDialogue(item);
		if (!dialogue)
			return;

		array<string> ids = {};
		MCF_Dialogue_Library.GetInstance().GetIds(ids);

		int index = Math.Round(var.GetFloat()) - 1;

		if (index < 0 || index >= ids.Count())
		{
			dialogue.SetConversation(string.Empty);
			return;
		}

		dialogue.SetConversation(ids[index]);
	}

	protected MCF_Dialogue_Component GetDialogue(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_Dialogue_Component.Cast(owner.FindComponent(MCF_Dialogue_Component));
	}
}

//! Which library conversation this person runs once they have been subdued.
//! Same one-based index into the library as the ordinary conversation.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_DialogueInterrogationEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_Dialogue_Component dialogue = GetDialogue(item);
		if (!dialogue)
			return null;

		array<string> ids = {};
		MCF_Dialogue_Library.GetInstance().GetIds(ids);

		return SCR_BaseEditorAttributeVar.CreateFloat(ids.Find(dialogue.GetInterrogationId()) + 1);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_Dialogue_Component dialogue = GetDialogue(item);
		if (!dialogue)
			return;

		array<string> ids = {};
		MCF_Dialogue_Library.GetInstance().GetIds(ids);

		int index = Math.Round(var.GetFloat()) - 1;

		if (index < 0 || index >= ids.Count())
		{
			dialogue.SetInterrogation(string.Empty);
			return;
		}

		dialogue.SetInterrogation(ids[index]);
	}

	protected MCF_Dialogue_Component GetDialogue(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_Dialogue_Component.Cast(owner.FindComponent(MCF_Dialogue_Component));
	}
}

//! What has been done to this person: free, complying, restrained.
//!
//! Editable so a Game Master can set a scene up as already-captured without
//! having to act it out, and can undo a subdue that went wrong.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_DialogueCaptiveEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_AI_DispositionComponent disposition = GetDisposition(item);
		if (!disposition)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(disposition.GetCaptiveState());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_AI_DispositionComponent disposition = GetDisposition(item);
		if (!disposition)
			return;

		int value = Math.Round(var.GetFloat());
		value = Math.ClampInt(value, MCF_ECaptiveState.FREE, MCF_ECaptiveState.RESTRAINED);

		disposition.SetCaptiveState(value);
	}

	protected MCF_AI_DispositionComponent GetDisposition(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_AI_DispositionComponent.Cast(owner.FindComponent(MCF_AI_DispositionComponent));
	}
}

//! Which clip in the restrained-pose graph plays.
//!
//! EXISTS ONLY TO BE HUNTED WITH. The narrative graph holds a hundred
//! variants and nothing anywhere says which number is which -- the clips are
//! binary and the graph is binary. So the number is a slider, and finding the
//! right pose is a matter of restraining somebody and dialling until it looks
//! right, which takes a minute and no recompiles.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_RestraintPoseEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_AI_SubjectControlComponent control = GetControl(item);
		if (!control)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(control.GetPoseVariant());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_AI_SubjectControlComponent control = GetControl(item);
		if (!control)
			return;

		control.SetPoseVariant(Math.Round(var.GetFloat()));
	}

	protected MCF_AI_SubjectControlComponent GetControl(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_AI_SubjectControlComponent.Cast(owner.FindComponent(MCF_AI_SubjectControlComponent));
	}
}

//! How far this person trusts the players.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_DialogueTrustEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_AI_DispositionComponent disposition = GetDisposition(item);
		if (!disposition)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(disposition.GetTrust());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_AI_DispositionComponent disposition = GetDisposition(item);
		if (!disposition)
			return;

		disposition.SetTrust(var.GetFloat());
	}

	protected MCF_AI_DispositionComponent GetDisposition(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_AI_DispositionComponent.Cast(owner.FindComponent(MCF_AI_DispositionComponent));
	}
}

//! How frightened this person is.
//!
//! Reads and writes the PERSONAL share only. What the area's hostility adds is
//! the hostility system's business and is undone by its own decay -- a Game
//! Master who drags this to zero in a village at eighty has not made anybody
//! calm, and the panel should not pretend otherwise.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_DialogueFearEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_AI_DispositionComponent disposition = GetDisposition(item);
		if (!disposition)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(disposition.GetPersonalFear());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_AI_DispositionComponent disposition = GetDisposition(item);
		if (!disposition)
			return;

		disposition.SetFear(var.GetFloat());
	}

	protected MCF_AI_DispositionComponent GetDisposition(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_AI_DispositionComponent.Cast(owner.FindComponent(MCF_AI_DispositionComponent));
	}
}
