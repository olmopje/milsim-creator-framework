//! Makes any prefab an intel object.
//!
//! Put this on a letter, a phone, a photo, a notebook, a body -- anything a
//! player can walk up to. The prefab decides what it looks like in the world;
//! this component decides what it says and how it reads.
//!
//! WHY A COMPONENT AND NOT A SET OF PREFABS: a unit will want a phone with
//! their own messages in it, not the one shipped in MCF. Making the content an
//! attribute means a mission maker builds their own intel out of any model in
//! the game without touching script.
//!
//! ON SECRECY, HONESTLY: text authored here lives in the prefab, and prefab
//! data reaches every client that streams the entity in. So this is secrecy in
//! the fiction, not against a determined datamine. What actually matters for
//! play is enforced elsewhere and properly: intel does not reach the shared
//! operations board until a commander logs it, and that store is server-held
//! and filtered per player like tasks are. Text a Game Master writes at
//! runtime goes through that same server-side path and never sits in a prefab.
//!
//! HARD-WON PREFAB DETAIL, repeated because it cost a full test round on the
//! task board: an action registers fine and still never appears if its
//! UserActionContext has no position. An empty `Position {}` leaves the
//! interaction with no anchor in the world and no radius fixes it. Give the
//! context a `Position PointInfo { Offset 0 <height> 0 }`.

[ComponentEditorProps(category: "MCF/Intel", description: "Makes this object readable as intel: a letter, a phone, a document.")]
class MCF_Intel_CarrierComponentClass : ScriptComponentClass
{
}

class MCF_Intel_CarrierComponent : ScriptComponent
{
	[Attribute(defvalue: "Document", uiwidget: UIWidgets.EditBox, desc: "Name shown at the top of the viewer -- 'Nokia 3310', 'Handwritten letter', 'Field notebook'.")]
	protected string m_sDeviceName;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EIntelView), desc: "How this reads. DOCUMENT is one page. DEVICE is a list of entries, like a phone. MAP is not built yet.")]
	protected MCF_EIntelView m_eView;

	[Attribute(defvalue: "Read", uiwidget: UIWidgets.EditBox, desc: "Verb on the interaction prompt: Read, Examine, Search, Study.")]
	protected string m_sActionVerb;

	[Attribute(desc: "What this object contains. A letter has one entry; a phone has one per message.")]
	protected ref array<ref MCF_Intel_Entry> m_aEntries;

	//! The model to render behind the screen, if this object wants to be drawn
	//! as itself rather than as a flat panel.
	//!
	//! WHY THE OBJECT NAMES ITS OWN MODEL, rather than the shell knowing which
	//! model goes with which view. A phone lives in MCF_Devices and the shell
	//! lives here, and Ops must not learn what is in Devices -- that is the
	//! whole reason presentation moved here in the first place. So the prefab
	//! points at its own model and the shell renders whatever it is handed.
	//! Leave it empty and the shell falls back to the drawn panel, which is
	//! also what happens if the preview system is unavailable.
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, params: "et", desc: "Prefab whose model is rendered behind the screen. Empty draws a plain panel instead.")]
	protected ResourceName m_sPreviewPrefab;

	//! The runtime override, empty until a Game Master edits this object.
	//!
	//! WHY A REPLICATED PROPERTY AND NOT AN RPC: an edit sent as a broadcast
	//! only reaches the machines that are listening at that moment. A client
	//! joining later, or one that streams this object in for the first time
	//! afterwards, would still read the prefab's original text. A replicated
	//! property is carried in the entity's own state, so joining, streaming
	//! and reconnecting all resolve themselves without any push-on-join code.
	[RplProp(onRplName: "OnContentReplicated")]
	protected string m_sContentOverride;

	string GetDeviceName()
	{
		return m_sDeviceName;
	}

	MCF_EIntelView GetView()
	{
		return m_eView;
	}

	//! Empty means "draw the flat panel". Never assume a model is there.
	ResourceName GetPreviewPrefab()
	{
		return m_sPreviewPrefab;
	}

	string GetActionVerb()
	{
		if (m_sActionVerb.IsEmpty())
			return "Read";

		return m_sActionVerb;
	}

	//! \return The readable items, oldest first as authored.
	int GetEntries(notnull out array<MCF_Intel_Entry> outEntries)
	{
		outEntries.Clear();

		if (!m_aEntries)
			return 0;

		foreach (MCF_Intel_Entry entry : m_aEntries)
		{
			if (entry)
				outEntries.Insert(entry);
		}

		return outEntries.Count();
	}

	bool HasContent()
	{
		array<MCF_Intel_Entry> entries = {};
		return GetEntries(entries) > 0;
	}

	// ------------------------------------------------- runtime authoring

	//! Field separators for the wire format. Deliberately different characters
	//! from the ones MCF_Task uses, so that a stray tab typed by a Game Master
	//! into a body of text cannot split a record in half.
	protected static const string ENTRY_SEP = "<<e>>";
	protected static const string FIELD_SEP = "<<f>>";

	//! Packs the whole content into one string for the wire.
	//!
	//! One string rather than parallel arrays because an RPC with three
	//! same-length arrays is three chances for them to arrive out of step, and
	//! because the number of pages is itself editable.
	string SerializeContent()
	{
		array<MCF_Intel_Entry> entries = {};
		GetEntries(entries);

		string result = m_sDeviceName + FIELD_SEP + m_eView.ToString() + FIELD_SEP + m_sActionVerb;

		foreach (MCF_Intel_Entry entry : entries)
		{
			// The app is appended LAST so that content written before it
			// existed still parses -- see the fallback in
			// ApplySerializedContent. Adding a field anywhere but the end
			// would silently shift every field after it.
			result = result + ENTRY_SEP + Clean(entry.m_sHeading) + FIELD_SEP + Clean(entry.m_sTimestamp) + FIELD_SEP + Clean(entry.m_sBody) + FIELD_SEP + entry.m_eApp.ToString();
		}

		return result;
	}

	//! Replaces the whole content. Called on every machine, either from the
	//! server-side edit or from the replication callback below.
	void ApplySerializedContent(string data)
	{
		array<string> blocks = {};
		data.Split(ENTRY_SEP, blocks, false);

		if (blocks.IsEmpty())
			return;

		array<string> header = {};
		blocks[0].Split(FIELD_SEP, header, false);

		if (header.Count() > 0)
			m_sDeviceName = header[0];

		if (header.Count() > 1)
			m_eView = header[1].ToInt();

		if (header.Count() > 2)
			m_sActionVerb = header[2];

		m_aEntries = {};

		for (int i = 1; i < blocks.Count(); i++)
		{
			array<string> fields = {};
			blocks[i].Split(FIELD_SEP, fields, false);

			MCF_Intel_Entry entry = new MCF_Intel_Entry();

			if (fields.Count() > 0)
				entry.m_sHeading = fields[0];

			if (fields.Count() > 1)
				entry.m_sTimestamp = fields[1];

			if (fields.Count() > 2)
				entry.m_sBody = fields[2];

			// Older content has three fields and no app. GENERAL is the
			// right answer for it: no particular section.
			if (fields.Count() > 3)
				entry.m_eApp = fields[3].ToInt();

			m_aEntries.Insert(entry);
		}

		MCF_Core_Log.Debug("intel object rewritten: '" + m_sDeviceName + "' with " + m_aEntries.Count().ToString() + " page(s)");
	}

	//! Server side. Stores the edit as replicated state and applies it here.
	//!
	//! Everyone else -- present, streaming in, or joining an hour later --
	//! gets it through OnContentReplicated.
	void SetContentFromServer(string data)
	{
		if (!Replication.IsServer())
			return;

		m_sContentOverride = data;
		ApplySerializedContent(data);

		Replication.BumpMe();
	}

	//! Server side. Rewrites this object as a single-page document.
	//!
	//! Exists so that one prefab can be many documents. A mission maker builds
	//! one "handwritten letter" and a trigger fills in what this particular
	//! letter says, instead of needing a prefab per piece of intel. The wire
	//! format is built here rather than by the caller, because the separators
	//! are this class's private business and a caller that assembled the
	//! string itself would break the moment they changed.
	void SetSinglePageFromServer(string deviceName, string heading, string timestamp, string body)
	{
		if (!Replication.IsServer())
			return;

		string device = deviceName;
		if (device.IsEmpty())
			device = m_sDeviceName;

		string data = Clean(device) + FIELD_SEP + m_eView.ToString() + FIELD_SEP + Clean(GetActionVerb())
			+ ENTRY_SEP + Clean(heading) + FIELD_SEP + Clean(timestamp) + FIELD_SEP + Clean(body);

		SetContentFromServer(data);
	}

	//! Runs on every client when the override arrives or changes.
	protected void OnContentReplicated()
	{
		if (m_sContentOverride.IsEmpty())
			return;

		ApplySerializedContent(m_sContentOverride);
	}

	//! Strips the separators out of authored text so a Game Master cannot
	//! break the format by typing it.
	protected string Clean(string value)
	{
		string result = value;
		result.Replace(ENTRY_SEP, " ");
		result.Replace(FIELD_SEP, " ");
		result.Replace("\n", " ");
		result.Replace("\r", "");
		return result;
	}
}
