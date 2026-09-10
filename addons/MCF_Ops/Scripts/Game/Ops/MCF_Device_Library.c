//! Device profiles authored once, assigned to whatever needs them.
//!
//! This is MCF_Dialogue_Library with the nouns changed, and deliberately so.
//! The problem is identical -- authored text that cannot live on the object
//! because editor attributes carry twelve bytes and one prefab is behind a
//! thousand objects -- and a second solution to a solved problem is a second
//! thing to maintain and a second thing to learn.
//!
//! ADD SHIPPED ONES to Configs/Devices/MCF_DeviceProfiles.conf. The ids are
//! what a device refers to, so renaming one breaks the missions using it. Treat
//! them the way you would treat an event name.
//!
//! Loaded lazily: a mission with no devices should not pay for the file, and a
//! mission with devices asks the first time somebody looks at one.

class MCF_Device_Library
{
	protected static const ResourceName CONFIG = "{6A1C4F0B39D34600}Configs/Devices/MCF_DeviceProfiles.conf";

	protected static const string KEY_INDEX = "deviceProfiles";
	protected static const string KEY_PREFIX = "deviceProfile.";

	private static ref MCF_Device_Library s_Instance;

	protected ref MCF_Device_ProfilesConfig m_Config;
	protected bool m_bLoadAttempted;

	//! Profiles written by a Game Master during the mission. Kept apart from the
	//! config ones so the config stays the mod's own content and a session's
	//! additions can be saved, loaded and listed as a group.
	protected ref array<ref MCF_Device_Profile> m_aRuntime = {};

	static MCF_Device_Library GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Device_Library();

		return s_Instance;
	}

	//! \return The profile with this id, or null.
	MCF_Device_Profile Find(string profileId)
	{
		if (profileId.IsEmpty())
			return null;

		LoadConfig();

		// Runtime first: a profile a Game Master wrote this session shadows a
		// shipped one with the same id, so they can rewrite a shipped device
		// for one mission without the mod's copy changing underneath them.
		foreach (MCF_Device_Profile authored : m_aRuntime)
		{
			if (authored && authored.m_sId == profileId)
				return authored;
		}

		if (m_Config && m_Config.m_aProfiles)
		{
			foreach (MCF_Device_Profile profile : m_Config.m_aProfiles)
			{
				if (profile && profile.m_sId == profileId)
					return profile;
			}
		}

		return null;
	}

	//! Every id, for the editor's list and for logging.
	//!
	//! Config order first, then anything authored this session. The order is
	//! what any index-based picker points at, so it has to be stable: a new
	//! profile appends rather than inserting.
	int GetIds(notnull out array<string> outIds)
	{
		outIds.Clear();

		LoadConfig();

		if (m_Config && m_Config.m_aProfiles)
		{
			foreach (MCF_Device_Profile profile : m_Config.m_aProfiles)
			{
				if (profile && !profile.m_sId.IsEmpty())
					outIds.Insert(profile.m_sId);
			}
		}

		foreach (MCF_Device_Profile authored : m_aRuntime)
		{
			if (authored && !authored.m_sId.IsEmpty() && !outIds.Contains(authored.m_sId))
				outIds.Insert(authored.m_sId);
		}

		return outIds.Count();
	}

	//! Whether this id belongs to the mod rather than to the session. The editor
	//! greys DELETE for these instead of failing silently.
	bool IsShipped(string profileId)
	{
		LoadConfig();

		if (!m_Config || !m_Config.m_aProfiles)
			return false;

		foreach (MCF_Device_Profile profile : m_Config.m_aProfiles)
		{
			if (profile && profile.m_sId == profileId)
				return true;
		}

		return false;
	}

	// -------------------------------------------------- runtime authoring

	//! Server side. Adds a profile or replaces the one with that id.
	bool Upsert(MCF_Device_Profile profile)
	{
		if (!Replication.IsServer())
		{
			MCF_Core_Log.Warn("device library written on a client -- ignored, the library is server state");
			return false;
		}

		if (!profile || profile.m_sId.IsEmpty())
			return false;

		// Commas separate the persistence index. An id containing one would
		// split into two ids that resolve to nothing, and the profile would
		// vanish on the next restart with no error anywhere.
		profile.m_sId = SanitiseId(profile.m_sId);

		foreach (int i, MCF_Device_Profile existing : m_aRuntime)
		{
			if (existing && existing.m_sId == profile.m_sId)
			{
				m_aRuntime[i] = profile;
				Save();
				MCF_Core_Log.Debug("device profile '" + profile.m_sId + "' rewritten");
				return true;
			}
		}

		m_aRuntime.Insert(profile);
		Save();
		MCF_Core_Log.Debug("device profile '" + profile.m_sId + "' added to the library");
		return true;
	}

	//! Server side. Removes a profile a Game Master wrote.
	//!
	//! Only runtime ones. A profile that ships with the mod is content, not
	//! session state -- deleting it would leave every mission referring to it
	//! broken with no way back short of reinstalling.
	bool Delete(string profileId)
	{
		if (!Replication.IsServer() || profileId.IsEmpty())
			return false;

		foreach (int i, MCF_Device_Profile profile : m_aRuntime)
		{
			if (!profile || profile.m_sId != profileId)
				continue;

			m_aRuntime.Remove(i);
			MCF_Core_PersistentStore.GetInstance().Remove(KEY_PREFIX + profileId);
			Save();

			MCF_Core_Log.Debug("device profile '" + profileId + "' deleted");
			return true;
		}

		return false;
	}

	//! Ids are keys in a comma-delimited index, so they may not contain the
	//! delimiters.
	static string SanitiseId(string wanted)
	{
		string result = wanted;
		result.Replace(",", "_");
		result.Replace(" ", "_");
		result.Replace("\n", "");
		result.Replace("\r", "");
		return result;
	}

	//! An id nothing is using yet, so "save as new" never silently overwrites.
	string MakeUniqueId(string wanted)
	{
		string candidate = wanted;
		if (candidate.IsEmpty())
			candidate = "device";

		if (!Find(candidate))
			return candidate;

		for (int i = 2; i < 500; i++)
		{
			string numbered = candidate + "_" + i.ToString();
			if (!Find(numbered))
				return numbered;
		}

		return candidate;
	}

	// ------------------------------------------------------- persistence

	void Save()
	{
		if (!Replication.IsServer())
			return;

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		string index = "";
		foreach (MCF_Device_Profile profile : m_aRuntime)
		{
			if (!profile)
				continue;

			if (!index.IsEmpty())
				index = index + ",";
			index = index + profile.m_sId;

			store.Set(KEY_PREFIX + profile.m_sId, MCF_Device_Script.Serialize(profile));
		}

		store.Set(KEY_INDEX, index);
		store.Save();
	}

	void LoadRuntime()
	{
		m_aRuntime = {};

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		string index = store.Get(KEY_INDEX, "");
		if (index.IsEmpty())
			return;

		array<string> ids = {};
		index.Split(",", ids, true);

		foreach (string id : ids)
		{
			if (id.IsEmpty())
				continue;

			MCF_Device_Profile profile = MCF_Device_Script.Deserialize(store.Get(KEY_PREFIX + id, ""));
			if (!profile)
			{
				MCF_Core_Log.Warn("could not restore device profile '" + id + "'");
				continue;
			}

			m_aRuntime.Insert(profile);
		}

		MCF_Core_Log.Debug("restored " + m_aRuntime.Count().ToString() + " authored device profile(s) from previous sessions");
	}

	protected void LoadConfig()
	{
		if (m_bLoadAttempted)
			return;

		m_bLoadAttempted = true;

		Resource resource = BaseContainerTools.LoadContainer(CONFIG);
		if (!resource || !resource.IsValid())
		{
			MCF_Core_Log.Warn("could not load the device profile library at " + CONFIG);
			return;
		}

		m_Config = MCF_Device_ProfilesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));

		if (!m_Config)
		{
			MCF_Core_Log.Warn("the device profile library loaded but is not an MCF_Device_ProfilesConfig");
			return;
		}

		int count;
		if (m_Config.m_aProfiles)
			count = m_Config.m_aProfiles.Count();

		MCF_Core_Log.Debug("device profile library loaded with " + count.ToString() + " profile(s)");
	}
}
