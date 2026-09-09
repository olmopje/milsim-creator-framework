//! Ambient actor archetype (ARCHITECTURE.md 5.5) -- roams between a fixed
//! list of Lifestyle POI tags. AdvanceToNextPOI() now actually moves the
//! owner there via MCF_AI_SimpleMoverComponent (straight-line movement,
//! not real pathfinding) if one is present on the same entity; without
//! one, it still tracks which POI it should be heading to but doesn't
//! move.

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
	protected MCF_AI_SimpleMoverComponent m_Mover;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_iCurrentIndex = -1;
		m_Mover = MCF_AI_SimpleMoverComponent.Cast(owner.FindComponent(MCF_AI_SimpleMoverComponent));
	}

	string GetArchetype()
	{
		return m_sArchetype;
	}

	//! Advances to the next POI tag in the list, wrapping around, and
	//! moves there if a MCF_AI_SimpleMoverComponent is present. Returns
	//! an empty string if no POI tags are configured.
	string AdvanceToNextPOI()
	{
		if (!m_aPoiTags || m_aPoiTags.IsEmpty())
			return "";

		m_iCurrentIndex = (m_iCurrentIndex + 1) % m_aPoiTags.Count();
		string tag = m_aPoiTags[m_iCurrentIndex];

		if (m_Mover)
		{
			IEntity target = MCF_Core_TagRegistry.GetInstance().GetByTag(tag);
			if (target)
				m_Mover.MoveTo(target.GetOrigin());
		}

		return tag;
	}

	string GetCurrentPOITag()
	{
		if (m_iCurrentIndex < 0 || !m_aPoiTags || m_aPoiTags.IsEmpty())
			return "";

		return m_aPoiTags[m_iCurrentIndex];
	}
}
