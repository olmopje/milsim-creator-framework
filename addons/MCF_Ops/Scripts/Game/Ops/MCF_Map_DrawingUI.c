//! Freehand drawing on the map, inside the map's own drawing tool.
//!
//! WHY THIS IS A `modded class` AND NOT A COMPONENT OF OUR OWN. The game
//! already has a drawing component on the map: SCR_MapDrawingUI, listed in
//! Configs/Map/MapFullscreen.conf, which is the config every fullscreen map in
//! the mission opens with -- the player's own M map and the board's Control map
//! window both. Modding it means MCF's freehand appears in both without a line
//! of wiring, without copying a vanilla config into the mod, and without a
//! second component competing with vanilla's for the same mouse button.
//!
//! What vanilla's drawing gives a player is up to nine straight lines that only
//! that player can see and that are forgotten when the mission ends. MCF adds
//! the other half: a line you draw with your hand, that everybody sees, on
//! every map and every board, until somebody rubs it out.
//!
//! WHERE THE CONTROLS LIVE. In the map's own right-click menu, as a category
//! filled in through SCR_MapRadialUI's GetOnMenuInitInvoker. Nothing of ours is
//! bolted onto the screen: no panel, no extra button, no keybind entry for
//! something that exists only while a map is open.
//!
//! WHY PICKING A COLOUR IS ALSO WHAT TURNS DRAWING ON. Draw mode sets the
//! cursor's CS_DRAW state, and CS_DRAW is in STATE_CTXMENU_RESTRICTED -- while
//! it is on, the right-click menu will not open. So a menu that could only turn
//! drawing on would be a menu you could never use to turn it off. Six coloured
//! entries do the whole job in one click, and right-click stops drawing, which
//! is what a pencil should do anyway.
modded class SCR_MapDrawingUI
{
	//! How far the cursor travels before another point is kept, and how many
	//! points one stroke may hold.
	//!
	//! THINNING IS NOT AN OPTIMISATION HERE, it is what makes the feature
	//! affordable at all: every drawing in the mission replicates as one
	//! string, so a stroke sampled every frame would be a thousand points
	//! nobody can tell apart. Eight metres is finer than the line is wide on
	//! any board at any zoom worth drawing at.
	protected static const float MCF_STEP_METRES = 8;
	protected static const int MCF_MAX_POINTS = 160;

	protected static const ref array<string> MCF_COLOUR_NAMES = {
		"White", "Red", "Blue", "Green", "Yellow", "Black"
	};

	//! The canvas is already in vanilla's map layout and nothing in the game
	//! claims it: SCR_MapConstants names a DrawingWidget for exactly this and
	//! no vanilla script reads it. The strokes were always meant to go there.
	protected CanvasWidget m_wMCFCanvas;

	//! The commands are a field because the canvas takes a POINTER to the
	//! array rather than a copy -- the engine says so in as many words.
	protected ref array<ref CanvasWidgetCommand> m_aMCFCommands = {};

	//! The stroke in progress, flat: x, z, x, z. Not sent until the button
	//! comes up; a half-drawn line is nobody else's business.
	protected ref array<float> m_aMCFStroke = {};

	protected bool m_bMCFFreehand;
	protected bool m_bMCFHeld;
	protected int m_iMCFColour;

	protected SCR_MapRadialUI m_MCFRadial;

	//------------------------------------------------------------------------
	// THE RIGHT-CLICK MENU
	//------------------------------------------------------------------------

	//! SCR_MapRadialUI event, fired after the menu has been cleared and is
	//! waiting to be filled. Entries are rebuilt every time it opens, which is
	//! what lets the labels say what the current colour is.
	protected void MCF_FillRadialMenu()
	{
		if (!m_MCFRadial)
			return;

		SCR_SelectionMenuCategoryEntry category = m_MCFRadial.AddRadialCategory("Freehand drawing");
		if (!category)
			return;

		foreach (int i, string name : MCF_COLOUR_NAMES)
		{
			SCR_SelectionMenuEntry entry = m_MCFRadial.AddRadialEntry("Draw: " + name, category);
			if (!entry)
				continue;

			// The index travels with the entry rather than in a field, because
			// one handler for six entries beats six handlers.
			entry.SetId(i.ToString());
			entry.GetOnPerform().Insert(MCF_OnColourPerformed);
		}

		SCR_SelectionMenuEntry mine = m_MCFRadial.AddRadialEntry("Rub out my drawings", category);
		if (mine)
			mine.GetOnPerform().Insert(MCF_OnClearMinePerformed);

		// Everybody's drawings is a Game Master's business, and only a real
		// one: a limited editor is photo mode, not command.
		SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
		if (editor && !editor.IsLimited())
		{
			SCR_SelectionMenuEntry all = m_MCFRadial.AddRadialEntry("Rub out all drawings", category);
			if (all)
				all.GetOnPerform().Insert(MCF_OnClearAllPerformed);
		}
	}

	//------------------------------------------------------------------------
	protected void MCF_OnColourPerformed(SCR_SelectionMenuEntry entry)
	{
		if (!entry)
			return;

		m_iMCFColour = entry.GetId().ToInt();

		// A frame later, because the menu is still closing and the cursor's
		// contextual-menu state has to be off before draw mode may go on.
		GetGame().GetCallqueue().CallLater(MCF_StartFreehand, 100, false);
	}

	//------------------------------------------------------------------------
	protected void MCF_StartFreehand()
	{
		MCF_SetFreehand(true);
	}

	//------------------------------------------------------------------------
	protected void MCF_OnClearMinePerformed(SCR_SelectionMenuEntry entry)
	{
		MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();
		if (drawings)
			drawings.AskClearMine();
	}

	//------------------------------------------------------------------------
	protected void MCF_OnClearAllPerformed(SCR_SelectionMenuEntry entry)
	{
		MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();
		if (drawings)
			drawings.AskClearAll();
	}

	//------------------------------------------------------------------------
	// DRAW MODE
	//------------------------------------------------------------------------

	//! WHY "MapSelect" AND NOT A KEY OF OUR OWN. It is the action the map
	//! already binds to the left mouse button in MapContext, which is the
	//! button anybody would draw with, and a binding of our own would mean a
	//! keybinds entry for something that exists only inside an open map.
	//! Vanilla's own line drawing listens to exactly this action.
	//!
	//! IT IS ONLY LISTENED TO WHILE FREEHAND IS ON. The left button means
	//! "select" on a map, and taking it silently would break the markers the
	//! map exists to place.
	protected void MCF_SetFreehand(bool on)
	{
		if (on == m_bMCFFreehand)
			return;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		if (on)
		{
			// The cursor module owns whether drawing is allowed at all here,
			// and it is also what puts the pencil on the cursor. Asking it is
			// how our drawing obeys the same rules vanilla's does.
			if (!m_CursorModule || !m_CursorModule.HandleDraw(true))
				return;

			// Two drawing modes on one mouse button is one too many.
			if (m_bIsDrawModeActive)
				SetDrawMode(false);

			input.AddActionListener("MapSelect", EActionTrigger.DOWN, MCF_OnDown);
			input.AddActionListener("MapSelect", EActionTrigger.UP, MCF_OnUp);

			// The way out. Draw mode blocks the right-click menu, so the
			// right button has nothing else to do while drawing -- and
			// "right-click to put the pencil down" is what a pencil should do.
			input.AddActionListener("MapContextualMenu", EActionTrigger.UP, MCF_OnStopDrawing);
		}
		else
		{
			input.RemoveActionListener("MapSelect", EActionTrigger.DOWN, MCF_OnDown);
			input.RemoveActionListener("MapSelect", EActionTrigger.UP, MCF_OnUp);
			input.RemoveActionListener("MapContextualMenu", EActionTrigger.UP, MCF_OnStopDrawing);

			if (m_CursorModule)
				m_CursorModule.HandleDraw(false);
		}

		m_bMCFFreehand = on;
		m_bMCFHeld = false;
		m_aMCFStroke.Clear();
	}

	//------------------------------------------------------------------------
	protected void MCF_OnStopDrawing(float value, EActionTrigger reason)
	{
		MCF_SetFreehand(false);
	}

	//------------------------------------------------------------------------
	protected void MCF_OnDown()
	{
		m_bMCFHeld = true;
		m_aMCFStroke.Clear();
		MCF_AddPoint();
	}

	//------------------------------------------------------------------------
	protected void MCF_OnUp()
	{
		m_bMCFHeld = false;

		// Two points is a line; one is a click somebody changed their mind
		// about, and a dot nobody meant is worse than nothing.
		if (m_aMCFStroke.Count() >= 4)
		{
			MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();
			if (drawings)
				drawings.AskAdd(m_aMCFStroke, m_iMCFColour);
		}

		m_aMCFStroke.Clear();
	}

	//------------------------------------------------------------------------
	protected void MCF_AddPoint()
	{
		if (!m_MapEntity)
			return;

		float worldX, worldZ;
		m_MapEntity.GetMapCursorWorldPosition(worldX, worldZ);

		int count = m_aMCFStroke.Count();

		if (count >= 2)
		{
			float lastX = m_aMCFStroke[count - 2];
			float lastZ = m_aMCFStroke[count - 1];

			if (Math.AbsFloat(worldX - lastX) < MCF_STEP_METRES && Math.AbsFloat(worldZ - lastZ) < MCF_STEP_METRES)
				return;
		}

		// A cap, because a line long enough to cross the island twice is
		// somebody leaning on the mouse rather than drawing.
		if (count >= MCF_MAX_POINTS * 2)
			return;

		m_aMCFStroke.Insert(worldX);
		m_aMCFStroke.Insert(worldZ);
	}

	//------------------------------------------------------------------------
	// THE PICTURE
	//------------------------------------------------------------------------

	//! THE LINE IN PROGRESS IS DRAWN LOCALLY AND SENT ONLY ON RELEASE. It is
	//! the difference between a line that follows your hand and one that
	//! follows the server, and it costs nothing: the points are already here.
	protected void MCF_Redraw()
	{
		if (!m_wMCFCanvas || !m_MapEntity)
			return;

		m_aMCFCommands.Clear();

		MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();

		if (drawings)
		{
			foreach (MCF_Map_Stroke stroke : drawings.GetStrokes())
			{
				MCF_AddCommand(stroke.m_aPoints, MCF_Map_DrawingComponent.Colour(stroke.m_iColour));
			}
		}

		if (m_aMCFStroke.Count() >= 4)
			MCF_AddCommand(m_aMCFStroke, MCF_Map_DrawingComponent.Colour(m_iMCFColour));

		m_wMCFCanvas.SetDrawCommands(m_aMCFCommands);
	}

	//------------------------------------------------------------------------
	//! One stroke, in the map's own pixels.
	//!
	//! WorldToScreen is the map's own conversion and it already carries the
	//! pan, so a stroke stays on the ground while the map moves underneath it.
	//! That is the whole reason the points are kept in metres.
	protected void MCF_AddCommand(notnull array<float> points, int colour)
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

		m_aMCFCommands.Insert(line);
	}

	//------------------------------------------------------------------------
	// VANILLA'S EVENTS
	//------------------------------------------------------------------------

	//! Vanilla's straight lines and our freehand are the same mouse button, so
	//! whichever starts puts the other down.
	override void SetDrawMode(bool state, bool cacheDrawn = false)
	{
		if (state && m_bMCFFreehand)
			MCF_SetFreehand(false);

		super.SetDrawMode(state, cacheDrawn);
	}

	//------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);

		m_wMCFCanvas = CanvasWidget.Cast(config.RootWidgetRef.FindAnyWidget(SCR_MapConstants.DRAWING_WIDGET_NAME));

		if (!m_wMCFCanvas)
			MCF_Core_Log.Warn("map drawing: this map layout carries no " + SCR_MapConstants.DRAWING_WIDGET_NAME + ", so freehand strokes will not be drawn on it");

		m_MCFRadial = SCR_MapRadialUI.GetInstance();

		if (m_MCFRadial)
			m_MCFRadial.GetOnMenuInitInvoker().Insert(MCF_FillRadialMenu);
		else
			MCF_Core_Log.Warn("map drawing: this map has no radial menu, so there is nowhere to offer freehand drawing");
	}

	//------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		GetGame().GetCallqueue().Remove(MCF_StartFreehand);

		MCF_SetFreehand(false);

		if (m_MCFRadial)
			m_MCFRadial.GetOnMenuInitInvoker().Remove(MCF_FillRadialMenu);

		m_MCFRadial = null;
		m_wMCFCanvas = null;

		super.OnMapClose(config);
	}

	//------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);

		if (m_bMCFFreehand && m_bMCFHeld)
			MCF_AddPoint();

		MCF_Redraw();
	}
}
