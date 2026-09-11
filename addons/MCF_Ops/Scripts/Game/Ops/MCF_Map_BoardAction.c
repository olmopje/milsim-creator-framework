//! Walk up to a map board and take the map.
//!
//! WHAT THIS IS NOT is a cursor on the board's surface. A board is a texture
//! on a mesh; putting a working mouse on it would mean raycasting the screen
//! cursor onto the panel, converting the hit into widget coordinates, and
//! feeding synthetic events into a widget tree that is not in the screen
//! hierarchy. None of that is needed for what a person actually wants here.
//!
//! WHAT IT IS: the board hands you the game's real map, with everything the
//! real map already does -- zoom, pan, markers, channels, the lot -- and the
//! board mirrors it while you have it. So the commander drives, and everybody
//! standing at the board watches the same thing move. That is better than a
//! cursor on a panel, and it is one call rather than a subsystem.
//!
//! The map gadget is used when the player carries one, because that is the
//! path the rest of the game expects and it leaves the gadget's own state
//! consistent. Without one the menu is opened directly: the board IS the map,
//! and telling somebody standing in front of a map board that they cannot
//! read a map would be a rule invented for no reason.
class MCF_Map_BoardAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Use map";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// The vanilla route first. SetMapMode is what the map gadget's own
		// input does, so the gadget stays in step with the map.
		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(pUserEntity);
		if (gadgets)
		{
			IEntity carried = gadgets.GetGadgetByType(EGadgetType.MAP);
			if (carried)
			{
				SCR_MapGadgetComponent gadget = SCR_MapGadgetComponent.Cast(carried.FindComponent(SCR_MapGadgetComponent));
				if (gadget)
				{
					gadget.SetMapMode(true);
					return;
				}
			}
		}

		MenuManager menus = GetGame().GetMenuManager();
		if (menus)
			menus.OpenMenu(ChimeraMenuPreset.MapMenu);
	}
}
