//! Answers one question and then gets deleted.
//!
//! CAN THE GAME'S OWN MAP BE PUT SOMEWHERE WE CHOOSE, and can it be captured
//! by a render target? Everything about the map board rests on that, and it
//! cannot be answered by reading: nothing in SCR_MapEntity hands a widget or
//! a screen rectangle across to the native side, so where the map's pixels
//! land is invisible from script.
//!
//! What reading did establish, and what this screen tests:
//!
//!  - The map widget is its own class, `MapWidgetClass`, carrying SizeInUnits
//!    and OffsetPixels. Vanilla's own MapMini.layout puts one at 300x300 in a
//!    corner, which is a minimap -- so the map is drawn inside its widget and
//!    not over the whole screen. TEST A proves that for a widget of ours.
//!  - `OpenMap` takes the root widget off the config and does exactly one
//!    thing with it: FindAnyWidget("MapWidget"). It never asks a menu for
//!    anything.
//!  - Opening a map calls SetCharacterCameraRenderActive(false). That is an
//!    optimisation for a fullscreen map and there is a public counterpart, so
//!    this screen turns the camera straight back on and reports whether the
//!    map minds.
//!
//! TEST B puts the same widgets inside an RTTextureWidget. A render target
//! shows nothing until a material samples it, so this half cannot show a
//! picture -- what it reports is whether the map widget inside one is laid
//! out at all. PixelPerUnit() coming back at or below zero is the tell that
//! it is not, and that no board can work this way.
class MCF_Map_ProbeMenu : ChimeraMenuBase
{
	protected SCR_MapEntity m_MapEntity;
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

		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity)
		{
			Say("no SCR_MapEntity in this world -- the map is a world entity and this terrain has none");
			return;
		}

		// ---- what the widgets say about themselves before anything is opened
		CanvasWidget plain = CanvasWidget.Cast(root.FindAnyWidget("MapWidget"));
		CanvasWidget inRT = CanvasWidget.Cast(root.FindAnyWidget("ProbeMapWidget"));

		Say("MapWidget casts to CanvasWidget: " + (plain != null).ToString());
		Say("the one inside the render target casts too: " + (inRT != null).ToString());

		// ---- open the map into OUR widget
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
		{
			Say("no game mode -- cannot reach the map config");
			return;
		}

		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
		{
			Say("the game mode carries no SCR_MapConfigComponent");
			return;
		}

		MapConfiguration config = m_MapEntity.SetupMapConfig(EMapEntityMode.MINIMAP, configComp.GetGadgetMapConfig(), root);
		if (!config)
		{
			Say("SetupMapConfig returned nothing");
			return;
		}

		m_MapEntity.OpenMap(config);
		Say("OpenMap called with our own root widget");

		// ---- and give the character its camera back
		//
		// OpenMap switches it off. CloseMap switches it on. Nothing says the
		// map needs it off, so: on.
		PlayerController controller = GetGame().GetPlayerController();
		if (controller)
		{
			controller.SetCharacterCameraRenderActive(true);
			Say("character camera switched back on");
		}

		// The map stalls a frame waiting for its widget to lay out, so asking
		// now would only ever read the fallback.
		GetGame().GetCallqueue().CallLater(Measure, 500, false);
	}

	//------------------------------------------------------------------------
	//! Everything worth knowing, a few frames after the map has settled.
	protected void Measure()
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		CanvasWidget plain = CanvasWidget.Cast(root.FindAnyWidget("MapWidget"));
		CanvasWidget inRT = CanvasWidget.Cast(root.FindAnyWidget("ProbeMapWidget"));

		if (plain)
		{
			float w, h;
			plain.GetScreenSize(w, h);
			Say("A  laid out at " + w.ToString() + " x " + h.ToString()
				+ ", PixelPerUnit " + plain.PixelPerUnit().ToString()
				+ ", zoom " + plain.GetZoom().ToString());
		}

		if (inRT)
		{
			float rw, rh;
			inRT.GetScreenSize(rw, rh);

			// THE TELL. At or below zero means the widget is not being laid
			// out inside the render target, and nothing downstream of it can
			// be right -- SCR_MapEntity itself falls back to 0.01 here with
			// the comment "should never happen".
			Say("B  laid out at " + rw.ToString() + " x " + rh.ToString()
				+ ", PixelPerUnit " + inRT.PixelPerUnit().ToString()
				+ "   <- at or below zero means a render target cannot hold a map");
		}

		if (m_MapEntity)
		{
			Say("map reports open: " + m_MapEntity.IsOpen().ToString()
				+ ", zoom " + m_MapEntity.GetCurrentZoom().ToString());

			vector min, max;
			m_MapEntity.GetMapVisibleFrame(min, max);
			Say("visible world frame " + min.ToString() + " .. " + max.ToString());
		}

		Say("");
		Say("NOW LOOK AT THE SCREEN. Terrain in the left box means the map draws inside a widget we made, and a board is possible. Terrain over the whole screen instead means it draws to the screen and cannot be captured.");
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
