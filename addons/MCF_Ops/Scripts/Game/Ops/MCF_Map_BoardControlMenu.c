//! Standing at a map board and taking the map.
//!
//! WHY THIS IS A WINDOW AND NOT A CURSOR ON THE PANEL. A board is a texture
//! on a mesh, and a widget is a screen rectangle -- widgets have no
//! perspective, so a board seen at an angle can never BE a widget. Putting a
//! working cursor on the panel therefore means projecting the mouse ray onto
//! the mesh, converting the hit to board pixels, drawing our own pointer, and
//! then building our own context menu and our own marker placement, because
//! vanilla's live inside the map menu and do not come out.
//!
//! So the person driving gets the board as a window instead. Everyone else
//! goes on watching the panel in the world, which follows what the driver
//! does. That is the trade: the driver looks at a window, the room looks at
//! the board.
//!
//! AND IT IS VANILLA'S WINDOW. The layout's MapFrame inherits
//! UI/layouts/Map/Map.layout, and SCR_MapMenuUI is thirty lines that do
//! nothing but ask the game mode for a map config and open the map into their
//! own root. Do the same and the markers, the right-click menu, the ruler and
//! the tools are the game's own -- not reimplemented, not approximated, and
//! they will keep working when the game changes them.
//!
//! WHAT MAKES IT THE BOARD'S MAP rather than the player's: it opens on the
//! board's replicated view, and while it is open the board follows it. Close
//! it and the board keeps where it was left.
class MCF_Map_BoardControlMenu : ChimeraMenuBase
{
	//! Which board is being driven. Static because a menu is constructed by
	//! the menu manager, which takes no arguments -- the same hand-off every
	//! other MCF screen uses.
	protected static MCF_Map_BoardComponent s_Pending;

	protected MCF_Map_BoardComponent m_Board;
	protected SCR_MapEntity m_MapEntity;

	//! Ticks before the view is pushed to the board again. The board only
	//! needs to be roughly live, and every push is an RPC.
	protected static const float PUSH_SECONDS = 0.25;
	protected float m_fSincePush;

	protected bool m_bOpened;

	//------------------------------------------------------------------------
	static bool OpenFor(notnull MCF_Map_BoardComponent board)
	{
		s_Pending = board;

		MenuManager menus = GetGame().GetMenuManager();
		if (!menus)
			return false;

		return menus.OpenMenu(ChimeraMenuPreset.MCF_MapBoardControl) != null;
	}

	//------------------------------------------------------------------------
	override void OnMenuInit()
	{
		m_Board = s_Pending;
		s_Pending = null;

		m_MapEntity = SCR_MapEntity.GetMapInstance();
	}

	//------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		if (!m_MapEntity)
		{
			MCF_Core_Log.Warn("map board control: this world has no map entity");
			return;
		}

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		SCR_MapConfigComponent configs = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configs)
		{
			MCF_Core_Log.Warn("map board control: the game mode carries no SCR_MapConfigComponent, so there is no map config to open with");
			return;
		}

		// THE GADGET CONFIG, not one of ours. It is the config the player's
		// own map uses, which is exactly the point: the same tools, the same
		// right-click menu, the same markers.
		MapConfiguration config = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, configs.GetGadgetMapConfig(), GetRootWidget());
		if (!config)
			return;

		m_MapEntity.OpenMap(config);
		m_bOpened = true;

		// The map refuses a zoom or a pan for its first frames, so the board's
		// view is applied a moment later rather than now.
		GetGame().GetCallqueue().CallLater(ShowBoardView, 400, false);
	}

	//------------------------------------------------------------------------
	//! Open where the board is looking, not where this player last left their
	//! own map. Walking up to a board and finding somebody else's view is the
	//! entire point.
	protected void ShowBoardView()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen() || !m_Board)
			return;

		float centreX, centreZ;
		m_Board.GetViewCentre(centreX, centreZ);

		float ppu = m_Board.GetViewPPU();
		if (ppu <= 0)
			ppu = m_MapEntity.GetMinZoom();

		m_MapEntity.ZoomPanSmooth(ppu, centreX, centreZ, 0.1);
	}

	//------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (!m_bOpened || !m_Board || !m_MapEntity || !m_MapEntity.IsOpen())
			return;

		m_fSincePush += tDelta;
		if (m_fSincePush < PUSH_SECONDS)
			return;

		m_fSincePush = 0;

		// WHAT THE ROOM SEES. The driver's view, four times a second, onto
		// the board -- so the panel in the world pans and zooms with them
		// while they work.
		float centreX, centreZ;
		m_MapEntity.GetMapCenterWorldPosition(centreX, centreZ);

		m_Board.AskView(centreX, centreZ, m_MapEntity.GetCurrentZoom());
	}

	//------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(ShowBoardView);

		if (m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();

		m_bOpened = false;
		m_Board = null;
	}
}
