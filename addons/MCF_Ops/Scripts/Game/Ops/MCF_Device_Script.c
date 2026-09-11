//! A device profile as one line of text, and back again.
//!
//! WHY TEXT AND NOT A BINARY FORMAT. It has to survive two journeys: into
//! MCF_Core_PersistentStore, which stores strings, and across an RPC to the
//! Game Master editing it. One representation for both means there is one place
//! a profile can be malformed instead of two.
//!
//! THE SEPARATORS ARE THE WHOLE DESIGN, so they are worth stating plainly:
//!
//!   <<p>>   between the profile header and the apps
//!   <<a>>   between apps
//!   <<i>>   between items
//!   <<f>>   between fields
//!
//! They are long and angular because the alternative is a comma, and a mission
//! maker who types a comma into a message would then split it into two fields
//! that resolve to nothing. Every field is cleaned on the way out rather than
//! escaped: a separator typed into text is replaced with a space, which loses a
//! character and never loses a message. MCF_Intel_CarrierComponent already
//! works this way and the reasoning is the same.
//!
//! APPEND FIELDS, NEVER INSERT. A profile written by an earlier version is read
//! by a later one, from a store that survives restarts. Reading is therefore
//! written to tolerate FEWER fields than it expects, and every reader takes the
//! count into account rather than assuming.

class MCF_Device_Script
{
	protected static const string SEP_PROFILE = "<<p>>";
	protected static const string SEP_APP = "<<a>>";
	protected static const string SEP_ITEM = "<<i>>";
	protected static const string SEP_FIELD = "<<f>>";

	static string Serialize(MCF_Device_Profile profile)
	{
		if (!profile)
			return "";

		string result = Clean(profile.m_sId)
			+ SEP_FIELD + Clean(profile.m_sDeviceName)
			+ SEP_FIELD + profile.m_eShell.ToString();

		result = result + SEP_PROFILE;

		if (!profile.m_aApps)
			return result;

		bool firstApp = true;
		foreach (MCF_Device_App app : profile.m_aApps)
		{
			if (!app)
				continue;

			if (!firstApp)
				result = result + SEP_APP;
			firstApp = false;

			result = result + SerializeApp(app);
		}

		return result;
	}

	protected static string SerializeApp(notnull MCF_Device_App app)
	{
		string result = app.m_eKind.ToString()
			+ SEP_FIELD + Clean(app.m_sLabel)
			+ SEP_FIELD + Clean(app.m_sEmptyText);

		if (!app.m_aItems)
			return result;

		foreach (MCF_Device_Item item : app.m_aItems)
		{
			if (!item)
				continue;

			// APPENDED, NEVER INSERTED -- see the note at the top of this file.
			// A profile written before pictures existed has three fields here
			// and is read back correctly because DeserializeApp counts.
			result = result + SEP_ITEM
				+ Clean(item.m_sHeading) + SEP_FIELD
				+ Clean(item.m_sTimestamp) + SEP_FIELD
				+ Clean(item.m_sBody) + SEP_FIELD
				+ Clean(item.m_sImage) + SEP_FIELD
				+ Clean(item.m_sImageUrl) + SEP_FIELD
				+ Flag(item.m_bNew) + SEP_FIELD
				+ Clean(item.m_sStyle);
		}

		return result;
	}

	static MCF_Device_Profile Deserialize(string text)
	{
		if (text.IsEmpty())
			return null;

		array<string> halves = {};
		text.Split(SEP_PROFILE, halves, false);
		if (halves.IsEmpty())
			return null;

		array<string> header = {};
		halves[0].Split(SEP_FIELD, header, false);
		if (header.IsEmpty())
			return null;

		MCF_Device_Profile profile = new MCF_Device_Profile();
		profile.m_sId = header[0];

		if (header.Count() > 1)
			profile.m_sDeviceName = header[1];

		if (header.Count() > 2)
			profile.m_eShell = header[2].ToInt();

		profile.m_aApps = {};

		if (halves.Count() < 2 || halves[1].IsEmpty())
			return profile;

		array<string> apps = {};
		halves[1].Split(SEP_APP, apps, false);

		foreach (string encoded : apps)
		{
			MCF_Device_App app = DeserializeApp(encoded);
			if (app)
				profile.m_aApps.Insert(app);
		}

		return profile;
	}

	protected static MCF_Device_App DeserializeApp(string text)
	{
		if (text.IsEmpty())
			return null;

		array<string> parts = {};
		text.Split(SEP_ITEM, parts, false);
		if (parts.IsEmpty())
			return null;

		array<string> header = {};
		parts[0].Split(SEP_FIELD, header, false);
		if (header.IsEmpty())
			return null;

		MCF_Device_App app = new MCF_Device_App();
		app.m_eKind = header[0].ToInt();

		if (header.Count() > 1)
			app.m_sLabel = header[1];

		if (header.Count() > 2)
			app.m_sEmptyText = header[2];

		app.m_aItems = {};

		for (int i = 1; i < parts.Count(); i++)
		{
			array<string> fields = {};
			parts[i].Split(SEP_FIELD, fields, false);
			if (fields.IsEmpty())
				continue;

			MCF_Device_Item item = new MCF_Device_Item();
			item.m_sHeading = fields[0];

			if (fields.Count() > 1)
				item.m_sTimestamp = fields[1];

			if (fields.Count() > 2)
				item.m_sBody = fields[2];

			if (fields.Count() > 3)
				item.m_sImage = fields[3];

			if (fields.Count() > 4)
				item.m_sImageUrl = fields[4];

			if (fields.Count() > 5)
				item.m_bNew = fields[5].ToInt() != 0;

			if (fields.Count() > 6)
				item.m_sStyle = fields[6];

			app.m_aItems.Insert(item);
		}

		return app;
	}

	//! Takes the separators out of authored text.
	//!
	//! Replaced with a space rather than escaped. Escaping means an unescaper,
	//! and an unescaper is a second place the format can be wrong. A mission
	//! maker who types "<<f>>" into a message loses five characters they did
	//! not mean to type; the alternative is losing the message.
	protected static string Flag(bool value)
	{
		if (value)
			return "1";

		return "0";
	}

	protected static string Clean(string value)
	{
		string result = value;
		result.Replace(SEP_PROFILE, " ");
		result.Replace(SEP_APP, " ");
		result.Replace(SEP_ITEM, " ");
		result.Replace(SEP_FIELD, " ");
		result.Replace("\n", " ");
		result.Replace("\r", "");
		return Trim(result);
	}

	//! Space off both ends.
	//!
	//! WHY THIS IS NOT COSMETIC. A url typed or pasted with a stray space in
	//! front is still a url to a human and is not one to RestApi: the request
	//! goes out and comes back with http 0, which reads as a network failure
	//! rather than as a typing mistake. Nothing between the edit box and the
	//! socket would have caught it. Every authored field goes through Clean, so
	//! this is the one place that has to know.
	//!
	//! Written out rather than calling the engine's trim, because this runs on
	//! every field of every profile and the loop is three lines.
	static string Trim(string value)
	{
		int first = 0;
		int last = value.Length() - 1;

		while (first <= last && IsSpace(value.Get(first)))
		{
			first++;
		}

		while (last >= first && IsSpace(value.Get(last)))
		{
			last--;
		}

		if (last < first)
			return "";

		return value.Substring(first, last - first + 1);
	}

	protected static bool IsSpace(string character)
	{
		return character == " " || character == "\t" || character == "\n" || character == "\r";
	}
}
