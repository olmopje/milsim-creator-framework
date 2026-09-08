//! Central Tick Manager (ARCHITECTURE.md 3, 7.1, 7.8) -- one place that
//! accumulates elapsed time and publishes "MCF_Core_TickCritical" /
//! "MCF_Core_TickCosmetic" on the Event Bus at their configured
//! intervals. Anything needing periodic work subscribes to one of these
//! two events instead of running its own per-entity EOnFrame, which is
//! exactly the anti-pattern this component exists to avoid.
//!
//! Update() must be called every frame/update by whatever drives the
//! game loop -- there is no automatic per-frame hook wired up yet (same
//! documented gap as every other "manual driver" component in this
//! framework).

[ComponentEditorProps(category: "MCF/Core", description: "Central tick source -- publishes critical/cosmetic tick events instead of per-entity EOnFrame.")]
class MCF_Core_TickManagerComponentClass : ScriptComponentClass
{
}

class MCF_Core_TickManagerComponent : ScriptComponent
{
	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Seconds between \"MCF_Core_TickCritical\" events -- for gameplay-deciding checks (e.g. ROE compliance status).")]
	protected float m_fCriticalInterval;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Seconds between \"MCF_Core_TickCosmetic\" events -- for decor/cosmetic checks (e.g. Ambient Life animation choices).")]
	protected float m_fCosmeticInterval;

	protected float m_fCriticalAccumulator;
	protected float m_fCosmeticAccumulator;

	override void EOnInit(IEntity owner)
	{
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_Core_TickCritical");
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_Core_TickCosmetic");
	}

	//! Call every frame/update with the elapsed time since the last call.
	void Update(float deltaTime)
	{
		m_fCriticalAccumulator += deltaTime;
		if (m_fCriticalAccumulator >= m_fCriticalInterval)
		{
			m_fCriticalAccumulator = 0;
			MCF_Core_EventManager.GetInstance().Publish("MCF_Core_TickCritical", this);
		}

		m_fCosmeticAccumulator += deltaTime;
		if (m_fCosmeticAccumulator >= m_fCosmeticInterval)
		{
			m_fCosmeticAccumulator = 0;
			MCF_Core_EventManager.GetInstance().Publish("MCF_Core_TickCosmetic", this);
		}
	}
}
