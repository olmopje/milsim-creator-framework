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

	//! An index into MCF_Map_DrawingComponent's palette, not a colour. A
	//! colour is four floats and this is one small number on the wire.
	int m_iColour;

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
	//! The palette, by index. Small and fixed, because the index is what
	//! travels: naming a colour costs one number instead of four floats, and
	//! a shared palette is also what stops a briefing turning into a rainbow.
	protected static const ref array<int> PALETTE = {
		0xFFFFFFFF,		// white
		0xFFE03030,		// red
		0xFF3070E0,		// blue
		0xFF40C040,		// green
		0xFFE0C020,		// yellow
		0xFF101010		// black
	};

	//! How much drawing the mission may hold, in characters of the
	//! replicated string. Past this the oldest stroke goes, which is the
	//! right one to lose: a briefing moves on.
	protected static const int BUDGET = 24000;

	[RplProp(onRplName: "OnStrokesReplicated")]
	protected string m_sStrokes;

	protected ref array<ref MCF_Map_Stroke> m_aStrokes = {};
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();

	protected static MCF_Map_DrawingComponent s_Instance;

	//------------------------------------------------------------------------
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
	static int Colour(int index)
	{
		if (index < 0 || index >= PALETTE.Count())
			return PALETTE[0];

		return PALETTE[index];
	}

	//------------------------------------------------------------------------
	static int ColourCount()
	{
		return PALETTE.Count();
	}

	//------------------------------------------------------------------------
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
	void AskAdd(notnull array<float> points, int colour)
	{
		if (points.Count() < 4)
			return;

		string packed = Pack(points);
		int player = LocalPlayer();

		if (Replication.IsServer())
			RpcAsk_Add(packed, colour, player);
		else
			Rpc(RpcAsk_Add, packed, colour, player);
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
	protected void RpcAsk_Add(string packed, int colour, int player)
	{
		if (packed.IsEmpty())
			return;

		string line = colour.ToString() + "|" + player.ToString() + "|" + packed;

		if (!m_sStrokes.IsEmpty())
			line = m_sStrokes + ";" + line;

		m_sStrokes = Trim(line);

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

			if (parts.Count() < 3)
				continue;

			if (parts[1].ToInt() == player)
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
	protected string Pack(notnull array<float> points)
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

			if (parts.Count() < 3)
				continue;

			array<string> numbers = {};
			parts[2].Split(",", numbers, true);

			// A stroke is a pair of numbers per point, so an odd count is a
			// stroke that arrived broken and is better skipped than drawn
			// with its last point invented.
			if (numbers.Count() < 4 || numbers.Count() % 2 != 0)
				continue;

			MCF_Map_Stroke stroke = new MCF_Map_Stroke();
			stroke.m_iColour = parts[0].ToInt();
			stroke.m_iOwner = parts[1].ToInt();

			foreach (string number : numbers)
			{
				stroke.m_aPoints.Insert(number.ToFloat());
			}

			m_aStrokes.Insert(stroke);
		}

		m_OnChanged.Invoke();
	}
}
