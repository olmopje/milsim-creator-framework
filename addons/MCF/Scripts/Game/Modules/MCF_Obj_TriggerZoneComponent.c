//! Fires an Event Bus event when activated. The Phase 0 proof-of-concept
//! node for the narrative/objective layer (ARCHITECTURE.md 4.1, 9).
//!
//! Set m_sTriggerEvent to have this zone activate from the Event Bus, the
//! same way every other MCF node is wired -- by event name, no entity
//! references. Without it, Activate() is only reachable from script, which
//! meant nothing in the framework could ever fire this node and a mission
//! maker could place it but never use it.
//!
//! Collision-volume-driven auto-detection is still a separate follow-up: this
//! zone has no shape of its own. For "player walks into an area", use
//! MCF_Obj_ProximityTriggerComponent, which actually detects.

[ComponentEditorProps(category: "MCF/Objective", description: "Fires an Event Bus event when activated.")]
class MCF_Obj_TriggerZoneComponentClass : ScriptComponentClass
{
}

class MCF_Obj_TriggerZoneComponent : ScriptComponent
{
	[Attribute(defvalue: "MCF_Obj_ZoneActivated", uiwidget: UIWidgets.EditBox, desc: "Event name published on the Event Bus when this zone activates. Follows the Module_Action naming contract (ARCHITECTURE.md 3.1).")]
	protected string m_sEventName;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that activates this zone. Leave empty to activate it only from script.")]
	protected string m_sTriggerEvent;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "If true, this zone only fires once -- repeated Activate() calls after the first do nothing.")]
	protected bool m_bTriggerOnce;

	protected bool m_bHasTriggered;
	protected ScriptInvoker m_TriggerInvoker;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		// Reaction and logic nodes are server-side, like the detection nodes
		// that feed them. On a client these would subscribe to events that
		// never fire there -- harmless today, but it leaves the client holding
		// framework state it should not have, and if anything ever did publish
		// locally the two machines would diverge.
		if (!Replication.IsServer())
			return;

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sEventName);

		if (!m_sTriggerEvent.IsEmpty())
		{
			m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
			m_TriggerInvoker.Insert(OnTriggerEvent);
			MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_Obj_TriggerZoneComponent (activation)");
		}

		MCF_Core_Log.Debug("TriggerZone init, activated by '" + m_sTriggerEvent + "' publishes '" + m_sEventName + "'");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTriggerEvent);
	}

	protected void OnTriggerEvent(Managed payload)
	{
		Activate();
	}

	//! Publishes m_sEventName on the Event Bus so any module can react
	//! without a hard reference to this entity.
	void Activate()
	{
		if (m_bTriggerOnce && m_bHasTriggered)
			return;

		m_bHasTriggered = true;
		MCF_Core_Log.Debug("TriggerZone ACTIVATED, publishing " + m_sEventName);
		MCF_Core_EventManager.GetInstance().Publish(m_sEventName, this);
	}

	bool HasTriggered()
	{
		return m_bHasTriggered;
	}
}
