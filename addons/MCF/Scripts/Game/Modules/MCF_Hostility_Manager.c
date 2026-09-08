//! Server-side hostility/reputation tracker, per area (ARCHITECTURE.md
//! 5.1). Areas are identified by a plain string key for now -- Faction
//! Alias integration (using an area+faction pair instead of a bare
//! string) is a follow-up, pending verification of SCR_FactionAliasComponent's
//! actual API rather than guessing at it.
//!
//! Publishes "Hostility_Changed" (payload: this) whenever a value changes,
//! so civilian behavior hooks and Objective intel gates can react via the
//! Event Bus instead of polling.
//!
//! Decay is a manually-called method for now (ApplyDecay), since there is
//! no Tick Manager yet to call it automatically on an interval.

class MCF_Hostility_Manager
{
	private static ref MCF_Hostility_Manager s_Instance;

	protected ref map<string, float> m_mHostilityByArea;
	protected string m_sLastChangedArea;

	void MCF_Hostility_Manager()
	{
		m_mHostilityByArea = new map<string, float>();
	}

	static MCF_Hostility_Manager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Hostility_Manager();
		return s_Instance;
	}

	float GetHostility(string areaKey)
	{
		if (!m_mHostilityByArea.Contains(areaKey))
			return 0;
		return m_mHostilityByArea.Get(areaKey);
	}

	//! Adds delta to the area's hostility value (positive for e.g. civilian
	//! casualties, negative for e.g. aid delivered) and publishes
	//! "Hostility_Changed". Clamped to 0-100.
	void AddImpact(string areaKey, float delta)
	{
		float current = GetHostility(areaKey);
		float updated = Math.Clamp(current + delta, 0, 100);
		m_mHostilityByArea.Set(areaKey, updated);

		m_sLastChangedArea = areaKey;
		MCF_Core_EventManager.GetInstance().Publish("Hostility_Changed", this);
	}

	//! Reduces every tracked area's hostility toward 0 by
	//! ratePerSecond * deltaTime. Call periodically -- no automatic tick
	//! yet, see file header.
	void ApplyDecay(float deltaTime, float ratePerSecond)
	{
		float amount = ratePerSecond * deltaTime;
		if (amount <= 0)
			return;

		for (int i = 0; i < m_mHostilityByArea.Count(); i++)
		{
			string areaKey = m_mHostilityByArea.GetKey(i);
			float value = m_mHostilityByArea.GetElement(i);
			if (value <= 0)
				continue;

			m_mHostilityByArea.Set(areaKey, Math.Max(0, value - amount));
		}
	}

	//! Identifies which area's AddImpact()/decay most recently changed, for
	//! listeners of "Hostility_Changed" that need to know which area to
	//! re-check rather than re-scanning everything.
	string GetLastChangedArea()
	{
		return m_sLastChangedArea;
	}
}
