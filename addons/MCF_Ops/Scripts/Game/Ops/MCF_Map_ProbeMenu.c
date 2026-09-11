//! Answers one question and then gets deleted.
//!
//! CAN THE GAME'S OWN MAP BE PUT SOMEWHERE WE CHOOSE, and can a render target
//! hold one? Everything about the map board rests on that, and it cannot be
//! answered by reading: nothing in SCR_MapEntity hands a widget or a screen
//! rectangle across to the native side, so where the map's pixels land is
//! invisible from script.
//!
//! What the first run taught, both of them my mistakes rather than the
//! engine's:
//!
//!  - Slots with `Anchor` plus a size came out 0 x 0, so the render target
//!    asked for a texture of zero size and the log filled with "Out of memory
//!    when requested 0, Type: Video". The layout now copies MapMini.layout's
//!    slot form exactly: no Anchor, position and size, offsets as the
//!    negative of position+size.
//!  - `SetupMapConfig` with the gadget config brings the whole component set,
//!    and SCR_MapCursorModule.InitWidgets went looking for widgets that a
//!    board has no reason to own. So the config is built here instead, with
//!    no modules and no components at all.
//!
//! THAT SECOND ONE IS NOT ONLY A FIX. A board wants a map and nothing else --
//! no cursor, no tool menu, no ruler -- so building the configuration by hand
//! is what the real thing will do too. It also steps around SetupMapConfig
//! handing back and mutating the shared m_ActiveMapCfg.
class MCF_Map_ProbeMenu : ChimeraMenuBase
{
	protected SCR_MapEntity m_MapEntity;
	protected string m_sOpenIn = "nothing";
	protected RichTextWidget m_wReport;
	protected ref array<string> m_aLines = {};

	//------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_wReport = RichTextWidget.Cast(root.FindAnyWidget("Report"));

		SCR_ButtonTextComponent close = SCR_ButtonTextComponent.GetButtonText("ButtonClose", root);
		if (close)
			close.m_OnClicked.Insert(OnCloseClicked);

		SCR_ButtonTextComponent plainButton = SCR_ButtonTextComponent.GetButtonText("ButtonPlain", root);
		if (plainButton)
			plainButton.m_OnClicked.Insert(OnPlainClicked);

		SCR_ButtonTextComponent probeButton = SCR_ButtonTextComponent.GetButtonText("ButtonProbe", root);
		if (probeButton)
			probeButton.m_OnClicked.Insert(OnProbeClicked);

		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity)
		{
			Say("no SCR_MapEntity in this world -- the map is a world entity and this terrain has none");
			return;
		}

		Say("stage one already answered: the map draws in a widget of ours, and a map widget inside a render target lays out with a real PixelPerUnit.");
		Say("stage two asks the one thing left: does the MAP bind to the widget inside the render target, or only to the plain one.");
		Say("");
		Say("OPEN INTO A puts the map in the left box. OPEN INTO B hands OpenMap the render target's subtree instead -- if the numbers for B then move off their defaults the way A's do, the map is driving a widget inside a render target and a board on a wall is only a material away.");

		OpenInto("A", "PlainHost");
	}

	//------------------------------------------------------------------------
	protected void OnPlainClicked(SCR_ButtonTextComponent button)
	{
		OpenInto("A", "PlainHost");
	}

	protected void OnProbeClicked(SCR_ButtonTextComponent button)
	{
		OpenInto("B", "Probe");
	}

	//------------------------------------------------------------------------
	//! Opens the map into one host or the other.
	//!
	//! BOTH HOSTS CONTAIN A WIDGET CALLED "MapWidget". OpenMap does
	//! FindAnyWidget for that name on whatever root it is handed, so handing
	//! it one host or the other is the entire difference -- same map, same
	//! config, one of two widgets.
	protected void OpenInto(string label, string hostName)
	{
		Widget root = GetRootWidget();
		if (!root || !m_MapEntity)
			return;

		Widget host = root.FindAnyWidget(hostName);
		if (!host)
		{
			Say("no host called " + hostName);
			return;
		}

		if (m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();

		MapConfiguration config = BuildConfig(host);
		if (!config)
			return;

		m_sOpenIn = label;
		m_MapEntity.OpenMap(config);
		Say("");
		Say("opened into " + label + " (" + hostName + ")");

		// OpenMap switches the character camera off; CloseMap switches it on.
		// Nothing says the map needs it off, so: on.
		PlayerController controller = GetGame().GetPlayerController();
		if (controller)
			controller.SetCharacterCameraRenderActive(true);

		// The map stalls a frame waiting for its widget to lay out, so asking
		// now would only ever read the fallback.
		GetGame().GetCallqueue().Remove(Measure);
		GetGame().GetCallqueue().CallLater(Measure, 800, false);
	}

	//------------------------------------------------------------------------
	//! A map and nothing else.
	//!
	//! Four configs, no modules, no components. The defaults are named in
	//! SCR_MapConstants, which is what SetupMapConfig falls back to as well.
	protected MapConfiguration BuildConfig(notnull Widget root)
	{
		MapConfiguration config = new MapConfiguration();

		config.RootWidgetRef = root;
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
			Say("the default layers config would not load -- without layers there is no map");
			return null;
		}

		config.LayerCount = config.LayerConfig.m_aLayers.Count();
		Say("config built: " + config.LayerCount.ToString() + " layer(s)"
			+ ", props " + (config.MapPropsConfig != null).ToString()
			+ ", descriptor defaults " + (config.DescriptorDefsConfig != null).ToString()
			+ ", descriptor visibility " + (config.DescriptorVisibilityConfig != null).ToString());

		return config;
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
	//! Everything worth knowing, once the map has had time to settle.
	protected void Measure()
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		Widget plainHost = root.FindAnyWidget("PlainHost");
		Widget probeHost = root.FindAnyWidget("Probe");

		if (plainHost)
			SaySize("A", CanvasWidget.Cast(plainHost.FindAnyWidget("MapWidget")));

		if (probeHost)
			SaySize("B", CanvasWidget.Cast(probeHost.FindAnyWidget("MapWidget")));

		Say("the map was opened into " + m_sOpenIn + " -- whichever of the two moved off zoom 1 is the one it bound to");

		if (m_MapEntity)
		{
			Say("map reports open: " + m_MapEntity.IsOpen().ToString()
				+ ", zoom " + m_MapEntity.GetCurrentZoom().ToString());

			vector min, max;
			m_MapEntity.GetMapVisibleFrame(min, max);
			Say("visible world frame " + min.ToString() + " .. " + max.ToString()
				+ "   <- two identical corners means it is showing nothing");
		}

		Say("");
		Say("LOOK AT THE LEFT BOX. Terrain there means the map draws inside a widget we made, and a board is possible.");
	}

	//------------------------------------------------------------------------
	protected void SaySize(string label, CanvasWidget box)
	{
		if (!box)
		{
			Say(label + ": widget not found");
			return;
		}

		float w, h;
		box.GetScreenSize(w, h);

		Say(label + ": " + w.ToString() + " x " + h.ToString()
			+ ", PixelPerUnit " + box.PixelPerUnit().ToString()
			+ ", zoom " + box.GetZoom().ToString());
	}

	//------------------------------------------------------------------------
	protected void Say(string line)
	{
		MCF_Core_Log.Warn("map probe: " + line);
		m_aLines.Insert(line);

		if (!m_wReport)
			return;

		string all;
		foreach (string held : m_aLines)
		{
			if (!all.IsEmpty())
				all = all + "\n";

			all = all + held;
		}

		m_wReport.SetText(all);
	}

	//------------------------------------------------------------------------
	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}

	//------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(Measure);

		if (m_MapEntity)
			m_MapEntity.CloseMap();

		super.OnMenuClose();
	}
}
