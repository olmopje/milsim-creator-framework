// MCF_Core_EventManager
//
// Central event bus for the Milsim Creator Framework.
//
// All inter-module communication goes through this class -- modules never
// reference each other directly (see ARCHITECTURE.md section 3.1,
// "Event-contract" and "Event Bus-lifecycle-koppeling").
//
// Event naming convention: "Module_Action", e.g. "Objective_Complete",
// "Hostility_ThresholdCrossed". Names are not enforced at compile time here;
// the Module Registry is the place that will validate registered event names
// against this pattern (not yet implemented -- see MCF_Core_ModuleRegistry).
//
// USAGE PATTERN (matches the standard Enfusion ScriptInvoker idiom -- see
// https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceScriptInvokerBase.html):
//
//   MCF_Core_EventManager.GetInstance().GetInvoker("Objective_Complete").Insert(OnObjectiveComplete);
//   ...
//   MCF_Core_EventManager.GetInstance().Publish("Objective_Complete", payload);
//   ...
//   MCF_Core_EventManager.GetInstance().GetInvoker("Objective_Complete").Remove(OnObjectiveComplete);
//
// DESIGN NOTE on lifecycle cleanup (ARCHITECTURE.md 3.1's "Event Bus-lifecycle-koppeling"):
// an earlier version of this class tried to centralise Subscribe/Unsubscribe
// through a wrapper that stored the handler reference itself, typed as
// ScriptInvokerBase. That does not compile -- ScriptInvokerBase is a template
// base and needs type parameters we do not have a generic way to supply from
// a caller in Enforce Script. Consequence: the Event Bus itself only manages
// invokers, not individual subscriptions. Automatic per-owner cleanup instead
// belongs on the SUBSCRIBING side, where the concrete function reference is
// actually known -- see MCF_Core_ObjectIdentityComponent (not yet written),
// which should keep its own small list of (eventName, function) pairs and
// call GetInvoker(eventName).Remove(function) for each one from its own
// EOnDeactivate. This file does not implement that yet.
//
// NOT YET TESTED end-to-end -- this version fixes the specific compile
// errors from the first attempt (ScriptInvokerBase misuse, duplicate
// GetInstance) but has not been exercised by a real Subscribe/Publish call
// in Workbench yet. Verify before other modules depend on it.

class MCF_Core_EventManager
{
	private static ref MCF_Core_EventManager s_Instance;

	// One ScriptInvoker per event name, created lazily on first GetInvoker() call.
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
	//! Callers Insert()/Remove() their own callback function directly on the
	//! returned invoker -- see the usage pattern in the file header comment.
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

	//! Fire all handlers subscribed to eventName. payload is passed through
	//! as-is; each module defines its own payload class for its own events
	//! (documented per module, not enforced centrally). Does nothing if
	//! nothing has ever called GetInvoker() for this eventName (no invoker
	//! exists yet, so there is nothing to notify).
	void Publish(string eventName, Managed payload = null)
	{
		ScriptInvoker invoker = m_mEventInvokers.Get(eventName);
		if (invoker)
			invoker.Invoke(payload);
	}

	//! Diagnostic helper for the debug overlay (ARCHITECTURE.md section 7.7) --
	//! not wired up yet, just exposed so that module can read it once it exists.
	int GetActiveEventCount()
	{
		return m_mEventInvokers.Count();
	}
}
