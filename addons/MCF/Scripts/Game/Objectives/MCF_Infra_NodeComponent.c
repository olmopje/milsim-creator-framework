//! Infrastructure graph node (ARCHITECTURE.md 5.2). Each node depends on
//! zero or more other nodes by tag. A node is only active if it is not
//! itself sabotaged AND every dependency is active -- e.g. a comms tower
//! depending on a cable segment depending on a generator.
//!
//! All nodes publish/listen to one shared event, "MCF_Infra_NodeStatusChanged",
//! with the firing node as payload. A listener identifies which dependency
//! changed by reading the sender's own tag from the payload, which avoids
//! needing a distinct callback per dependency (Enforce Script has no
//! closures to generate those dynamically).
//!
//! Repair is supported (Repair() flips m_bSelfActive back on), but nothing
//! calls it automatically yet -- that is the AI Commando-Watchdog-style
//! follow-up mentioned in ARCHITECTURE.md 5.2 ("Repairability").

[ComponentEditorProps(category: "MCF/Infrastructure", description: "Infrastructure graph node -- active only if not sabotaged and all dependencies are active.")]
class MCF_Infra_NodeComponentClass : ScriptComponentClass
{
}

class MCF_Infra_NodeComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "This node's own tag, referenced by other nodes' Depends On list.")]
	protected string m_sOwnTag;

	[Attribute(desc: "Tags of nodes this one depends on. If any is inactive, this node becomes inactive too.")]
	protected ref array<string> m_aDependsOnTags;

	protected bool m_bSelfActive;
	protected bool m_bNodeActive;
	protected ref map<string, bool> m_mDependencyActive;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_bSelfActive = true;
		m_bNodeActive = true;
		m_mDependencyActive = new map<string, bool>();

		if (m_aDependsOnTags)
		{
			foreach (string depTag : m_aDependsOnTags)
				m_mDependencyActive.Set(depTag, true);
		}

		MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Infra_NodeStatusChanged").Insert(OnAnyNodeStatusChanged);
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_Infra_NodeStatusChanged");
	}

	override void OnDelete(IEntity owner)
	{
		MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Infra_NodeStatusChanged").Remove(OnAnyNodeStatusChanged);
	}

	protected void OnAnyNodeStatusChanged(Managed payload)
	{
		MCF_Infra_NodeComponent sender = MCF_Infra_NodeComponent.Cast(payload);
		if (!sender || sender == this)
			return;

		string senderTag = sender.GetOwnTag();
		if (!m_mDependencyActive.Contains(senderTag))
			return;

		m_mDependencyActive.Set(senderTag, sender.IsNodeActive());
		RecomputeActive();
	}

	protected void RecomputeActive()
	{
		bool dependenciesOk = true;
		for (int i = 0; i < m_mDependencyActive.Count(); i++)
		{
			if (!m_mDependencyActive.GetElement(i))
			{
				dependenciesOk = false;
				break;
			}
		}

		bool newActive = m_bSelfActive && dependenciesOk;
		if (newActive == m_bNodeActive)
			return;

		m_bNodeActive = newActive;
		MCF_Core_EventManager.GetInstance().Publish("MCF_Infra_NodeStatusChanged", this);
	}

	//! Marks this node itself as broken (e.g. destroyed, sabotaged).
	//! Propagates to dependents via the shared status event.
	void Sabotage()
	{
		if (!m_bSelfActive)
			return;

		m_bSelfActive = false;
		RecomputeActive();
	}

	//! Marks this node itself as fixed. Dependents only re-activate once
	//! this node AND all of their other dependencies are active again.
	void Repair()
	{
		if (m_bSelfActive)
			return;

		m_bSelfActive = true;
		RecomputeActive();
	}

	bool IsNodeActive()
	{
		return m_bNodeActive;
	}

	string GetOwnTag()
	{
		return m_sOwnTag;
	}
}
