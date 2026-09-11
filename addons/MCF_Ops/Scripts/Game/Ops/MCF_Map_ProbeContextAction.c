//! Opens the map probe from the Game Master's right-click menu.
//!
//! A PROBE, NOT A FEATURE. Delete this, MCF_Map_ProbeMenu, MCF_MapProbe.layout
//! and the entries in MCF_MenuPresets / chimeraMenus / MCF_ContextActions once
//! the answer is written into docs/research/map-board-and-drawing.md.
//!
//! Offered on anything, because the question has nothing to do with what was
//! right-clicked -- it only needs a Game Master and an open world.
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class MCF_Map_ProbeContextAction : SCR_SelectedEntitiesContextAction
{
	//! Local UI only -- never routed to the server. See
	//! MCF_Intel_EditContextAction for why that is what IsServer false means.
	override bool IsServer()
	{
		return false;
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return true;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return true;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager)
			menuManager.OpenMenu(ChimeraMenuPreset.MCF_MapProbe);
	}
}
