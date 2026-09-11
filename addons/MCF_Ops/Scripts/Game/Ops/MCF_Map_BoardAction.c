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
		outName = "Open my map";
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


//! Zoom the board in or out, for everybody looking at it.
//!
//! THE BOARD IS NOT A SETTING IN ONE PERSON'S CLIENT. Several people stand
//! at it and they are talking about what they can all see, so the change goes
//! to the server and comes back to everyone. Nobody's own map moves.
class MCF_Map_BoardZoomAction : ScriptedUserAction
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Steps to zoom. 1 is in, -1 is out.", params: "-4 4")]
	protected int m_iDelta;

	protected MCF_Map_BoardComponent m_Board;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Board = MCF_Map_BoardComponent.Cast(pOwnerEntity.FindComponent(MCF_Map_BoardComponent));
	}

	override bool HasLocalEffectOnlyScript()
	{
		// The action itself is local; what it does is an RPC of its own, which
		// is the honest way round -- the action system would otherwise run
		// this on the server with no idea what it means.
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		if (m_iDelta >= 0)
			outName = "Zoom in";
		else
			outName = "Zoom out";

		return true;
	}

	//! Hidden at the end of its travel rather than greyed. A prompt that can
	//! do nothing is a prompt that promises something.
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Board)
			return false;

		if (m_iDelta > 0)
			return m_Board.GetZoomStep() < 4;

		return m_Board.GetZoomStep() > 0;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_Board)
			m_Board.AskZoom(m_iDelta);
	}
}

//! Put the board's view over where the person standing at it is.
//!
//! WHY THIS IS THE PAN. A board on a wall has no cursor, and a set of four
//! arrow prompts would be a bad imitation of one. "Where I am" is the thing
//! people actually want a map centred on, and it is one prompt.
class MCF_Map_BoardCentreAction : ScriptedUserAction
{
	protected MCF_Map_BoardComponent m_Board;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Board = MCF_Map_BoardComponent.Cast(pOwnerEntity.FindComponent(MCF_Map_BoardComponent));
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Centre map here";
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_Board != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_Board && pUserEntity)
			m_Board.AskCentre(pUserEntity.GetOrigin());
	}
}
