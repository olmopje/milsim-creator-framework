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
//! THE BOARD DOES NOT OPEN THE MAP, and that is the whole design. There is
//! one SCR_MapEntity and one open map, and while a board holds it open the
//! player's M does the wrong thing -- whatever asks "is a map open" has
//! already been told yes. Handing it back does not work either: closing the
//! map calls EnableVisualisation(false) and the board goes blank.
//!
//! So the board splits the two things SCR_MapEntity does at once:
//!
//!   OPEN is bookkeeping -- m_bIsOpen, the widget it panned last, the
//!   modules it activated, the character camera. That is the PLAYER'S, and
//!   the board leaves it alone.
//!
//!   DRAWING is three native calls on the entity: EnableVisualisation(true),
//!   SetFrame(worldRect), and the layer. Those have nothing to do with
//!   m_bIsOpen, and a MapWidget draws whenever they are set.
//!
//! The board therefore opens the map ONCE, at birth, to have SCR_MapEntity
//! do the setup no one should retype -- layers, props, descriptors, the
//! widget's SizeInUnits, the zoom -- reads the island's frame off it, and
//! closes it again. From then on it never touches the map's state: it only
//! keeps visualisation on and re-states its own frame. IsOpen() stays false,
//! so M works, and the board keeps drawing.
//!
//! While somebody's real map IS open the board touches nothing at all. Their
//! frame is the entity's frame for those seconds and the board follows it --
//! liveable, and the alternative is fighting them for it every frame.
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

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Draw the map's grid on the board. The grid belongs to the map entity rather than to this board, so two boards that disagree about it will take turns winning.")]
	protected bool m_bShowGrid;

	//! How often the board looks at where the viewer is and whether it still
	//! has to say "keep drawing".
	protected static const int TICK_MS = 250;

	protected Widget m_wRoot;
	protected RTTextureWidget m_wRenderTarget;

	//! The sheet of white over the map. A map has no opacity of its own to
	//! turn down, and dimming the board's material would take the frame with
	//! it, so the fade is a panel on top of the map inside the texture.
	protected Widget m_wWhiteout;
	protected float m_fWhite = -1;

	protected SCR_MapEntity m_MapEntity;
	protected bool m_bRaised;

	//! Whether the one-off setup open has happened and the island's frame is
	//! known. Until then the board has nothing to re-state.
	protected bool m_bPrimed;
	protected bool m_bPriming;

	//! THE BOARD'S OWN VIEW, in the four numbers the map entity keeps rather
	//! than the widget. Read once, off the fitted map, and put back every
	//! tick -- which is what makes the board independent of whatever a player
	//! last did with their own map.
	protected float m_fZoomLevel;
	protected vector m_vPan;
	protected int m_iLayer = -1;

	//! The world rectangle the board draws, read off the map after it was
	//! zoomed out to fit the island.
	protected vector m_vFrameMin;
	protected vector m_vFrameMax;

	//! The widget the setup open ran against, so we can tell our own map
	//! apart from a player's.
	protected CanvasWidget m_wMapWidget;

	//! Whether we are the reason visualisation is on.
	protected bool m_bVisualising;

	//! Ticks of drawing still owed for something that is not the map -- the
	//! fade. The texture keeps whatever was last drawn into it.
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
		if (m_iFramesPerSecond > 0)
			m_wRenderTarget.SetMaxFPS(m_iFramesPerSecond);

		if (m_fResolutionScale > 0 && m_fResolutionScale < 1)
		{
			m_wRenderTarget.SetResolutionScale(m_fResolutionScale);
			m_wRenderTarget.ToggleFSR(true);
		}

		m_bRaised = true;

		// White until proven near. A board that flashes the whole island for
		// one frame before deciding nobody is looking is worse than one that
		// takes a quarter of a second to light up.
		Whiten(1);

		GetGame().GetCallqueue().CallLater(Watch, TICK_MS, true);
	}

	//------------------------------------------------------------------------
	protected void Watch()
	{
		Decide();
		Paint();
	}

	//------------------------------------------------------------------------
	//! Four times a second: how far away is the viewer, and does the board
	//! still have to tell the map to draw.
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

		// SOMEBODY'S REAL MAP IS OPEN. Their frame, their zoom, their grid --
		// the board is not worth taking any of it off them, and it is only
		// for as long as they are reading. We touch nothing.
		//
		// OURS, during the one-off setup, is not somebody's: that is what the
		// widget comparison is for. Reading our own open as a player's is
		// what made the board prime itself over and over, once a second,
		// forever -- and an open map four times a second is also exactly what
		// stops M from working.
		if (m_MapEntity.IsOpen())
		{
			if (m_MapEntity.GetMapWidget() != m_wMapWidget)
				m_bVisualising = false;

			return;
		}

		if (m_bPriming)
			return;

		if (!m_bPrimed)
		{
			if (wanted)
				Prime();

			return;
		}

		if (!wanted)
		{
			// Nobody near. Stop drawing the world for a board nobody can see,
			// but only if we are the one who asked for it.
			if (m_bVisualising)
			{
				m_MapEntity.EnableVisualisation(false);
				m_bVisualising = false;
			}

			return;
		}

		// THE BOARD'S OWN VIEW, RE-STATED. Zoom, pan, layer and grid live on
		// the map entity rather than on the widget, so a player who closes
		// their map leaves all four wherever they finished. Rather than
		// setting the board up again every time that happens -- which is a
		// second of flicker and a second of the map being open -- the board
		// remembers its own four numbers and puts them back.
		//
		// None of these touches m_bIsOpen, so as far as the rest of the game
		// is concerned no map is open and M does what it always did.
		m_MapEntity.EnableVisualisation(true);

		if (m_iLayer >= 0 && m_MapEntity.GetLayerIndex() != m_iLayer)
			m_MapEntity.SetLayer(m_iLayer);

		m_MapEntity.EnableGrid(m_bShowGrid);

		if (m_fZoomLevel > 0)
			m_MapEntity.ZoomChange(m_fZoomLevel);

		m_MapEntity.PosChange(m_vPan[0], m_vPan[1]);
		m_MapEntity.SetFrame(m_vFrameMin, m_vFrameMax);

		m_bVisualising = true;
	}

	//------------------------------------------------------------------------
	//! The one-off setup open.
	//!
	//! SCR_MapEntity.OpenMap does a page of work that nobody should retype:
	//! it initialises the layers from the config, applies the map props and
	//! the descriptor defaults, gives our widget its SizeInUnits in metres,
	//! and works out the zoom bounds. We want all of it -- we just do not
	//! want the map left open afterwards. So: open, fit, read the frame,
	//! close.
	protected void Prime()
	{
		m_bPriming = true;

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
			m_bPriming = false;
			return;
		}

		config.LayerCount = config.LayerConfig.m_aLayers.Count();

		m_MapEntity.OpenMap(config);
		m_wMapWidget = m_MapEntity.GetMapWidget();

		// OpenMap switches the character camera's render off, which is right
		// for a map that fills the screen and wrong for one on a board.
		PlayerController controller = GetGame().GetPlayerController();
		if (controller)
			controller.SetCharacterCameraRenderActive(true);

		// SCR_MapEntity counts down FRAME_DELAY frames after an open before it
		// accepts a zoom or a pan, and UpdateViewPort needs a frame after that
		// to have a visible frame worth reading.
		GetGame().GetCallqueue().CallLater(FinishPriming, 700, false);
	}

	//------------------------------------------------------------------------
	protected void FinishPriming()
	{
		m_bPriming = false;

		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		// Somebody opened their own map inside our 700 ms. Theirs; try again
		// on a later tick.
		if (m_MapEntity.GetMapWidget() != m_wMapWidget)
			return;

		// The whole island, fitted to the widget's height: ZoomOut is minimum
		// zoom plus CenterMap, and minimum zoom is screen height over map size
		// in metres.
		m_MapEntity.ZoomOut();
		m_MapEntity.UpdateViewPort();
		m_MapEntity.GetMapVisibleFrame(m_vFrameMin, m_vFrameMax);

		// The view, in the entity's own terms, so it can be re-stated later.
		// ZoomChange takes a RATIO, not a pixels-per-metre: SetZoom computes it
		// as target over the widget's PixelPerUnit, and PixelPerUnit is the
		// widget's fixed layout scale (1024 px over 4096 m = 0.25), so the
		// ratio is stable. GetCurrentPan is already DPI-scaled, which is
		// exactly what PosChange wants back.
		m_vPan = m_MapEntity.GetCurrentPan();
		m_iLayer = m_MapEntity.GetLayerIndex();

		float base = m_wMapWidget.PixelPerUnit();
		if (base > 0)
			m_fZoomLevel = m_MapEntity.GetCurrentZoom() / base;

		m_MapEntity.CloseMap();

		m_bPrimed = true;

		MCF_Core_Log.Debug("map board primed at zoom ratio " + m_fZoomLevel.ToString() + ", layer " + m_iLayer.ToString());
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

		// The fade changed, so the picture has to be drawn again even if the
		// map is not being drawn at all. Three ticks is enough to land.
		m_iPaint = 3;
	}

	//------------------------------------------------------------------------
	//! ONE PLACE DECIDES WHETHER THE BOARD IS BEING DRAWN, because the texture
	//! keeps whatever was put in it last and two callers arguing over
	//! SetEnabled is how a board ends up frozen on the wrong frame.
	protected void Paint()
	{
		bool draw = m_fWhite < 1 || m_iPaint > 0;

		if (m_iPaint > 0)
			m_iPaint--;

		if (m_wRenderTarget)
			m_wRenderTarget.SetEnabled(draw);
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
			callqueue.Remove(FinishPriming);
		}

		if (m_MapEntity)
		{
			// Only ours, and only if nobody is reading a map: turning
			// visualisation off under an open map would blank their screen
			// because a board was deleted.
			if (m_bVisualising && !m_MapEntity.IsOpen())
				m_MapEntity.EnableVisualisation(false);

			if (m_MapEntity.IsOpen() && m_MapEntity.GetMapWidget() == m_wMapWidget)
				m_MapEntity.CloseMap();
		}

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
