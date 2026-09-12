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

	//! How often the driver's view is pushed to the board. Ten a second is
	//! enough for the board to look live once it chases between them; every`n	//! push is an RPC, so this is the one number that costs a server.
	protected static const float PUSH_SECONDS = 0.1;
	protected float m_fSincePush;

	protected bool m_bOpened;

	//! Drawing. Off until somebody turns it on, because the left mouse button
	//! already means "select" on a map and taking it silently would break the
	//! markers this window exists to place.
	protected bool m_bDrawing;
	protected bool m_bHeld;
	protected int m_iColour;

	//! The stroke being drawn, flat: x, z, x, z. Not sent until the button
	//! comes up -- a half-drawn line is nobody else's business.
	protected ref array<float> m_aStroke = {};

	//! The commands the window draws, kept in a field because the canvas
	//! takes a pointer to the array rather than a copy.
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};
	protected CanvasWidget m_wDrawing;

	//! How far the cursor has to travel before another point is kept, and how
	//! many points one stroke may have. The whole set of drawings replicates
	//! as one string, so a stroke sampled every frame would be a thousand
	//! points nobody can tell apart.
	protected static const float STEP_METRES = 8;
	protected static const int MAX_POINTS = 160;

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

		// THE CANVAS IS ALREADY IN VANILLA'S LAYOUT and nothing in the game
		// claims it: SCR_MapConstants names a DrawingWidget for exactly this
		// and no vanilla script reads it. So the drawings go where the map
		// itself always meant them to go.
		m_wDrawing = CanvasWidget.Cast(GetRootWidget().FindAnyWidget(SCR_MapConstants.DRAWING_WIDGET_NAME));

		if (!m_wDrawing)
			MCF_Core_Log.Warn("map board control: the map layout carries no DrawingWidget, so strokes will not be drawn in the window");

		SetDrawing(true);

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

		if (m_bDrawing && m_bHeld)
			AddStrokePoint();

		DrawStrokes();

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
	// DRAWING
	//------------------------------------------------------------------------

	//! WHY "MapSelect" AND NOT A KEY OF OUR OWN. It is the action the map
	//! already binds to the left mouse button in MapContext, which is the
	//! button anybody would draw with, and adding a binding of our own would
	//! mean a keybinds entry for something that only exists inside one window.
	//! Vanilla's own drawing listens to exactly this action.
	//!
	//! It is only listened to while draw mode is on, so clicking a marker
	//! still does what it always did.
	protected void SetDrawing(bool on)
	{
		if (on == m_bDrawing)
			return;

		m_bDrawing = on;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		if (on)
		{
			input.AddActionListener("MapSelect", EActionTrigger.DOWN, OnDrawDown);
			input.AddActionListener("MapSelect", EActionTrigger.UP, OnDrawUp);
		}
		else
		{
			input.RemoveActionListener("MapSelect", EActionTrigger.DOWN, OnDrawDown);
			input.RemoveActionListener("MapSelect", EActionTrigger.UP, OnDrawUp);
			m_aStroke.Clear();
		}
	}

	//------------------------------------------------------------------------
	protected void OnDrawDown()
	{
		m_bHeld = true;
		m_aStroke.Clear();
		AddStrokePoint();
	}

	//------------------------------------------------------------------------
	protected void OnDrawUp()
	{
		m_bHeld = false;

		// Two points is a line; one is a click somebody changed their mind
		// about, and a dot nobody meant is worse than nothing.
		if (m_aStroke.Count() >= 4)
		{
			MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();
			if (drawings)
				drawings.AskAdd(m_aStroke, m_iColour);
		}

		m_aStroke.Clear();
	}

	//------------------------------------------------------------------------
	//! One point, if it is far enough from the last one to be worth keeping.
	//!
	//! THINNING IS NOT AN OPTIMISATION HERE, it is what makes the feature
	//! affordable at all: the whole set of drawings is replicated as one
	//! string, so a stroke sampled every frame would be a thousand points
	//! nobody can see the difference between.
	protected void AddStrokePoint()
	{
		if (!m_MapEntity)
			return;

		float worldX, worldZ;
		m_MapEntity.GetMapCursorWorldPosition(worldX, worldZ);

		int count = m_aStroke.Count();

		if (count >= 2)
		{
			float lastX = m_aStroke[count - 2];
			float lastZ = m_aStroke[count - 1];

			if (Math.AbsFloat(worldX - lastX) < STEP_METRES && Math.AbsFloat(worldZ - lastZ) < STEP_METRES)
				return;
		}

		// A cap, because a line long enough to cross the island twice is
		// somebody leaning on the mouse rather than drawing.
		if (count >= MAX_POINTS * 2)
			return;

		m_aStroke.Insert(worldX);
		m_aStroke.Insert(worldZ);
	}


	//------------------------------------------------------------------------
	//! Everything drawn on the map, plus whatever is being drawn right now.
	//!
	//! THE LINE IN PROGRESS IS DRAWN LOCALLY AND SENT ONLY ON RELEASE. It is
	//! the difference between a line that follows your hand and one that
	//! follows the server, and it costs nothing: the points are already here.
	protected void DrawStrokes()
	{
		if (!m_wDrawing || !m_MapEntity)
			return;

		m_aCommands.Clear();

		MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();

		if (drawings)
		{
			foreach (MCF_Map_Stroke stroke : drawings.GetStrokes())
			{
				AddStrokeCommand(stroke.m_aPoints, MCF_Map_DrawingComponent.Colour(stroke.m_iColour));
			}
		}

		if (m_aStroke.Count() >= 4)
			AddStrokeCommand(m_aStroke, MCF_Map_DrawingComponent.Colour(m_iColour));

		m_wDrawing.SetDrawCommands(m_aCommands);
	}

	//------------------------------------------------------------------------
	//! One stroke, in the map widget's own pixels.
	//!
	//! WorldToScreen is the map's own conversion and it already carries the
	//! pan, so a stroke stays on the ground while the map moves underneath
	//! it -- which is the whole reason the points are stored in metres.
	protected void AddStrokeCommand(notnull array<float> points, int colour)
	{
		array<float> pixels = {};

		for (int i = 0; i + 1 < points.Count(); i += 2)
		{
			int screenX, screenY;
			m_MapEntity.WorldToScreen(points[i], points[i + 1], screenX, screenY, true);

			pixels.Insert(screenX);
			pixels.Insert(screenY);
		}

		if (pixels.Count() < 4)
			return;

		LineDrawCommand line = new LineDrawCommand();
		line.m_Vertices = pixels;
		line.m_iColor = colour;
		line.m_fWidth = 4;
		line.m_fOutlineWidth = 2;
		line.m_iOutlineColor = 0xC0000000;

		m_aCommands.Insert(line);
	}

	//------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(ShowBoardView);
		SetDrawing(false);
		m_wDrawing = null;

		if (m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();

		m_bOpened = false;
		m_Board = null;
	}
}
