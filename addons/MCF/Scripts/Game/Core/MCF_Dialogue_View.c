//! One screenful of conversation, as the server hands it to a client.
//!
//! The client is never told the graph. It is told what this person is saying
//! right now, which replies exist, and which of those are available -- nothing
//! about trust, fear, flags or where a reply leads. A client that held the
//! tree could read ahead and see every outcome, and a client that held the
//! numbers could compute exactly how to play every civilian in the mission.
//! Both would be worse games, and neither is needed to draw a screen.
//!
//! The reply the client sends back is an INDEX into this view, not a node id
//! and not an effect. The server re-derives the same view from its own state
//! and applies index N of it. So a client can pick a reply it was not offered
//! only by picking one that does not exist, which is a bounds check.
//!
//! Serialises itself the way MCF_Intel_Record does, and for the same reason:
//! one implementation shared by the wire means the two cannot drift.

//! One reply as the player sees it.
class MCF_Dialogue_ChoiceView
{
	//! Position in the view. What the client sends back.
	int m_iIndex;

	string m_sText;

	//! Whether it can be picked. A locked reply is still sent, so the player
	//! can see there was something there they could not say -- that is the
	//! whole feedback loop for trust and fear. Replies the author marked
	//! hidden are not sent at all.
	bool m_bEnabled;

	//! Shown when locked. "He is too frightened to answer that."
	string m_sLockedReason;
}

class MCF_Dialogue_View
{
	protected static const string ENTRY_SEP = "<<d>>";
	protected static const string FIELD_SEP = "<<v>>";

	string m_sSpeaker;

	//! Which node this is. Sent back with the reply so the server can refuse
	//! a click that arrives after the conversation has already moved on --
	//! double-clicks and lag both produce that, and without this the second
	//! click applies a reply from a screen the player is no longer looking at.
	string m_sNodeId;

	string m_sText;

	//! True when there is nothing more to say. The client shows a way out and
	//! no replies.
	bool m_bEnded;

	ref array<ref MCF_Dialogue_ChoiceView> m_aChoices;

	void MCF_Dialogue_View()
	{
		m_aChoices = {};
	}

	string Serialize()
	{
		string ended = "0";
		if (m_bEnded)
			ended = "1";

		string result = Clean(m_sSpeaker) + FIELD_SEP + Clean(m_sNodeId) + FIELD_SEP + Clean(m_sText) + FIELD_SEP + ended;

		foreach (MCF_Dialogue_ChoiceView choice : m_aChoices)
		{
			string enabled = "0";
			if (choice.m_bEnabled)
				enabled = "1";

			result = result + ENTRY_SEP + choice.m_iIndex.ToString() + FIELD_SEP + Clean(choice.m_sText) + FIELD_SEP + enabled + FIELD_SEP + Clean(choice.m_sLockedReason);
		}

		return result;
	}

	static MCF_Dialogue_View Deserialize(string data)
	{
		if (data.IsEmpty())
			return null;

		array<string> blocks = {};
		data.Split(ENTRY_SEP, blocks, false);

		if (blocks.IsEmpty())
			return null;

		array<string> header = {};
		blocks[0].Split(FIELD_SEP, header, false);

		MCF_Dialogue_View view = new MCF_Dialogue_View();

		if (header.Count() > 0)
			view.m_sSpeaker = header[0];

		if (header.Count() > 1)
			view.m_sNodeId = header[1];

		if (header.Count() > 2)
			view.m_sText = header[2];

		if (header.Count() > 3)
			view.m_bEnded = header[3] == "1";

		for (int i = 1; i < blocks.Count(); i++)
		{
			array<string> fields = {};
			blocks[i].Split(FIELD_SEP, fields, false);

			MCF_Dialogue_ChoiceView choice = new MCF_Dialogue_ChoiceView();

			if (fields.Count() > 0)
				choice.m_iIndex = fields[0].ToInt();

			if (fields.Count() > 1)
				choice.m_sText = fields[1];

			if (fields.Count() > 2)
				choice.m_bEnabled = fields[2] == "1";

			if (fields.Count() > 3)
				choice.m_sLockedReason = fields[3];

			view.m_aChoices.Insert(choice);
		}

		return view;
	}

	//! Strips the separators out of authored text so a mission maker cannot
	//! break the format by typing it.
	protected static string Clean(string value)
	{
		if (value.IsEmpty())
			return value;

		string result = value;
		result.Replace(ENTRY_SEP, " ");
		result.Replace(FIELD_SEP, " ");
		result.Replace("\r", "");
		return result;
	}
}
