//! Conversations authored once, assigned to whoever needs them.
//!
//! WHY A LIBRARY AND NOT ATTRIBUTES ON EACH PERSON. Every civilian in the game
//! now carries a dialogue component, because talking to people has to work on
//! the people already in the mission rather than only on ones placed from
//! MCF's own list. But that means the conversation cannot live on the person:
//! there is one prefab behind a thousand civilians, and a Game Master cannot
//! type into a script attribute at runtime -- editor attributes pack
//! everything into a vector and carry twelve bytes, so they can hold a number
//! and never a sentence. That constraint is why the intel system needed its
//! own authoring screen, and it applies here unchanged.
//!
//! So conversations are data, authored in the Workbench where text belongs,
//! and assigned in the world by id. One "frightened farmer" conversation can
//! then be given to any farmer, in any mission, by anybody.
//!
//! ADD YOURS to Configs/Dialogue/MCF_Conversations.conf. The ids are what the
//! assign node refers to, so renaming one breaks the missions that use it --
//! treat them the way you would treat an event name.

//! One authored conversation.
[BaseContainerProps()]
class MCF_Dialogue_Conversation
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Id this conversation is assigned by. Treat it like an event name: missions refer to it, so renaming breaks them.")]
	string m_sId;

	[Attribute(defvalue: "Civilian", uiwidget: UIWidgets.EditBox, desc: "Name shown at the top of the conversation.")]
	string m_sSpeakerName;

	[Attribute(defvalue: "Talk", uiwidget: UIWidgets.EditBox, desc: "Verb on the interaction prompt: Talk, Question, Interrogate.")]
	string m_sActionVerb;

	[Attribute(defvalue: "start", uiwidget: UIWidgets.EditBox, desc: "Node the conversation opens on.")]
	string m_sStartNodeId;

	[Attribute(desc: "The conversation itself.")]
	ref array<ref MCF_Dialogue_Node> m_aNodes;
}

//! The config asset. One of these, listing every conversation in the mod.
[BaseContainerProps(configRoot: true)]
class MCF_Dialogue_LibraryConfig
{
	[Attribute(desc: "Every conversation MCF knows about.")]
	ref array<ref MCF_Dialogue_Conversation> m_aConversations;
}

//! Loads the config once and answers lookups.
//!
//! Loaded lazily rather than at mission start: a mission with no conversations
//! should not pay for the file, and a mission with conversations asks for one
//! the first time somebody is assigned a conversation, which is long before
//! anybody can walk up to them.
class MCF_Dialogue_Library
{
	protected static const ResourceName CONFIG = "{6A1C4F0B39D2A300}Configs/Dialogue/MCF_Conversations.conf";

	private static ref MCF_Dialogue_Library s_Instance;

	protected ref MCF_Dialogue_LibraryConfig m_Config;
	protected bool m_bLoadAttempted;

	static MCF_Dialogue_Library GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Dialogue_Library();
		return s_Instance;
	}

	//! \return The conversation with this id, or null.
	MCF_Dialogue_Conversation Find(string conversationId)
	{
		if (conversationId.IsEmpty())
			return null;

		LoadConfig();

		// Runtime first: a conversation a Game Master wrote this session
		// shadows a shipped one with the same id.
		foreach (MCF_Dialogue_Conversation authored : m_aRuntime)
		{
			if (authored && authored.m_sId == conversationId)
				return authored;
		}

		if (m_Config && m_Config.m_aConversations)
		{
			foreach (MCF_Dialogue_Conversation conversation : m_Config.m_aConversations)
			{
				if (conversation && conversation.m_sId == conversationId)
					return conversation;
			}
		}

		return null;
	}

	//! Every id, for logging and for anything that wants to offer a choice.
	int GetIds(notnull out array<string> outIds)
	{
		outIds.Clear();

		LoadConfig();

		// Config order first, then anything authored this session. The order
		// is the meaning of the Game Master's Conversation slider, so it has
		// to be stable: a new conversation appends rather than inserting, and
		// nobody's slider changes what it points at.
		if (m_Config && m_Config.m_aConversations)
		{
			foreach (MCF_Dialogue_Conversation conversation : m_Config.m_aConversations)
			{
				if (conversation && !conversation.m_sId.IsEmpty())
					outIds.Insert(conversation.m_sId);
			}
		}

		foreach (MCF_Dialogue_Conversation authored : m_aRuntime)
		{
			if (authored && !authored.m_sId.IsEmpty() && !outIds.Contains(authored.m_sId))
				outIds.Insert(authored.m_sId);
		}

		return outIds.Count();
	}

	// -------------------------------------------------- runtime authoring

	//! Conversations written by a Game Master during the mission. Kept apart
	//! from the config ones so that the config remains the mod's own content
	//! and a session's additions can be saved, loaded and listed as a group.
	protected ref array<ref MCF_Dialogue_Conversation> m_aRuntime = {};

	protected static const string KEY_INDEX = "conversations";
	protected static const string KEY_PREFIX = "conversation.";

	//! Server side. Adds a conversation or replaces the one with that id.
	//!
	//! A runtime conversation SHADOWS a config one of the same id rather than
	//! merging with it, so a Game Master can rewrite a shipped conversation for
	//! one mission without the mod's own copy changing underneath them.
	bool Upsert(MCF_Dialogue_Conversation conversation)
	{
		if (!Replication.IsServer())
		{
			MCF_Core_Log.Warn("conversation library written on a client -- ignored, the library is server state");
			return false;
		}

		if (!conversation || conversation.m_sId.IsEmpty())
			return false;

		// Commas are the separator in the persistence index and in the list
		// sent to the editor. An id containing one would split into two ids
		// that resolve to nothing, and the conversation would vanish on the
		// next restart with no error anywhere.
		conversation.m_sId = SanitiseId(conversation.m_sId);

		foreach (int i, MCF_Dialogue_Conversation existing : m_aRuntime)
		{
			if (existing && existing.m_sId == conversation.m_sId)
			{
				m_aRuntime[i] = conversation;
				Save();
				MCF_Core_Log.Debug("conversation '" + conversation.m_sId + "' rewritten");
				return true;
			}
		}

		m_aRuntime.Insert(conversation);
		Save();
		MCF_Core_Log.Debug("conversation '" + conversation.m_sId + "' added to the library");
		return true;
	}

	//! Server side. Removes a conversation a Game Master wrote.
	//!
	//! Only runtime ones. A conversation that ships with the mod is content,
	//! not session state -- deleting it would leave a mission that referred to
	//! it broken with no way to get it back short of reinstalling.
	bool Delete(string conversationId)
	{
		if (!Replication.IsServer() || conversationId.IsEmpty())
			return false;

		foreach (int i, MCF_Dialogue_Conversation conversation : m_aRuntime)
		{
			if (!conversation || conversation.m_sId != conversationId)
				continue;

			m_aRuntime.Remove(i);
			MCF_Core_PersistentStore.GetInstance().Remove(KEY_PREFIX + conversationId);
			Save();

			MCF_Core_Log.Debug("conversation '" + conversationId + "' deleted");
			return true;
		}

		return false;
	}

	//! Whether this id belongs to the mod rather than to the session. The
	//! editor greys DELETE for these instead of failing silently.
	bool IsShipped(string conversationId)
	{
		LoadConfig();

		if (!m_Config || !m_Config.m_aConversations)
			return false;

		foreach (MCF_Dialogue_Conversation conversation : m_Config.m_aConversations)
		{
			if (conversation && conversation.m_sId == conversationId)
				return true;
		}

		return false;
	}

	//! Ids are used as separators-delimited keys, so they may not contain the
	//! separators.
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
			candidate = "conversation";

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

	void Save()
	{
		if (!Replication.IsServer())
			return;

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		string index = "";
		foreach (int i, MCF_Dialogue_Conversation conversation : m_aRuntime)
		{
			if (!conversation)
				continue;

			if (!index.IsEmpty())
				index = index + ",";
			index = index + conversation.m_sId;

			store.Set(KEY_PREFIX + conversation.m_sId, MCF_Dialogue_Script.Serialize(conversation));
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

			MCF_Dialogue_Conversation conversation = MCF_Dialogue_Script.Deserialize(store.Get(KEY_PREFIX + id, ""));
			if (!conversation)
			{
				MCF_Core_Log.Warn("could not restore conversation '" + id + "'");
				continue;
			}

			m_aRuntime.Insert(conversation);
		}

		MCF_Core_Log.Debug("restored " + m_aRuntime.Count().ToString() + " authored conversation(s) from previous sessions");
	}

	protected void LoadConfig()
	{
		if (m_bLoadAttempted)
			return;

		m_bLoadAttempted = true;

		Resource resource = BaseContainerTools.LoadContainer(CONFIG);
		if (!resource || !resource.IsValid())
		{
			MCF_Core_Log.Warn("could not load the conversation library at " + CONFIG);
			return;
		}

		m_Config = MCF_Dialogue_LibraryConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));

		if (!m_Config)
		{
			MCF_Core_Log.Warn("the conversation library loaded but is not an MCF_Dialogue_LibraryConfig");
			return;
		}

		int count;
		if (m_Config.m_aConversations)
			count = m_Config.m_aConversations.Count();

		MCF_Core_Log.Debug("conversation library loaded with " + count.ToString() + " conversation(s)");
	}
}
