//! Whether somebody does as they are told when a player shouts at them.
//!
//! THIS COMPONENT WAS UNREACHABLE FOR MOST OF ITS LIFE. It shipped early,
//! knew whether a person was armed, rolled against a compliance chance and
//! penalised the area's hostility when a player pointed a rifle at a civilian
//! for no reason -- and nothing ever called any of it. No action, no key, no
//! trigger. It was finally connected when shouting was built, and the roll it
//! had been carrying all along (AttemptCompliance) was replaced by
//! WillSurrender below rather than kept beside it: two rolls that disagree are
//! how you end up debugging the wrong one.
//!
//! EVERY TERM IS A DIAL. "Civilians give up sooner than soldiers" is a mission
//! maker's decision, not a rule of the framework, so it is expressed as
//! numbers on the person rather than as a branch in here.
//!
//! Aim detection (IsBeingAimedAt) is a distance-and-angle approximation, not a
//! line-of-sight raycast -- it will not notice a wall between the player and
//! the person. Accepted limitation, not an oversight. It is currently used by
//! nothing: the shout resolves by radius, and this is left as the hook for the
//! day someone wants "only who I am actually pointing at".

[ComponentEditorProps(category: "MCF/AI", description: "Can be forced into compliance at gunpoint (drop weapon / stand back).")]
class MCF_AI_ComplianceComponentClass : ScriptComponentClass
{
}

class MCF_AI_ComplianceComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "True if this NPC is armed (offers \"drop weapon\"); false for unarmed NPCs (offers \"stand back\").")]
	protected bool m_bArmed;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Base chance (0-1) this NPC complies when commanded.")]
	protected float m_fBaseComplianceChance;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Hostility area key to penalize on unjustified use against this NPC (e.g. \"stand back\" on a non-threatening unarmed civilian).")]
	protected string m_sHostilityAreaKey;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Hostility penalty applied to m_sHostilityAreaKey on unjustified use.")]
	protected float m_fUnjustifiedPenalty;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Maximum distance (metres) a player can be to command this NPC.")]
	protected float m_fMaxCommandDistance;

	[Attribute(defvalue: "20", uiwidget: UIWidgets.EditBox, desc: "Maximum angle (degrees) between the player's aim direction and this NPC for the player to be considered \"aiming at\" it.")]
	protected float m_fMaxAimAngleDegrees;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.Slider, params: "0 1 0.05", desc: "How much of this person's fear counts towards giving up. A frightened man surrenders sooner.")]
	protected float m_fFearWeight;

	[Attribute(defvalue: "0.3", uiwidget: UIWidgets.Slider, params: "0 1 0.05", desc: "How much less likely an ARMED person is to give up. Set high for soldiers, irrelevant for civilians.")]
	protected float m_fArmedResistance;

	[Attribute(defvalue: "0.25", uiwidget: UIWidgets.Slider, params: "0 1 0.05", desc: "How much a raised weapon adds. This is the difference between shouting and threatening.")]
	protected float m_fWeaponRaisedWeight;

	[Attribute(defvalue: "25", uiwidget: UIWidgets.Slider, params: "0 100 5", desc: "How much fear giving up puts into somebody.")]
	protected float m_fFearOnSurrender;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Whether shouting at this person without cause raises the area's hostility. Off for anybody who is fair game.")]
	protected bool m_bPunishUnjustified;

	protected bool m_bCompliant;

	float GetFearOnSurrender()
	{
		return m_fFearOnSurrender;
	}

	//! Whether this person does as they are told when somebody shouts.
	//!
	//! EVERY TERM IS A DIAL, because "civilians more likely than enemies" is a
	//! mission maker's decision and not a rule of the framework. A civilian
	//! prefab leaves m_bArmed off and keeps the resistance term at zero; a
	//! soldier turns it up and drops the base chance.
	//!
	//! FEAR CUTS BOTH WAYS and that is deliberate. A frightened man gives up
	//! sooner -- and once he has, the interrogation system finds him nearly
	//! useless, because a frightened person tells you what he thinks you want
	//! to hear. The fast way to make somebody comply is the slow way to learn
	//! anything from him.
	//!
	//! \param distance How far the shouter is.
	//! \param weaponRaised Whether the shouter meant it.
	//! \param fear The subject's current fear, 0-100.
	bool WillSurrender(float distance, bool weaponRaised, float fear)
	{
		if (m_bCompliant)
			return true;

		if (distance > m_fMaxCommandDistance)
			return false;

		float chance = m_fBaseComplianceChance;

		chance = chance + (fear * 0.01) * m_fFearWeight;

		if (weaponRaised)
			chance = chance + m_fWeaponRaisedWeight;

		if (m_bArmed)
			chance = chance - m_fArmedResistance;

		// Close range is more frightening than shouting from across a field.
		// Linear from nothing at maximum range to a tenth in their face.
		float closeness = 1 - (distance / m_fMaxCommandDistance);
		chance = chance + closeness * 0.1;

		chance = Math.Clamp(chance, 0, 1);

		bool complied = Math.RandomFloat01() < chance;

		if (complied)
		{
			m_bCompliant = true;
			MCF_Core_EventManager.GetInstance().Publish("MCF_AI_ComplianceGranted", this);
		}

		MCF_Core_Log.Debug("shout answered: chance=" + chance.ToString() + " complied=" + complied.ToString());
		return complied;
	}

	//! The cost of pointing a rifle at somebody who was not a threat.
	//!
	//! Split out from the roll because it applies whether or not they did as
	//! they were told -- the village saw you do it either way.
	void PunishIfUnjustified()
	{
		if (!m_bPunishUnjustified || m_bArmed || m_sHostilityAreaKey.IsEmpty())
			return;

		MCF_Hostility_Manager.GetInstance().AddImpact(m_sHostilityAreaKey, m_fUnjustifiedPenalty);
	}

	//! Approximates "is playerEntity aiming at this NPC" using distance
	//! plus angle between aimDirection and the direction toward this NPC --
	//! not a true line-of-sight check, see file header.
	bool IsBeingAimedAt(IEntity playerEntity, vector aimDirection)
	{
		if (!playerEntity || !GetOwner())
			return false;

		vector playerPos = playerEntity.GetOrigin();
		vector npcPos = GetOwner().GetOrigin();

		float distance = vector.Distance(playerPos, npcPos);
		if (distance > m_fMaxCommandDistance)
			return false;

		vector toNpc = npcPos - playerPos;
		toNpc.Normalize();
		vector aimNormalized = aimDirection;
		aimNormalized.Normalize();

		float dot = vector.Dot(aimNormalized, toNpc);
		float angleRadians = Math.Acos(Math.Clamp(dot, -1, 1));
		float angleDegrees = angleRadians * Math.RAD2DEG;

		return angleDegrees <= m_fMaxAimAngleDegrees;
	}


	bool IsArmed()
	{
		return m_bArmed;
	}

	bool IsCompliant()
	{
		return m_bCompliant;
	}
}
