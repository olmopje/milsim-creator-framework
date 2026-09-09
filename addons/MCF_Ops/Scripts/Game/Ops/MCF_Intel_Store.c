//! Server-authoritative store of intel that has been logged to the operations
//! board, persisted across restarts.
//!
//! The line this class draws is the whole intel design. An object in the world
//! holds what it says; this holds what the force *knows*. Nothing crosses from
//! one to the other by itself -- a person has to carry the object to somebody
//! who can enter it. That is why logging is a server call and not a side
//! effect of reading.
//!
//! Mirrors MCF_Task_Store deliberately, down to the persistence keys and
//! the clear-then-resend transport. Two stores that behave differently would
//! be two sets of bugs; one that behaves the same is one set of lessons.
//!
//! ON VISIBILITY: intel belongs to the faction that logged it. The faction is
//! captured on the record at the moment of logging, not looked up from the
//! logger afterwards, so a player who changes sides does not take the report
//! with them. Records written before the field existed carry no faction and
//! stay visible to everyone, because a mission losing intel it already had
//! looks like a bug even when it is a policy.
//!
//! The per-player transport was already in place, so this cost one field:
//! SendIntelToPlayer filters through IsVisibleTo, and the faction-change hook
//! re-sends, which means switching sides swaps what a player holds rather than
//! leaving a stale copy on their board.


class MCF_Intel_Store
{
	protected static const string KEY_INDEX = "intel";
	protected static const string KEY_NEXT_ID = "intelNextId";
	protected static const string KEY_PREFIX = "intel.";

	private static ref MCF_Intel_Store s_Instance;

	protected ref map<string, ref MCF_Intel_Record> m_mRecords;
	protected int m_iNextId;

	void MCF_Intel_Store()
	{
		m_mRecords = new map<string, ref MCF_Intel_Record>();
		m_iNextId = 1;
	}

	static MCF_Intel_Store GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Intel_Store();
		return s_Instance;
	}

	// ---------------------------------------------------------------- write

	//! Enters a piece of intel into the system. Server only -- ids must be
	//! handed out in one place or two machines will issue the same reference.
	MCF_Intel_Record Log(string source, string heading, string timestamp, string body, int loggedByPlayerId, string factionKey)
	{
		if (!Replication.IsServer())
		{
			MCF_Core_Log.Warn("IntelStore.Log called on a client -- ignored, intel is server state");
			return null;
		}

		MCF_Intel_Record record = new MCF_Intel_Record();
		record.m_sId = "i" + m_iNextId.ToString();
		m_iNextId++;

		record.m_sSource = source;
		record.m_sHeading = heading;
		record.m_sTimestamp = timestamp;
		record.m_sBody = body;
		record.m_iLoggedByPlayerId = loggedByPlayerId;
		record.m_sFactionKey = factionKey;

		m_mRecords.Set(record.m_sId, record);
		MCF_Core_Log.Debug("IntelStore logged " + record.Describe());

		Save();
		return record;
	}

	//! Removes a record for good. Reference numbers are not reused, for the
	//! same reason orders do not reuse theirs: somebody has said it out loud.
	bool Delete(string recordId)
	{
		if (!Replication.IsServer())
			return false;

		if (!m_mRecords.Contains(recordId))
			return false;

		MCF_Core_Log.Debug("IntelStore deleting " + recordId);

		m_mRecords.Remove(recordId);
		MCF_Core_PersistentStore.GetInstance().Remove(KEY_PREFIX + recordId);
		Save();
		return true;
	}

	//! Applies a record sent by the server. Client mirror only -- does not
	//! persist, because a client's view is a copy and not a source of truth.
	MCF_Intel_Record ApplySerialized(string data)
	{
		MCF_Intel_Record record = MCF_Intel_Record.Deserialize(data);
		if (!record)
		{
			MCF_Core_Log.Warn("IntelStore could not deserialise a record");
			return null;
		}

		m_mRecords.Set(record.m_sId, record);
		return record;
	}

	//! Empties this machine's mirror.
	//!
	//! Refuses on a server for the same reason the task store does: on a
	//! hosted server the mirror and the authoritative store are the same
	//! object, and clearing it there would wipe the mission's intel.
	void ClearMirror()
	{
		if (Replication.IsServer())
			return;

		m_mRecords.Clear();
	}

	// ------------------------------------------------------------------ read

	MCF_Intel_Record GetRecord(string recordId)
	{
		MCF_Intel_Record record;
		if (m_mRecords.Find(recordId, record))
			return record;
		return null;
	}

	int GetAll(notnull out array<MCF_Intel_Record> outRecords)
	{
		outRecords.Clear();
		for (int i = 0; i < m_mRecords.Count(); i++)
			outRecords.Insert(m_mRecords.GetElement(i));
		return outRecords.Count();
	}

	int Count()
	{
		return m_mRecords.Count();
	}

	//! Whether a player may see a logged record.
	//!
	//! Intel belongs to the faction that entered it. That is the whole reason
	//! the carrying mechanic exists: a letter found by the other side is a
	//! letter the other side knows, and nothing about finding it should tell
	//! us. Tasks withhold by assignment; intel withholds by side.
	//!
	//! TWO DELIBERATE ESCAPE HATCHES. A record with no faction is legacy data
	//! and stays visible -- old missions must not appear to lose their intel.
	//! A player with no faction (unassigned, spectating, in the lobby) sees
	//! nothing rather than everything, because "not on a side yet" is not the
	//! same as "on every side".
	bool IsVisibleTo(notnull MCF_Intel_Record record, int playerId, string factionKey)
	{
		if (record.m_sFactionKey.IsEmpty())
			return true;

		if (factionKey.IsEmpty())
			return false;

		return record.m_sFactionKey == factionKey;
	}


	int GetVisibleTo(int playerId, string factionKey, notnull out array<MCF_Intel_Record> outRecords)
	{
		outRecords.Clear();

		array<MCF_Intel_Record> all = {};
		GetAll(all);

		foreach (MCF_Intel_Record record : all)
		{
			if (IsVisibleTo(record, playerId, factionKey))
				outRecords.Insert(record);
		}

		return outRecords.Count();
	}

	// --------------------------------------------------------- persistence

	void Save()
	{
		if (!Replication.IsServer())
			return;

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		string index = "";
		for (int i = 0; i < m_mRecords.Count(); i++)
		{
			MCF_Intel_Record record = m_mRecords.GetElement(i);

			if (i > 0)
				index = index + ",";
			index = index + record.m_sId;

			store.Set(KEY_PREFIX + record.m_sId, record.Serialize());
		}

		store.Set(KEY_INDEX, index);
		store.SetInt(KEY_NEXT_ID, m_iNextId);
		store.Save();
	}

	void Load()
	{
		m_mRecords.Clear();

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();
		m_iNextId = store.GetInt(KEY_NEXT_ID, 1);

		string index = store.Get(KEY_INDEX, "");
		if (index.IsEmpty())
		{
			MCF_Core_Log.Debug("IntelStore: no stored intel");
			return;
		}

		array<string> ids = {};
		index.Split(",", ids, true);

		foreach (string id : ids)
		{
			if (id.IsEmpty())
				continue;

			MCF_Intel_Record record = MCF_Intel_Record.Deserialize(store.Get(KEY_PREFIX + id, ""));
			if (!record)
			{
				MCF_Core_Log.Warn("IntelStore could not restore record '" + id + "'");
				continue;
			}

			m_mRecords.Set(record.m_sId, record);
		}

		MCF_Core_Log.Debug("IntelStore loaded " + m_mRecords.Count().ToString() + " record(s) from previous sessions");
	}
}
