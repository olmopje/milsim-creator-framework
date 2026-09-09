//! Priority queue for text lines -- the simplified stand-in for the Voice
//! Line module (ARCHITECTURE.md 4.4) while audio is parked.
//!
//! Publishes "MCF_Voice_LineDisplayed" (payload: MCF_Voice_LinePayload) when a
//! line becomes active. MCF_UI_LineDisplayComponent consumes that on the
//! server and broadcasts it to clients.
//!
//! Each line carries its audience with it, so a node can say who a message is
//! for at the point it creates the message. The queue does not interpret the
//! audience; it only carries it through to the display layer, which is where
//! the decision is made -- locally on each client.
//!
//! DEADLOCK HAZARD: if a line is published and nothing ever calls
//! MarkLineFinished(), m_bBusy stays true and every later line queues up
//! forever. This is a static singleton, so that stuck state also survives the
//! World Editor -> play mode transition. It bit us exactly that way once.
//! Reset() exists for that reason and is called from
//! MCF_Core_GameModeComponent.OnGameModeStart().

//! Who a line is meant for. Only EVERYONE, FACTION and PLAYER are implemented;
//! GROUP needs the command hierarchy work that comes with the task system --
//! see docs/research/mcf-task-system-design.md.
enum MCF_EAudience
{
	EVERYONE,
	FACTION,
	GROUP,
	PLAYER
}

class MCF_Voice_LinePayload
{
	string m_sText;
	MCF_EAudience m_eAudience;
	//! Faction key when the audience is FACTION.
	string m_sFactionKey;
	//! Player id when the audience is PLAYER.
	int m_iPlayerId;

	void MCF_Voice_LinePayload(string text, MCF_EAudience audience = MCF_EAudience.EVERYONE, string factionKey = "", int playerId = 0)
	{
		m_sText = text;
		m_eAudience = audience;
		m_sFactionKey = factionKey;
		m_iPlayerId = playerId;
	}
}

class MCF_Voice_LineQueueManager
{
	private static ref MCF_Voice_LineQueueManager s_Instance;

	protected ref array<ref MCF_Core_LineQueueEntry> m_aQueue;
	protected bool m_bBusy;

	void MCF_Voice_LineQueueManager()
	{
		m_aQueue = new array<ref MCF_Core_LineQueueEntry>();

		// Declare the event we publish, so the validation pass does not
		// report the display component as listening for something nothing
		// sends. The game mode resets this queue before running validation,
		// so this constructor has always run by then.
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_Voice_LineDisplayed");
	}

	static MCF_Voice_LineQueueManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Voice_LineQueueManager();
		return s_Instance;
	}

	//! Drops any queued lines and clears the busy flag. Call at mission start
	//! so state left over from the World Editor cannot stall the queue for a
	//! whole play session.
	void Reset()
	{
		m_aQueue.Clear();
		m_bBusy = false;
		MCF_Core_Log.Debug("LineQueue reset");
	}

	//! Adds text to the queue. Higher priority values jump ahead of lower
	//! ones already queued. Processes immediately if nothing is active.
	void Enqueue(string text, int priority, MCF_EAudience audience = MCF_EAudience.EVERYONE, string factionKey = "", int playerId = 0)
	{
		MCF_Core_LineQueueEntry entry = new MCF_Core_LineQueueEntry();
		entry.m_sText = text;
		entry.m_iPriority = priority;
		entry.m_eAudience = audience;
		entry.m_sFactionKey = factionKey;
		entry.m_iPlayerId = playerId;

		int insertAt = m_aQueue.Count();
		for (int i = 0; i < m_aQueue.Count(); i++)
		{
			if (priority > m_aQueue[i].m_iPriority)
			{
				insertAt = i;
				break;
			}
		}

		m_aQueue.InsertAt(entry, insertAt);

		if (!m_bBusy)
			ProcessNext();
	}

	protected void ProcessNext()
	{
		if (m_aQueue.IsEmpty())
		{
			m_bBusy = false;
			return;
		}

		MCF_Core_LineQueueEntry entry = m_aQueue[0];
		m_aQueue.RemoveOrdered(0);

		m_bBusy = true;
		MCF_Core_EventManager.GetInstance().Publish("MCF_Voice_LineDisplayed", new MCF_Voice_LinePayload(entry.m_sText, entry.m_eAudience, entry.m_sFactionKey, entry.m_iPlayerId));
	}

	//! Call once the currently displayed line has been handed on. Lets the
	//! next queued line through.
	void MarkLineFinished()
	{
		m_bBusy = false;
		ProcessNext();
	}

	int GetQueueLength()
	{
		return m_aQueue.Count();
	}

	bool IsBusy()
	{
		return m_bBusy;
	}
}

//! One queued line. A class rather than parallel arrays so that adding a field
//! (audience, and whatever comes after it) does not mean adding another array
//! and another place to keep the indices in step.
class MCF_Core_LineQueueEntry
{
	string m_sText;
	int m_iPriority;
	MCF_EAudience m_eAudience;
	string m_sFactionKey;
	int m_iPlayerId;
}
