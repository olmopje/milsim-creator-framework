//! Tracks which events scenario nodes publish and which they consume, so
//! misconfigured event names (typos, forgotten links) show up as a clear
//! log warning instead of a silent no-op (ARCHITECTURE.md 3.1, "Validation
//! pass at mission init").
//!
//! Every node that publishes or listens to an event should register that
//! in its own EOnInit. RunValidation() then reports any consumed event
//! that has no registered publisher.
//!
//! Not yet wired to an automatic mission-start trigger -- no game mode
//! component exists yet to call it at the right time. Call it manually
//! for now; a future game mode component will call it automatically once
//! all entities have initialized.

class MCF_Core_ValidationRegistry
{
	private static ref MCF_Core_ValidationRegistry s_Instance;

	protected ref set<string> m_setPublishedEvents;
	protected ref map<string, ref array<string>> m_mConsumers;

	void MCF_Core_ValidationRegistry()
	{
		m_setPublishedEvents = new set<string>();
		m_mConsumers = new map<string, ref array<string>>();
	}

	static MCF_Core_ValidationRegistry GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_ValidationRegistry();
		return s_Instance;
	}

	//! Call from a node's EOnInit for every event name it can publish.
	void RegisterPublisher(string eventName)
	{
		if (!eventName.IsEmpty())
			m_setPublishedEvents.Insert(eventName);
	}

	//! Call from a node's EOnInit for every event name it listens to.
	//! description identifies the node/attribute for the warning message,
	//! e.g. "MCF_Obj_LogicComponent on 'Ambush_Logic' (input event)".
	void RegisterConsumer(string eventName, string description)
	{
		if (eventName.IsEmpty())
			return;

		ref array<string> descriptions = m_mConsumers.Get(eventName);
		if (!descriptions)
		{
			descriptions = new array<string>();
			m_mConsumers.Set(eventName, descriptions);
		}
		descriptions.Insert(description);
	}

	//! Logs a warning for every consumed event with no registered
	//! publisher. Returns the number of warnings logged.
	int RunValidation()
	{
		int warningCount = 0;

		for (int i = 0; i < m_mConsumers.Count(); i++)
		{
			string eventName = m_mConsumers.GetKey(i);
			if (m_setPublishedEvents.Contains(eventName))
				continue;

			ref array<string> descriptions = m_mConsumers.GetElement(i);
			foreach (string description : descriptions)
			{
				Print(string.Format("MCF Validation (W): event \"%1\" is never published, but is expected by %2", eventName, description));
				warningCount++;
			}
		}

		return warningCount;
	}
}
