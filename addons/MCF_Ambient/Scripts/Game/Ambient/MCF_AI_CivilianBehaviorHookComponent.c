//! Placed on civilian AI entities to classify a simple behavior profile
//! based on the area's hostility value (ARCHITECTURE.md 5.3). Does not
//! wire into an actual behavior tree/animation system yet -- that is a
//! separate, deeper AI integration. This only exposes the classification
//! (GetBehaviorProfile()) for something else to read.
//!
//! Listens to "Hostility_Changed" and re-checks its own area's value each
//! time, rather than polling.

[ComponentEditorProps(category: "MCF/AI", description: "Reads area hostility and exposes a simple behavior profile classification.")]
class MCF_AI_CivilianBehaviorHookComponentClass : ScriptComponentClass
{
}

class MCF_AI_CivilianBehaviorHookComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Area key this civilian belongs to, matching the key used with MCF_Hostility_Manager.AddImpact().")]
	protected string m_sAreaKey;

	[Attribute(defvalue: "30", uiwidget: UIWidgets.EditBox, desc: "Hostility value (0-100) above which this civilian's profile becomes \"fearful\".")]
	protected float m_fFearfulThreshold;

	[Attribute(defvalue: "70", uiwidget: UIWidgets.EditBox, desc: "Hostility value (0-100) above which this civilian's profile becomes \"hostile\".")]
	protected float m_fHostileThreshold;

	protected ScriptInvoker m_HostilityInvoker;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_HostilityInvoker = MCF_Core_EventManager.GetInstance().GetInvoker("Hostility_Changed");
		m_HostilityInvoker.Insert(OnHostilityChanged);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_HostilityInvoker)
			m_HostilityInvoker.Remove(OnHostilityChanged);
	}

	protected void OnHostilityChanged(Managed payload)
	{
		// Re-check happens on demand via GetBehaviorProfile() -- nothing
		// to cache here yet, this handler exists so future logic (e.g.
		// triggering an animation change) has a hook to react from.
	}

	//! Returns "neutral", "fearful", or "hostile" based on the current
	//! hostility value for this civilian's area.
	string GetBehaviorProfile()
	{
		float hostility = MCF_Hostility_Manager.GetInstance().GetHostility(m_sAreaKey);

		if (hostility >= m_fHostileThreshold)
			return "hostile";
		if (hostility >= m_fFearfulThreshold)
			return "fearful";
		return "neutral";
	}
}
