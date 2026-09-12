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

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Draw the markers that this viewer's own client can see. Two players on different sides looking at the same board will not see the same markers, which is the only honest answer.")]
	protected bool m_bShowMarkers;

	[Attribute(defvalue: "28", uiwidget: UIWidgets.EditBox, desc: "How big a marker is drawn on the board, in board pixels. The board is 1024 x 700.", params: "4 200")]
	protected float m_fMarkerSize;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Draw the marker's own icon. Off draws a plain coloured disc instead, which is immune to whatever the icon's transparency does and reads further away.")]
	protected bool m_bMarkerIcons;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Draw the map's grid on the board. The grid belongs to the map entity rather than to this board, so two boards that disagree about it will take turns winning.")]
	protected bool m_bShowGrid;

	//! MEASURED, AND THE ANSWER IS NO -- leave this off.
	//!
	//! The idea was sound and worth trying: SCR_MapEntity is the singleton,
	//! MapEntity is not, so a board could spawn one of its own and have a
	//! view nobody else can disturb. It does not work. There is one native
	//! map renderer behind every MapEntity, and a second entity writes into
	//! the same state. Calling InitializeLayers on it rebuilt the layers the
	//! REAL map was using: the board and the player's own map both lost their
	//! terrain and were left with contours, roads and descriptors on a flat
	//! ground.
	//!
	//! There is no API that binds a MapWidget to a particular MapEntity, and
	//! this is why. Do not try it again without new evidence.
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Measured and does not work: a second map entity writes into the same native state and destroys the real map's layers. Left here as the record, not as a setting.")]
	protected bool m_bOwnMapEntity;

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

	//! Our own layer over the map, and the commands drawn into it. THE ARRAY
	//! IS A FIELD BECAUSE THE ENGINE SAYS SO: "the caller needs to keep the
	//! array alive - the callee takes just a pointer to it".
	protected CanvasWidget m_wOverlay;
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};

	//! Pixels per metre at the board's current zoom, worked out in ComputeView
	//! and used again for every marker.
	protected float m_fPPU;

	protected SCR_MapEntity m_MapEntity;
	protected bool m_bRaised;

	//! Whether the one-off setup open has happened and the island's frame is
	//! known. Until then the board has nothing to re-state.
	protected bool m_bPrimed;
	protected bool m_bPriming;

	//! THE BOARD'S SHARED TRUTH, and it is only this board's. Two numbers:
	//! how far in it is zoomed, and what it is looking at. Everybody who can
	//! see the board sees the same two, because they are replicated on the
	//! board's own entity -- and nobody's personal map is touched by either,
	//! because the board draws through a map entity of its own.
	//!
	//! Boards do not share this. One board, one view, changed by whoever
	//! walks up to that board.
	//! Pixels per metre. Zero means "the whole island, fitted", which is where
	//! a board starts and what it comes back to.
	//!
	//! IT IS A FLOAT AND NOT A STEP COUNT because the board is driven from a
	//! real map now, and a real map zooms continuously. Steps were right when
	//! the only control was a prompt on the panel.
	[RplProp(onRplName: "OnViewReplicated")]
	protected float m_fViewPPU;

	[RplProp(onRplName: "OnViewReplicated")]
	protected float m_fCentreX;

	[RplProp(onRplName: "OnViewReplicated")]
	protected float m_fCentreZ;

	//! Until somebody moves it, the board looks at the middle of the world.
	//! A zero centre is a real coordinate, so it cannot double as "unset".
	[RplProp(onRplName: "OnViewReplicated")]
	protected bool m_bCentreSet;

	//------------------------------------------------------------------------
	float GetViewPPU()
	{
		return m_fViewPPU;
	}

	//------------------------------------------------------------------------
	void GetViewCentre(out float x, out float z)
	{
		if (m_bCentreSet)
		{
			x = m_fCentreX;
			z = m_fCentreZ;
			return;
		}

		if (!m_MapEntity)
			return;

		x = m_MapEntity.GetMapSizeX() * 0.5;
		z = m_MapEntity.GetMapSizeY() * 0.5;
	}

	//------------------------------------------------------------------------
	//! Client side: ask for a view. The server owns the answer, because the
	//! board is a thing in the world that several people are looking at and
	//! not a setting in one person's client.
	void AskView(float centreX, float centreZ, float ppu)
	{
		Rpc(RpcAsk_View, centreX, centreZ, ppu);
	}

	//------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_View(float centreX, float centreZ, float ppu)
	{
		// Nothing worth a replication for a mouse that moved two metres.
		if (m_bCentreSet
			&& Math.AbsFloat(ppu - m_fViewPPU) < 0.0001
			&& Math.AbsFloat(centreX - m_fCentreX) < 1
			&& Math.AbsFloat(centreZ - m_fCentreZ) < 1)
			return;

		m_fViewPPU = ppu;
		m_fCentreX = centreX;
		m_fCentreZ = centreZ;
		m_bCentreSet = true;

		Replication.BumpMe();

		// A listen server is its own client and gets no replication callback
		// for its own write.
		OnViewReplicated();
	}

	//------------------------------------------------------------------------
	//! Runs on every machine when the board's view changes.
	protected void OnViewReplicated()
	{
		ComputeView();
	}

	//------------------------------------------------------------------------
	//! The board's two numbers, turned into the four the map entity wants.
	//!
	//! THIS IS SCR_MapEntity'S OWN ARITHMETIC, not an invention. Minimum zoom
	//! in UpdateZoomBounds is screen height over map size in metres -- the
	//! whole island fitted to the widget -- and each step doubles it.
	//! ZoomChange takes a ratio against the widget's fixed layout scale, and
	//! PosChange is fed exactly what WorldToScreen returns for the point that
	//! should end up in the middle, which is what CenterMap does.
	protected void ComputeView()
	{
		if (!m_wMapWidget || !m_MapEntity)
			return;

		float widgetW, widgetH;
		m_wMapWidget.GetScreenSize(widgetW, widgetH);

		float sizeX = m_MapEntity.GetMapSizeX();
		float sizeY = m_MapEntity.GetMapSizeY();

		if (widgetW <= 0 || widgetH <= 0 || sizeX <= 0 || sizeY <= 0)
			return;

		// Zero means "the whole island", which is where a board starts and
		// what it comes back to.
		float ppu = m_fViewPPU;
		if (ppu <= 0)
			ppu = widgetH / sizeY;

		m_fPPU = ppu;

		float basePPU = m_wMapWidget.PixelPerUnit();
		if (basePPU > 0)
			m_fZoomLevel = ppu / basePPU;

		float centreX = sizeX * 0.5;
		float centreZ = sizeY * 0.5;

		if (m_bCentreSet)
		{
			centreX = m_fCentreX;
			centreZ = m_fCentreZ;
		}

		vector offset = m_MapEntity.Offset();

		// PAN IS THE MAP'S TOP-LEFT CORNER IN THE WIDGET, not the screen
		// position of the point being centred -- which is what the first
		// version assumed, and the reason the setup open logged
		// "primed pan <162, 0, 0>" against a computed <350, 350, 0>.
		//
		// The measurement makes it plain. Widget 1024 x 700, island 4096 m,
		// fitted zoom 0.170898 px/m, so the island draws 700 x 700: centred
		// it sits (1024 - 700) / 2 = 162 from the left and 0 from the top,
		// which is exactly what ZoomOut and CenterMap produced. So the pan is
		// half the widget MINUS where the centred point falls in map pixels.
		float pixelX = (centreX - offset[0]) * ppu;
		float pixelY = ((sizeY - centreZ) + offset[2]) * ppu;

		m_vPan = Vector(widgetW * 0.5 - pixelX, widgetH * 0.5 - pixelY, 0);

		float halfW = (widgetW / ppu) * 0.5;
		float halfH = (widgetH / ppu) * 0.5;

		m_vFrameMin = Vector(centreX - halfW, 0, centreZ - halfH);
		m_vFrameMax = Vector(centreX + halfW, 0, centreZ + halfH);
	}


	//! THE BOARD'S OWN VIEW. Zoom and offset go to the WIDGET (see ApplyView)
	//! and belong to this board alone; the layer and the frame are the
	//! entity's, and are therefore shared with whoever else is using the map.
	protected float m_fZoomLevel;
	protected vector m_vPan;
	protected int m_iLayer = -1;

	//! The board's own map, when it has one. Everything below draws through
	//! this instead of the shared entity the moment it exists.
	protected MCF_Map_BoardEntity m_OwnMap;

	//! Kept from the setup open so the board's own map can be built from the
	//! same configuration the real one used.
	protected ref SCR_MapLayersBase m_LayersConfig;
	protected ref SCR_MapPropsBase m_PropsConfig;

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
		m_wOverlay = CanvasWidget.Cast(m_wRoot.FindAnyWidget("Overlay"));

		if (!m_wOverlay)
			MCF_Core_Log.Warn("map board: the layout carries no Overlay canvas, so nothing will be drawn on the map");

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

		// Somebody else's map is open on THIS client: the entity is theirs
		// until they close it, and every line below reads this.
		bool foreign = m_MapEntity.IsOpen() && m_MapEntity.GetMapWidget() != m_wMapWidget;

		// THE MAP ENTITY IS PER CLIENT, and that is what makes this feature
		// possible at all. A player opening their own map takes the map over
		// on THEIR machine only; every other client's board is untouched. So
		// the board is never wrong for anybody else -- the only screen it can
		// be wrong on is the screen of the person who is busy reading a map
		// instead of looking at the board.
		//
		// So on that one screen we do not show them a lie. The board goes
		// white for as long as their map is open, and comes back the moment
		// they close it. Nobody standing at the board sees anything change.
		float white = FadeFor(ViewerDistance(owner));

		if (foreign)
			white = 1;

		Whiten(white);

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
		// The setup open is ours and has to be left alone to finish.
		if (m_bPriming)
			return;

		if (!m_bPrimed)
		{
			// The setup open needs the map to itself for its half second.
			if (wanted && !foreign)
				Prime();

			return;
		}

		if (!wanted)
		{
			// Nobody near. Stop drawing the world for a board nobody can see,
			// but only if we are the one who asked for it.
			if (!m_bVisualising)
				return;

			m_bVisualising = false;

			if (m_OwnMap)
				m_OwnMap.Hide();
			else if (!m_MapEntity.IsOpen())
				m_MapEntity.EnableVisualisation(false);

			return;
		}

		// The board's own map, when it has one: same four numbers, written to
		// an entity nobody else reads.
		if (m_OwnMap)
		{
			m_OwnMap.ShowView(m_fZoomLevel, m_vPan, m_vFrameMin, m_vFrameMax, m_bShowGrid);
			m_bVisualising = true;
			return;
		}

		// Everything from here is the entity's, and on this client the entity
		// is theirs while their map is open. We write none of it, and the
		// board is white anyway.
		if (foreign)
			return;

		// THE BOARD'S OWN VIEW. Re-stated every tick rather than set once,
		// because a player's map leaves the entity wherever they finished
		// and the board has to be able to take it back without ceremony.
		ApplyView();
		m_bVisualising = true;

		m_MapEntity.EnableVisualisation(true);

		if (m_iLayer >= 0 && m_MapEntity.GetLayerIndex() != m_iLayer)
			m_MapEntity.SetLayer(m_iLayer);

		m_MapEntity.EnableGrid(m_bShowGrid);
		m_MapEntity.SetFrame(m_vFrameMin, m_vFrameMax);

		DrawOverlay();
	}

	//------------------------------------------------------------------------
	//! Everything on the board that the engine does not draw for us.
	//!
	//! WHY WE DRAW MARKERS OURSELVES rather than borrowing the ones on the
	//! map. Vanilla's markers are widgets parented to the map menu's own
	//! frame and repositioned by a component that only runs between
	//! OnMapOpen and OnMapClose -- a board is neither, so it gets none of
	//! them. But the DATA is a static singleton and readable at any time, and
	//! a MapWidget is a CanvasWidget, so drawing them is an array of commands.
	//! That is less code than borrowing would have been, and it is ours: no
	//! frame, no open map and no other player can take it away.
	//!
	//! WHOSE MARKERS. The ones this viewer's own client holds. A marker
	//! belonging to another faction is dropped by SCR_MapMarkerManagerComponent
	//! before we ever see it, so two players on different sides looking at the
	//! same board do not see the same markers -- which is the only honest
	//! answer, and it means the board needs no permission code of its own.
	protected void DrawOverlay()
	{
		if (!m_wOverlay)
			return;

		m_aCommands.Clear();

		if (m_bShowMarkers)
			AddMarkers();

		m_wOverlay.SetDrawCommands(m_aCommands);
	}

	//------------------------------------------------------------------------
	protected void AddMarkers()
	{
		SCR_MapMarkerManagerComponent markers = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markers)
			return;

		SCR_MapMarkerConfig config = markers.GetMarkerConfig();

		// BOTH LISTS. A marker that scrolled out of the open map's frame is
		// MOVED into the disabled list rather than hidden, so reading only
		// the first one gives a board that loses markers depending on where
		// somebody else last left their own map. Vanilla unions them too.
		array<SCR_MapMarkerBase> placed = markers.GetStaticMarkers();

		foreach (SCR_MapMarkerBase parked : markers.GetDisabledMarkers())
		{
			placed.Insert(parked);
		}

		foreach (SCR_MapMarkerBase marker : placed)
		{
			AddMarker(marker, config);
		}
	}

	//------------------------------------------------------------------------
	protected void AddMarker(SCR_MapMarkerBase marker, SCR_MapMarkerConfig config)
	{
		if (!marker)
			return;

		int world[2];
		marker.GetWorldPos(world);

		vector at;
		if (!WorldToBoard(world[0], world[1], at))
			return;

		Color tint = Color.FromInt(Color.WHITE);
		ResourceName imageset, glow;
		string quad;
		bool drawn = false;

		// The icon and the colour are INDICES on the marker, not resources.
		// They resolve through the config entry for that marker's type.
		if (config)
		{
			SCR_MapMarkerEntryPlaced entry = SCR_MapMarkerEntryPlaced.Cast(config.GetMarkerEntryConfigByType(marker.GetType()));
			if (entry)
			{
				tint = entry.GetColorEntry(marker.GetColorEntry());

				if (m_bMarkerIcons && entry.GetIconEntry(marker.GetIconEntry(), imageset, glow, quad))
				{
					ImageDrawCommand icon = m_wOverlay.CreateCommandFromImageSet(imageset, quad, Vector(m_fMarkerSize, m_fMarkerSize, 0));
					if (icon)
					{
						icon.m_Position = Vector(at[0] - m_fMarkerSize * 0.5, at[1] - m_fMarkerSize * 0.5, 0);
						icon.m_iColor = tint.PackToInt();
						icon.m_fRotation = marker.GetRotation();

						// WITHOUT BLEND THE ICON'S TRANSPARENCY IS NOT
						// TRANSPARENT. CreateCommandFromImageSet hands back a
						// command flagged STRETCH and nothing else, so every
						// pixel the icon meant to leave alone is drawn -- as
						// black, in a neat box around the marker. BLEND is the
						// engine's own word for it: "Widget will be
						// alpha-blended".
						icon.m_iFlags = WidgetFlags.STRETCH | WidgetFlags.BLEND;

						m_aCommands.Insert(icon);
						drawn = true;
					}
				}
			}
		}

		// A marker type we have no icon for is still a marker somebody placed
		// and still worth a dot. Silence would read as "there is nothing
		// there", which is the one thing a map must never say wrongly.
		//
		// The disc is drawn with a dark ring around it so it reads on pale
		// sea and on dark hills alike -- a board is looked at from across a
		// room, where a bare coloured dot on green is no dot at all.
		if (!drawn)
		{
			array<float> circle = {};
			m_wOverlay.TessellateCircle(Vector(at[0], at[1], 0), m_fMarkerSize * 0.3, 16, circle);

			PolygonDrawCommand disc = new PolygonDrawCommand();
			disc.m_Vertices = circle;
			disc.m_iColor = tint.PackToInt();
			m_aCommands.Insert(disc);

			LineDrawCommand ring = new LineDrawCommand();
			ring.m_Vertices = circle;
			ring.m_iColor = ARGB(220, 0, 0, 0);
			ring.m_fWidth = 2;
			ring.m_bShouldEnclose = true;
			m_aCommands.Insert(ring);
		}

		string label = marker.GetCustomText();
		if (label.IsEmpty())
			return;

		TextDrawCommand text = new TextDrawCommand();
		text.m_sText = label;
		text.m_Position = Vector(at[0] + m_fMarkerSize * 0.6, at[1] - m_fMarkerSize * 0.3, 0);
		text.m_fSize = 18;
		text.m_iColor = tint.PackToInt();
		m_aCommands.Insert(text);
	}

	//------------------------------------------------------------------------
	//! A world position, in board pixels.
	//!
	//! The same arithmetic the board's own view is built from, run the other
	//! way: the pan is where the map's top-left corner sits in the widget, so
	//! a world point is the pan plus its offset in map pixels. The Y flip is
	//! the map's own -- SCR_MapEntity.WorldToScreen opens with
	//! worldY = m_iMapSizeY - worldY.
	protected bool WorldToBoard(float worldX, float worldZ, out vector at)
	{
		if (!m_MapEntity || m_fPPU <= 0)
			return false;

		float sizeY = m_MapEntity.GetMapSizeY();
		if (sizeY <= 0)
			return false;

		vector offset = m_MapEntity.Offset();

		at = Vector(m_vPan[0] + (worldX - offset[0]) * m_fPPU,
			m_vPan[1] + ((sizeY - worldZ) + offset[2]) * m_fPPU, 0);

		return true;
	}

	//------------------------------------------------------------------------
	//! THE WIDGET'S ZOOM IS NOT THE MAP'S ZOOM. Measured, in game, and it is
	//! worth writing down because the API makes the opposite look true.
	//!
	//! CanvasWidgetBase carries SetZoom and SetOffsetPx per widget, and the
	//! very first probe measured two MapWidgets holding two different zooms
	//! at the same moment. All of that is real -- it is the canvas's own
	//! transform, the space draw commands are placed in.
	//!
	//! It is NOT what the engine renders the map at. Setting it moves some
	//! detail around and leaves the map where it was; the picture follows the
	//! ENTITY, through ZoomChange and PosChange. One map entity, one rendered
	//! view. A board cannot hold a different one at the same instant.
	//!
	//! Which costs far less than it sounds, because the map entity is per
	//! client. See Decide: the only screen a board can be wrong on is the
	//! screen of somebody who has their own map open, and that person is
	//! looking at their map. They get a blank board for those seconds.
	protected void ApplyView()
	{
		if (!m_MapEntity)
			return;

		if (m_fZoomLevel > 0)
			m_MapEntity.ZoomChange(m_fZoomLevel);

		m_MapEntity.PosChange(m_vPan[0], m_vPan[1]);
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

		m_LayersConfig = SCR_MapLayersBase.Cast(LoadConfig(SCR_MapConstants.CFG_LAYERS_DEFAULT));
		m_PropsConfig = SCR_MapPropsBase.Cast(LoadConfig(SCR_MapConstants.CFG_PROPS_DEFAULT));

		config.LayerConfig = m_LayersConfig;
		config.MapPropsConfig = m_PropsConfig;
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

		// THE ARITHMETIC, CHECKED AGAINST THE REAL THING. ComputeView works
		// the same view out from the board's own two numbers; at zoom step 0
		// with no centre set that is "the whole island, centred", which is
		// exactly what ZoomOut and CenterMap just did. If these two lines
		// disagree, the formula is wrong and everything the board does with
		// zoom and pan from here is wrong with it.
		float primedZoom = m_fZoomLevel;
		vector primedPan = m_vPan;

		ComputeView();

		string check = "map board view | primed zoom " + primedZoom.ToString();
		check = check + " pan " + primedPan.ToString();
		check = check + " | computed zoom " + m_fZoomLevel.ToString();
		check = check + " pan " + m_vPan.ToString();
		check = check + " | layer " + m_iLayer.ToString();

		MCF_Core_Log.Warn(check);

		// A MAP OF THE BOARD'S OWN, built from the same configuration and
		// handed the numbers the real map just worked out. From here the board
		// never writes to the shared entity again.
		if (!m_bOwnMapEntity)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		m_OwnMap = MCF_Map_BoardEntity.Spawn(owner);

		if (!m_OwnMap)
		{
			MCF_Core_Log.Warn("map board: could not spawn a map entity of its own -- falling back to the shared one");
			return;
		}

		m_OwnMap.Setup(m_LayersConfig, m_PropsConfig, m_iLayer);
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

		if (m_OwnMap)
		{
			m_OwnMap.Hide();
			SCR_EntityHelper.DeleteEntityAndChildren(m_OwnMap);
			m_OwnMap = null;
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
