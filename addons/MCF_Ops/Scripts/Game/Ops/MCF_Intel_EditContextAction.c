//! Adds "Edit intel" to the Game Master's right-click menu on an MCF intel
//! object.
//!
//! WHY NOT A NORMAL EDITOR ATTRIBUTE. The vanilla attribute panel cannot carry
//! text at all. `SCR_BaseEditorAttributeVar` stores every value in a single
//! `vector`, replicates a fixed 12-byte snapshot, and offers exactly four
//! factories -- int, float, bool, vector. There is no string variant, no
//! text-entry attribute layout, and no edit-box attribute UI component.
//! Writing a *custom* attribute class does not help either: however custom the
//! class, the value still has to fit in that vector. So free text has to leave
//! the attribute system entirely.
//!
//! A context action does exactly that. It runs on the Game Master's own
//! client, opens MCF's own screen, and the typed text goes to the server over
//! MCF's own RPC where no such limit exists.
//!
//! ON IsServer() RETURNING FALSE: that is what keeps this local.
//! SCR_BaseActionsEditorComponent.ActionPerform sends an action to the server
//! only when IsServer() is true; otherwise it runs where it was clicked --
//! which is the only machine that has a Game Master looking at a screen.
//! Vanilla says the same thing in SCR_LoadSessionToolbarAction, comment and
//! all: "The action opens local UI".
//!
//! ON OVERRIDING THE FOUR-ARGUMENT Perform: SCR_SelectedEntitiesContextAction
//! calls its per-entity helper once for every selected entity. Using that
//! helper would open one editor window per selected object. Overriding the
//! outer form means the menu opens once, on the first thing selected.

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class MCF_Intel_EditContextAction : SCR_SelectedEntitiesContextAction
{
	//! Local UI only -- never routed to the server.
	override bool IsServer()
	{
		return false;
	}

	//! Only offered on things that carry intel and do not draw as a device.
	//! Devices have their own screen -- see GetCarrier for why the two must
	//! not overlap.
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetCarrier(selectedEntity) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetCarrier(selectedEntity) != null;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent selected : selectedEntities)
		{
			MCF_Intel_CarrierComponent carrier = GetCarrier(selected);
			if (!carrier)
				continue;

			MCF_Intel_EditorMenu.OpenFor(carrier, selected);
			return;
		}

		// Nothing selected carried intel: fall back to whatever is under the
		// cursor, which is how a right-click on an unselected object arrives.
		MCF_Intel_CarrierComponent hovered = GetCarrier(hoveredEntity);
		if (hovered)
			MCF_Intel_EditorMenu.OpenFor(hovered, hoveredEntity);
	}

	protected MCF_Intel_CarrierComponent GetCarrier(SCR_EditableEntityComponent editable)
	{
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		MCF_Intel_CarrierComponent carrier = MCF_Intel_CarrierComponent.Cast(owner.FindComponent(MCF_Intel_CarrierComponent));
		if (!carrier)
			return null;

		// The mirror image of MCF_Device_EditContextAction's filter, and it has
		// to stay a mirror: a phone offered this screen is a phone one APPLY
		// away from being a flat document, because the VIEW toggle here only
		// knows DOCUMENT and DEVICE and the payload carries the view whether
		// the author touched it or not.
		if (MCF_Intel_CarrierComponent.IsDeviceView(carrier.GetView()))
			return null;

		return carrier;
	}
}
