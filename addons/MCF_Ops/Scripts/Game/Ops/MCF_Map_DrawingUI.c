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
	protected bool m_bMCFStroking;
	protected int m_iMCFColour;

	protected SCR_MapRadialUI m_MCFRadial;

	//! DIAGNOSTICS. On until the freehand has been seen working on a board and
	//! in a player's own map; then this line and every MCF_Say below it go.
	//! The failure this is here to catch is a silent one -- draw mode refused,
	//! or points collected and drawn into the wrong coordinate space -- and
	//! neither shows up as an error.
	protected static const bool MCF_DIAG = false;

	protected int m_iMCFSaidCommands = -1;
	protected int m_iMCFStartTries;
	protected float m_fMCFSaidX;
	protected float m_fMCFSaidY;
	protected int m_iMCFHeldFrames;

	//------------------------------------------------------------------------
	protected void MCF_Say(string message)
	{
		if (MCF_DIAG)
			MCF_Core_Log.Warn("map drawing: " + message);
	}

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

		MCF_Say("filling the radial menu");

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

		MCF_Say("colour " + m_iMCFColour + " picked");

		// NOT THIS FRAME. The radial is still closing, and the cursor keeps
		// CS_CONTEXTUAL_MENU until it has -- which is one of the states that
		// makes HandleDraw refuse. Retried rather than assumed, because how
		// long the close takes is the menu's business, not ours.
		m_iMCFStartTries = 0;
		GetGame().GetCallqueue().CallLater(MCF_StartFreehand, 150, true);
	}

	//------------------------------------------------------------------------
	protected void MCF_StartFreehand()
	{
		m_iMCFStartTries++;

		if (MCF_SetFreehand(true) || m_iMCFStartTries >= 10)
		{
			GetGame().GetCallqueue().Remove(MCF_StartFreehand);

			if (!m_bMCFFreehand)
				MCF_Say("gave up turning draw mode on after " + m_iMCFStartTries + " tries");
		}
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
	protected bool MCF_SetFreehand(bool on)
	{
		if (on == m_bMCFFreehand)
			return true;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return false;

		if (on)
		{
			if (!m_CursorModule)
			{
				MCF_Say("no cursor module, so draw mode cannot be asked for");
				return false;
			}

			// Two drawing modes on one mouse button is one too many, and
			// vanilla's has to go first: it holds CS_DRAW itself, and
			// HandleDraw refuses to hand out a state that is already set.
			if (m_bIsDrawModeActive)
				SetDrawMode(false);

			// The cursor module owns whether drawing is allowed at all here,
			// and it is also what puts the pencil on the cursor. Asking it is
			// how our drawing obeys the same rules vanilla's does.
			if (!m_CursorModule.HandleDraw(true))
			{
				MCF_Say("draw mode refused, cursor state " + m_CursorModule.GetCursorState());
				return false;
			}

			// ONE TRIGGER, NOT TWO. MapSelect is an edge-triggered click: the
			// engine delivers DOWN and UP four milliseconds apart however long
			// the button is actually held, so a drag cannot be read from it at
			// all. Vanilla's own line tool is click-to-start, click-to-finish
			// for exactly this reason, and freehand works the same way: click
			// once, move the mouse and the line follows it, click again to let
			// go. It is also the easier hand on a big map.
			input.AddActionListener("MapSelect", EActionTrigger.UP, MCF_OnClick);

			// The way out. Draw mode blocks the right-click menu, so the
			// right button has nothing else to do while drawing -- and
			// "right-click to put the pencil down" is what a pencil should do.
			input.AddActionListener("MapContextualMenu", EActionTrigger.UP, MCF_OnStopDrawing);
		}
		else
		{
			input.RemoveActionListener("MapSelect", EActionTrigger.UP, MCF_OnClick);
			input.RemoveActionListener("MapContextualMenu", EActionTrigger.UP, MCF_OnStopDrawing);

			if (m_CursorModule)
				m_CursorModule.HandleDraw(false);
		}

		m_bMCFFreehand = on;
		m_bMCFStroking = false;
		m_aMCFStroke.Clear();

		MCF_Say("freehand " + on);

		return true;
	}

	//------------------------------------------------------------------------
	//! Right-click throws away the stroke being drawn if there is one, and
	//! otherwise puts the pencil down. Two meanings on one button, but in the
	//! order anybody would expect: undo the thing in your hand first.
	protected void MCF_OnStopDrawing(float value, EActionTrigger reason)
	{
		if (m_bMCFStroking)
		{
			m_bMCFStroking = false;
			m_aMCFStroke.Clear();

			MCF_Say("stroke cancelled");
			return;
		}

		MCF_SetFreehand(false);
	}

	//------------------------------------------------------------------------
	//! One click: begins a stroke, or finishes the one in progress.
	protected void MCF_OnClick()
	{
		if (m_bMCFStroking)
		{
			m_bMCFStroking = false;

			MCF_Say("stroke finished with " + (m_aMCFStroke.Count() / 2) + " points");

			// Two points is a line; one is a click somebody changed their mind
			// about, and a dot nobody meant is worse than nothing.
			if (m_aMCFStroke.Count() >= 4)
			{
				MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();

				string state = "component " + (drawings != null).ToString();
				state = state + ", server " + Replication.IsServer().ToString();

				if (drawings)
				{
					drawings.AskAdd(m_aMCFStroke, m_iMCFColour);
					state = state + ", strokes now " + drawings.GetStrokes().Count().ToString();
				}

				MCF_Say("sent: " + state);
			}

			m_aMCFStroke.Clear();
			return;
		}

		m_bMCFStroking = true;
		m_aMCFStroke.Clear();
		MCF_AddPoint();

		MCF_Say("stroke started");
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

		// A REFERENCE LINE AT KNOWN COORDINATES, while draw mode is on. It
		// settles in one look what no amount of reading the engine's headers
		// could: whether this canvas draws at all, and whether a command's
		// numbers are screen pixels or workspace units. If the magenta line
		// runs from near the top-left corner to about a quarter across, the
		// canvas works and the space is units; if it is absent, the canvas is
		// not the place to draw; if it is somewhere else entirely, the space
		// is scaled. Goes with the rest of the diagnostics.
		if (MCF_DIAG && m_bMCFFreehand)
		{
			array<float> corner = {100, 100, 400, 400};

			LineDrawCommand probe = new LineDrawCommand();
			probe.m_Vertices = corner;
			probe.m_iColor = 0xFFFF00FF;
			probe.m_fWidth = 6;

			m_aMCFCommands.Insert(probe);
		}

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

		// Said once per change, not once per frame.
		if (m_aMCFCommands.Count() != m_iMCFSaidCommands)
		{
			m_iMCFSaidCommands = m_aMCFCommands.Count();

			if (m_iMCFSaidCommands > 0)
			{
				// THE ONE THING READING THE SOURCE COULD NOT SETTLE: which
				// space a canvas draw command is in. The canvas is anchored to
				// the whole map frame but declares SizeInUnits 1024 1024, and
				// WorldToScreen hands back DPI-SCALED pixels -- vanilla
				// DPIUnscales them before giving them to a FrameSlot. If a
				// command is in unscaled units instead, every stroke is drawn
				// off the side of the screen and looks like nothing happened.
				float canvasW, canvasH, screenW, screenH;
				m_wMCFCanvas.GetScreenSize(canvasW, canvasH);
				GetGame().GetWorkspace().GetScreenSize(screenW, screenH);

				string say = m_iMCFSaidCommands.ToString();
				say = say + " commands; canvas " + canvasW.ToString();
				say = say + "x" + canvasH.ToString();
				say = say + "; screen " + screenW.ToString();
				say = say + "x" + screenH.ToString();
				say = say + "; first point " + m_fMCFSaidX.ToString();
				say = say + "," + m_fMCFSaidY.ToString();

				MCF_Say(say);
			}
		}
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

		m_fMCFSaidX = pixels[0];
		m_fMCFSaidY = pixels[1];

		// THE HALO IS ITS OWN COMMAND, not m_fOutlineWidth. Setting the outline
		// fields on a line swallowed the colour whole -- a red stroke drew
		// black -- while the magenta reference line, which sets nothing but
		// m_iColor, was exactly the colour asked for. So the outline fields
		// mean something other than what their names suggest, and rather than
		// tune a field whose behaviour is a guess, the black goes down first
		// as a wider line of its own and the colour is laid on top. This is
		// the same technique the board already uses for marker icons.
		LineDrawCommand halo = new LineDrawCommand();
		halo.m_Vertices = pixels;
		halo.m_iColor = 0xC0000000;
		halo.m_fWidth = 8;

		m_aMCFCommands.Insert(halo);

		LineDrawCommand line = new LineDrawCommand();
		line.m_Vertices = pixels;
		line.m_iColor = colour;
		line.m_fWidth = 4;

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

		m_iMCFSaidCommands = -1;

		MCF_Say("map opened, mode " + config.MapEntityMode
			+ ", canvas " + (m_wMCFCanvas != null)
			+ ", radial " + (m_MCFRadial != null));
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

		if (m_bMCFFreehand && m_bMCFStroking)
			MCF_AddPoint();

		MCF_Redraw();

		// DOES HOLDING THE BUTTON SHOW UP AT ALL? The listeners only report
		// DOWN and UP, and those arrived four milliseconds apart every time --
		// but that is the listener's story, not necessarily the input's. This
		// asks the action for its raw value every frame instead, and reports
		// how many frames in a row it stayed pressed. One frame means the
		// engine really does not expose a held left button on this action and
		// click-click is the only honest interaction; a long run means it does
		// and drawing can follow the hand after all.
		if (MCF_DIAG && m_bMCFFreehand)
		{
			InputManager input = GetGame().GetInputManager();

			if (input && input.GetActionValue("MapSelect") > 0)
			{
				m_iMCFHeldFrames++;
			}
			else if (m_iMCFHeldFrames > 0)
			{
				MCF_Say("button was down for " + m_iMCFHeldFrames + " frames");
				m_iMCFHeldFrames = 0;
			}
		}
	}
}
