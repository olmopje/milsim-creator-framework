//! Lifestyle POI (ARCHITECTURE.md 5.5) -- a role-tagged location with a
//! limited number of slots (how many ambient actors can occupy it at
//! once). Purely a slot-tracking data holder; movement/animation of
//! actors toward it is not wired up here.

[ComponentEditorProps(category: "MCF/AI", description: "Role-tagged location with a limited number of occupancy slots.")]
class MCF_AI_LifestylePOIComponentClass : ScriptComponentClass
{
}

class MCF_AI_LifestylePOIComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Role of this POI, e.g. \"Shop\", \"House\", \"MarketStall\", \"Checkpoint\", \"Well\". Free text for now.")]
	protected string m_sRole;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Maximum number of ambient actors that can occupy this POI at once.")]
	protected int m_iSlotCount;

	protected int m_iOccupancy;

	//! Claims a slot if one is free. Returns false if the POI is full.
	bool TryOccupy()
	{
		if (m_iOccupancy >= m_iSlotCount)
			return false;

		m_iOccupancy++;
		return true;
	}

	//! Frees a previously claimed slot.
	void Release()
	{
		if (m_iOccupancy > 0)
			m_iOccupancy--;
	}

	bool HasFreeSlot()
	{
		return m_iOccupancy < m_iSlotCount;
	}

	string GetRole()
	{
		return m_sRole;
	}
}
