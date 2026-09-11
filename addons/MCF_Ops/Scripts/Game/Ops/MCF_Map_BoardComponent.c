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
//! SCR_MapEntity opened into it. This works: the board shows the world's real
//! map, the one with everybody's markers on it.
//!
//! THE MAP IS NOT OURS TO KEEP, and that cost a day to find. SCR_MapEntity is
//! a singleton with one open map. Two to four seconds after a board comes up,
//! something else opens one -- the spawn screen, the Game Master, a player's
//! own map -- and SCR_MapEntity closes ours to make room. Nothing ever hands
//! it back. So the board watches, and takes the map the moment nobody else is
//! holding it. It never takes it FROM anyone: if a map is open, it is
//! somebody's, and we wait. A board is blank exactly while someone is reading
//! a map, which is exactly when nobody is looking at the board.
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

	[Attribute(defvalue: "40", uiwidget: UIWidgets.EditBox, desc: "How close a viewer has to be, in metres, for the map to be drawn at all. Beyond this the board is a blank white board and costs nothing.", params: "0 1000")]
	protected float m_fActivationDistance;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Over how many metres the map fades out to white before the activation distance. 5 means it starts going at 35 m and is white at 40 m.", params: "0 200")]
	protected float m_fFadeBand;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Seconds between refreshes. The board grabs the map for about a second, draws one picture and gives it straight back -- it never holds the map, because while it does nobody can open theirs.", params: "1 120")]
	protected float m_fRefreshSeconds;

	//! How often the board looks at where the viewer is and whose turn it is.
	protected static const int TICK_MS = 250;

	//! How many of those ticks one picture takes. FitBoard runs half a second
	//! after the open, so this has to be comfortably more than that.
	protected static const int HOLD_TICKS = 6;

	protected Widget m_wRoot;
	protected RTTextureWidget m_wRenderTarget;

	//! The sheet of white over the map. A map has no opacity of its own to
	//! turn down, and dimming the board's material would take the frame with
	//! it, so the fade is a panel on top of the map inside the texture.
	protected Widget m_wWhiteout;
	protected float m_fWhite = -1;

	protected SCR_MapEntity m_MapEntity;
	protected bool m_bRaised;

	//! The widget the map was opened into, so the watchdog can tell ours from
	//! somebody else's.
	protected CanvasWidget m_wMapWidget;

	//! Whether the board is currently holding the map to draw a picture.
	protected bool m_bHolding;

	//! Ticks spent in whichever of the two states we are in.
	protected int m_iTicks;

	//! Ticks of drawing still owed for a change that is not the map -- the
	//! fade. The texture keeps whatever was last drawn into it, so a board
	//! that is fading has to be redrawn even when it holds no map.
	protected int m_iPaint;

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
		// there is nothing here anyone else needs to be told about, and every
		// client works out for itself how far away it is standing.
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

		m_wWhiteout = m_wRoot.FindAnyWidget("Whiteout");

		m_wRenderTarget.SetRenderTarget(owner);

		// The two dials the engine hands over for exactly this, and the reason
		// a board is affordable at all. A map is still; ten frames a second at
		// half resolution is more than it needs.
		if (m_fResolutionScale > 0 && m_fResolutionScale < 1)
		{
			m_wRenderTarget.SetResolutionScale(m_fResolutionScale);
			m_wRenderTarget.ToggleFSR(true);
		}

		// Off until there is something to draw. Acquire turns it on.
		m_wRenderTarget.SetEnabled(false);
		m_bRaised = true;

		// White until proven near. A board that flashes the whole island for
		// one frame before deciding nobody is looking is worse than one that
		// takes a quarter of a second to light up.
		Whiten(1);

		GetGame().GetCallqueue().CallLater(Watch, TICK_MS, true);
	}

	//------------------------------------------------------------------------
	//! Four times a second: how far away is the viewer, and whose turn it is
	//! to hold the map.
	//!
	//! A BOARD MUST NOT KEEP THE MAP, and that is not politeness, it is the
	//! only way the rest of the game still works. There is one map, and while
	//! a board holds it open the player's own M does nothing -- whatever
	//! reads "is the map open" to decide has already been told yes.
	//!
	//! So the board TAKES A PICTURE instead of holding a window. It grabs the
	//! map for about a second, lets it draw, and gives it straight back; the
	//! render target keeps the last frame it drew, so the board goes on
	//! showing that picture until the next refresh. Which is what a map on a
	//! wall is anyway -- paper does not update at ten frames a second, and
	//! five seconds is fast enough for markers.
	protected void Watch()
	{
		Decide();
		Paint();
	}

	//------------------------------------------------------------------------
	protected void Decide()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_bRaised)
			return;

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();

		if (!m_MapEntity)
			return;

		Whiten(FadeFor(ViewerDistance(owner)));

		// Fully white means there is nothing to see and nothing to pay for.
		bool wanted = m_fWhite < 1;

		bool open = m_MapEntity.IsOpen();
		bool ours = open && m_MapEntity.GetMapWidget() == m_wMapWidget;

		if (m_bHolding)
		{
			// Taken off us mid-picture: somebody opened their own map. Their
			// turn; we go back to waiting and keep whatever we had drawn.
			if (!ours)
			{
				m_bHolding = false;
				m_iTicks = 0;
				MCF_Core_Log.Debug("map board: the map was taken mid-refresh");
				return;
			}

			m_iTicks++;

			if (!wanted || m_iTicks >= HOLD_TICKS)
				Release();

			return;
		}

		// ---- resting, showing the last picture
		if (!wanted)
			return;

		// Somebody else is reading a map. Never take it off them.
		if (open)
			return;

		m_iTicks++;

		if (m_iTicks * TICK_MS < m_fRefreshSeconds * 1000)
			return;

		Acquire(owner);
	}

	//------------------------------------------------------------------------
	//! ONE PLACE DECIDES WHETHER THE BOARD IS BEING DRAWN, because the
	//! texture keeps whatever was put in it last and two callers arguing over
	//! SetEnabled is how a board ends up frozen on the wrong frame.
	protected void Paint()
	{
		bool draw = m_bHolding || m_iPaint > 0;

		if (m_iPaint > 0)
			m_iPaint--;

		if (m_wRenderTarget)
			m_wRenderTarget.SetEnabled(draw);
	}

	//------------------------------------------------------------------------
	//! Take the map for one picture.
	protected void Acquire(IEntity owner)
	{
		m_bHolding = true;
		m_iTicks = 0;

		OpenMapOntoBoard(owner);
	}

	//------------------------------------------------------------------------
	//! Give it back, and stop redrawing so the texture keeps the frame.
	protected void Release()
	{
		m_bHolding = false;
		m_iTicks = 0;

		if (m_MapEntity && m_MapEntity.IsOpen() && m_MapEntity.GetMapWidget() == m_wMapWidget)
			m_MapEntity.CloseMap();

		// THE PICTURE HAS TO SURVIVE THIS. The tick below stops the render
		// pass, and the texture the board samples is expected to keep the last
		// frame drawn into it. If the board goes blue or black instead, that
		// expectation is wrong and the refresh has to keep a copy another way.
	}

	//------------------------------------------------------------------------
	//! How white the board should be at this distance: 0 near, 1 past the
	//! activation distance, walked evenly across the fade band in between.
	protected float FadeFor(float distance)
	{
		if (m_fActivationDistance <= 0)
			return 0;

		if (distance >= m_fActivationDistance)
			return 1;

		float begins = m_fActivationDistance - m_fFadeBand;

		if (m_fFadeBand <= 0 || distance <= begins)
			return 0;

		return (distance - begins) / m_fFadeBand;
	}

	//------------------------------------------------------------------------
	//! Metres from the board to whoever is looking.
	//!
	//! THE CAMERA, NOT THE CHARACTER, because a Game Master flying around has
	//! no character where their eyes are, and a board should light up for the
	//! camera that is actually pointed at it.
	protected float ViewerDistance(notnull IEntity owner)
	{
		vector eye;
		bool found = false;

		CameraManager cameras = GetGame().GetCameraManager();
		if (cameras)
		{
			CameraBase camera = cameras.CurrentCamera();
			if (camera)
			{
				eye = camera.GetOrigin();
				found = true;
			}
		}

		if (!found)
		{
			PlayerController controller = GetGame().GetPlayerController();
			if (controller)
			{
				IEntity player = controller.GetControlledEntity();
				if (player)
				{
					eye = player.GetOrigin();
					found = true;
				}
			}
		}

		// No viewer to measure against is not a reason to blank the board.
		if (!found)
			return 0;

		return vector.Distance(eye, owner.GetOrigin());
	}

	//------------------------------------------------------------------------
	protected void Whiten(float white)
	{
		if (Math.AbsFloat(white - m_fWhite) < 0.002)
			return;

		m_fWhite = white;

		if (m_wWhiteout)
			m_wWhiteout.SetOpacity(white);

		// The fade changed, so the picture has to be drawn again even if no
		// map is held. Three ticks is enough for the change to land.
		m_iPaint = 3;

		if (!m_wRenderTarget)
			return;

		// A sheet of plain white does not need ten frames a second.
		if (white >= 1)
			m_wRenderTarget.SetMaxFPS(1);
		else if (m_iFramesPerSecond > 0)
			m_wRenderTarget.SetMaxFPS(m_iFramesPerSecond);
	}

	//------------------------------------------------------------------------
	//! Opens the game's map into the board's own widget.
	//!
	//! THE CONFIGURATION IS BUILT HERE rather than taken from SetupMapConfig,
	//! for two reasons that both matter. A board wants a map and nothing else
	//! -- no cursor, no tool menu, no ruler -- and the components that come
	//! with the gadget config go looking for widgets a board has no reason to
	//! own and throw once a frame when they miss. And SetupMapConfig hands
	//! back the shared m_ActiveMapCfg and rewrites its root widget, which is
	//! the open map's configuration being altered under it.
	protected void OpenMapOntoBoard(IEntity owner)
	{
		if (!m_MapEntity)
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
		m_wMapWidget = m_MapEntity.GetMapWidget();

		// OpenMap switches the character camera's render off, which is right
		// for a map that fills the screen and wrong for one on a board: leave
		// it off and the player is looking at nothing.
		PlayerController controller = GetGame().GetPlayerController();
		if (controller)
			controller.SetCharacterCameraRenderActive(true);

		GetGame().GetCallqueue().CallLater(FitBoard, 500, false);
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
	//!
	//! Half a second late, because SCR_MapEntity counts down FRAME_DELAY
	//! frames after an open before it accepts a zoom or a pan.
	protected void FitBoard()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		if (m_MapEntity.GetMapWidget() != m_wMapWidget)
			return;

		m_MapEntity.ZoomOut();
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
		// Every one of this component's timers points at a method on an object
		// that is about to stop existing.
		ScriptCallQueue callqueue = GetGame().GetCallqueue();
		if (callqueue)
		{
			callqueue.Remove(Raise);
			callqueue.Remove(Watch);
			callqueue.Remove(FitBoard);
		}

		// Only ours. Closing a map somebody else opened would blank their
		// screen because a board was deleted.
		if (m_MapEntity && m_MapEntity.IsOpen() && m_MapEntity.GetMapWidget() == m_wMapWidget)
			m_MapEntity.CloseMap();

		m_MapEntity = null;
		m_wMapWidget = null;

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wWhiteout = null;

		// MANDATORY, and the engine says so: the render target has to be taken
		// off the entity's mesh before the widget goes. Leaving it is a
		// dangling pointer into a deleted widget.
		if (m_wRenderTarget && owner && !owner.IsDeleted())
			m_wRenderTarget.RemoveRenderTarget(owner);

		m_wRenderTarget = null;

		super.OnDelete(owner);
	}
}
