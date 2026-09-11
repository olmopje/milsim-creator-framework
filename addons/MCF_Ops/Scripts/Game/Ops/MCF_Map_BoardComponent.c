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

	//! The ballistic table carries this and its material reads it. Ours uses
	//! that mesh for now, so it carries it too, set to visible.
	int m_iOpacityMapId = 1;

	protected Widget m_wRoot;
	protected RTTextureWidget m_wRenderTarget;
	protected SCR_MapEntity m_MapEntity;
	protected bool m_bRaised;

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

		// OpenMap switches the character camera off -- an optimisation for a
		// map that fills the screen, which this one does not. Measured: the
		// map carries on perfectly well with it back on.
		PlayerController controller = GetGame().GetPlayerController();
		if (controller)
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
		}

		if (m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();

		m_MapEntity = null;

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;

		// MANDATORY, and the engine says so: the render target has to be taken
		// off the entity's mesh before the widget goes. Leaving it is a
		// dangling pointer into a deleted widget.
		if (m_wRenderTarget && owner && !owner.IsDeleted())
			m_wRenderTarget.RemoveRenderTarget(owner);

		m_wRenderTarget = null;

		super.OnDelete(owner);
	}
}
