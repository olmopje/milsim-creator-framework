//! Adds "Edit device" to the Game Master's right-click menu on anything that
//! carries intel and draws itself as a device.
//!
//! WHY NOT AN EDITOR ATTRIBUTE. The attribute panel carries what it can, which
//! is numbers: SCR_BaseEditorAttributeVar packs every value into one vector and
//! replicates twelve bytes. It cannot carry a message. So the words leave the
//! attribute system entirely and go over MCF's own RPC, the same route intel
//! and conversation authoring already take.
//!
//! ON IsServer() RETURNING FALSE: that is what keeps this local.
//! SCR_BaseActionsEditorComponent.ActionPerform routes an action to the server
//! only when IsServer() is true; otherwise it runs where it was clicked, which
//! is the only machine with a Game Master looking at a screen.
//!
//! ON OVERRIDING THE FOUR-ARGUMENT Perform: the per-entity helper is called
//! once for every selected entity, which would open one editor per selection.

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class MCF_Device_EditContextAction : SCR_SelectedEntitiesContextAction
{
	//! Local UI only -- never routed to the server.
	override bool IsServer()
	{
		return false;
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetDevice(selectedEntity) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetDevice(selectedEntity) != null;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent selected : selectedEntities)
		{
			MCF_Intel_CarrierComponent device = GetDevice(selected);
			if (!device)
				continue;

			MCF_Device_EditorMenu.OpenFor(device, selected);
			return;
		}

		// Nothing selected was a device: fall back to whatever is under the
		// cursor, which is how a right-click on an unselected object arrives.
		MCF_Intel_CarrierComponent hovered = GetDevice(hoveredEntity);
		if (hovered)
			MCF_Device_EditorMenu.OpenFor(hovered, hoveredEntity);
	}

	//! Only things that draw as a device.
	//!
	//! A letter carries intel too, and rewriting it is what "Edit intel"
	//! already does. Offering both on both would be two screens that do
	//! overlapping things to the same object, which is how a Game Master ends
	//! up not knowing which one they are supposed to use.
	protected MCF_Intel_CarrierComponent GetDevice(SCR_EditableEntityComponent editable)
	{
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		MCF_Intel_CarrierComponent carrier = MCF_Intel_CarrierComponent.Cast(owner.FindComponent(MCF_Intel_CarrierComponent));
		if (!carrier)
			return null;

		MCF_EIntelView view = carrier.GetView();
		if (view == MCF_EIntelView.PHONE || view == MCF_EIntelView.LAPTOP || view == MCF_EIntelView.DEVICE)
			return carrier;

		return null;
	}
}
