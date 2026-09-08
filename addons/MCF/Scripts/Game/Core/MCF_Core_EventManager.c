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
// the Module Registry is the place that validates registered event names
// against this pattern (not yet implemented -- see MCF_Core_ModuleRegistry).
//
// Lifecycle: call UnsubscribeAllForOwner() when an owning entity is about to
// be deleted/despawned, so no dangling subscriptions remain. This is not
// automatic (Enforce Script has no destructor hook that reliably fires before
// entity deletion in every case), so callers are responsible for it -- see
// the TODO in MCF_Core_ObjectIdentityComponent for a component-level wrapper
// that will call this automatically from its own EOnDeactivate.
//
// NOT YET TESTED IN WORKBENCH. Written from the architecture spec; verify the
// ScriptInvoker call signatures compile against the actual Reforger SDK
// before relying on this in other modules.

class MCF_Core_EventSubscription
{
	string m_sEventName;
	ScriptInvokerBase m_Handler;
	IEntity m_Owner;

	void MCF_Core_EventSubscription(string eventName, ScriptInvokerBase handler, IEntity owner)
	{
		m_sEventName = eventName;
		m_Handler = handler;
		m_Owner = owner;
	}
}

class MCF_Core_EventManager
{
	private static ref MCF_Core_EventManager s_Instance;

	// One ScriptInvoker per event name, created lazily on first subscribe.
	protected ref map<string, ref ScriptInvoker> m_mEventInvokers;

	// All active subscriptions, kept so UnsubscribeAllForOwner() can find and
	// remove every subscription belonging to a given entity without callers
	// having to track their own handler references.
	protected ref array<ref MCF_Core_EventSubscription> m_aSubscriptions;

	void MCF_Core_EventManager()
	{
		m_mEventInvokers = new map<string, ref ScriptInvoker>();
		m_aSubscriptions = new array<ref MCF_Core_EventSubscription>();
	}

	static MCF_Core_EventManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_EventManager();
		return s_Instance;
	}

	//! Subscribe a handler to an event. If owner is provided, the subscription
	//! is tracked against that entity so UnsubscribeAllForOwner() can clean it
	//! up later -- always pass owner for anything attached to a spawnable
	//! entity to avoid dangling listeners (see ARCHITECTURE.md 3.1).
	void Subscribe(string eventName, ScriptInvokerBase handler, IEntity owner = null)
	{
		if (eventName.IsEmpty() || !handler)
			return;

		ScriptInvoker invoker = m_mEventInvokers.Get(eventName);
		if (!invoker)
		{
			invoker = new ScriptInvoker();
			m_mEventInvokers.Set(eventName, invoker);
		}

		invoker.Insert(handler);
		m_aSubscriptions.Insert(new MCF_Core_EventSubscription(eventName, handler, owner));
	}

	//! Remove a single handler from a single event.
	void Unsubscribe(string eventName, ScriptInvokerBase handler)
	{
		ScriptInvoker invoker = m_mEventInvokers.Get(eventName);
		if (invoker)
			invoker.Remove(handler);

		for (int i = m_aSubscriptions.Count() - 1; i >= 0; i--)
		{
			MCF_Core_EventSubscription sub = m_aSubscriptions[i];
			if (sub.m_sEventName == eventName && sub.m_Handler == handler)
			{
				m_aSubscriptions.Remove(i);
				break;
			}
		}
	}

	//! Fire all handlers subscribed to eventName. payload is passed through
	//! as-is; each module defines its own payload class for its own events
	//! (documented per module, not enforced centrally).
	void Publish(string eventName, Managed payload = null)
	{
		ScriptInvoker invoker = m_mEventInvokers.Get(eventName);
		if (invoker)
			invoker.Invoke(payload);
	}

	//! Remove every subscription belonging to owner, regardless of event name.
	//! Call this from an entity's cleanup path (e.g. EOnDeactivate on a
	//! MCF_Core_ObjectIdentityComponent) before the entity is deleted.
	void UnsubscribeAllForOwner(IEntity owner)
	{
		if (!owner)
			return;

		for (int i = m_aSubscriptions.Count() - 1; i >= 0; i--)
		{
			MCF_Core_EventSubscription sub = m_aSubscriptions[i];
			if (sub.m_Owner != owner)
				continue;

			ScriptInvoker invoker = m_mEventInvokers.Get(sub.m_sEventName);
			if (invoker)
				invoker.Remove(sub.m_Handler);

			m_aSubscriptions.Remove(i);
		}
	}

	//! Diagnostic helper for the debug overlay (ARCHITECTURE.md section 7.7) --
	//! not wired up yet, just exposed so that module can read it once it exists.
	int GetActiveSubscriptionCount()
	{
		return m_aSubscriptions.Count();
	}
}
