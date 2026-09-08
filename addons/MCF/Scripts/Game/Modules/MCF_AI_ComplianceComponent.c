//! Compliance/ROE node (ARCHITECTURE.md 5.7). Represents an NPC that can
//! be forced into compliance at gunpoint: "drop weapon" (armed NPCs) or
//! "stand back" (unarmed NPCs).
//!
//! Aim detection (IsBeingAimedAt) is a self-built distance+angle
//! approximation, not a true line-of-sight raycast -- we could not
//! confirm a raycast API without guessing, so this checks "is the NPC
//! within range and within a cone in front of the player" using plain
//! vector math instead. It will not detect a wall between player and
//! NPC; that is an accepted limitation, not an oversight.
//!
//! Compliance chance and consequence hooks are simplified: base chance is
//! an attribute, no morale/threat-state modifiers yet (ARCHITECTURE.md
//! 5.3's threat-state concept isn't built). Consequence coupling to the
//! Hostility manager covers the two documented cases: correct use is a
//! no-op (neutral), incorrect use against an unarmed non-threat is a
//! hostility penalty.

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

	protected bool m_bCompliant;

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

	//! Attempts to force this NPC into compliance. isJustified reflects
	//! whether the NPC was an actual threat (drop weapon on an armed
	//! hostile) vs not (stand back on a non-threatening civilian) -- the
	//! caller decides that; this component only applies the consequence.
	//! Returns true if the NPC complied.
	bool AttemptCompliance(bool isJustified)
	{
		if (m_bCompliant)
			return true;

		bool complied = Math.RandomFloat01() < m_fBaseComplianceChance;
		if (complied)
		{
			m_bCompliant = true;
			MCF_Core_EventManager.GetInstance().Publish("MCF_AI_ComplianceGranted", this);
		}

		if (!isJustified && !m_sHostilityAreaKey.IsEmpty())
			MCF_Hostility_Manager.GetInstance().AddImpact(m_sHostilityAreaKey, m_fUnjustifiedPenalty);

		return complied;
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
