//! Walking a prisoner in front of you.
//!
//! WHY NOT THE OBVIOUS THING. A rigid coupling -- hand physically on the
//! shoulder -- is not available. Character linking exists in the engine and
//! lists Character_Base.et under "Forbidden linking"; there is no script call
//! to start a link at all. The carry mods that do exist work because their
//! subject is UNCONSCIOUS: an inert body can be parented to a bone with its
//! physics switched off, and the Dragger mod's own description says the
//! animation is IK on the dragger's arm, not on the person being dragged. A
//! prisoner who is walking has to produce his own walk, so none of that
//! applies.
//!
//! WHY NOT A WAYPOINT OR A MOVE BEHAVIOUR. Both pathfind to a destination,
//! reach it, and stop. Re-issuing them every few metres is what made the first
//! attempt shuffle: arrive, stop, get told again, start. The stutter was never
//! the AI being stupid, it was being asked to arrive over and over.
//!
//! WHAT THIS DOES INSTEAD. A combat-move request -- the mechanism vanilla uses
//! for the half-second sidestep when a player walks into an AI. It is built
//! for short, repeated, immediate movement rather than for going somewhere,
//! which is exactly the shape of being marched. Each one lasts a little longer
//! than the tick that issues it, so the next arrives before the last expires
//! and the walk is continuous.
//!
//! Steering falls out of it: the target is a point a few metres in front of
//! the escort, so turning swings the target and the prisoner arcs, and
//! standing still lets him arrive and stop.
//!
//! THE BEHAVIOUR TREE IS BORROWED. Combat-move requests only execute inside a
//! behaviour that asks for them, and AvoidCharacter.bt is the vanilla tree
//! that does. Reusing it is a bet that it does nothing avoid-specific beyond
//! running the request; if it turns out to, this needs its own tree.

class MCF_AI_MarchBehavior : SCR_AIBehaviorBase
{
	protected vector m_vMarchTo;
	protected float m_fDuration;
	protected EMovementType m_eMovementType;

	void MCF_AI_MarchBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector marchTo, float duration)
	{
		m_vMarchTo = marchTo;
		m_fDuration = duration;
		m_eMovementType = EMovementType.WALK;


		// No look action: a man being marched watches where he is going, and
		// letting the look system fight for his head made him swivel.
		m_bAllowLook = false;

		// This is the flag that makes combat-move requests run at all.
		m_bUseCombatMove = true;

		m_sBehaviorTree = "{A75A34B4B237851F}AI/BehaviorTrees/Chimera/Soldier/AvoidCharacter.bt";

		// Above anything the prisoner might otherwise decide, and at player
		// level, because this is an order from a person and not the AI's own
		// judgement.
		SetPriority(SCR_AIActionBase.PRIORITY_BEHAVIOR_AVOID_CHARACTER);
	}

	//! Re-points an ALREADY RUNNING march without tearing it down.
	//!
	//! THIS IS WHAT FIXED THE STUTTER. The first version failed the behaviour
	//! and added a fresh one every tick, so twice a second the prisoner was
	//! taken off what he was doing and put back on it -- which is a stop and a
	//! start, twice a second, no matter how well the move requests overlap.
	//! The behaviour stays; only the request underneath it is renewed.
	void Retarget(vector marchTo, EMovementType movementType)
	{
		m_vMarchTo = marchTo;
		m_eMovementType = movementType;

		Issue();
	}

	override void OnActionSelected()
	{
		Issue();
	}

	protected void Issue()
	{
		if (!m_Utility)
			return;

		SCR_AICombatMoveRequest_Move request = new SCR_AICombatMoveRequest_Move();


		request.m_eReason = SCR_EAICombatMoveReason.CHARACTER_AVOIDANCE;
		request.m_vTargetPos = m_vMarchTo;
		request.m_vMovePos = m_vMarchTo;
		request.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;

		// No cover search. A prisoner being walked somewhere is not looking
		// for somewhere to hide, and asking for cover made the path wander.
		request.m_bTryFindCover = false;
		request.m_bFailIfNoCover = false;
		request.m_bCheckCoverVisibility = false;

		// Pace comes from the escort, not from a constant. A prisoner walking
		// while you jog falls behind and then sprints to catch up, which is
		// the single most obvious way this stops looking like an escort.
		request.m_eMovementType = m_eMovementType;
		request.m_fMoveDuration_s = m_fDuration;

		request.m_bAimAtTarget = false;
		request.m_bAimAtTargetEnd = false;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(m_Utility.GetOwner().FindComponent(SCR_CharacterControllerComponent));
		if (controller)
		{
			request.m_eStanceMoving = controller.GetStance();
			request.m_eStanceEnd = request.m_eStanceMoving;
		}

		m_Utility.m_CombatMoveState.ApplyNewRequest(request);
	}

	//! SAFE, not COMBAT. A prisoner walking where he is told is not in a
	//! firefight, and marking it as combat would drag the threat system in.
	override int GetCause()
	{
		return SCR_EAIBehaviorCause.SAFE;
	}
}
