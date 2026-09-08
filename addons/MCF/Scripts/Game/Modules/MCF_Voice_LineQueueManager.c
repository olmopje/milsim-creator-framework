//! Priority queue for text lines -- the simplified stand-in for the Voice
//! Line module (ARCHITECTURE.md 4.4) while audio is parked. Only one line
//! is "active" at a time; higher-priority lines jump ahead of queued ones.
//!
//! Publishes "MCF_Voice_LineDisplayed" (payload: MCF_Voice_LinePayload)
//! when a line becomes active. Whatever shows the text on screen calls
//! MarkLineFinished() when done, which lets the next queued line through.

class MCF_Voice_LinePayload
{
	string m_sText;

	void MCF_Voice_LinePayload(string text)
	{
		m_sText = text;
	}
}

class MCF_Voice_LineQueueManager
{
	private static ref MCF_Voice_LineQueueManager s_Instance;

	protected ref array<string> m_aQueuedText;
	protected ref array<int> m_aQueuedPriority;
	protected bool m_bBusy;

	void MCF_Voice_LineQueueManager()
	{
		m_aQueuedText = new array<string>();
		m_aQueuedPriority = new array<int>();
	}

	static MCF_Voice_LineQueueManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Voice_LineQueueManager();
		return s_Instance;
	}

	//! Adds text to the queue. Higher priority values jump ahead of lower
	//! ones already queued. Processes immediately if nothing is active.
	void Enqueue(string text, int priority)
	{
		int insertAt = m_aQueuedPriority.Count();
		for (int i = 0; i < m_aQueuedPriority.Count(); i++)
		{
			if (priority > m_aQueuedPriority[i])
			{
				insertAt = i;
				break;
			}
		}

		m_aQueuedText.InsertAt(text, insertAt);
		m_aQueuedPriority.InsertAt(priority, insertAt);

		if (!m_bBusy)
			ProcessNext();
	}

	protected void ProcessNext()
	{
		if (m_aQueuedText.IsEmpty())
		{
			m_bBusy = false;
			return;
		}

		string text = m_aQueuedText[0];
		m_aQueuedText.RemoveOrdered(0);
		m_aQueuedPriority.RemoveOrdered(0);

		m_bBusy = true;
		MCF_Core_EventManager.GetInstance().Publish("MCF_Voice_LineDisplayed", new MCF_Voice_LinePayload(text));
	}

	//! Call once the currently displayed line has finished (e.g. after a
	//! fixed duration). Lets the next queued line through.
	void MarkLineFinished()
	{
		m_bBusy = false;
		ProcessNext();
	}

	int GetQueueLength()
	{
		return m_aQueuedText.Count();
	}
}
