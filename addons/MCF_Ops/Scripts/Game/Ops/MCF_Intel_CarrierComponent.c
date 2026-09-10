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

	//! Which authored profile this device carries, if any.
	//!
	//! WHY AN ID AND NOT THE CONTENT ITSELF. Content on the prefab is content
	//! on every object made from that prefab, and a Game Master cannot type
	//! into a prefab at runtime anyway -- an editor attribute carries twelve
	//! bytes. So the content lives in MCF_Device_Library, authored in one place,
	//! and the object says which of it to show.
	//!
	//! Empty means "use my own entries", which is what every prefab written
	//! before profiles existed does, and it keeps doing it.
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Id of the device profile this carries. Empty uses the entries below instead.")]
	protected string m_sProfileId;

	[Attribute(desc: "What this object contains. A letter has one entry; a phone has one per message.")]
	protected ref array<ref MCF_Intel_Entry> m_aEntries;

	//! The model to render behind the screen, if this object wants to be drawn
	//! as itself rather than as a flat panel.
	//!
	//! WHY THE OBJECT NAMES ITS OWN MODEL, rather than the shell knowing which
	//! model goes with which view. A phone and its shell live together and the shell
	//! lives here, and Ops must not learn what is in Devices -- that is the
	//! whole reason presentation moved here in the first place. So the prefab
	//! points at its own model and the shell renders whatever it is handed.
	//! Leave it empty and the shell falls back to the drawn panel, which is
	//! also what happens if the preview system is unavailable.
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, params: "et", desc: "Prefab whose model is rendered behind the screen. Empty draws a plain panel instead.")]
	protected ResourceName m_sPreviewPrefab;

	//! How big that model really is, in metres.
	//!
	//! THE SHELL CANNOT GUESS THIS AND MUST NOT. It shapes the preview box to
	//! the model's proportions and asks the preview where the model's corners
	//! landed; both need real dimensions. Hard-coding a phone's would be the
	//! one line that stops a laptop from reusing the same screen.
	//!
	//! X is across the face, Z along its length, Y its thickness -- the axes as
	//! the model is built, lying flat.
	[Attribute(defvalue: "0.07 0.009 0.149", desc: "Size of the preview model in metres: across the face, thickness, along its length.")]
	protected vector m_vPreviewSize;

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

	//! A whole device profile written onto THIS object by a Game Master.
	//!
	//! WHY IT LIVES HERE AND NOT IN THE LIBRARY. MCF_Device_Library is server
	//! state. The device shell runs on the client that opened the phone, and
	//! reads its content locally as it browses -- so a profile that exists only
	//! on the server is a profile that client cannot see, and the shell falls
	//! back to the entries below: the wrong contents, with no error anywhere.
	//!
	//! Shipped profiles come from a .conf and are therefore on every machine
	//! already, which is why assigning one by id works. Authored ones travel as
	//! replicated state on the object they were written for -- the same route
	//! m_sContentOverride takes, for the same reason, and it inherits the same
	//! properties: a client joining an hour later or streaming this object in
	//! for the first time gets it without any push-on-join code.
	//!
	//! The trade is that a profile written this way belongs to this device
	//! rather than to the mission. Reusable profiles are authored in the .conf
	//! and assigned by id; this is for making THIS phone different.
	[RplProp(onRplName: "OnProfileReplicated")]
	protected string m_sProfileOverride;

	//! Every carrier in this world, on whichever machine is asking.
	//!
	//! WHY A REGISTRY. Something occasionally needs "a device, any device" --
	//! the self test needs one to write a profile onto, and a laptop shell will
	//! want to find its neighbours. Walking the world for a component is slow
	//! and needs a starting point; registering on creation costs one insert.
	//!
	//! Held weakly in the sense that entries are removed on delete. A stale
	//! entry here would hand out a component whose entity is gone, which reads
	//! as a null-reference bug three files away from its cause.
	protected static ref array<MCF_Intel_CarrierComponent> s_aAll = {};

	//! The first device registered in this world, or null if there are none.
	static MCF_Intel_CarrierComponent FirstRegistered()
	{
		foreach (MCF_Intel_CarrierComponent carrier : s_aAll)
		{
			if (carrier && carrier.GetOwner())
				return carrier;
		}

		return null;
	}

	static int GetAllRegistered(notnull out array<MCF_Intel_CarrierComponent> outCarriers)
	{
		outCarriers.Clear();

		foreach (MCF_Intel_CarrierComponent carrier : s_aAll)
		{
			if (carrier && carrier.GetOwner())
				outCarriers.Insert(carrier);
		}

		return outCarriers.Count();
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_aAll.Insert(this);
	}

	override void OnDelete(IEntity owner)
	{
		int at = s_aAll.Find(this);
		if (at >= 0)
			s_aAll.Remove(at);

		super.OnDelete(owner);
	}

	string GetDeviceName()
	{
		return m_sDeviceName;
	}

	MCF_EIntelView GetView()
	{
		return m_eView;
	}

	//! True for the views that draw themselves as a device -- a screen with a
	//! list of entries -- and false for the ones that draw as a flat page.
	//!
	//! ONE LIST, IN ONE PLACE. The Game Master's "Edit device" action offers
	//! itself on exactly the views this returns true for, and "Edit intel" on
	//! exactly the ones it returns false for. Kept as two hand-written lists
	//! they drift, and the drift is silent: an object gets both screens, or
	//! neither. A new device-shaped view is added here and nowhere else.
	static bool IsDeviceView(MCF_EIntelView view)
	{
		return view == MCF_EIntelView.PHONE || view == MCF_EIntelView.LAPTOP || view == MCF_EIntelView.DEVICE;
	}

	//! Empty means "draw the flat panel". Never assume a model is there.
	ResourceName GetPreviewPrefab()
	{
		return m_sPreviewPrefab;
	}

	vector GetPreviewSize()
	{
		return m_vPreviewSize;
	}

	//! Empty means "read my own entries".
	string GetProfileId()
	{
		return m_sProfileId;
	}

	//! A serialised profile written onto this object, or empty. Beats the
	//! profile id: an edit made to this device is more specific than the
	//! library entry it started from.
	string GetProfileOverride()
	{
		return m_sProfileOverride;
	}

	//! Server side. Writes a profile onto this device and tells everyone.
	void SetProfileFromServer(string serialisedProfile)
	{
		if (!Replication.IsServer())
			return;

		m_sProfileOverride = serialisedProfile;
		Replication.BumpMe();
	}

	//! Runs on every client when the profile arrives or changes.
	//!
	//! Nothing to apply: the shell reads GetProfileOverride the next time it
	//! opens. A phone already open in somebody's hands keeps what it was
	//! showing, which is the right answer -- content changing under a reader
	//! mid-sentence would be worse than one screen being a few seconds old.
	//! A Game Master rewrote this device and the change has reached this
	//! machine.
	//!
	//! SAYS WHAT ARRIVED, not that something did. "device profile replicated"
	//! cannot tell a late-joining client that got the right profile from one
	//! that got an empty string, and late join is exactly the case this field
	//! exists for and the one that has never been watched.
	protected void OnProfileReplicated()
	{
		if (m_sProfileOverride.IsEmpty())
		{
			MCF_Core_Log.Debug("device profile cleared on '" + m_sDeviceName + "'");
			return;
		}

		MCF_Device_Profile parsed = MCF_Device_Script.Deserialize(m_sProfileOverride);

		if (!parsed)
		{
			MCF_Core_Log.Warn("device profile arrived on '" + m_sDeviceName + "' but could not be read back -- "
				+ m_sProfileOverride.Length().ToString() + " character(s)");
			return;
		}

		int apps;
		if (parsed.m_aApps)
			apps = parsed.m_aApps.Count();

		MCF_Core_Log.Debug("device profile '" + parsed.m_sId + "' arrived on '" + m_sDeviceName
			+ "': " + apps.ToString() + " app(s), " + m_sProfileOverride.Length().ToString() + " character(s)");
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
