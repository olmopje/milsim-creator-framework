//! Named copies of the whole persistent store.
//!
//! WHAT THIS IS FOR. A mission maker sets a scenario up -- the taskings, the
//! intel already known, the conversations, what is on the phones -- and then
//! runs it on Friday, and again next Friday, and wants the second Friday to
//! start where the first one did rather than where it ended. That is a
//! snapshot: not a backup taken in case something breaks, but a starting
//! position kept on purpose.
//!
//! WHY WHOLE-STORE AND NOT PER SET. The sets refer to each other -- a tasking
//! drafted from an intel report names that report's id -- so a snapshot of
//! intel alone would restore into a mission whose taskings point at reports
//! that are no longer there. Saving everything is both simpler and the only
//! version that is always consistent.
//!
//! WHY AN INDEX FILE AND NOT A DIRECTORY LISTING. Listing a directory from
//! script means FileIO.FindFiles and a callback per entry; an index file is
//! four lines of code and one read. It also means a snapshot folder somebody
//! copied files into by hand shows nothing until they are listed, which is the
//! right way round: this owns the folder, not the other way about.

class MCF_Core_DataSnapshots
{
	protected static const string FOLDER = "$profile:MCF/snapshots/";
	protected static const string INDEX = "$profile:MCF/snapshots/index.txt";

	//! Snapshot names are filenames, so they may not contain a path.
	protected static const int MAX_NAME = 48;

	//! Enough for a season of missions and few enough that the list stays
	//! readable. A limit that is never hit is still worth having: without one,
	//! a stuck loop writes files until the disk is full.
	protected static const int MAX_SNAPSHOTS = 40;

	static string PathFor(string name)
	{
		return FOLDER + Sanitise(name) + ".txt";
	}

	static bool Exists(string name)
	{
		return FileIO.FileExists(PathFor(name));
	}

	//! The names that have been saved, newest last.
	static int List(notnull out array<string> outNames)
	{
		outNames.Clear();

		if (!FileIO.FileExists(INDEX))
			return 0;

		FileHandle file = FileIO.OpenFile(INDEX, FileMode.READ);
		if (!file)
			return 0;

		string line;
		while (file.ReadLine(line) >= 0)
		{
			if (line.IsEmpty())
				continue;

			// A snapshot whose file was deleted by hand is dropped from the
			// listing rather than offered and then failing to load.
			if (FileIO.FileExists(PathFor(line)))
				outNames.Insert(line);
		}

		file.Close();
		return outNames.Count();
	}

	//! Writes the current store under a name, replacing an earlier one with the
	//! same name.
	static bool Save(string name)
	{
		string clean = Sanitise(name);
		if (clean.IsEmpty())
		{
			MCF_Core_Log.Warn("a snapshot needs a name");
			return false;
		}

		array<string> names = {};
		List(names);

		if (!names.Contains(clean) && names.Count() >= MAX_SNAPSHOTS)
		{
			MCF_Core_Log.Warn("there are already " + MAX_SNAPSHOTS.ToString()
				+ " snapshots -- delete one before saving another");
			return false;
		}

		FileIO.MakeDirectory("$profile:MCF/");
		FileIO.MakeDirectory(FOLDER);

		if (!MCF_Core_PersistentStore.GetInstance().SaveTo(PathFor(clean)))
			return false;

		if (!names.Contains(clean))
		{
			names.Insert(clean);
			WriteIndex(names);
		}

		MCF_Core_Log.Debug("saved snapshot '" + clean + "'");
		return true;
	}

	//! Replaces the live store with a snapshot and tells everyone to reload.
	static bool Load(string name)
	{
		string clean = Sanitise(name);
		string path = PathFor(clean);

		if (!FileIO.FileExists(path))
		{
			MCF_Core_Log.Warn("no snapshot called '" + clean + "'");
			return false;
		}

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		if (!store.LoadFrom(path))
			return false;

		// Written straight back out, so the live file on disk matches what is
		// in memory. Otherwise a crash between here and the next ordinary save
		// would come back up as the mission the snapshot replaced.
		store.Save();

		MCF_Core_DataSets.Reloaded();

		MCF_Core_Log.Debug("restored snapshot '" + clean + "'");
		return true;
	}

	static bool Delete(string name)
	{
		string clean = Sanitise(name);

		FileIO.DeleteFile(PathFor(clean));

		array<string> names = {};
		List(names);

		int at = names.Find(clean);
		if (at >= 0)
			names.Remove(at);

		WriteIndex(names);

		MCF_Core_Log.Debug("deleted snapshot '" + clean + "'");
		return true;
	}

	// ------------------------------------------------------------- internals

	protected static void WriteIndex(notnull array<string> names)
	{
		FileIO.MakeDirectory("$profile:MCF/");
		FileIO.MakeDirectory(FOLDER);

		FileHandle file = FileIO.OpenFile(INDEX, FileMode.WRITE);
		if (!file)
		{
			MCF_Core_Log.Warn("could not write the snapshot index at " + INDEX);
			return;
		}

		foreach (string name : names)
		{
			file.WriteLine(name);
		}

		file.Close();
	}

	//! Space off both ends. Written out rather than calling the engine's trim,
	//! because a name typed with a trailing space becomes a different filename
	//! from the same name typed without one, and that is a bug nobody can see.
	protected static string Trim(string value)
	{
		int first = 0;
		int last = value.Length() - 1;

		while (first <= last && value.Get(first) == " ")
		{
			first++;
		}

		while (last >= first && value.Get(last) == " ")
		{
			last--;
		}

		if (last < first)
			return "";

		return value.Substring(first, last - first + 1);
	}

	//! A name becomes a filename and a line in the index, so it may contain
	//! neither a path separator nor a newline.
	protected static string Sanitise(string name)
	{
		string result = name;
		result.Replace("/", "_");
		result.Replace("\\", "_");
		result.Replace(":", "_");
		result.Replace("..", "_");
		result.Replace("\n", "");
		result.Replace("\r", "");
		result.Replace("\t", " ");

		result = Trim(result);

		if (result.Length() > MAX_NAME)
			result = result.Substring(0, MAX_NAME);

		return result;
	}
}
