//! AI Command Watchdog (ARCHITECTURE.md 5.11) -- a patch for stuck AI
//! (e.g. not exiting a vehicle, follow order breaking), not a structural
//! fix. Detects an entity that hasn't moved for m_fStuckThresholdSeconds,
//! publishes a retry request first, and only after a repeat failure
//! publishes a forced-correction request.
//!
//! CheckStuck() must be called periodically -- there is no Tick Manager
//! yet, so nothing calls it automatically.
//!
//! Safe-position validation is NOT implemented. ApplyForcedCorrection()
//! trusts the position it is given; the caller is responsible for
//! confirming it is not inside geometry/water (e.g. via a navmesh query)
//! before calling. This is an explicit, documented gap, not an oversight.

[ComponentEditorProps(category: "MCF/AI", description: "Detects a stuck entity and requests a retry, then a forced correction.")]
class MCF_AI_CommandWatchdogComponentClass : ScriptComponentClass
{
}

class MCF_AI_CommandWatchdogComponent : ScriptComponent
{
	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Seconds without movement before this entity is considered stuck.")]
	protected float m_fStuckThresholdSeconds;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Minimum distance (metres) counted as \"has moved\" since the last check.")]
	protected float m_fMinMovementDistance;

	protected vector m_vLastPosition;
	protected float m_fLastMovedTime;
	protected bool m_bRetried;
	protected int m_iInterventionCount;

	override void EOnInit(IEntity owner)
	{
		m_vLastPosition = owner.GetOrigin();
		m_fLastMovedTime = GetGame().GetWorld().GetWorldTime();

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_AI_CommandRetryRequested");
		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_AI_CommandForceCorrectionRequested");
	}

	//! Call periodically for the owning entity. Compares current position
	//! against the last recorded one; if unmoved past the threshold,
	//! publishes a retry request first, then a forced-correction request
	//! if it is still stuck on the next call after that.
	void CheckStuck(IEntity owner)
	{
		vector current = owner.GetOrigin();
		float distance = vector.Distance(current, m_vLastPosition);
		float now = GetGame().GetWorld().GetWorldTime();

		if (distance >= m_fMinMovementDistance)
		{
			m_vLastPosition = current;
			m_fLastMovedTime = now;
			m_bRetried = false;
			return;
		}

		float stuckDuration = now - m_fLastMovedTime;
		if (stuckDuration < m_fStuckThresholdSeconds)
			return;

		m_iInterventionCount++;

		if (!m_bRetried)
		{
			m_bRetried = true;
			MCF_Core_EventManager.GetInstance().Publish("MCF_AI_CommandRetryRequested", this);
			return;
		}

		MCF_Core_EventManager.GetInstance().Publish("MCF_AI_CommandForceCorrectionRequested", this);
	}

	//! Moves owner to safePosition, which MUST already be validated safe
	//! by the caller -- see file header. Resets stuck tracking.
	void ApplyForcedCorrection(IEntity owner, vector safePosition)
	{
		owner.SetOrigin(safePosition);
		m_vLastPosition = safePosition;
		m_fLastMovedTime = GetGame().GetWorld().GetWorldTime();
		m_bRetried = false;
	}

	//! For the debug overlay (ARCHITECTURE.md 7.7) -- how many times this
	//! watchdog has had to intervene, so the threshold can be tuned based
	//! on real usage rather than guesswork.
	int GetInterventionCount()
	{
		return m_iInterventionCount;
	}
}
