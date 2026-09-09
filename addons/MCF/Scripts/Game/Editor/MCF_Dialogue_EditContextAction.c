//! Adds "Edit conversation" to the Game Master's right-click menu on anybody
//! who can be talked to -- which, since MCF's Character_Base override, is
//! everybody.
//!
//! WHY NOT AN EDITOR ATTRIBUTE. The attribute panel already carries the three
//! things it can: Conversation, Trust and Fear, all numbers. It cannot carry a
//! sentence, because SCR_BaseEditorAttributeVar packs every value into one
//! vector and replicates twelve bytes. A custom attribute class does not
//! escape that -- the value still has to fit. So the words leave the attribute
//! system entirely and go over MCF's own RPC, the same route intel authoring
//! takes.
//!
//! ON IsServer() RETURNING FALSE: that is what keeps this local.
//! SCR_BaseActionsEditorComponent.ActionPerform routes an action to the server
//! only when IsServer() is true; otherwise it runs where it was clicked, which
//! is the only machine with a Game Master looking at a screen.
//!
//! ON OVERRIDING THE FOUR-ARGUMENT Perform: the per-entity helper is called
//! once for every selected entity, which would open one editor per selected
//! person. The outer form opens it once.

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class MCF_Dialogue_EditContextAction : SCR_SelectedEntitiesContextAction
{
	//! Local UI only -- never routed to the server.
	override bool IsServer()
	{
		return false;
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetDialogue(selectedEntity) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetDialogue(selectedEntity) != null;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent selected : selectedEntities)
		{
			MCF_Dialogue_Component dialogue = GetDialogue(selected);
			if (!dialogue)
				continue;

			MCF_Dialogue_EditorMenu.OpenFor(dialogue, selected);
			return;
		}

		// Nothing selected could talk: fall back to whatever is under the
		// cursor, which is how a right-click on an unselected person arrives.
		MCF_Dialogue_Component hovered = GetDialogue(hoveredEntity);
		if (hovered)
			MCF_Dialogue_EditorMenu.OpenFor(hovered, hoveredEntity);
	}

	protected MCF_Dialogue_Component GetDialogue(SCR_EditableEntityComponent editable)
	{
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return MCF_Dialogue_Component.Cast(owner.FindComponent(MCF_Dialogue_Component));
	}
}
