//! A board in the world with the game's own map on it.
//!
//! HOW A UI GETS ONTO A SURFACE, which is vanilla's recipe and not a trick --
//! SCR_DataDisplayGadget does exactly this to put a ballistic page on the
//! range table you hold in your hands:
//!
//!   1. CreateWidgets(layout) with NO PARENT. The tree lives outside the
//!      screen hierarchy, so it is never drawn to the screen.
//!   2. RTTextureWidget.SetRenderTarget(owner) -- the entity's material can
//!      now sample it as $rendertarget.
//!   3. RemoveRenderTarget(owner) on the way out. Not optional; the engine's
//!      own comment says it must be called when the widget goes away.
//!
//! WHAT IS OURS RATHER THAN THEIRS is what goes in the tree: a MapWidget, and
//! SCR_MapEntity opened into it. That was measured before it was built --
//! see docs/research/map-board-and-drawing.md for the numbers, including the
//! one that settles it: a map widget inside a render target moves from the
//! layout's own PixelPerUnit to the map's the moment the map is opened into
//! it, so the map really does drive a widget in there.
//!
//! THE ONE HARD LIMIT, and it is the engine's: there is one SCR_MapEntity and
//! one map open at a time. A board holding the map means the player opening
//! theirs takes it; closing theirs gives it back. The board is blank for as
//! long as somebody has their own map up, which is the moment they are not
//! looking at the board -- liveable, but it is a fact about the design and
//! not a bug to be chased.
[ComponentEditorProps(category: "MCF/Ops", description: "Shows the world's own map on this object's surface.")]
class MCF_Map_BoardComponentClass : ScriptComponentClass
{
}

class MCF_Map_BoardComponent : ScriptComponent
{
	[Attribute(defvalue: "{6A1C4F0B39E11000}UI/layouts/MCF/MCF_MapBoardRender.layout", uiwidget: UIWidgets.ResourceNamePicker, desc: "What gets drawn onto the board.", params: "layout")]
	protected ResourceName m_sLayout;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "How often the board redraws, in frames per second. A map does not move on its own, so this can be low -- it is the main thing a board costs.")]
	protected int m_iFramesPerSecond;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Render scale, 0.1 to 1. Below 1 the board is drawn smaller and upscaled, which is most of the rest of what it costs.", params: "0.1 1")]
	protected float m_fResolutionScale;

	//! DIAGNOSTIC, AND IT ANSWERS ONE QUESTION. The board draws its clear
	//! colour and nothing else, which proves the render target works and
	//! leaves exactly one suspect: whether the map's terrain is drawn into
	//! the widget it was given, or into the screen. Put the same widget tree
	//! ON the screen and look:
	//!
	//!   map appears on screen  -> it renders to the screen pass and a render
	//!                             target can never catch it; the board has
	//!                             to be fed some other way.
	//!   still only blue        -> the map is not drawing at all and the
	//!                             fault is in the configuration we build.
	//!
	//! Off for a real board -- a 1024 x 700 panel over the corner of the
	//! screen is not something anybody wants twice.
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Debug only: also hang the board's widget tree on the screen, to see whether the map draws there.")]
	protected bool m_bDebugOnScreen;

	//! MEASURED IN GAME, AND IT NARROWS THE WHOLE PROBLEM. With the Game
	//! Master's editor camera the map draws; the moment the same player
	//! spawns into a character it goes blank. The only thing that separates
	//! those two states is the character camera, which OpenMap deliberately
	//! switches off and which this component switched back on.
	//!
	//! So the line in OpenMap is very likely not an optimisation at all: the
	//! map looks like a top-down render of the world, and the engine renders
	//! one scene view at a time -- the character's, or the map's.
	//!
	//! Turning this off proves it. The world stops being drawn, which is why
	//! nobody would ship it that way; it is here to settle the question.
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Keep the character camera rendering. Off means the world is not drawn at all -- a test, not a setting.")]
	protected bool m_bKeepCharacterCamera;

	//! The ballistic table carries this and its material reads it. Ours uses
	//! that mesh for now, so it carries it too, set to visible.
	int m_iOpacityMapId = 1;

	//! The diagnostic host: the same map widget with no render target around
	//! it, hung on the screen. Only ever made when m_bDebugOnScreen is set.
	protected static const ResourceName SCREEN_LAYOUT = "{6A1C4F0B39E11010}UI/layouts/MCF/MCF_MapBoardScreen.layout";

	protected Widget m_wRoot;
	protected Widget m_wScreenRoot;
	protected RTTextureWidget m_wRenderTarget;
	protected SCR_MapEntity m_MapEntity;
	protected bool m_bRaised;

	//! The widget the map was opened into, so the watchdog can tell ours
	//! from somebody else's.
	protected CanvasWidget m_wMapWidget;

	//! Whether the map has been taken off us. Kept so the log says it once
	//! rather than every two seconds.
	protected bool m_bLost;

	//------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		// A dedicated server has no workspace and nothing to draw into. This
		// is a client-side picture and nothing else -- no replication, because
		// there is nothing here anyone else needs to be told about.
		if (!GetGame().GetWorkspace())
			return;

		// A moment, because SetRenderTarget wants the object's mesh ready and
		// the map wants its world. A board that comes up a second late is a
		// board nobody notices coming up.
		GetGame().GetCallqueue().CallLater(Raise, 1000, false, owner);
	}

	//------------------------------------------------------------------------
	protected void Raise(IEntity owner)
	{
		if (!owner || m_bRaised)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		if (m_sLayout.IsEmpty())
		{
			MCF_Core_Log.Warn("map board: no layout named, so there is nothing to draw");
			return;
		}

		// NO PARENT. That is the whole reason this never appears on screen.
		m_wRoot = workspace.CreateWidgets(m_sLayout);
		if (!m_wRoot)
		{
			MCF_Core_Log.Warn("map board: the render layout would not load");
			return;
		}

		m_wRenderTarget = RTTextureWidget.Cast(m_wRoot.FindAnyWidget("RTTexture0"));
		if (!m_wRenderTarget)
		{
			MCF_Core_Log.Warn("map board: the layout carries no RTTexture0");
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
			return;
		}

		m_wRenderTarget.SetRenderTarget(owner);

		// The two dials the engine hands over for exactly this, and the reason
		// a board is affordable at all. A map is still; ten frames a second at
		// half resolution is more than it needs.
		if (m_iFramesPerSecond > 0)
			m_wRenderTarget.SetMaxFPS(m_iFramesPerSecond);

		if (m_fResolutionScale > 0 && m_fResolutionScale < 1)
		{
			m_wRenderTarget.SetResolutionScale(m_fResolutionScale);
			m_wRenderTarget.ToggleFSR(true);
		}

		m_wRenderTarget.SetEnabled(true);
		m_bRaised = true;

		// The diagnostic host, on the screen, with nothing between the map
		// widget and the eye. The workspace is a Widget, so it can be a
		// parent -- and being in its hierarchy is exactly the difference
		// between a tree that is drawn and one that is only captured.
		if (m_bDebugOnScreen)
		{
			m_wScreenRoot = workspace.CreateWidgets(SCREEN_LAYOUT, workspace);

			if (!m_wScreenRoot)
				MCF_Core_Log.Warn("map board: the screen debug layout would not load");
		}

		OpenMapOntoBoard(owner);
	}

	//------------------------------------------------------------------------
	//! Opens the game's map into the board's own widget.
	//!
	//! THE CONFIGURATION IS BUILT HERE rather than taken from
	//! SetupMapConfig, for two reasons that both matter. A board wants a map
	//! and nothing else -- no cursor, no tool menu, no ruler -- and the
	//! components that come with the gadget config go looking for widgets a
	//! board has no reason to own and throw once a frame when they miss. And
	//! SetupMapConfig hands back the shared m_ActiveMapCfg and rewrites its
	//! root widget, which is the open map's configuration being altered under
	//! it.
	protected void OpenMapOntoBoard(IEntity owner)
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity)
		{
			MCF_Core_Log.Warn("map board: this world has no map entity, so there is no map to show");
			return;
		}

		MapConfiguration config = new MapConfiguration();

		// The diagnostic host wins when it exists, because the whole point of
		// it is to see what this same map does with no render target in the way.
		if (m_wScreenRoot)
			config.RootWidgetRef = m_wScreenRoot;
		else
			config.RootWidgetRef = m_wRenderTarget;
		config.MapEntityMode = EMapEntityMode.MINIMAP;
		config.Modules = {};
		config.Components = {};
		config.OtherComponents = 0;

		config.LayerConfig = SCR_MapLayersBase.Cast(LoadConfig(SCR_MapConstants.CFG_LAYERS_DEFAULT));
		config.MapPropsConfig = SCR_MapPropsBase.Cast(LoadConfig(SCR_MapConstants.CFG_PROPS_DEFAULT));
		config.DescriptorDefsConfig = SCR_MapDescriptorDefaults.Cast(LoadConfig(SCR_MapConstants.CFG_DESCTYPES_DEFAULT));
		config.DescriptorVisibilityConfig = SCR_MapDescriptorVisibilityBase.Cast(LoadConfig(SCR_MapConstants.CFG_DESCVIEW_DEFAULT));

		if (!config.LayerConfig)
		{
			MCF_Core_Log.Warn("map board: the default layers config would not load, and without layers there is no map");
			return;
		}

		config.LayerCount = config.LayerConfig.m_aLayers.Count();

		m_MapEntity.OpenMap(config);
		m_wMapWidget = m_MapEntity.GetMapWidget();

		// OpenMap switches the character camera off, and putting it back was
		// the first guess. It is now the prime suspect instead: see
		// m_bKeepCharacterCamera. With the editor camera the map draws, with
		// a character it does not, and this line is the difference.
		PlayerController controller = GetGame().GetPlayerController();
		if (controller && m_bKeepCharacterCamera)
			controller.SetCharacterCameraRenderActive(true);

		// Not now: SCR_MapEntity counts down FRAME_DELAY frames after an open
		// before it will accept a zoom or a pan, and says so in the log if you
		// ask early. Half a second is a long time in frames and nothing at all
		// to a board that takes a second to come up anyway.
		GetGame().GetCallqueue().CallLater(FitBoard, 500, false);

		MCF_Core_Log.Debug("map board raised with " + config.LayerCount.ToString() + " layer(s)");
	}

	//------------------------------------------------------------------------
	//! The whole world on the board, once the map will listen.
	//!
	//! A BOARD IS NOT A MAP A PLAYER IS DRIVING. It opens at whatever zoom the
	//! map happened to be left at, which for a thing hanging on a wall is the
	//! wrong answer every time -- you walk up to a wall map to see where
	//! everything is, not to read one grid square.
	//!
	//! ZoomOut() is vanilla's own: minimum zoom and then CenterMap(). Minimum
	//! zoom is computed in UpdateZoomBounds as screen height over map size in
	//! metres, so it is exactly "the whole island, fitted to the height" --
	//! which is why the widget's spare width shows the map's sea colour.
	protected void FitBoard()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		m_MapEntity.ZoomOut();

		MCF_Core_Log.Debug("map board fitted at zoom " + m_MapEntity.GetCurrentZoom().ToString());

		GetGame().GetCallqueue().CallLater(DumpBoard, 1500, false);

		// THE MAP IS NOT OURS TO KEEP. SCR_MapEntity is a singleton with one
		// open map, and the log says plainly what happens: two to four
		// seconds after the board comes up, something else opens a map --
		// the spawn screen, the Game Master, the player's own -- and
		// SCR_MapEntity closes ours to make room. The board goes blank and
		// stays blank, because nothing ever gives it back.
		//
		// So the board watches, and takes the map back the moment nobody
		// else is holding it. It never takes it FROM anyone: if a map is
		// open, it is somebody's and we wait. That is the honest shape of
		// this feature -- the board is live whenever no one is reading a map,
		// which is exactly when anybody is looking at the board.
		GetGame().GetCallqueue().CallLater(Watch, 2000, true);
	}

	//------------------------------------------------------------------------
	protected void Watch()
	{
		if (!m_MapEntity)
			return;

		if (m_MapEntity.IsOpen())
		{
			// Open, but is it ours? A widget that is not the one in our tree
			// means somebody else is holding the map, and we leave it alone.
			CanvasWidget held = m_MapEntity.GetMapWidget();

			if (held != m_wMapWidget && !m_bLost)
			{
				m_bLost = true;

				float w, h;
				if (held)
					held.GetScreenSize(w, h);

				MCF_Core_Log.Warn("map board: the map was taken, now drawn into a "
					+ w.ToString() + " x " + h.ToString() + " widget");
			}

			return;
		}

		// Nobody is holding it. Take it back.
		if (!m_bLost)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		m_bLost = false;
		MCF_Core_Log.Warn("map board: the map is free again, reclaiming it");
		OpenMapOntoBoard(owner);
	}

	//------------------------------------------------------------------------
	//! Every number the board's picture depends on, in one line.
	//!
	//! The board shows its clear colour and nothing else, so the render target
	//! demonstrably works and the map demonstrably does not draw into it. This
	//! prints the things that could still be wrong before the pixels: whether
	//! the widget has a size at all, where the engine thinks it is on the
	//! screen, what zoom it settled on, and which patch of world it has been
	//! told to draw. A visible frame that is off the island, or a zero size,
	//! is a bug we can fix; correct numbers with a blank board mean the map
	//! is not rendered in this pass and the feature needs another route.
	protected void DumpBoard()
	{
		if (!m_MapEntity)
			return;

		// NOT "map": the compiler reserves it for the container type, and the
		// error it gives is about a variable name rather than a type.
		CanvasWidget mapWidget = m_MapEntity.GetMapWidget();
		if (!mapWidget)
		{
			MCF_Core_Log.Warn("map board: the map entity holds no map widget");
			return;
		}

		float sizeX, sizeY, posX, posY;
		mapWidget.GetScreenSize(sizeX, sizeY);
		mapWidget.GetScreenPos(posX, posY);

		vector frameMin, frameMax;
		m_MapEntity.GetMapVisibleFrame(frameMin, frameMax);

		// One + chain of this length is "Formula too complex" to the Enforce
		// compiler, so it is built in pieces.
		string line = "map board dump | open " + m_MapEntity.IsOpen().ToString();
		line = line + " | widget " + sizeX.ToString() + " x " + sizeY.ToString();
		line = line + " at " + posX.ToString() + "," + posY.ToString();
		line = line + " | ppu " + mapWidget.PixelPerUnit().ToString();
		line = line + " | zoom " + m_MapEntity.GetCurrentZoom().ToString();
		line = line + " (min " + m_MapEntity.GetMinZoom().ToString();
		line = line + ", max " + m_MapEntity.GetMaxZoom().ToString() + ")";
		line = line + " | world " + m_MapEntity.GetMapSizeX().ToString() + " m";
		line = line + " | frame " + frameMin.ToString() + " .. " + frameMax.ToString();
		line = line + " | on screen " + m_bDebugOnScreen.ToString();

		if (m_wScreenRoot)
		{
			line = line + " | screen host parented " + (m_wScreenRoot.GetParent() != null).ToString();
			line = line + ", visible " + m_wScreenRoot.IsVisibleInHierarchy().ToString();
		}

		MCF_Core_Log.Warn(line);
	}

	//------------------------------------------------------------------------
	protected Managed LoadConfig(ResourceName path)
	{
		Resource container = BaseContainerTools.LoadContainer(path);
		if (!container)
			return null;

		return BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer());
	}

	//------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		// Both of this component's timers point at a method on an object that
		// is about to stop existing.
		ScriptCallQueue callqueue = GetGame().GetCallqueue();
		if (callqueue)
		{
			callqueue.Remove(Raise);
			callqueue.Remove(FitBoard);
			callqueue.Remove(DumpBoard);
			callqueue.Remove(Watch);
		}

		if (m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();

		m_MapEntity = null;

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;

		if (m_wScreenRoot)
			m_wScreenRoot.RemoveFromHierarchy();

		m_wScreenRoot = null;

		// MANDATORY, and the engine says so: the render target has to be taken
		// off the entity's mesh before the widget goes. Leaving it is a
		// dangling pointer into a deleted widget.
		if (m_wRenderTarget && owner && !owner.IsDeleted())
			m_wRenderTarget.RemoveRenderTarget(owner);

		m_wRenderTarget = null;

		super.OnDelete(owner);
	}
}
