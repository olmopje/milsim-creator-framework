//! One freeform stroke somebody drew on the map.
//!
//! WORLD POINTS, NOT SCREEN POINTS. A stroke stored where it was drawn on
//! somebody's screen is wrong the moment anybody pans, zooms or looks at it
//! on a board of a different size. Stored in metres it is the same line for
//! everyone, on every map and every board, forever.
class MCF_Map_Stroke
{
	//! Flat: x, z, x, z. Flat because that is what LineDrawCommand wants and
	//! turning a list of pairs into it every frame would be work for nothing.
	ref array<float> m_aPoints = {};

	//! Given by the server, never by a client, so that "rub this one out" names
	//! a stroke rather than a position in a list that somebody else may have
	//! shifted in the meantime.
	int m_iId;

	//! An index into the marker colour palette, not a colour. A colour is four
	//! floats and this is one small number on the wire -- and being an index
	//! into the GAME'S palette is what keeps a red line the same red as a red
	//! marker.
	int m_iColour;

	//! WIDTH IN METRES, NOT PIXELS. A line is a thing lying on the ground, so
	//! it has a width on the ground: it grows when the map is zoomed in and
	//! shrinks when zoomed out, the same way the line's own length does. A
	//! width in pixels would be a line that is a hair at one zoom and a smear
	//! at another.
	int m_iWidth;

	//! Who drew it, so they can rub out their own and a Game Master can rub
	//! out anybody's.
	int m_iOwner;
}

//------------------------------------------------------------------------------------------------
//! What everyone has drawn on the map.
//!
//! WHY THIS IS NOT ON THE BOARD. A drawing belongs to the mission, not to a
//! panel: it is meant to be on the commander's own map, on every board in
//! every room, and on the map of somebody who joins an hour later. So it sits
//! on the game mode, the way the marker manager does, and the boards and the
//! maps are readers.
//!
//! WHY A SERIALISED STRING RATHER THAN A PROPER LIST. Enfusion replicates a
//! string property and hands it to anybody who joins without a line of
//! push-on-join code; a variable-length list of objects needs RplSave/RplLoad
//! and a broadcast for every change, and then the same join code anyway. MCF
//! already carries device content this way for exactly that reason. The cost
//! is that every change re-sends the set, which is why the set is kept small
//! on purpose: points are metres rounded to whole numbers, a stroke is
//! thinned as it is drawn, and the oldest stroke is dropped once the string
//! would outgrow its budget.
[ComponentEditorProps(category: "MCF/Ops", description: "Holds the freeform drawings players make on the map.")]
class MCF_Map_DrawingComponentClass : SCR_BaseGameModeComponentClass
{
}

//! ON THE BASE CLASS. This was a plain ScriptComponent and it never came to
//! life: GetInstance() returned null on every machine, so every stroke was
//! collected, sent, and dropped on the floor. Every MCF component that works
//! on the game mode derives from SCR_BaseGameModeComponent, which is the
//! supported base for one -- and it also complains loudly in its constructor
//! if it is ever attached to something that is not a game mode, which is a
//! better way to learn that than a silently empty drawing.
class MCF_Map_DrawingComponent : SCR_BaseGameModeComponent
{
	//! The widths a line may have, in metres on the ground.
	static const ref array<int> WIDTHS = { 6, 14, 30 };

	static const ref array<string> WIDTH_NAMES = { "Thin", "Medium", "Thick" };

	//! Only used when the marker config cannot be reached -- see LoadPalette.
	protected static const ref array<int> FALLBACK_PALETTE = {
		0xFFFFFFFF, 0xFFE03030, 0xFF3070E0, 0xFF40C040, 0xFFE0C020, 0xFF101010
	};

	protected static const ref array<string> FALLBACK_NAMES = {
		"White", "Red", "Blue", "Green", "Yellow", "Black"
	};

	//! THE GAME'S OWN MARKER COLOURS, read once out of the marker config. A
	//! drawing and a marker are the same briefing in the same room, so a red
	//! line should be the same red as a red marker rather than a red somebody
	//! picked separately. Filled by LoadPalette on first use.
	protected static ref array<int> s_aColours;
	protected static ref array<string> s_aColourNames;

	//! How much drawing the mission may hold, in characters of the
	//! replicated string. Past this the oldest stroke goes, which is the
	//! right one to lose: a briefing moves on.
	protected static const int BUDGET = 24000;

	[RplProp(onRplName: "OnStrokesReplicated")]
	protected string m_sStrokes;

	//! Server side only. Never reset while the mission runs, so an id is never
	//! reused and a delete can never land on the wrong stroke.
	protected int m_iNextId = 1;

	protected ref array<ref MCF_Map_Stroke> m_aStrokes = {};
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();

	protected static MCF_Map_DrawingComponent s_Instance;

	//------------------------------------------------------------------------
	//! FOUND, NOT REMEMBERED. Registering in an init hook means trusting that
	//! the hook ran before the first person opened a map; asking the game mode
	//! for the component is true whenever it is asked. The static is kept as a
	//! cache, not as the source of truth.
	static MCF_Map_DrawingComponent GetInstance()
	{
		if (s_Instance)
			return s_Instance;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		s_Instance = MCF_Map_DrawingComponent.Cast(gameMode.FindComponent(MCF_Map_DrawingComponent));

		return s_Instance;
	}

	//------------------------------------------------------------------------
	// THE PALETTE
	//------------------------------------------------------------------------

	//! Read the colours a player can already give a map marker, and use those.
	//!
	//! PACKED BY HAND, NOT BY PackToInt. A draw command wants 0xAARRGGBB and
	//! Color's own packing order is not worth guessing at when getting it
	//! wrong shows up as a line in the wrong colour -- which this feature has
	//! already been bitten by once.
	protected static void LoadPalette()
	{
		if (s_aColours)
			return;

		s_aColours = {};
		s_aColourNames = {};

		SCR_MapMarkerManagerComponent markers = SCR_MapMarkerManagerComponent.GetInstance();
		if (markers)
		{
			SCR_MapMarkerConfig config = markers.GetMarkerConfig();
			if (config)
			{
				SCR_MapMarkerEntryPlaced placed = SCR_MapMarkerEntryPlaced.Cast(config.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
				if (placed)
				{
					array<ref SCR_MarkerColorEntry> entries = placed.GetColorEntries();

					if (entries)
					{
						foreach (SCR_MarkerColorEntry entry : entries)
						{
							s_aColours.Insert(Pack(entry.GetColor()));
							s_aColourNames.Insert(entry.GetName());
						}
					}
				}
			}
		}

		if (!s_aColours.IsEmpty())
			return;

		// No marker config in this world. A drawing is still worth having.
		foreach (int colour : FALLBACK_PALETTE)
		{
			s_aColours.Insert(colour);
		}

		foreach (string name : FALLBACK_NAMES)
		{
			s_aColourNames.Insert(name);
		}
	}

	//------------------------------------------------------------------------
	protected static int Pack(Color colour)
	{
		int a = Math.Round(colour.A() * 255);
		int r = Math.Round(colour.R() * 255);
		int g = Math.Round(colour.G() * 255);
		int b = Math.Round(colour.B() * 255);

		int packed = a << 24;
		packed = packed | (r << 16);
		packed = packed | (g << 8);
		packed = packed | b;

		return packed;
	}

	//------------------------------------------------------------------------
	static int Colour(int index)
	{
		LoadPalette();

		if (index < 0 || index >= s_aColours.Count())
			return s_aColours[0];

		return s_aColours[index];
	}

	//------------------------------------------------------------------------
	static string ColourName(int index)
	{
		LoadPalette();

		if (index < 0 || index >= s_aColourNames.Count())
			return "Colour";

		string name = s_aColourNames[index];

		if (name.IsEmpty())
			return "Colour " + (index + 1).ToString();

		return name;
	}

	//------------------------------------------------------------------------
	static int ColourCount()
	{
		LoadPalette();

		return s_aColours.Count();
	}

	//------------------------------------------------------------------------
	static int Width(int index)
	{
		if (index < 0 || index >= WIDTHS.Count())
			return WIDTHS[0];

		return WIDTHS[index];
	}

	//------------------------------------------------------------------------
	static string WidthName(int index)
	{
		if (index < 0 || index >= WIDTH_NAMES.Count())
			return WIDTH_NAMES[0];

		return WIDTH_NAMES[index];
	}

	//------------------------------------------------------------------------
	void MCF_Map_DrawingComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------
	void ~MCF_Map_DrawingComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	//------------------------------------------------------------------------
	array<ref MCF_Map_Stroke> GetStrokes()
	{
		return m_aStrokes;
	}

	//------------------------------------------------------------------------
	//! Fires on every machine when the drawings change, so a board or a map
	//! rebuilds its commands instead of rebuilding them every frame for
	//! nothing.
	ScriptInvoker GetOnChanged()
	{
		return m_OnChanged;
	}

	//------------------------------------------------------------------------
	//! May this player rub this stroke out? Their own, always; anybody's, if
	//! they are a full Game Master.
	static bool MayRemove(notnull MCF_Map_Stroke stroke)
	{
		SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
		if (editor && !editor.IsLimited())
			return true;

		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return false;

		return stroke.m_iOwner == controller.GetPlayerId();
	}

	//------------------------------------------------------------------------
	// ASKING
	//------------------------------------------------------------------------

	//! Asked from wherever somebody drew; carried out on the server, because
	//! everybody is looking at the same drawing.
	//!
	//! WHY THE SERVER CALLS ITS OWN HANDLER INSTEAD OF SENDING TO ITSELF.
	//! Rpc(..., RplRcver.Server) sends a message TO the server. On a machine
	//! that already IS the server -- a listen server, a Game Master hosting,
	//! and every Workbench play session -- there is nobody to send it to and
	//! the call quietly does nothing. The symptom is exact and was paid for:
	//! the line you were drawing vanished on the closing click and nothing
	//! replaced it, because the stroke was never recorded.
	void AskAdd(notnull array<float> points, int colour, int widthMetres)
	{
		if (points.Count() < 4)
			return;

		string packed = PackPoints(points);
		int player = LocalPlayer();

		if (Replication.IsServer())
			RpcAsk_Add(packed, colour, widthMetres, player);
		else
			Rpc(RpcAsk_Add, packed, colour, widthMetres, player);
	}

	//------------------------------------------------------------------------
	void AskRemove(int id)
	{
		int player = LocalPlayer();

		if (Replication.IsServer())
			RpcAsk_Remove(id, player);
		else
			Rpc(RpcAsk_Remove, id, player);
	}

	//------------------------------------------------------------------------
	void AskClearMine()
	{
		AskClear(false);
	}

	//------------------------------------------------------------------------
	void AskClearAll()
	{
		AskClear(true);
	}

	//------------------------------------------------------------------------
	protected void AskClear(bool everybody)
	{
		int player = LocalPlayer();

		if (Replication.IsServer())
			RpcAsk_Clear(everybody, player);
		else
			Rpc(RpcAsk_Clear, everybody, player);
	}

	//------------------------------------------------------------------------
	//! WHY THE CALLER SENDS ITS OWN ID. An Enfusion RPC does not carry who
	//! sent it, and asking the server for "the" player controller gives the
	//! server's own -- which is nobody at all on a dedicated server. So the
	//! client names itself, the way vanilla's own player-addressed RPCs do.
	//! The cost is that a modified client could rub out somebody else's lines;
	//! for a drawing on a briefing map that is a smaller problem than the
	//! feature not working on a dedicated server at all.
	protected int LocalPlayer()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return -1;

		return controller.GetPlayerId();
	}

	//------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Add(string packed, int colour, int widthMetres, int player)
	{
		if (packed.IsEmpty())
			return;

		string line = m_iNextId.ToString();
		line = line + "|" + colour.ToString();
		line = line + "|" + widthMetres.ToString();
		line = line + "|" + player.ToString();
		line = line + "|" + packed;

		m_iNextId++;

		if (!m_sStrokes.IsEmpty())
			line = m_sStrokes + ";" + line;

		m_sStrokes = Trim(line);

		Replication.BumpMe();
		OnStrokesReplicated();
	}

	//------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Remove(int id, int player)
	{
		array<string> lines = {};
		m_sStrokes.Split(";", lines, true);

		string kept;

		foreach (string line : lines)
		{
			array<string> parts = {};
			line.Split("|", parts, true);

			if (parts.Count() < 5)
				continue;

			// The one asked for goes, but only for the person who drew it --
			// a Game Master's own client already knows it may, and the server
			// checks again here because a client's word is not a permission.
			if (parts[0].ToInt() == id && parts[3].ToInt() == player)
				continue;

			if (!kept.IsEmpty())
				kept = kept + ";";

			kept = kept + line;
		}

		if (kept == m_sStrokes)
			return;

		m_sStrokes = kept;

		Replication.BumpMe();
		OnStrokesReplicated();
	}

	//------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Clear(bool everybody, int player)
	{
		array<string> lines = {};
		m_sStrokes.Split(";", lines, true);

		string kept;

		foreach (string line : lines)
		{
			if (everybody)
				break;

			array<string> parts = {};
			line.Split("|", parts, true);

			if (parts.Count() < 5)
				continue;

			if (parts[3].ToInt() == player)
				continue;

			if (!kept.IsEmpty())
				kept = kept + ";";

			kept = kept + line;
		}

		m_sStrokes = kept;

		Replication.BumpMe();
		OnStrokesReplicated();
	}

	//------------------------------------------------------------------------
	// THE WIRE
	//------------------------------------------------------------------------

	//! Metres, rounded. A drawing is a gesture, not a survey: whole metres
	//! are finer than the line is wide on any board, and they halve what
	//! travels.
	protected string PackPoints(notnull array<float> points)
	{
		string packed;

		foreach (int i, float value : points)
		{
			if (i > 0)
				packed = packed + ",";

			packed = packed + Math.Round(value).ToString();
		}

		return packed;
	}

	//------------------------------------------------------------------------
	//! Drop the oldest strokes until the set fits its budget.
	protected string Trim(string all)
	{
		while (all.Length() > BUDGET)
		{
			int cut = all.IndexOf(";");
			if (cut < 0)
				return all;

			all = all.Substring(cut + 1, all.Length() - cut - 1);
		}

		return all;
	}

	//------------------------------------------------------------------------
	//! Runs on every machine when the drawings change.
	protected void OnStrokesReplicated()
	{
		m_aStrokes.Clear();

		array<string> lines = {};
		m_sStrokes.Split(";", lines, true);

		foreach (string line : lines)
		{
			array<string> parts = {};
			line.Split("|", parts, true);

			if (parts.Count() < 5)
				continue;

			array<string> numbers = {};
			parts[4].Split(",", numbers, true);

			// A stroke is a pair of numbers per point, so an odd count is a
			// stroke that arrived broken and is better skipped than drawn
			// with its last point invented.
			if (numbers.Count() < 4 || numbers.Count() % 2 != 0)
				continue;

			MCF_Map_Stroke stroke = new MCF_Map_Stroke();
			stroke.m_iId = parts[0].ToInt();
			stroke.m_iColour = parts[1].ToInt();
			stroke.m_iWidth = parts[2].ToInt();
			stroke.m_iOwner = parts[3].ToInt();

			foreach (string number : numbers)
			{
				stroke.m_aPoints.Insert(number.ToFloat());
			}

			m_aStrokes.Insert(stroke);
		}

		m_OnChanged.Invoke();
	}
}
