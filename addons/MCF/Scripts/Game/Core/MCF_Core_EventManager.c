//! Central event bus for the Milsim Creator Framework.
//!
//! All inter-module communication goes through this class -- modules never
//! reference each other directly (see ARCHITECTURE.md section 3.1).
//!
//! Naming convention: event names follow "Module_Action", e.g.
//! "Objective_Complete", "Hostility_ThresholdCrossed".
//!
//! Usage:
//!   MCF_Core_EventManager.GetInstance().GetInvoker("Objective_Complete").Insert(OnObjectiveComplete);
//!   MCF_Core_EventManager.GetInstance().Publish("Objective_Complete", payload);
//!   MCF_Core_EventManager.GetInstance().GetInvoker("Objective_Complete").Remove(OnObjectiveComplete);
//!
//! Lifecycle cleanup: the Event Bus does not track individual subscriptions
//! centrally. Each subscribing component removes its own callback from its
//! own EOnDeactivate, since it already holds the concrete function reference
//! needed for ScriptInvoker.Remove().

class MCF_Core_EventManager
{
	private static ref MCF_Core_EventManager s_Instance;

	protected ref map<string, ref ScriptInvoker> m_mEventInvokers;

	void MCF_Core_EventManager()
	{
		m_mEventInvokers = new map<string, ref ScriptInvoker>();
	}

	static MCF_Core_EventManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_EventManager();
		return s_Instance;
	}

	//! Returns the ScriptInvoker for eventName, creating it on first use.
	ScriptInvoker GetInvoker(string eventName)
	{
		ScriptInvoker invoker = m_mEventInvokers.Get(eventName);
		if (!invoker)
		{
			invoker = new ScriptInvoker();
			m_mEventInvokers.Set(eventName, invoker);
		}
		return invoker;
	}

	//! Fires all handlers subscribed to eventName with the given payload.
	void Publish(string eventName, Managed payload = null)
	{
		ScriptInvoker invoker = m_mEventInvokers.Get(eventName);
		if (invoker)
			invoker.Invoke(payload);
	}

	//! Diagnostic helper for the debug overlay (ARCHITECTURE.md section 7).
	int GetActiveEventCount()
	{
		return m_mEventInvokers.Count();
	}
}
