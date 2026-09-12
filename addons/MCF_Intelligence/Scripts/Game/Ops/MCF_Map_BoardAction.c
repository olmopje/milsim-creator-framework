//! Walk up to a map board and take the map.
//!
//! ONE PROMPT, NOT FOUR. The board used to offer Zoom in, Zoom out, Centre
//! map here and Open my map, which is a remote control for a map rather than
//! a map: you could nudge it but you could not read it, could not put a
//! marker where you meant, and could not point at anything.
//!
//! So the board hands you the map instead -- the game's own map window,
//! opened on the BOARD'S view and driving it while you have it. Everything
//! the M map does, you can do here: pan, zoom, place and remove markers, the
//! right-click menu, the ruler. None of it is reimplemented, because the
//! window's MapFrame is vanilla's own layout.
//!
//! And the room watches. While you work, the panel in the world follows what
//! you are doing, on everybody else's screen -- so a briefing is one person
//! driving and everyone else looking at the board.
class MCF_Map_BoardAction : ScriptedUserAction
{
	protected MCF_Map_BoardComponent m_Board;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Board = MCF_Map_BoardComponent.Cast(pOwnerEntity.FindComponent(MCF_Map_BoardComponent));

		// Logged because "the prompt does not appear" has two entirely
		// different causes -- never registered on the entity, or registered
		// and the UI declining to show it -- and without this there is no way
		// to tell them apart.
		if (!m_Board)
			MCF_Core_Log.Warn("MCF_Map_BoardAction sits on an entity with no MCF_Map_BoardComponent -- it will never do anything");
	}

	override bool HasLocalEffectOnlyScript()
	{
		// Opening a window affects nobody else. What it then changes about
		// the board goes to the server as its own request.
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Control map";
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_Board != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_Board)
			MCF_Map_BoardControlMenu.OpenFor(m_Board);
	}
}
