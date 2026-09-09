//! Squad Cohesion / C2 layer (ARCHITECTURE.md 5.10). Addresses the
//! "squad membership has no meaning" complaint from
//! docs/research/mission-maker-pain-points.md.
//!
//! Every feature has its own on/off attribute -- the mission maker
//! decides which parts are active per scenario. All three default to
//! off, so nothing is imposed on a scenario that doesn't ask for it.
//!
//! Scope: this component holds squad membership and the muster check.
//! The map layer and radio-respawn hint are data/event only -- they
//! expose state for a UI to read, they do not draw any UI themselves.
//! Building the actual widgets is separate UI work.

[ComponentEditorProps(category: "MCF/Squad", description: "Squad cohesion features -- position sharing, muster gate, radio respawn hint. Each toggleable.")]
class MCF_Squad_CohesionComponentClass : ScriptComponentClass
{
}

class MCF_Squad_CohesionComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Squad identifier. Members of the same squad share this key.")]
	protected string m_sSquadKey;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enable squad position sharing (members visible to each other only, never the whole army).")]
	protected bool m_bEnablePositionSharing;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enable the muster gate -- IsSquadMustered() checks whether members are gathered within the muster radius.")]
	protected bool m_bEnableMusterGate;

	[Attribute(defvalue: "50", uiwidget: UIWidgets.EditBox, desc: "Muster radius in metres. All members must be within this distance of the muster point.")]
	protected float m_fMusterRadius;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enable the radio respawn hint -- publishes an event a UI can show, making the existing but little-known mechanic visible.")]
	protected bool m_bEnableRadioRespawnHint;

	protected ref array<IEntity> m_aMembers;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_aMembers = new array<IEntity>();

		if (m_bEnableRadioRespawnHint)
			MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher("MCF_Squad_RadioRespawnHint");
	}

	void RegisterMember(IEntity member)
	{
		if (member && m_aMembers.Find(member) == -1)
			m_aMembers.Insert(member);
	}

	void UnregisterMember(IEntity member)
	{
		int index = m_aMembers.Find(member);
		if (index != -1)
			m_aMembers.Remove(index);
	}

	//! Returns the squad's member entities, or an empty array if position
	//! sharing is disabled. A map UI reads this to draw markers -- it is
	//! deliberately squad-scoped, never army-wide.
	array<IEntity> GetVisibleMembers()
	{
		if (!m_bEnablePositionSharing)
			return new array<IEntity>();

		return m_aMembers;
	}

	//! Returns true if every registered member is within m_fMusterRadius
	//! of musterPoint. Returns true when the muster gate is disabled, so
	//! it never blocks a scenario that doesn't use it.
	bool IsSquadMustered(vector musterPoint)
	{
		if (!m_bEnableMusterGate)
			return true;

		if (m_aMembers.IsEmpty())
			return false;

		foreach (IEntity member : m_aMembers)
		{
			if (!member)
				continue;

			float distance = vector.Distance(member.GetOrigin(), musterPoint);
			if (distance > m_fMusterRadius)
				return false;
		}

		return true;
	}

	//! Publishes the radio respawn hint for a UI to display. Does nothing
	//! if the hint is disabled.
	void PublishRadioRespawnHint()
	{
		if (!m_bEnableRadioRespawnHint)
			return;

		MCF_Core_EventManager.GetInstance().Publish("MCF_Squad_RadioRespawnHint", this);
	}

	string GetSquadKey()
	{
		return m_sSquadKey;
	}
}
