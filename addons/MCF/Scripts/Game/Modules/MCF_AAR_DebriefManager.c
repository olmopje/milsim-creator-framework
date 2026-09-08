//! After-Action Review / Debrief module (ARCHITECTURE.md 5.8). Passively
//! listens to Event Bus events already published by earlier modules and
//! logs them with a timestamp, so a session can be summarized afterward.
//!
//! Only listens to a fixed set of well-known event names for now
//! (objective outcomes, hostility changes, compliance outcomes). A
//! generic "log everything published" approach isn't possible without
//! the Event Bus exposing a way to observe all events, which it
//! currently doesn't (see MCF_Core_EventManager) -- this is an explicit
//! subscribe-per-event-name list instead.

class MCF_AAR_LogEntry
{
	string m_sEventName;
	float m_fTimestamp;

	void MCF_AAR_LogEntry(string eventName, float timestamp)
	{
		m_sEventName = eventName;
		m_fTimestamp = timestamp;
	}
}

class MCF_AAR_DebriefManager
{
	private static ref MCF_AAR_DebriefManager s_Instance;

	protected ref array<ref MCF_AAR_LogEntry> m_aLog;
	protected bool m_bListening;

	void MCF_AAR_DebriefManager()
	{
		m_aLog = new array<ref MCF_AAR_LogEntry>();
	}

	static MCF_AAR_DebriefManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_AAR_DebriefManager();
		return s_Instance;
	}

	//! Starts listening to the fixed set of tracked events. Call once,
	//! e.g. from a future game mode's OnGameStart.
	void StartListening()
	{
		if (m_bListening)
			return;

		m_bListening = true;

		MCF_Core_EventManager manager = MCF_Core_EventManager.GetInstance();
		manager.GetInvoker("Objective_Complete").Insert(OnObjectiveComplete);
		manager.GetInvoker("Objective_Fail").Insert(OnObjectiveFail);
		manager.GetInvoker("Hostility_Changed").Insert(OnHostilityChanged);
		manager.GetInvoker("MCF_AI_ComplianceGranted").Insert(OnComplianceGranted);
	}

	protected void OnObjectiveComplete(Managed payload)
	{
		Log("Objective_Complete");
	}

	protected void OnObjectiveFail(Managed payload)
	{
		Log("Objective_Fail");
	}

	protected void OnHostilityChanged(Managed payload)
	{
		Log("Hostility_Changed");
	}

	protected void OnComplianceGranted(Managed payload)
	{
		Log("MCF_AI_ComplianceGranted");
	}

	protected void Log(string eventName)
	{
		float timestamp = GetGame().GetWorld().GetWorldTime();
		m_aLog.Insert(new MCF_AAR_LogEntry(eventName, timestamp));
	}

	//! Builds a plain-text summary: one line per logged event, in order.
	string BuildSummary()
	{
		string summary = "";
		foreach (MCF_AAR_LogEntry entry : m_aLog)
			summary += string.Format("[%1s] %2\n", entry.m_fTimestamp, entry.m_sEventName);

		return summary;
	}

	int GetLogCount()
	{
		return m_aLog.Count();
	}
}
