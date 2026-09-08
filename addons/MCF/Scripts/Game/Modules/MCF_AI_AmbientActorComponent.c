//! Ambient actor archetype (ARCHITECTURE.md 5.5) -- roams between a fixed
//! list of Lifestyle POI tags. Advancing to the next POI is a manual call
//! for now (AdvanceToNextPOI()); actual movement and animation cycling
//! are not wired up, same pattern as the other manual-trigger nodes in
//! this framework.

[ComponentEditorProps(category: "MCF/AI", description: "Roams between a list of Lifestyle POI tags. Archetype is free text (\"Shopkeeper\", \"Customer\", \"MilitiaOffDuty\", etc).")]
class MCF_AI_AmbientActorComponentClass : ScriptComponentClass
{
}

class MCF_AI_AmbientActorComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Archetype label, e.g. \"Shopkeeper\", \"Customer\", \"MilitiaOffDuty\". Free text for now.")]
	protected string m_sArchetype;

	[Attribute(desc: "Tags of MCF_Core_ObjectIdentityComponent-tagged Lifestyle POI entities this actor cycles between.")]
	protected ref array<string> m_aPoiTags;

	protected int m_iCurrentIndex;

	override void EOnInit(IEntity owner)
	{
		m_iCurrentIndex = -1;
	}

	string GetArchetype()
	{
		return m_sArchetype;
	}

	//! Advances to the next POI tag in the list, wrapping around. Returns
	//! an empty string if no POI tags are configured.
	string AdvanceToNextPOI()
	{
		if (!m_aPoiTags || m_aPoiTags.IsEmpty())
			return "";

		m_iCurrentIndex = (m_iCurrentIndex + 1) % m_aPoiTags.Count();
		return m_aPoiTags[m_iCurrentIndex];
	}

	string GetCurrentPOITag()
	{
		if (m_iCurrentIndex < 0 || !m_aPoiTags || m_aPoiTags.IsEmpty())
			return "";

		return m_aPoiTags[m_iCurrentIndex];
	}
}
