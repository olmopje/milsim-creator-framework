//! What is on a device, in the order it should be shown.
//!
//! WHY THIS IS SEPARATE FROM THE SCREEN THAT DRAWS IT. There is going to be a
//! second device shell -- a laptop, wide, with folders down a side instead of
//! tiles in a grid -- and it asks exactly the same three questions this one
//! does: which apps are there, what is in this one, and what does that item
//! say. If the answers live in the menu class, the second shell either copies
//! them or inherits from a menu it has nothing else in common with. Neither
//! ends well.
//!
//! SO THE PRESENTER MUST NOT KNOW WHICH LAYOUT IT IS FEEDING. No widget names
//! here, no row layouts, no visibility. It answers questions. That is the
//! single rule that keeps the laptop cheap, and the moment something in here
//! mentions a tile or a column it has been broken.
//!
//! Today it reads MCF_Intel_CarrierComponent's entries directly. When authored
//! device profiles land (docs/architecture/DEVICE_CONTENT.md) this is the class
//! that reads a profile instead, and neither shell has to notice.

class MCF_Device_Presenter
{
	protected MCF_Intel_CarrierComponent m_Carrier;
	protected ref array<MCF_Intel_Entry> m_aEntries = {};

	void Bind(MCF_Intel_CarrierComponent carrier)
	{
		m_Carrier = carrier;
		m_aEntries.Clear();

		if (m_Carrier)
			m_Carrier.GetEntries(m_aEntries);
	}

	string DeviceName()
	{
		if (!m_Carrier)
			return "";

		return m_Carrier.GetDeviceName();
	}

	int TotalItems()
	{
		return m_aEntries.Count();
	}

	//! Every item on the device, ignoring which app it belongs to. What the
	//! paper and notepad skins read: they have no apps, only pages.
	int GetAllItems(notnull out array<MCF_Intel_Entry> outItems)
	{
		outItems.Clear();

		foreach (MCF_Intel_Entry entry : m_aEntries)
		{
			outItems.Insert(entry);
		}

		return outItems.Count();
	}

	//! What goes on the home screen.
	//!
	//! A FIXED SET, AND EMPTY ONES STAY. The first version listed only apps
	//! with something in them, on the reasoning that an icon opening on nothing
	//! is worse than no icon. That was wrong about what the object is: a phone
	//! found in a field has Settings and a call log whether or not this one was
	//! used, and a home screen that changes shape per device reads as a menu
	//! wearing device colours.
	//!
	//! An empty app opens and says so, which is also information -- a phone
	//! with no contacts and no call history is a phone somebody was careful
	//! with.
	//!
	//! The order is the order they appear: what carries conversation first,
	//! then what was written down, then the device itself.
	int GetApps(notnull out array<int> outApps)
	{
		outApps.Clear();

		outApps.Insert(MCF_EIntelApp.MESSAGES);
		outApps.Insert(MCF_EIntelApp.CALLS);
		outApps.Insert(MCF_EIntelApp.CONTACTS);
		outApps.Insert(MCF_EIntelApp.EMAIL);
		outApps.Insert(MCF_EIntelApp.NOTES);
		outApps.Insert(MCF_EIntelApp.PHOTOS);
		outApps.Insert(MCF_EIntelApp.FILES);
		outApps.Insert(MCF_EIntelApp.SETTINGS);

		// GENERAL is not an app, it is "no app named" -- what every entry
		// written before apps existed reads as. It earns a tile only when
		// something is filed there, so old content stays reachable without
		// putting a nameless icon on every device.
		if (HasItems(MCF_EIntelApp.GENERAL))
			outApps.Insert(MCF_EIntelApp.GENERAL);

		return outApps.Count();
	}

	//! What is in one app, including anything the device can say about itself
	//! when nobody has written into it.
	int GetItems(int app, notnull out array<MCF_Intel_Entry> outItems)
	{
		outItems.Clear();

		foreach (MCF_Intel_Entry entry : m_aEntries)
		{
			if (entry.m_eApp == app)
				outItems.Insert(entry);
		}

		if (outItems.IsEmpty())
			Synthesise(app, outItems);

		return outItems.Count();
	}

	bool HasItems(int app)
	{
		foreach (MCF_Intel_Entry entry : m_aEntries)
		{
			if (entry.m_eApp == app)
				return true;
		}

		return false;
	}

	//! The name under the tile.
	string AppLabel(int app)
	{
		switch (app)
		{
			case MCF_EIntelApp.MESSAGES: return "MESSAGES";
			case MCF_EIntelApp.CALLS:    return "PHONE";
			case MCF_EIntelApp.CONTACTS: return "CONTACTS";
			case MCF_EIntelApp.EMAIL:    return "MAIL";
			case MCF_EIntelApp.NOTES:    return "NOTES";
			case MCF_EIntelApp.PHOTOS:   return "PHOTOS";
			case MCF_EIntelApp.FILES:    return "FILES";
			case MCF_EIntelApp.SETTINGS: return "SETTINGS";
		}

		return "INBOX";
	}

	//! Said in the app's own terms rather than one blank "nothing here",
	//! because which drawer is empty is itself worth knowing.
	string EmptyText(int app)
	{
		switch (app)
		{
			case MCF_EIntelApp.MESSAGES: return "No messages.";
			case MCF_EIntelApp.CALLS:    return "No calls in the log.";
			case MCF_EIntelApp.CONTACTS: return "No contacts saved.";
			case MCF_EIntelApp.EMAIL:    return "No mail.";
			case MCF_EIntelApp.NOTES:    return "No notes.";
			case MCF_EIntelApp.PHOTOS:   return "No photos.";
			case MCF_EIntelApp.FILES:    return "No files.";
		}

		return "Nothing here.";
	}

	//! Settings is the one app with something to say about a device nobody has
	//! written into: what it is, and whether it was locked. A mission maker who
	//! files real entries under SETTINGS -- a joined network, an account, a
	//! registered number -- replaces this entirely.
	protected void Synthesise(int app, notnull array<MCF_Intel_Entry> outItems)
	{
		if (app != MCF_EIntelApp.SETTINGS || !m_Carrier)
			return;

		outItems.Insert(MakeItem("Device", m_Carrier.GetDeviceName()));
		outItems.Insert(MakeItem("Security", "Screen lock was in use"));
		outItems.Insert(MakeItem("Storage", m_aEntries.Count().ToString() + " item(s) on this device"));
	}

	protected MCF_Intel_Entry MakeItem(string heading, string body)
	{
		MCF_Intel_Entry entry = new MCF_Intel_Entry();
		entry.m_sHeading = heading;
		entry.m_sBody = body;
		return entry;
	}
}
