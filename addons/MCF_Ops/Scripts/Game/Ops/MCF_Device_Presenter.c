//! What is on a device, in the order it should be shown.
//!
//! WHY THIS IS SEPARATE FROM THE SCREEN THAT DRAWS IT. There is going to be a
//! second device shell -- a laptop, wide, with folders down a side instead of
//! tiles in a grid -- and it asks exactly the same three questions this one
//! does: which apps are there, what is in this one, and what does that item
//! say. If the answers live in the menu class, the second shell either copies
//! them or inherits from a menu it has nothing else in common with.
//!
//! SO THE PRESENTER MUST NOT KNOW WHICH LAYOUT IT IS FEEDING. No widget names
//! here, no row layouts, no visibility. It answers questions. That is the one
//! rule that keeps the laptop cheap, and the moment something in here mentions
//! a tile or a column it has been broken.
//!
//! IT ALWAYS READS A PROFILE, EVEN WHEN THERE ISN'T ONE. A device with a
//! profile id gets the authored profile from the library. A device without one
//! gets a profile synthesised from the entries on its own component, exactly as
//! it behaved before profiles existed. That is what lets every prefab written
//! so far keep working untouched while there is still a single code path here:
//! two paths would be two things to keep in step, and the one nobody is looking
//! at would rot.

class MCF_Device_Presenter
{
	protected MCF_Intel_CarrierComponent m_Carrier;
	protected ref MCF_Device_Profile m_Profile;

	void Bind(MCF_Intel_CarrierComponent carrier)
	{
		m_Carrier = carrier;
		m_Profile = null;

		if (!m_Carrier)
			return;

		string profileId = m_Carrier.GetProfileId();
		if (!profileId.IsEmpty())
		{
			m_Profile = MCF_Device_Library.GetInstance().Find(profileId);

			if (!m_Profile)
				MCF_Core_Log.Warn("device asks for profile '" + profileId + "' which the library does not have -- falling back to its own entries");
		}

		if (!m_Profile)
			m_Profile = SynthesiseFromEntries();
	}

	string DeviceName()
	{
		// The profile names the device, unless the object was given a name of
		// its own -- a phone taken from a specific person can say so without
		// needing a profile all to itself.
		if (m_Carrier)
		{
			string own = m_Carrier.GetDeviceName();
			if (!own.IsEmpty())
				return own;
		}

		if (m_Profile)
			return m_Profile.m_sDeviceName;

		return "";
	}

	int TotalItems()
	{
		int total;

		if (!m_Profile || !m_Profile.m_aApps)
			return 0;

		foreach (MCF_Device_App app : m_Profile.m_aApps)
		{
			if (app && app.m_aItems)
				total += app.m_aItems.Count();
		}

		return total;
	}

	//! Every item on the device, ignoring which app it is under. What the paper
	//! and notepad skins read: they have no apps, only pages.
	int GetAllItems(notnull out array<ref MCF_Device_Item> outItems)
	{
		outItems.Clear();

		if (!m_Profile || !m_Profile.m_aApps)
			return 0;

		foreach (MCF_Device_App app : m_Profile.m_aApps)
		{
			if (!app || !app.m_aItems)
				continue;

			foreach (MCF_Device_Item item : app.m_aItems)
			{
				outItems.Insert(item);
			}
		}

		return outItems.Count();
	}

	//! What goes on the home screen: the profile's apps, in the profile's order.
	//!
	//! EMPTY APPS STAY. An app that is on the device is on the device whether
	//! or not it was used, and a home screen that changes shape per device
	//! reads as a menu wearing device colours. Empty is also information: a
	//! phone with no contacts and no call history is a phone somebody was
	//! careful with.
	int GetApps(notnull out array<MCF_Device_App> outApps)
	{
		outApps.Clear();

		if (!m_Profile || !m_Profile.m_aApps)
			return 0;

		foreach (MCF_Device_App app : m_Profile.m_aApps)
		{
			if (app)
				outApps.Insert(app);
		}

		return outApps.Count();
	}

	//! What is in one app, including anything the device can say about itself
	//! when nobody has written into it.
	int GetItems(MCF_Device_App app, notnull out array<ref MCF_Device_Item> outItems)
	{
		outItems.Clear();

		if (!app)
			return 0;

		if (app.m_aItems)
		{
			foreach (MCF_Device_Item item : app.m_aItems)
			{
				outItems.Insert(item);
			}
		}

		if (outItems.IsEmpty())
			Synthesise(app.m_eKind, outItems);

		return outItems.Count();
	}

	// ------------------------------------------------------------ internals

	//! A profile made out of the entries on the object itself.
	//!
	//! One app per kind that has something filed under it, in enum order, plus
	//! the eight a phone is expected to have. This is what a device without a
	//! profile shows, and it is exactly what the shell showed before profiles
	//! existed -- the fallback has to be indistinguishable or the migration is
	//! a change nobody asked for.
	protected MCF_Device_Profile SynthesiseFromEntries()
	{
		MCF_Device_Profile profile = new MCF_Device_Profile();
		profile.m_sDeviceName = "";
		profile.m_aApps = {};

		array<MCF_Intel_Entry> entries = {};
		if (m_Carrier)
			m_Carrier.GetEntries(entries);

		array<int> kinds = {};
		kinds.Insert(MCF_EIntelApp.MESSAGES);
		kinds.Insert(MCF_EIntelApp.CALLS);
		kinds.Insert(MCF_EIntelApp.CONTACTS);
		kinds.Insert(MCF_EIntelApp.EMAIL);
		kinds.Insert(MCF_EIntelApp.NOTES);
		kinds.Insert(MCF_EIntelApp.PHOTOS);
		kinds.Insert(MCF_EIntelApp.FILES);
		kinds.Insert(MCF_EIntelApp.SETTINGS);

		// GENERAL is not an app, it is "no app named" -- what every entry
		// written before apps existed reads as. It earns a tile only when
		// something is filed there, so old content stays reachable without a
		// nameless icon appearing on every device.
		foreach (MCF_Intel_Entry entry : entries)
		{
			if (entry && entry.m_eApp == MCF_EIntelApp.GENERAL)
			{
				kinds.Insert(MCF_EIntelApp.GENERAL);
				break;
			}
		}

		foreach (int kind : kinds)
		{
			MCF_Device_App app = new MCF_Device_App();
			app.m_eKind = kind;
			app.m_aItems = {};

			foreach (MCF_Intel_Entry entry : entries)
			{
				if (!entry || entry.m_eApp != kind)
					continue;

				MCF_Device_Item item = new MCF_Device_Item();
				item.m_sHeading = entry.m_sHeading;
				item.m_sTimestamp = entry.m_sTimestamp;
				item.m_sBody = entry.m_sBody;
				app.m_aItems.Insert(item);
			}

			profile.m_aApps.Insert(app);
		}

		return profile;
	}

	//! Settings is the one app with something to say about a device nobody has
	//! written into: what it is, and whether it was locked. A mission maker who
	//! files real items under SETTINGS replaces this entirely.
	protected void Synthesise(int kind, notnull array<ref MCF_Device_Item> outItems)
	{
		if (kind != MCF_EIntelApp.SETTINGS)
			return;

		outItems.Insert(MakeItem("Device", DeviceName()));
		outItems.Insert(MakeItem("Security", "Screen lock was in use"));
		outItems.Insert(MakeItem("Storage", TotalItems().ToString() + " item(s) on this device"));
	}

	protected MCF_Device_Item MakeItem(string heading, string body)
	{
		MCF_Device_Item item = new MCF_Device_Item();
		item.m_sHeading = heading;
		item.m_sBody = body;
		return item;
	}
}
