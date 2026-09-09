//! Server-authoritative store of player-authored tasks, persisted across
//! restarts.
//!
//! This is what makes the persistent-server model possible: a commander plans
//! on Monday, the server restarts twice during the week, and the plan is still
//! there when the unit shows up on Saturday. See
//! docs/research/persistent-server-and-phases.md.
//!
//! Persisted through MCF_Core_PersistentStore, which writes to the server's
//! profile directory rather than the engine's world/session saves. That
//! separation is deliberate and load-bearing: world saves are invalidated when
//! the mod changes, and a unit running a server all week while the mod is
//! still being developed would otherwise lose a week of planning to a mod
//! update.
//!
//! Storage is one line per task, produced by MCF_Task.Serialize() -- the same
//! implementation the network layer uses, so the two cannot drift apart:
//!
//!     tasks=t1,t2
//!     task.t1=id=t1<TAB>title=Recon the north approach<TAB>...
//!
//! On a client this same class holds the mirror of what the server has told
//! this player about, which is deliberately not everything -- see
//! IsVisibleTo().

class MCF_Core_TaskStore
{
	protected static const string KEY_INDEX = "tasks";
	protected static const string KEY_NEXT_ID = "taskNextId";
	protected static const string KEY_PREFIX = "task.";

	private static ref MCF_Core_TaskStore s_Instance;

	protected ref map<string, ref MCF_Task> m_mTasks;
	protected int m_iNextId;

	void MCF_Core_TaskStore()
	{
		m_mTasks = new map<string, ref MCF_Task>();
		m_iNextId = 1;
	}

	static MCF_Core_TaskStore GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_TaskStore();
		return s_Instance;
	}

	// ---------------------------------------------------------------- write

	//! Creates a task and persists it. Server only -- task ids must be
	//! allocated in one place or two machines will hand out the same one.
	//! \return The new task, or null if not on the server.
	MCF_Task CreateTask(string title, int authorPlayerId = 0)
	{
		if (!Replication.IsServer())
		{
			MCF_Core_Log.Warn("TaskStore.CreateTask called on a client -- ignored, tasks are server state");
			return null;
		}

		MCF_Task task = new MCF_Task();
		task.m_sId = "t" + m_iNextId.ToString();
		m_iNextId++;
		task.m_sTitle = title;
		task.m_iAuthorPlayerId = authorPlayerId;

		m_mTasks.Set(task.m_sId, task);
		MCF_Core_Log.Debug("TaskStore created " + task.Describe());

		Save();
		return task;
	}

	//! Assigns a task and moves it out of DRAFT.
	bool AssignTask(string taskId, MCF_ETaskAssignee assigneeType, string assigneeId)
	{
		MCF_Task task = GetTask(taskId);
		if (!task)
			return false;

		task.m_eAssigneeType = assigneeType;
		task.m_sAssigneeId = assigneeId;
		task.m_eState = MCF_ETaskState.ASSIGNED;

		MCF_Core_Log.Debug("TaskStore assigned " + task.Describe());
		Save();
		return true;
	}

	//! Records the subordinate's back brief and marks the task acknowledged.
	bool AcknowledgeTask(string taskId, string backBrief)
	{
		MCF_Task task = GetTask(taskId);
		if (!task)
			return false;

		task.m_sBackBrief = backBrief;
		task.m_eState = MCF_ETaskState.ACKNOWLEDGED;

		MCF_Core_Log.Debug("TaskStore acknowledged " + task.Describe());
		Save();
		return true;
	}

	//! Replaces the written content of a task and nothing else.
	//!
	//! Separate from a general "apply this task object" on purpose. The text
	//! is the only part a player authors; id, author, state and assignee are
	//! decided by the server and must not be settable from an edit. Keeping
	//! that split here rather than in the RPC handler means it holds for any
	//! future caller too.
	bool UpdateText(string taskId, string title, string situation, string mission, string execution, string adminLogistics, string commandSignal)
	{
		MCF_Task task = GetTask(taskId);
		if (!task)
			return false;

		// An order with no name is unusable in a list, so refuse to erase it
		// rather than silently storing a blank.
		if (!title.IsEmpty())
			task.m_sTitle = title;

		task.m_sSituation = situation;
		task.m_sMission = mission;
		task.m_sExecution = execution;
		task.m_sAdminLogistics = adminLogistics;
		task.m_sCommandSignal = commandSignal;

		MCF_Core_Log.Debug("TaskStore text updated " + task.Describe());
		Save();
		return true;
	}


	bool SetState(string taskId, MCF_ETaskState state)
	{
		MCF_Task task = GetTask(taskId);
		if (!task)
			return false;

		task.m_eState = state;
		MCF_Core_Log.Debug("TaskStore state change " + task.Describe());
		Save();
		return true;
	}

	//! Removes an order permanently.
	//!
	//! Note what is NOT reset: m_iNextId keeps climbing. Reusing the number of
	//! a deleted order would hand a future order the same reference somebody
	//! already wrote down or said on the radio, and OPORD-003 meaning two
	//! different things across one session is worse than a gap in the
	//! numbering.
	bool DeleteTask(string taskId)
	{
		if (!Replication.IsServer())
		{
			MCF_Core_Log.Warn("TaskStore.DeleteTask called on a client -- ignored, orders are server state");
			return false;
		}

		MCF_Task task = GetTask(taskId);
		if (!task)
			return false;

		MCF_Core_Log.Debug("TaskStore deleting " + task.Describe());

		m_mTasks.Remove(taskId);

		// The stored line has to go too. Save() rewrites the index from what
		// is left in the map, so a stale task.<id> line would never be read
		// again -- but it would sit in the file forever, and a store that
		// only ever grows is a store nobody can read when something goes
		// wrong.
		MCF_Core_PersistentStore.GetInstance().Remove(KEY_PREFIX + taskId);
		Save();
		return true;
	}


	//! Inserts or replaces a task from serialised data. Used on the client to
	//! apply what the server sent; does not persist, because a client's view
	//! is a mirror and not a source of truth.
	MCF_Task ApplySerialized(string data)
	{
		MCF_Task task = MCF_Task.Deserialize(data);
		if (!task)
		{
			MCF_Core_Log.Warn("TaskStore could not deserialise a task");
			return null;
		}

		m_mTasks.Set(task.m_sId, task);
		return task;
	}

	//! Empties this machine's mirror of the task list.
	//!
	//! Client side only, and it refuses to run on a server -- on a hosted
	//! server the "mirror" and the authoritative store are the same singleton,
	//! so clearing it there would delete every task in the mission.
	//!
	//! Exists because re-sending a player their visible tasks cannot, on its
	//! own, take a task away. If another player accepts a board task, everyone
	//! else loses sight of it; without a clear, their client would keep
	//! showing the stale copy forever. Clear-then-resend is the only way to
	//! express a revocation without naming the task being revoked, which is
	//! the thing the visibility model exists to avoid.
	void ClearMirror()
	{
		if (Replication.IsServer())
			return;

		m_mTasks.Clear();
	}

	// ------------------------------------------------------------------ read

	MCF_Task GetTask(string taskId)
	{
		MCF_Task task;
		if (m_mTasks.Find(taskId, task))
			return task;
		return null;
	}

	int GetAllTasks(notnull out array<MCF_Task> outTasks)
	{
		outTasks.Clear();
		for (int i = 0; i < m_mTasks.Count(); i++)
			outTasks.Insert(m_mTasks.GetElement(i));
		return outTasks.Count();
	}

	int Count()
	{
		return m_mTasks.Count();
	}

	//! Whether a given player is entitled to see a task.
	//!
	//! This is why tasks are sent per player rather than broadcast: in milsim,
	//! a rifleman holding the commander's whole plan in memory is wrong in the
	//! fiction, not just wasteful. The server decides and sends only what each
	//! client may have.
	//!
	//! GROUP is not resolved yet -- it needs the squad membership work that
	//! comes with the command hierarchy. Until then a group-assigned task goes
	//! only to its author, which errs toward telling people too little rather
	//! than too much.
	bool IsVisibleTo(notnull MCF_Task task, int playerId, string factionKey)
	{
		// Your own drafts are yours alone.
		if (task.m_eState == MCF_ETaskState.DRAFT)
			return task.m_iAuthorPlayerId == playerId;

		// On the board: published to the force, not yet delegated. Everyone
		// can read the board.
		if (task.m_eState == MCF_ETaskState.PUBLISHED)
			return true;

		// The author always keeps sight of what they wrote.
		if (task.m_iAuthorPlayerId == playerId)
			return true;

		switch (task.m_eAssigneeType)
		{
			case MCF_ETaskAssignee.PLAYER:
				return task.m_sAssigneeId == playerId.ToString();

			case MCF_ETaskAssignee.FACTION:
				return !factionKey.IsEmpty() && task.m_sAssigneeId == factionKey;

			case MCF_ETaskAssignee.GROUP:
				// Cannot resolve group membership yet; withhold rather than
				// over-share.
				return false;
		}

		return false;
	}

	//! Every task this player may see.
	int GetTasksVisibleTo(int playerId, string factionKey, notnull out array<MCF_Task> outTasks)
	{
		outTasks.Clear();

		array<MCF_Task> all = {};
		GetAllTasks(all);

		foreach (MCF_Task task : all)
		{
			if (IsVisibleTo(task, playerId, factionKey))
				outTasks.Insert(task);
		}

		return outTasks.Count();
	}

	// --------------------------------------------------------- persistence

	//! Writes every task into the persistent store. Called after each change;
	//! task volumes are small enough that rewriting all of them is simpler
	//! than tracking dirty state, and far easier to reason about.
	void Save()
	{
		if (!Replication.IsServer())
			return;

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();

		string index = "";
		for (int i = 0; i < m_mTasks.Count(); i++)
		{
			MCF_Task task = m_mTasks.GetElement(i);

			if (i > 0)
				index = index + ",";
			index = index + task.m_sId;

			store.Set(KEY_PREFIX + task.m_sId, task.Serialize());
		}

		store.Set(KEY_INDEX, index);
		store.SetInt(KEY_NEXT_ID, m_iNextId);
		store.Save();
	}

	//! Rebuilds the task list from the persistent store. Call once at mission
	//! start, after MCF_Core_PersistentStore.Load().
	void Load()
	{
		m_mTasks.Clear();

		MCF_Core_PersistentStore store = MCF_Core_PersistentStore.GetInstance();
		m_iNextId = store.GetInt(KEY_NEXT_ID, 1);

		string index = store.Get(KEY_INDEX, "");
		if (index.IsEmpty())
		{
			MCF_Core_Log.Debug("TaskStore: no stored tasks");
			return;
		}

		array<string> ids = {};
		index.Split(",", ids, true);

		foreach (string id : ids)
		{
			if (id.IsEmpty())
				continue;

			MCF_Task task = MCF_Task.Deserialize(store.Get(KEY_PREFIX + id, ""));
			if (!task)
			{
				MCF_Core_Log.Warn("TaskStore could not restore task '" + id + "'");
				continue;
			}

			m_mTasks.Set(task.m_sId, task);
		}

		MCF_Core_Log.Debug("TaskStore loaded " + m_mTasks.Count().ToString() + " task(s) from previous sessions");

		array<MCF_Task> all = {};
		GetAllTasks(all);
		foreach (MCF_Task loaded : all)
			MCF_Core_Log.Debug("TaskStore   " + loaded.Describe());
	}
}
