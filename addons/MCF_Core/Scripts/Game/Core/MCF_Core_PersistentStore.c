//! Server-side key/value store that survives restarts.
//!
//! Writes to "$profile:MCF_store.txt", which resolves inside the profile
//! directory the server was launched with (-profile). That location is owned
//! by us and is deliberately NOT part of the engine's world/session save:
//!
//!   - World and session saves are tied to placed entities and are
//!     invalidated when the mod changes ("a large mod change" is effectively a
//!     world reset). A unit running a server all week while the mod is still
//!     being developed would lose a week of planning that way.
//!   - This store is plain data keyed by name, so iterating on the mod cannot
//!     destroy it.
//!
//! See docs/research/persistent-server-and-phases.md for why that separation
//! is a requirement rather than a preference.
//!
//! Format is one "key=value" per line. Deliberately trivial: values must not
//! contain a newline, and everything after the first "=" is the value, so "="
//! inside a value is safe. Structured records (a plan with SMEAC fields) will
//! need a real serialisation format -- this is the foundation, not the finished
//! store.
//!
//! Not networked. This is server-side state; replicating it to clients is a
//! separate concern (see docs/research/multiplayer-and-audience.md).

class MCF_Core_PersistentStore
{
	protected static const string STORE_PATH = "$profile:MCF_store.txt";

	private static ref MCF_Core_PersistentStore s_Instance;

	protected ref map<string, string> m_mData;
	protected bool m_bLoaded;

	void MCF_Core_PersistentStore()
	{
		m_mData = new map<string, string>();
	}

	static MCF_Core_PersistentStore GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_PersistentStore();
		return s_Instance;
	}

	//! Reads the store from disk, replacing anything held in memory.
	//! Safe to call when the file does not exist yet -- that is simply an
	//! empty store, which is what a first run looks like.
	void Load()
	{
		m_mData.Clear();
		m_bLoaded = true;

		if (!FileIO.FileExists(STORE_PATH))
		{
			MCF_Core_Log.Debug("PersistentStore: no store file yet at " + STORE_PATH + " -- starting empty");
			return;
		}

		FileHandle file = FileIO.OpenFile(STORE_PATH, FileMode.READ);
		if (!file)
		{
			MCF_Core_Log.Debug("PersistentStore: could not open " + STORE_PATH + " for reading");
			return;
		}

		int lineCount = 0;
		string line;
		while (file.ReadLine(line) >= 0)
		{
			int split = line.IndexOf("=");
			if (split <= 0)
				continue;

			string key = line.Substring(0, split);
			string value = line.Substring(split + 1, line.Length() - split - 1);
			m_mData.Set(key, value);
			lineCount++;
		}

		file.Close();
		MCF_Core_Log.Debug("PersistentStore: loaded " + lineCount.ToString() + " entries from " + STORE_PATH);
	}

	//! Writes the whole store back to disk.
	bool Save()
	{
		FileHandle file = FileIO.OpenFile(STORE_PATH, FileMode.WRITE);
		if (!file)
		{
			MCF_Core_Log.Debug("PersistentStore: could not open " + STORE_PATH + " for writing");
			return false;
		}

		foreach (string key, string value : m_mData)
		{
			file.WriteLine(key + "=" + value);
		}

		file.Close();
		MCF_Core_Log.Debug("PersistentStore: saved " + m_mData.Count().ToString() + " entries to " + STORE_PATH);
		return true;
	}

	string Get(string key, string defaultValue = "")
	{
		string value;
		if (m_mData.Find(key, value))
			return value;
		return defaultValue;
	}

	void Set(string key, string value)
	{
		m_mData.Set(key, value);
	}

	int GetInt(string key, int defaultValue = 0)
	{
		string value = Get(key, "");
		if (value.IsEmpty())
			return defaultValue;
		return value.ToInt();
	}

	void SetInt(string key, int value)
	{
		Set(key, value.ToString());
	}

	//! Forgets a key. The file only shrinks on the next Save().
	void Remove(string key)
	{
		m_mData.Remove(key);
	}

	//! Every key currently held, in no particular order.
	int GetKeys(notnull out array<string> outKeys)
	{
		outKeys.Clear();

		foreach (string key, string ignored : m_mData)
		{
			outKeys.Insert(key);
		}

		return outKeys.Count();
	}

	//! Forgets every key starting with a prefix.
	//!
	//! COLLECTED FIRST, REMOVED AFTER. Removing entries from a map while
	//! iterating it is how a loop starts skipping things, and the symptom --
	//! "clearing left two of the seven behind" -- reads like a bug in the data
	//! rather than in the loop.
	//!
	//! \return How many were forgotten.
	int RemoveByPrefix(string prefix)
	{
		if (prefix.IsEmpty())
			return 0;

		array<string> doomed = {};

		foreach (string key, string ignored : m_mData)
		{
			if (key.IndexOf(prefix) == 0)
				doomed.Insert(key);
		}

		foreach (string key : doomed)
		{
			m_mData.Remove(key);
		}

		return doomed.Count();
	}

	//! Empties the store. The file only shrinks on the next Save().
	void Clear()
	{
		m_mData.Clear();
	}

	//! Writes a copy somewhere else -- a snapshot, a backup.
	bool SaveTo(string path)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
		{
			MCF_Core_Log.Warn("PersistentStore: could not write " + path);
			return false;
		}

		foreach (string key, string value : m_mData)
		{
			file.WriteLine(key + "=" + value);
		}

		file.Close();
		return true;
	}

	//! Replaces everything held with the contents of another file.
	//!
	//! REPLACES, does not merge. A snapshot is a picture of a whole mission;
	//! merging it into whatever is there now would produce a state that never
	//! existed, and the reason a person restores a snapshot is to stop being
	//! where they are.
	bool LoadFrom(string path)
	{
		if (!FileIO.FileExists(path))
		{
			MCF_Core_Log.Warn("PersistentStore: no file at " + path);
			return false;
		}

		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
		{
			MCF_Core_Log.Warn("PersistentStore: could not read " + path);
			return false;
		}

		m_mData.Clear();

		int lineCount = 0;
		string line;
		while (file.ReadLine(line) >= 0)
		{
			int split = line.IndexOf("=");
			if (split <= 0)
				continue;

			string key = line.Substring(0, split);
			string value = line.Substring(split + 1, line.Length() - split - 1);
			m_mData.Set(key, value);
			lineCount++;
		}

		file.Close();

		MCF_Core_Log.Debug("PersistentStore: loaded " + lineCount.ToString() + " entries from " + path);
		return true;
	}


	bool Has(string key)
	{
		string ignored;
		return m_mData.Find(key, ignored);
	}

	int Count()
	{
		return m_mData.Count();
	}
}
