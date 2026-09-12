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
//! WHERE THE CONTROLS LIVE. One entry in the map's own right-click menu, added
//! through SCR_MapRadialUI's GetOnMenuInitInvoker, which opens a window built
//! out of the same pieces as the game's marker edit box: vanilla's colour row,
//! vanilla's slider, vanilla's navigation buttons. Nothing of ours sits on the
//! screen uninvited, and nothing of ours is a control the player has to learn.
//!
//! WHY THE WINDOW AND NOT A RING OF COLOURED ENTRIES. Choosing a colour and a
//! thickness is setting up a pen, and the game already has a window for that
//! kind of choice -- the one a player uses to colour a marker. A radial ring
//! per colour also grows by one entry for every colour the game adds.
//!
//! AND WHY THE WINDOW CANNOT BE WHAT TURNS DRAWING OFF. Draw mode sets the
//! cursor's CS_DRAW, and CS_DRAW is in STATE_CTXMENU_RESTRICTED -- while it is
//! on, the right-click menu will not open, so nothing reached through that menu
//! can be the way out. Right-click is: it throws away the stroke in your hand,
//! and with no stroke in hand it puts the pencil down.
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

	//! How close the cursor has to be to a line, in SCREEN pixels, for delete
	//! to take it. In pixels rather than metres because it is a question about
	//! aiming with a mouse, and a mouse does not know what a metre is.
	protected static const float MCF_REACH_PIXELS = 14;

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

	//! The pen window, and the pieces of the game's own UI it is built from.
	protected static const ResourceName MCF_BOX_LAYOUT = "{6A1C4F0B39E17000}UI/layouts/MCF/MCF_MapDrawBox.layout";
	protected static const ResourceName MCF_COLOUR_ENTRY_LAYOUT = "{8A5D43FC8AC6C171}UI/layouts/Map/MapColorSelectorEntry.layout";

	protected Widget m_wMCFBox;
	protected SCR_SliderComponent m_MCFSlider;
	protected ref array<SCR_ButtonImageComponent> m_aMCFColourButtons = {};

	protected bool m_bMCFFreehand;
	protected bool m_bMCFStroking;
	protected int m_iMCFColour;

	//! Thickness in metres on the ground, straight off the slider.
	protected int m_iMCFWidth = 14;

	//! The stroke the cursor is over, recomputed every frame while a map is
	//! open. Drawn brighter so that "delete takes this one" is something you
	//! can see before you press the key rather than after.
	protected int m_iMCFHovered = -1;

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

		// ONE ENTRY, NOT A RING OF COLOURS. Picking a colour and a thickness is
		// setting up a pen, and the game already has a window for exactly that
		// kind of choice -- the one a player uses to give a marker its colour.
		// So this opens ours, built from the same pieces.
		SCR_SelectionMenuEntry draw = m_MCFRadial.AddRadialEntry("Draw a line", category);
		if (draw)
			draw.GetOnPerform().Insert(MCF_OnDrawPerformed);

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
	// THE PEN WINDOW
	//------------------------------------------------------------------------

	//! Opened a moment later, for the same reason drawing is: the radial is
	//! still closing and the cursor is still holding CS_CONTEXTUAL_MENU.
	protected void MCF_OnDrawPerformed(SCR_SelectionMenuEntry entry)
	{
		GetGame().GetCallqueue().CallLater(MCF_OpenBox, 150, false);
	}

	//------------------------------------------------------------------------
	//! MODELLED ON THE MARKER EDIT BOX, PIECE FOR PIECE. The colour row is
	//! vanilla's MapColorSelectorLine filled with vanilla's colour buttons,
	//! the thickness is vanilla's WLib_Slider, and the two buttons are
	//! vanilla's navigation buttons bound to MenuSelect and MenuBack -- so
	//! this reads as part of the game rather than as something bolted on.
	protected void MCF_OpenBox()
	{
		if (m_wMCFBox || !m_RootWidget)
			return;

		m_wMCFBox = GetGame().GetWorkspace().CreateWidgets(MCF_BOX_LAYOUT, m_RootWidget);
		if (!m_wMCFBox)
		{
			MCF_Core_Log.Warn("map drawing: the pen window would not load");
			return;
		}

		MCF_BuildColours();

		Widget sliderRoot = m_wMCFBox.FindAnyWidget("SliderRoot");
		if (sliderRoot)
		{
			m_MCFSlider = SCR_SliderComponent.Cast(sliderRoot.FindHandler(SCR_SliderComponent));

			if (m_MCFSlider)
			{
				m_MCFSlider.SetValue(m_iMCFWidth);
				m_MCFSlider.m_OnChanged.Insert(MCF_OnThicknessChanged);
			}
		}

		SCR_InputButtonComponent confirm = MCF_BoxButton("ButtonDraw");
		if (confirm)
			confirm.m_OnClicked.Insert(MCF_OnBoxConfirmed);

		SCR_InputButtonComponent cancel = MCF_BoxButton("ButtonCancel");
		if (cancel)
			cancel.m_OnClicked.Insert(MCF_OnBoxCancelled);

		// The cursor module has to know a dialog is up, or the map goes on
		// panning and selecting underneath it.
		if (m_CursorModule)
			m_CursorModule.HandleDialog(true);
	}

	//------------------------------------------------------------------------
	protected SCR_InputButtonComponent MCF_BoxButton(string name)
	{
		Widget widget = m_wMCFBox.FindAnyWidget(name);
		if (!widget)
			return null;

		return SCR_InputButtonComponent.Cast(widget.FindHandler(SCR_InputButtonComponent));
	}

	//------------------------------------------------------------------------
	//! One button per colour the game offers a marker, built the way vanilla
	//! builds its own: a MapColorSelectorEntry per colour, tinted.
	protected void MCF_BuildColours()
	{
		Widget line = m_wMCFBox.FindAnyWidget("ColorSelectorLine");
		if (!line)
			return;

		m_aMCFColourButtons.Clear();

		int colours = MCF_Map_DrawingComponent.ColourCount();

		for (int i = 0; i < colours; i++)
		{
			Widget button = GetGame().GetWorkspace().CreateWidgets(MCF_COLOUR_ENTRY_LAYOUT, line);
			if (!button)
				continue;

			button.SetName("MCF_ColorEntry" + i.ToString());

			SCR_ButtonImageComponent component = SCR_ButtonImageComponent.Cast(button.FindHandler(SCR_ButtonImageComponent));
			if (!component)
				continue;

			component.GetImageWidget().SetColorInt(MCF_Map_DrawingComponent.Colour(i));
			component.m_OnClicked.Insert(MCF_OnColourClicked);

			m_aMCFColourButtons.Insert(component);
		}

		MCF_ShowSelectedColour();
	}

	//------------------------------------------------------------------------
	protected void MCF_OnColourClicked(SCR_ButtonBaseComponent component)
	{
		int index = m_aMCFColourButtons.Find(SCR_ButtonImageComponent.Cast(component));

		if (index >= 0)
			m_iMCFColour = index;

		MCF_ShowSelectedColour();
	}

	//------------------------------------------------------------------------
	//! Which colour is chosen, said in the only way a row of colours can say
	//! it: the chosen one is full size and lit, the rest are dimmed.
	protected void MCF_ShowSelectedColour()
	{
		foreach (int i, SCR_ButtonImageComponent button : m_aMCFColourButtons)
		{
			Widget image = button.GetImageWidget();
			if (!image)
				continue;

			if (i == m_iMCFColour)
				image.SetOpacity(1);
			else
				image.SetOpacity(0.45);
		}
	}

	//------------------------------------------------------------------------
	protected void MCF_OnThicknessChanged(SCR_SliderComponent slider, float value)
	{
		m_iMCFWidth = Math.Round(value);
	}

	//------------------------------------------------------------------------
	protected void MCF_OnBoxConfirmed(SCR_InputButtonComponent button)
	{
		MCF_CloseBox();

		m_iMCFStartTries = 0;
		GetGame().GetCallqueue().CallLater(MCF_StartFreehand, 150, true);
	}

	//------------------------------------------------------------------------
	protected void MCF_OnBoxCancelled(SCR_InputButtonComponent button)
	{
		MCF_CloseBox();
	}

	//------------------------------------------------------------------------
	protected void MCF_CloseBox()
	{
		if (!m_wMCFBox)
			return;

		m_wMCFBox.RemoveFromHierarchy();
		m_wMCFBox = null;
		m_MCFSlider = null;
		m_aMCFColourButtons.Clear();

		if (m_CursorModule)
			m_CursorModule.HandleDialog(false);
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
				if (drawings)
					drawings.AskAdd(m_aMCFStroke, m_iMCFColour, m_iMCFWidth);
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
				int colour = MCF_Map_DrawingComponent.Colour(stroke.m_iColour);

				// The one under the cursor is drawn white, so the answer to
				// "which line will delete take?" is on the screen before the
				// key is pressed rather than after.
				if (stroke.m_iId == m_iMCFHovered)
					colour = 0xFFFFFFFF;

				MCF_AddCommand(stroke.m_aPoints, colour, stroke.m_iWidth);
			}
		}

		if (m_aMCFStroke.Count() >= 4)
			MCF_AddCommand(m_aMCFStroke, MCF_Map_DrawingComponent.Colour(m_iMCFColour), m_iMCFWidth);

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
	protected void MCF_AddCommand(notnull array<float> points, int colour, int widthMetres)
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
		//
		// AND THE WIDTH IS METRES TURNED INTO PIXELS, not a fixed number of
		// pixels. GetCurrentZoom() is pixels per metre -- vanilla's own map
		// line multiplies a world vector by it to get a length on screen -- so
		// a stroke thickens as the map is zoomed in, exactly the way its
		// length does. Clamped, because a line nobody can see when zoomed out
		// and a line that swallows the island when zoomed in are both useless.
		float width = widthMetres * m_MapEntity.GetCurrentZoom();
		width = Math.Clamp(width, 2, 60);

		LineDrawCommand halo = new LineDrawCommand();
		halo.m_Vertices = pixels;
		halo.m_iColor = 0xC0000000;
		halo.m_fWidth = width + 4;

		m_aMCFCommands.Insert(halo);

		LineDrawCommand line = new LineDrawCommand();
		line.m_Vertices = pixels;
		line.m_iColor = colour;
		line.m_fWidth = width;

		m_aMCFCommands.Insert(line);
	}

	//------------------------------------------------------------------------
	// DELETING ONE LINE
	//------------------------------------------------------------------------

	//! Which stroke the cursor is over, or -1.
	//!
	//! WHY OUR OWN HIT TEST AND NOT VANILLA'S. Everything hover-related on the
	//! map keys off WIDGETS -- GetMapWidgetsUnderCursor, then the marker
	//! behind the widget. A stroke is not a widget and has no position: it is
	//! a run of points drawn straight onto a canvas, so there is nothing under
	//! the cursor to find. The distance from the cursor to the nearest segment
	//! is the honest equivalent, and it is measured in metres and compared in
	//! pixels so that the reach is the same flick of the wrist at every zoom.
	protected int MCF_StrokeUnderCursor()
	{
		MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();
		if (!drawings || !m_MapEntity)
			return -1;

		float zoom = m_MapEntity.GetCurrentZoom();
		if (zoom <= 0)
			return -1;

		float cursorX, cursorZ;
		m_MapEntity.GetMapCursorWorldPosition(cursorX, cursorZ);

		float reach = MCF_REACH_PIXELS / zoom;
		float best = reach;
		int found = -1;

		foreach (MCF_Map_Stroke stroke : drawings.GetStrokes())
		{
			// A thick line is easier to hit than a thin one, the way a thick
			// line is easier to hit with a real pencil.
			float own = reach + stroke.m_iWidth * 0.5;

			float distance = MCF_DistanceToStroke(stroke, cursorX, cursorZ, own);

			if (distance >= 0 && distance < best)
			{
				best = distance;
				found = stroke.m_iId;
			}
		}

		return found;
	}

	//------------------------------------------------------------------------
	//! Distance in metres from a point to the nearest segment of a stroke, or
	//! -1 when it is further away than `limit` from all of them.
	protected float MCF_DistanceToStroke(notnull MCF_Map_Stroke stroke, float x, float z, float limit)
	{
		array<float> points = stroke.m_aPoints;
		float best = -1;

		for (int i = 0; i + 3 < points.Count(); i += 2)
		{
			float distance = MCF_DistanceToSegment(x, z, points[i], points[i + 1], points[i + 2], points[i + 3]);

			if (distance > limit)
				continue;

			if (best < 0 || distance < best)
				best = distance;
		}

		return best;
	}

	//------------------------------------------------------------------------
	protected float MCF_DistanceToSegment(float x, float z, float ax, float az, float bx, float bz)
	{
		float dx = bx - ax;
		float dz = bz - az;

		float lengthSq = dx * dx + dz * dz;

		float t = 0;

		if (lengthSq > 0)
		{
			t = ((x - ax) * dx + (z - az) * dz) / lengthSq;
			t = Math.Clamp(t, 0, 1);
		}

		float nearestX = ax + dx * t;
		float nearestZ = az + dz * t;

		float offX = x - nearestX;
		float offZ = z - nearestZ;

		return Math.Sqrt(offX * offX + offZ * offZ);
	}

	//------------------------------------------------------------------------
	//! The same key that deletes a marker under the cursor, doing the same
	//! thing to a line. Nothing new to learn and nothing new to bind.
	protected void MCF_OnDeletePressed(float value, EActionTrigger reason)
	{
		if (m_iMCFHovered < 0)
			return;

		MCF_Map_DrawingComponent drawings = MCF_Map_DrawingComponent.GetInstance();
		if (!drawings)
			return;

		foreach (MCF_Map_Stroke stroke : drawings.GetStrokes())
		{
			if (stroke.m_iId != m_iMCFHovered)
				continue;

			// Asked here as well as checked on the server, so that somebody
			// else's line simply does not respond rather than appearing to go
			// and coming back a moment later.
			if (MCF_Map_DrawingComponent.MayRemove(stroke))
				drawings.AskRemove(stroke.m_iId);

			return;
		}
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

		// THE SAME KEY THAT DELETES A MARKER. Vanilla registers this exact
		// action to delete the marker under the cursor; a line under the
		// cursor now answers to it too, so there is nothing extra to learn.
		GetGame().GetInputManager().AddActionListener("MapMarkerDelete", EActionTrigger.DOWN, MCF_OnDeletePressed);

		m_iMCFHovered = -1;
		m_iMCFSaidCommands = -1;

		MCF_Say("map opened, mode " + config.MapEntityMode
			+ ", canvas " + (m_wMCFCanvas != null)
			+ ", radial " + (m_MCFRadial != null));
	}

	//------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		GetGame().GetCallqueue().Remove(MCF_StartFreehand);
		GetGame().GetInputManager().RemoveActionListener("MapMarkerDelete", EActionTrigger.DOWN, MCF_OnDeletePressed);
		GetGame().GetCallqueue().Remove(MCF_OpenBox);

		MCF_CloseBox();
		MCF_SetFreehand(false);

		m_iMCFHovered = -1;

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

		// Not while drawing: the line following your hand is always the
		// nearest one, so highlighting it would make delete look armed the
		// whole time you are drawing.
		if (m_bMCFStroking)
			m_iMCFHovered = -1;
		else
			m_iMCFHovered = MCF_StrokeUnderCursor();

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
