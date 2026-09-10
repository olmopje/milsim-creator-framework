//! The client's half of the self-test. Development only: this file lives in
//! MCF_Dev and never ships.
//!
//! WHY A CLIENT HALF EXISTS AT ALL. Three of the four things that have been
//! unproven for weeks are unprovable from the server, because what is being
//! asked is "did this arrive over there": a replicated device profile, a
//! photograph each machine fetches for itself, a line of text every client
//! decides about on its own. The server can only ever see that it sent
//! something. So the server asks, and the machine that knows answers.
//!
//! THE ANSWERS ARE FACTS, NOT VERDICTS. A client reports what it holds and
//! never whether that is correct -- the expectation lives on the server, in one
//! place, next to the thing that set it up. A client that decided for itself
//! whether it had passed would be a test that agrees with whatever the bug did.

modded class SCR_PlayerController
{
	//! Runs on the owning client. One question, one answer.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_SelfTestAsk(string check, string argument)
	{
		string answer = MCF_AnswerSelfTest(check, argument);

		MCF_Core_Log.Debug("selftest: asked '" + check + "', answering '" + answer + "'");

		Rpc(MCF_RpcAsk_SelfTestReport, check, answer);
	}

	//! Runs on the server. Hands the answer to whoever is running the test.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_SelfTestReport(string check, string answer)
	{
		if (!Replication.IsServer())
			return;

		MCF_Dev_SelfTest.Report(GetPlayerId(), check, answer);
	}

	//! Server side. Puts one question to this controller's player.
	void MCF_AskSelfTest(string check, string argument)
	{
		Rpc(MCF_RpcDo_SelfTestAsk, check, argument);
	}

	// ------------------------------------------------------------- answering

	protected string MCF_AnswerSelfTest(string check, string argument)
	{
		if (check == MCF_Dev_SelfTest.CHECK_FACTION)
			return MCF_AnswerFaction();

		if (check == MCF_Dev_SelfTest.CHECK_LINE)
			return MCF_AnswerLine();

		if (check == MCF_Dev_SelfTest.CHECK_PROFILE)
			return MCF_AnswerProfile(argument);

		if (check == MCF_Dev_SelfTest.CHECK_PICTURE)
			return MCF_AnswerPicture(argument);

		return "unknown check";
	}

	//! What this machine believes it holds, so the server can compare it with
	//! what it sent.
	protected string MCF_AnswerFaction()
	{
		array<MCF_Intel_Record> records = {};
		MCF_Intel_Store.GetInstance().GetAll(records);

		string ids = "";
		foreach (int i, MCF_Intel_Record record : records)
		{
			if (i > 0)
				ids = ids + ",";
			ids = ids + record.m_sId;
		}

		if (ids.IsEmpty())
			ids = "none";

		return MCF_Core_FactionHelper.GetPlayerFactionKey(GetPlayerId()) + "|" + ids;
	}

	//! Whether the last broadcast line was shown here or filtered out.
	//!
	//! Counters rather than a flag, because "shown" and "filtered" are two
	//! different outcomes and "neither" is a third -- a line that never arrived
	//! at all is the failure that a boolean would hide.
	protected string MCF_AnswerLine()
	{
		return MCF_Core_FactionHelper.GetPlayerFactionKey(GetPlayerId())
			+ "|" + MCF_UI_LineDisplayComponent.GetShownCount().ToString()
			+ "|" + MCF_UI_LineDisplayComponent.GetFilteredCount().ToString();
	}

	//! Which device profile this machine sees on one object.
	protected string MCF_AnswerProfile(string argument)
	{
		RplId targetId = argument.ToInt();

		// THE EDITABLE COMPONENT, matching what the server sent. Resolving this
		// as an RplComponent is what produced "no such object" on every client
		// for two runs: the server had already been corrected to send the
		// editable component's id and this end had not, so both halves were
		// individually reasonable and disagreed about what the number meant.
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(targetId));
		if (!editable)
			return "no such object";

		IEntity owner = editable.GetOwner();
		if (!owner)
			return "no such object";

		MCF_Intel_CarrierComponent carrier = MCF_Intel_CarrierComponent.Cast(owner.FindComponent(MCF_Intel_CarrierComponent));
		if (!carrier)
			return "not a device";

		// Not called 'override' -- that is a keyword, and Enforce rejects it as
		// an identifier the same way it rejects 'set' and 'reference'.
		string profile = carrier.GetProfileOverride();
		if (profile.IsEmpty())
			return "empty";

		MCF_Device_Profile parsed = MCF_Device_Script.Deserialize(profile);
		if (!parsed)
			return "unreadable";

		return parsed.m_sId;
	}

	//! Whether this machine has the picture, or is still getting it.
	//!
	//! Asking starts the fetch as a side effect, which is deliberate: the
	//! server asks twice, and the first ask is what gets the download moving.
	//! A single ask would always be answered "loading", because nothing can
	//! finish between starting it and reporting on it.
	protected string MCF_AnswerPicture(string argument)
	{
		MCF_Device_ImageCache.FetchUrl(argument);

		int state = MCF_Device_ImageCache.StateOf(MCF_Device_Script.Trim(argument));

		if (state == MCF_EImageState.READY)
			return "ready";

		if (state == MCF_EImageState.LOADING)
			return "loading";

		if (state == MCF_EImageState.FAILED)
			return "failed";

		return "none";
	}
}
