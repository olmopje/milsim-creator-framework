//! Adds "Mission data" to the Game Master's right-click menu on the operations
//! board.
//!
//! WHY THE BOARD AND NOT A MENU ENTRY. Every other authoring screen in MCF is
//! reached by right-clicking the thing it is about, and this is the one screen
//! that is about the mission rather than an object. The board is the closest
//! thing the mission has to a physical self: it is where the taskings and the
//! intel are read, so it is where clearing them belongs. It also means the
//! screen cannot be opened by accident from a menu while somebody is looking
//! for something else -- and this is the only screen in MCF that deletes.
//!
//! See MCF_Device_EditContextAction for why IsServer() returns false and why
//! the four-argument Perform is the one overridden.

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class MCF_Data_ManageContextAction : SCR_SelectedEntitiesContextAction
{
	//! Local UI only -- never routed to the server. What it asks for goes over
	//! MCF's own RPCs afterwards.
	override bool IsServer()
	{
		return false;
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetBoard(selectedEntity) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetBoard(selectedEntity) != null;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent selected : selectedEntities)
		{
			if (GetBoard(selected))
			{
				MCF_Data_ManagerMenu.Open();
				return;
			}
		}

		if (GetBoard(hoveredEntity))
			MCF_Data_ManagerMenu.Open();
	}

	protected MCF_Task_BoardComponent GetBoard(SCR_EditableEntityComponent editable)
	{
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return MCF_Task_BoardComponent.Cast(owner.FindComponent(MCF_Task_BoardComponent));
	}
}
