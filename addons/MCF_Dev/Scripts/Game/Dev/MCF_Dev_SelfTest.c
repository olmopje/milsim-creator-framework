//! Runs the checks that need more than one machine, on its own, and writes a
//! verdict.
//!
//! WHY THIS EXISTS. Four things have been built and unproven for weeks --
//! faction-scoped intel, replicated device profiles, per-client picture
//! fetching, the line audience filter -- and every one of them needs two peers
//! and twenty minutes of clicking to look at. That cost is why they stayed
//! unproven. It is also the wrong cost to keep paying: a person clicking
//! through a checklist gets an impression, and this project has already spent
//! an afternoon on the difference between an impression and a measurement.
//!
//! So: start the peers, and read the log. Nothing to click.
//!
//! HOW IT WORKS. Each check sets something up on the server, asks every
//! connected client one question about what they now hold, waits, and compares
//! the answers against what the server knows it did. The client reports facts
//! and never verdicts -- see MCF_PlayerController_SelfTest for why.
//!
//! IT ONLY EVER READS AND ASKS, with one exception: the intel check writes two
//! records, one per faction, because a filter cannot be observed without
//! something to filter. They are marked in their heading and cleared when the
//! run finishes, and the whole component lives in MCF_Dev, which never ships.
//!
//! WHAT A RUN LOOKS LIKE IN THE LOG:
//!
//!   [MCF] SELFTEST waiting for 2 player(s), have 1
//!   [MCF] SELFTEST start -- 2 player(s): 2 (USSR), 3 (US)
//!   [MCF] SELFTEST faction-intel: PASS -- player 2 (USSR) holds selftest-ussr and not selftest-us
//!   [MCF] SELFTEST line-audience: FAIL -- player 3 (US) filtered a line addressed to US
//!   [MCF] SELFTEST done -- 3 passed, 1 failed
//!
//! A FAILED CHECK NAMES THE MACHINE AND WHAT IT HELD. "line-audience: FAIL" on
//! its own would send somebody back to clicking, which is the thing this is
//! for avoiding.

[ComponentEditorProps(category: "MCF/Dev", description: "Runs the multiplayer checks by itself once enough players have joined. Development only.")]
class MCF_Dev_SelfTestComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_Dev_SelfTestComponent : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "0", desc: "Run the multiplayer checks once enough players have joined. Leave off for a normal session.")]
	protected bool m_bEnabled;

	[Attribute(defvalue: "2", desc: "How many players to wait for before starting. The checks need at least two, on two factions.")]
	protected int m_iExpectedPlayers;

	[Attribute(defvalue: "180", desc: "Seconds to wait for them before giving up and saying so.")]
	protected int m_iWaitSeconds;

	[Attribute(defvalue: "https://raw.githubusercontent.com/olmopje/milsim-creator-framework/main/content/images/locomotive.txt", desc: "The picture every client is asked to fetch.")]
	protected string m_sPictureUrl;

	protected float m_fWaited;
	protected bool m_bStarted;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!m_bEnabled)
			return;

		if (!Replication.IsServer())
			return;

		MCF_Core_Log.Debug("SELFTEST armed -- waiting for " + m_iExpectedPlayers.ToString() + " player(s)");
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bStarted)
			return;

		m_fWaited = m_fWaited + timeSlice;

		// WAITING FOR FACTIONS, NOT FOR BODIES. The first run started the moment
		// two players existed, which was before either had picked a side: every
		// filter check then compared nothing against nothing and failed for a
		// reason that had nothing to do with the code under test. A player
		// without a faction is a player who is not ready to be asked.
		int withFaction = MCF_Dev_SelfTest.PlayersWithAFaction().Count();

		if (withFaction >= m_iExpectedPlayers)
		{
			m_bStarted = true;
			MCF_Dev_SelfTest.Begin(m_sPictureUrl);
			return;
		}

		if (m_fWaited > m_iWaitSeconds)
		{
			m_bStarted = true;
			MCF_Core_Log.Warn("SELFTEST gave up waiting -- " + withFaction.ToString() + " of "
				+ m_iExpectedPlayers.ToString() + " player(s) had picked a faction after "
				+ m_iWaitSeconds.ToString() + "s. Everyone has to be on a side before the checks mean anything.");
			return;
		}

		// Once every few seconds, not every frame.
		if (m_fWaited > 5.0 && (m_fWaited - timeSlice) <= 5.0)
			MCF_Core_Log.Debug("SELFTEST waiting for " + m_iExpectedPlayers.ToString()
				+ " player(s) on a faction, have " + withFaction.ToString()
				+ " of " + ConnectedPlayers().Count().ToString() + " connected");
	}

	protected array<int> ConnectedPlayers()
	{
		array<int> players = {};

		PlayerManager manager = GetGame().GetPlayerManager();
		if (manager)
			manager.GetPlayers(players);

		return players;
	}
}

//! The run itself. Static because a check spans several frames and several
//! machines, and the thing keeping track has to outlive any one of them.
class MCF_Dev_SelfTest
{
	static const string CHECK_FACTION = "faction-intel";
	static const string CHECK_LINE = "line-audience";
	static const string CHECK_PROFILE = "device-profile";
	static const string CHECK_PICTURE = "picture-fetch";

	//! How long a client is given to answer. Generous: the picture check
	//! includes a fetch over the internet, and a check that fails because it
	//! was impatient is worse than one that takes an extra few seconds.
	protected static const int ANSWER_SECONDS = 8;

	protected static ref map<int, string> s_mAnswers;
	protected static string s_sRunning;
	protected static string s_sPictureUrl;

	protected static int s_iPassed;
	protected static int s_iFailed;

	//! Ids of the records this run created, so they can be taken away again.
	protected static ref array<string> s_aCreated;

	//! WHO THIS RUN IS ABOUT, decided once at the start.
	//!
	//! The first run read the player list afresh at each verdict, and somebody
	//! joining halfway through produced "player 3 did not answer" for a check
	//! that was never put to player 3. A run is about the people who were there
	//! when it began.
	protected static ref array<int> s_aPlayers;

	//! Set while the picture check is waiting for its second answer.
	protected static bool s_bPictureSecondPass;

	static void Begin(string pictureUrl)
	{
		s_sPictureUrl = pictureUrl;
		s_iPassed = 0;
		s_iFailed = 0;
		s_aCreated = {};
		s_mAnswers = new map<int, string>();
		s_bPictureSecondPass = false;

		s_aPlayers = PlayersWithAFaction();

		array<int> players = Players();

		string who = "";
		foreach (int i, int playerId : players)
		{
			if (i > 0)
				who = who + ", ";

			who = who + playerId.ToString() + " (" + FactionOf(playerId) + ")";
		}

		MCF_Core_Log.Debug("SELFTEST start -- " + players.Count().ToString() + " player(s): " + who);

		// Two factions or the filter checks cannot say anything. Said out loud
		// rather than reported as a pass, because "everyone saw everything" is
		// exactly what a broken filter looks like too.
		if (DistinctFactions(players).Count() < 2)
			MCF_Core_Log.Warn("SELFTEST: everybody is on the same faction -- the filter checks cannot distinguish a working filter from an absent one");

		StartFactionCheck();
	}

	//! Called from the player controller when a client answers.
	static void Report(int playerId, string check, string answer)
	{
		if (check != s_sRunning)
		{
			MCF_Core_Log.Debug("SELFTEST ignoring a late answer to '" + check + "' from player " + playerId.ToString());
			return;
		}

		s_mAnswers.Set(playerId, answer);
	}

	// ------------------------------------------------------ 1. faction intel

	protected static void StartFactionCheck()
	{
		s_sRunning = CHECK_FACTION;
		s_mAnswers.Clear();

		array<string> factions = DistinctFactions(Players());

		// One record per faction present, each named after the faction that
		// should be able to see it, so a wrong answer names its own fault.
		foreach (string faction : factions)
		{
			MCF_Intel_Record record = MCF_Intel_Store.GetInstance().Log(
				"Self test", "SELFTEST " + faction, "", "Written by the self test. Safe to delete.", 0, faction);

			if (record)
				s_aCreated.Insert(record.m_sId);
		}

		PushBoards();
		Ask(CHECK_FACTION, "");

		// CallLater is given the method directly. It must NOT be wrapped in a
		// helper taking a func: Enforce refuses a func as a parameter to a
		// script method, which is the same rule that shapes the device editor's
		// button handling.
		GetGame().GetCallqueue().CallLater(FinishFactionCheck, ANSWER_SECONDS * 1000, false);
	}

	protected static void FinishFactionCheck()
	{
		foreach (int playerId : Players())
		{
			string answer = AnswerFrom(playerId);
			if (answer.IsEmpty())
			{
				Fail(CHECK_FACTION, "player " + playerId.ToString() + " did not answer");
				continue;
			}

			array<string> parts = {};
			answer.Split("|", parts, false);
			if (parts.Count() < 2)
			{
				Fail(CHECK_FACTION, "player " + playerId.ToString() + " answered '" + answer + "', which is not an answer");
				continue;
			}

			string theirFaction = parts[0];
			string held = parts[1];

			bool holdsOwn = Holds(held, IdFor(theirFaction));
			bool holdsOther = false;

			foreach (string faction : DistinctFactions(Players()))
			{
				if (faction == theirFaction)
					continue;

				if (Holds(held, IdFor(faction)))
					holdsOther = true;
			}

			if (holdsOwn && !holdsOther)
				Pass(CHECK_FACTION, "player " + playerId.ToString() + " (" + theirFaction + ") holds its own record and not the other faction's");
			else if (!holdsOwn)
				Fail(CHECK_FACTION, "player " + playerId.ToString() + " (" + theirFaction + ") is missing its own faction's record -- holds " + held);
			else
				Fail(CHECK_FACTION, "player " + playerId.ToString() + " (" + theirFaction + ") can see another faction's record -- holds " + held);
		}

		StartLineCheck();
	}

	//! The record id this run created for a faction, found by its heading.
	protected static string IdFor(string faction)
	{
		array<MCF_Intel_Record> all = {};
		MCF_Intel_Store.GetInstance().GetAll(all);

		foreach (MCF_Intel_Record record : all)
		{
			if (record.m_sHeading == "SELFTEST " + faction)
				return record.m_sId;
		}

		return "";
	}

	protected static bool Holds(string csv, string id)
	{
		if (id.IsEmpty())
			return false;

		array<string> ids = {};
		csv.Split(",", ids, true);
		return ids.Contains(id);
	}

	// ----------------------------------------------------- 2. line audience

	protected static void StartLineCheck()
	{
		s_sRunning = CHECK_LINE;
		s_mAnswers.Clear();

		array<string> factions = DistinctFactions(Players());
		if (factions.IsEmpty())
		{
			StartProfileCheck();
			return;
		}

		// Addressed to the first faction present. Everyone else should filter
		// it out, and saying so is the whole point.
		string target = factions[0];

		MCF_UI_LineDisplayComponent display = MCF_UI_LineDisplayComponent.GetInstance();
		if (!display)
		{
			Fail(CHECK_LINE, "there is no line display in this world to broadcast through");
			StartProfileCheck();
			return;
		}

		MCF_Core_Log.Debug("SELFTEST broadcasting a line addressed to faction '" + target + "'");
		display.Broadcast("SELFTEST line for " + target, MCF_EAudience.FACTION, target, 0);

		Ask(CHECK_LINE, target);
		GetGame().GetCallqueue().CallLater(FinishLineCheck, ANSWER_SECONDS * 1000, false);
	}

	protected static void FinishLineCheck()
	{
		array<string> factions = DistinctFactions(Players());
		string target = "";
		if (!factions.IsEmpty())
			target = factions[0];

		foreach (int playerId : Players())
		{
			string answer = AnswerFrom(playerId);
			if (answer.IsEmpty())
			{
				Fail(CHECK_LINE, "player " + playerId.ToString() + " did not answer");
				continue;
			}

			array<string> parts = {};
			answer.Split("|", parts, false);
			if (parts.Count() < 3)
			{
				Fail(CHECK_LINE, "player " + playerId.ToString() + " answered '" + answer + "'");
				continue;
			}

			string theirFaction = parts[0];
			int shown = parts[1].ToInt();
			int filtered = parts[2].ToInt();

			bool shouldSee = theirFaction == target;

			if (shouldSee && shown > 0)
				Pass(CHECK_LINE, "player " + playerId.ToString() + " (" + theirFaction + ") was shown the line addressed to it");
			else if (!shouldSee && filtered > 0)
				Pass(CHECK_LINE, "player " + playerId.ToString() + " (" + theirFaction + ") filtered out a line for " + target);
			else if (shouldSee)
				Fail(CHECK_LINE, "player " + playerId.ToString() + " (" + theirFaction + ") never saw a line addressed to it -- shown " + shown.ToString() + ", filtered " + filtered.ToString());
			else
				Fail(CHECK_LINE, "player " + playerId.ToString() + " (" + theirFaction + ") did not filter a line for " + target + " -- shown " + shown.ToString() + ", filtered " + filtered.ToString());
		}

		StartProfileCheck();
	}

	// ---------------------------------------------------- 3. device profile

	protected static void StartProfileCheck()
	{
		s_sRunning = CHECK_PROFILE;
		s_mAnswers.Clear();

		MCF_Intel_CarrierComponent carrier = MCF_Intel_CarrierComponent.FirstRegistered();
		if (!carrier)
		{
			MCF_Core_Log.Warn("SELFTEST " + CHECK_PROFILE + ": SKIPPED -- there is no device in this world to write to");
			StartPictureCheck();
			return;
		}

		IEntity owner = carrier.GetOwner();

		// ADDRESSED BY ITS EDITABLE COMPONENT, not by an RplComponent. The
		// first run sent the RplComponent's id and every client answered "no
		// such object": what the replication tables hold for one of these is
		// the editable component, which is exactly what the device editor
		// already had to learn.
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(owner.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
		{
			MCF_Core_Log.Warn("SELFTEST " + CHECK_PROFILE + ": SKIPPED -- that device is not editable, so it cannot be addressed across machines");
			StartPictureCheck();
			return;
		}

		RplId targetId = Replication.FindItemId(editable);
		if (targetId == RplId.Invalid())
		{
			MCF_Core_Log.Warn("SELFTEST " + CHECK_PROFILE + ": SKIPPED -- that device has no replication id");
			StartPictureCheck();
			return;
		}

		// A profile whose id nothing else uses, so an answer naming it cannot
		// be a leftover from something a person did earlier.
		MCF_Device_Profile profile = new MCF_Device_Profile();
		profile.m_sId = "selftest_profile";
		profile.m_sDeviceName = "Self test device";
		profile.m_aApps = {};

		carrier.SetProfileFromServer(MCF_Device_Script.Serialize(profile));

		MCF_Core_Log.Debug("SELFTEST wrote 'selftest_profile' onto '" + carrier.GetDeviceName() + "'");

		Ask(CHECK_PROFILE, targetId.ToString());
		GetGame().GetCallqueue().CallLater(FinishProfileCheck, ANSWER_SECONDS * 1000, false);
	}

	protected static void FinishProfileCheck()
	{
		foreach (int playerId : Players())
		{
			string answer = AnswerFrom(playerId);

			if (answer == "selftest_profile")
				Pass(CHECK_PROFILE, "player " + playerId.ToString() + " sees the profile the server wrote");
			else if (answer.IsEmpty())
				Fail(CHECK_PROFILE, "player " + playerId.ToString() + " did not answer");
			else
				Fail(CHECK_PROFILE, "player " + playerId.ToString() + " sees '" + answer + "' instead of 'selftest_profile'");
		}

		StartPictureCheck();
	}

	// ----------------------------------------------------- 4. picture fetch

	protected static void StartPictureCheck()
	{
		s_sRunning = CHECK_PICTURE;
		s_mAnswers.Clear();

		if (s_sPictureUrl.IsEmpty())
		{
			MCF_Core_Log.Warn("SELFTEST " + CHECK_PICTURE + ": SKIPPED -- no url configured");
			Finish();
			return;
		}

		// TWO PASSES, and the first one is not a question. Asking once made
		// every client answer "loading", because the answer was given in the
		// same breath as starting the fetch -- the check was measuring its own
		// impatience. The first ask starts the download; the second, seconds
		// later, is the one that counts.
		Ask(CHECK_PICTURE, s_sPictureUrl);
		GetGame().GetCallqueue().CallLater(AskPictureAgain, ANSWER_SECONDS * 1000, false);
	}

	protected static void AskPictureAgain()
	{
		s_bPictureSecondPass = true;
		s_mAnswers.Clear();

		Ask(CHECK_PICTURE, s_sPictureUrl);
		GetGame().GetCallqueue().CallLater(FinishPictureCheck, ANSWER_SECONDS * 1000, false);
	}

	protected static void FinishPictureCheck()
	{
		foreach (int playerId : Players())
		{
			string answer = AnswerFrom(playerId);

			if (answer == "ready")
				Pass(CHECK_PICTURE, "player " + playerId.ToString() + " has the picture on disk");
			else if (answer == "loading")
				Fail(CHECK_PICTURE, "player " + playerId.ToString() + " was still fetching after "
					+ (ANSWER_SECONDS * 2).ToString() + "s -- slow, or never coming");
			else if (answer.IsEmpty())
				Fail(CHECK_PICTURE, "player " + playerId.ToString() + " did not answer");
			else
				Fail(CHECK_PICTURE, "player " + playerId.ToString() + " reports '" + answer + "'");
		}

		Finish();
	}

	// ------------------------------------------------------------- finishing

	protected static void Finish()
	{
		s_sRunning = "";

		// The records this run wrote go away again. A test that leaves its
		// fixtures behind turns into a mission with two reports nobody wrote.
		foreach (string id : s_aCreated)
		{
			MCF_Intel_Store.GetInstance().Delete(id);
		}

		PushBoards();

		MCF_Core_Log.Debug("SELFTEST done -- " + s_iPassed.ToString() + " passed, " + s_iFailed.ToString() + " failed");
	}

	// ------------------------------------------------------------- internals

	protected static void Ask(string check, string argument)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return;

		foreach (int playerId : Players())
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(manager.GetPlayerController(playerId));
			if (controller)
				controller.MCF_AskSelfTest(check, argument);
		}
	}

	protected static string AnswerFrom(int playerId)
	{
		string answer;
		if (s_mAnswers.Find(playerId, answer))
			return answer;

		return "";
	}

	protected static void Pass(string check, string detail)
	{
		s_iPassed++;
		MCF_Core_Log.Debug("SELFTEST " + check + ": PASS -- " + detail);
	}

	protected static void Fail(string check, string detail)
	{
		s_iFailed++;
		MCF_Core_Log.Warn("SELFTEST " + check + ": FAIL -- " + detail);
	}

	//! The players this run is about: the snapshot taken at the start, or
	//! everyone currently connected if no run is under way.
	protected static array<int> Players()
	{
		if (s_aPlayers && !s_aPlayers.IsEmpty())
			return s_aPlayers;

		return Connected();
	}

	protected static array<int> Connected()
	{
		array<int> players = {};

		PlayerManager manager = GetGame().GetPlayerManager();
		if (manager)
			manager.GetPlayers(players);

		return players;
	}

	//! Everyone who has actually picked a side. Used to decide when to start.
	static array<int> PlayersWithAFaction()
	{
		array<int> ready = {};

		foreach (int playerId : Connected())
		{
			if (!MCF_Core_FactionHelper.GetPlayerFactionKey(playerId).IsEmpty())
				ready.Insert(playerId);
		}

		return ready;
	}

	protected static string FactionOf(int playerId)
	{
		string faction = MCF_Core_FactionHelper.GetPlayerFactionKey(playerId);
		if (faction.IsEmpty())
			return "no faction";

		return faction;
	}

	protected static array<string> DistinctFactions(notnull array<int> players)
	{
		array<string> factions = {};

		foreach (int playerId : players)
		{
			string faction = MCF_Core_FactionHelper.GetPlayerFactionKey(playerId);
			if (faction.IsEmpty())
				continue;

			if (!factions.Contains(faction))
				factions.Insert(faction);
		}

		return factions;
	}

	//! Re-sends everyone their board, so a client's answer reflects what the
	//! server just did rather than what it did a minute ago.
	protected static void PushBoards()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		MCF_Ops_GameModeComponent ops = MCF_Ops_GameModeComponent.Cast(gameMode.FindComponent(MCF_Ops_GameModeComponent));
		if (ops)
			ops.RefreshTasksForAllPlayers();
	}
}
