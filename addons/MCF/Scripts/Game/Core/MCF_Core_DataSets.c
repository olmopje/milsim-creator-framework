//! What lives in the persistent store, described so it can be managed.
//!
//! WHY A REGISTRY AND NOT A LIST IN THE CORE. The core knows nothing about
//! taskings, intel, conversations or telephones, and it must not start to:
//! every module that persists something would otherwise have to be named in a
//! file one layer below it, and MCF_Core would end up depending on all eight
//! addons to be able to delete a key. So each module says what it owns, at
//! start-up, and the core only knows the shape of that statement.
//!
//! WHAT A SET IS. Every store in MCF follows the same three-key pattern,
//! because they were all copied from the first one:
//!
//!   tasks           an index: a comma-separated list of ids
//!   task.<id>       one entry per id
//!   taskNextId      the counter, so ids are never reused
//!
//! Naming those three is enough to clear a set completely, and enough to count
//! what is in it without parsing anything.
//!
//! CLEARING IS TWO HALVES AND THE SECOND IS EASY TO FORGET. Removing the keys
//! only empties the file; the module's runtime store is still holding what it
//! loaded, and the clients are still holding what they were sent. So anything
//! that clears fires m_OnReloaded afterwards, and every module reloads itself
//! from a store that is now empty. Without that, a mission maker clears the
//! board, sees nothing change, clears again, and restarts the server to find
//! it worked the first time.

class MCF_Core_DataSet
{
	//! Stable, lowercase, used on the wire. Not shown to anyone.
	string m_sId;

	//! What a mission maker reads on the button.
	string m_sLabel;

	//! One line saying what is lost, shown before anything is deleted.
	string m_sDescription;

	string m_sIndexKey;
	string m_sItemPrefix;
	string m_sCounterKey;

	void MCF_Core_DataSet(string id, string label, string description, string indexKey, string itemPrefix, string counterKey)
	{
		m_sId = id;
		m_sLabel = label;
		m_sDescription = description;
		m_sIndexKey = indexKey;
		m_sItemPrefix = itemPrefix;
		m_sCounterKey = counterKey;
	}

	//! How many entries this set holds, from the index alone.
	int Count()
	{
		string index = MCF_Core_PersistentStore.GetInstance().Get(m_sIndexKey, "");
		if (index.IsEmpty())
			return 0;

		array<string> ids = {};
		index.Split(",", ids, true);
		return ids.Count();
	}

	//! Removes every key this set owns. Does NOT save and does NOT reload --
	//! the registry does both, once, however many sets were cleared.
	//! \return How many keys were removed.
	int ClearKeys()
	{
		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		int removed = store.RemoveByPrefix(m_sItemPrefix);

		if (store.Has(m_sIndexKey))
		{
			store.Remove(m_sIndexKey);
			removed++;
		}

		// The counter goes too. Keeping it would mean a freshly cleared
		// mission starts numbering its first report at INTREP-047, which reads
		// as data that failed to delete rather than as a clean slate.
		if (store.Has(m_sCounterKey))
		{
			store.Remove(m_sCounterKey);
			removed++;
		}

		return removed;
	}
}

class MCF_Core_DataSets
{
	protected static ref array<ref MCF_Core_DataSet> s_aSets;

	//! Fired after the store on disk has changed underneath everyone.
	//!
	//! Every module that keeps a runtime copy of persisted data inserts its own
	//! reload here at start-up. An invoker rather than a stored method, because
	//! Enforce will not accept a func as a parameter to a script method -- the
	//! same constraint that shapes the device editor's button handling.
	protected static ref ScriptInvoker s_OnReloaded;

	static ScriptInvoker GetOnReloaded()
	{
		if (!s_OnReloaded)
			s_OnReloaded = new ScriptInvoker();

		return s_OnReloaded;
	}

	//! Declares a set. Called once per module, at game-mode start.
	//!
	//! Re-registering the same id replaces the previous declaration rather than
	//! adding a second one, because a game mode that restarts in the same
	//! process would otherwise accumulate duplicates and clear each set twice.
	static void Register(string id, string label, string description, string indexKey, string itemPrefix, string counterKey)
	{
		if (!s_aSets)
			s_aSets = {};

		foreach (int i, MCF_Core_DataSet existing : s_aSets)
		{
			if (existing.m_sId == id)
			{
				s_aSets[i] = new MCF_Core_DataSet(id, label, description, indexKey, itemPrefix, counterKey);
				return;
			}
		}

		s_aSets.Insert(new MCF_Core_DataSet(id, label, description, indexKey, itemPrefix, counterKey));
	}

	static int GetAll(notnull out array<MCF_Core_DataSet> outSets)
	{
		outSets.Clear();

		if (!s_aSets)
			return 0;

		foreach (MCF_Core_DataSet dataSet : s_aSets)
		{
			outSets.Insert(dataSet);
		}

		return outSets.Count();
	}

	static MCF_Core_DataSet Find(string id)
	{
		if (!s_aSets)
			return null;

		foreach (MCF_Core_DataSet dataSet : s_aSets)
		{
			if (dataSet.m_sId == id)
				return dataSet;
		}

		return null;
	}

	//! Clears one set, writes the file, and tells everyone to reload.
	//! \return How many keys went.
	static int Clear(string id)
	{
		MCF_Core_DataSet dataSet = Find(id);
		if (!dataSet)
		{
			MCF_Core_Log.Warn("no persistent data set called '" + id + "'");
			return 0;
		}

		int removed = dataSet.ClearKeys();

		MCF_Core_PersistentStore.GetInstance().Save();
		Reloaded();

		MCF_Core_Log.Debug("cleared persistent set '" + id + "' -- " + removed.ToString() + " key(s)");
		return removed;
	}

	//! Clears every registered set.
	//!
	//! NOT the same as emptying the file. Keys nobody declared -- the server run
	//! counter, and whatever a future module writes before it registers -- are
	//! left alone, because deleting data whose owner is unknown is how a "clear
	//! the mission" button quietly breaks something three modules away.
	static int ClearAll()
	{
		if (!s_aSets)
			return 0;

		int removed;

		foreach (MCF_Core_DataSet dataSet : s_aSets)
		{
			removed = removed + dataSet.ClearKeys();
		}

		MCF_Core_PersistentStore.GetInstance().Save();
		Reloaded();

		MCF_Core_Log.Debug("cleared every persistent set -- " + removed.ToString() + " key(s)");
		return removed;
	}

	//! Tells every module holding a runtime copy that the store has changed.
	static void Reloaded()
	{
		GetOnReloaded().Invoke();
	}
}
