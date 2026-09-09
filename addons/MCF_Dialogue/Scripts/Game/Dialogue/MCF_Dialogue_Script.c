//! Packs a whole conversation into one string, and reads it back.
//!
//! One implementation serves three jobs -- the wire, the persistent store, and
//! the Game Master's editor -- for the same reason MCF_Intel_Record has one:
//! three formats would be three chances to drift, and the one that drifts is
//! always the one nobody tested.
//!
//! THREE LEVELS OF SEPARATOR, because a conversation is three levels deep.
//! Splitting by <<n>> yields a header and a node per block; splitting a node
//! by <<c>> yields the node's own fields and a reply per block; the fields
//! inside each are <<f>> and <<x>> respectively. The sequences are deliberately
//! unlikely: a Game Master typing a sentence must not be able to split a
//! record in half, and every authored field is cleaned of them on the way in.
//!
//! NEWLINES ARE ESCAPED, not stripped. The persistent store is one key=value
//! per line, so a literal newline in a conversation would silently truncate
//! the file at that point and take every later conversation with it.

class MCF_Dialogue_Script
{
	protected static const string NODE_SEP = "<<n>>";
	protected static const string CHOICE_SEP = "<<c>>";
	protected static const string FIELD_SEP = "<<f>>";
	protected static const string CHOICE_FIELD_SEP = "<<x>>";
	protected static const string NEWLINE_ESCAPE = "<nl>";

	//! \return The whole conversation as one line of text.
	static string Serialize(notnull MCF_Dialogue_Conversation conversation)
	{
		string result = Clean(conversation.m_sId) + FIELD_SEP + Clean(conversation.m_sSpeakerName) + FIELD_SEP + Clean(conversation.m_sActionVerb) + FIELD_SEP + Clean(conversation.m_sStartNodeId);

		if (!conversation.m_aNodes)
			return result;

		foreach (MCF_Dialogue_Node node : conversation.m_aNodes)
		{
			if (!node)
				continue;

			result = result + NODE_SEP + Clean(node.m_sId) + FIELD_SEP + Clean(node.m_sText);

			if (!node.m_aChoices)
				continue;

			foreach (MCF_Dialogue_Choice choice : node.m_aChoices)
			{
				if (!choice)
					continue;

				// Built one field at a time rather than as one long chain.
				// Enforce's compiler gives up on a concatenation this size
				// with "Formula too complex", and it names the last token
				// rather than the length, so the message points at the wrong
				// thing entirely.
				string packed = CHOICE_SEP;
				packed = packed + Clean(choice.m_sText) + CHOICE_FIELD_SEP;
				packed = packed + Clean(choice.m_sNextNodeId) + CHOICE_FIELD_SEP;
				packed = packed + Clean(choice.m_sPublishEvent) + CHOICE_FIELD_SEP;
				packed = packed + choice.m_fTrustChange.ToString() + CHOICE_FIELD_SEP;
				packed = packed + choice.m_fFearChange.ToString() + CHOICE_FIELD_SEP;
				packed = packed + choice.m_fMinTrust.ToString() + CHOICE_FIELD_SEP;
				packed = packed + choice.m_fMaxFear.ToString() + CHOICE_FIELD_SEP;
				packed = packed + Clean(choice.m_sLockedReason) + CHOICE_FIELD_SEP;
				packed = packed + Clean(choice.m_sRequiresFlag) + CHOICE_FIELD_SEP;
				packed = packed + Clean(choice.m_sSetFlag);

				result = result + packed;
			}
		}

		return result;
	}

	//! \return The conversation, or null if the text is not one.
	static MCF_Dialogue_Conversation Deserialize(string data)
	{
		if (data.IsEmpty())
			return null;

		array<string> blocks = {};
		data.Split(NODE_SEP, blocks, false);

		if (blocks.IsEmpty())
			return null;

		array<string> header = {};
		blocks[0].Split(FIELD_SEP, header, false);

		MCF_Dialogue_Conversation conversation = new MCF_Dialogue_Conversation();
		conversation.m_aNodes = {};

		if (header.Count() > 0)
			conversation.m_sId = Restore(header[0]);

		if (header.Count() > 1)
			conversation.m_sSpeakerName = Restore(header[1]);

		if (header.Count() > 2)
			conversation.m_sActionVerb = Restore(header[2]);

		if (header.Count() > 3)
			conversation.m_sStartNodeId = Restore(header[3]);

		if (conversation.m_sId.IsEmpty())
			return null;

		for (int i = 1; i < blocks.Count(); i++)
		{
			array<string> nodeParts = {};
			blocks[i].Split(CHOICE_SEP, nodeParts, false);

			if (nodeParts.IsEmpty())
				continue;

			array<string> nodeFields = {};
			nodeParts[0].Split(FIELD_SEP, nodeFields, false);

			MCF_Dialogue_Node node = new MCF_Dialogue_Node();
			node.m_aChoices = {};

			if (nodeFields.Count() > 0)
				node.m_sId = Restore(nodeFields[0]);

			if (nodeFields.Count() > 1)
				node.m_sText = Restore(nodeFields[1]);

			for (int c = 1; c < nodeParts.Count(); c++)
			{
				array<string> fields = {};
				nodeParts[c].Split(CHOICE_FIELD_SEP, fields, false);

				MCF_Dialogue_Choice choice = new MCF_Dialogue_Choice();

				// Defaults matter here. A reply read back with m_fMaxFear at 0
				// would be locked for anybody with a shred of fear, which
				// looks exactly like the trust system being broken.
				choice.m_fMaxFear = 100;

				if (fields.Count() > 0)
					choice.m_sText = Restore(fields[0]);

				if (fields.Count() > 1)
					choice.m_sNextNodeId = Restore(fields[1]);

				if (fields.Count() > 2)
					choice.m_sPublishEvent = Restore(fields[2]);

				if (fields.Count() > 3)
					choice.m_fTrustChange = fields[3].ToFloat();

				if (fields.Count() > 4)
					choice.m_fFearChange = fields[4].ToFloat();

				if (fields.Count() > 5)
					choice.m_fMinTrust = fields[5].ToFloat();

				if (fields.Count() > 6)
					choice.m_fMaxFear = fields[6].ToFloat();

				if (fields.Count() > 7)
					choice.m_sLockedReason = Restore(fields[7]);

				// APPENDED, not inserted. The format is positional, so a new
				// field goes on the end and every count check above it stays
				// true -- conversations saved before these existed load with
				// them empty rather than shifting every value by one.
				if (fields.Count() > 8)
					choice.m_sRequiresFlag = Restore(fields[8]);

				if (fields.Count() > 9)
					choice.m_sSetFlag = Restore(fields[9]);

				node.m_aChoices.Insert(choice);
			}

			conversation.m_aNodes.Insert(node);
		}

		return conversation;
	}

	//! Strips the separators out of authored text so a Game Master cannot
	//! break the format by typing it, and escapes newlines so the persistent
	//! store's one-line-per-key format survives.
	protected static string Clean(string value)
	{
		if (value.IsEmpty())
			return value;

		string result = value;
		result.Replace(NODE_SEP, " ");
		result.Replace(CHOICE_SEP, " ");
		result.Replace(FIELD_SEP, " ");
		result.Replace(CHOICE_FIELD_SEP, " ");
		result.Replace("\r", "");
		result.Replace("\n", NEWLINE_ESCAPE);
		return result;
	}

	protected static string Restore(string value)
	{
		if (value.IsEmpty())
			return value;

		string result = value;
		result.Replace(NEWLINE_ESCAPE, "\n");
		return result;
	}
}
