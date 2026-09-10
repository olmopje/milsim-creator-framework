//! Physically handling a subdued person: tying their hands, letting them go,
//! and telling them where to walk.
//!
//! WHAT IS NOT HERE ANY MORE. Ordering somebody to give up, and telling a
//! crowd to keep back, used to be actions on the person -- and that was the
//! mistake. Shouting goes to a room, not to whoever you happen to be hovering
//! over, and a list of orders on one civilian read as a menu rather than as a
//! man with a rifle telling people what to do. Both moved to MCF_AI_Shout,
//! behind a key. What is left here is the part that really is done to one
//! person, up close, with your hands.
//!
//! SERVER-SIDE EFFECT. All of these change state everyone can see, so none is
//! local: HasLocalEffectOnlyScript returns false and the work is guarded on
//! the server.
//! Tying the hands of somebody who has already given up.
class MCF_AI_RestrainAction : ScriptedUserAction
{
	protected MCF_AI_DispositionComponent m_Disposition;

	[Attribute(defvalue: "15", uiwidget: UIWidgets.Slider, params: "0 100 5", desc: "How much more fear being restrained adds.")]
	protected float m_fFearOnRestrain;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Disposition = MCF_AI_DispositionComponent.Cast(pOwnerEntity.FindComponent(MCF_AI_DispositionComponent));
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	//! Only on somebody who has already complied. You do not get to tie up a
	//! man who is still standing there deciding.
	override bool CanBeShownScript(IEntity user)
	{
		return m_Disposition && m_Disposition.GetCaptiveState() == MCF_ECaptiveState.COMPLIANT;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Restrain";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !m_Disposition)
			return;

		m_Disposition.SetCaptiveState(MCF_ECaptiveState.RESTRAINED);
		m_Disposition.AdjustFear(m_fFearOnRestrain);

		// Hands behind the back, if an animation has been wired up. Silent
		// no-op until then.
		MCF_AI_SubjectControlComponent control = MCF_AI_SubjectControlComponent.Cast(pOwnerEntity.FindComponent(MCF_AI_SubjectControlComponent));
		if (control)
			control.EnterRestrainedPose();
	}
}

//! Cutting somebody loose again.
class MCF_AI_ReleaseAction : ScriptedUserAction
{
	protected MCF_AI_DispositionComponent m_Disposition;

	[Attribute(defvalue: "20", uiwidget: UIWidgets.Slider, params: "0 100 5", desc: "How much trust letting somebody go buys back.")]
	protected float m_fTrustOnRelease;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Disposition = MCF_AI_DispositionComponent.Cast(pOwnerEntity.FindComponent(MCF_AI_DispositionComponent));
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_Disposition && m_Disposition.GetCaptiveState() != MCF_ECaptiveState.FREE;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Let them go";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !m_Disposition)
			return;

		m_Disposition.SetCaptiveState(MCF_ECaptiveState.FREE);

		MCF_AI_SubjectControlComponent releaseControl = MCF_AI_SubjectControlComponent.Cast(pOwnerEntity.FindComponent(MCF_AI_SubjectControlComponent));
		if (releaseControl)
		{
			releaseControl.LeaveRestrainedPose();

			// A standing order outlives nothing here: somebody let go is not
			// still being marched.
			releaseControl.ClearOrder();
		}

		// Letting somebody go is the one thing in this whole system that buys
		// trust back after taking it at gunpoint. Without it, a player who
		// subdues the wrong person has no way to make it right, and the
		// mechanic becomes a one-way door.
		m_Disposition.AdjustTrust(m_fTrustOnRelease);
	}
}

//! Base for the standing orders, so the four of them differ only in which
//! order they give and when they are offered.
class MCF_AI_SubjectOrderActionBase : ScriptedUserAction
{
	protected MCF_AI_SubjectControlComponent m_Control;
	protected MCF_AI_DispositionComponent m_Disposition;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Control = MCF_AI_SubjectControlComponent.Cast(pOwnerEntity.FindComponent(MCF_AI_SubjectControlComponent));
		m_Disposition = MCF_AI_DispositionComponent.Cast(pOwnerEntity.FindComponent(MCF_AI_DispositionComponent));
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	//! Whether this person has given up. Ordering somebody about who has not
	//! is not a thing MCF offers -- that is what the gunpoint order is for.
	protected bool IsSubdued()
	{
		return m_Disposition && m_Disposition.GetCaptiveState() >= MCF_ECaptiveState.COMPLIANT;
	}
}

//! "Come with me" -- walking behind the escort.
class MCF_AI_EscortFollowAction : MCF_AI_SubjectOrderActionBase
{
	override bool CanBeShownScript(IEntity user)
	{
		return m_Control && IsSubdued() && m_Control.GetOrder() != MCF_ESubjectOrder.FOLLOW;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Come with me";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !m_Control)
			return;

		m_Control.Order(MCF_ESubjectOrder.FOLLOW, pUserEntity);
	}
}

//! "Walk in front" -- marched ahead, which is what you do with somebody whose
//! hands are tied and whose back you would rather not turn on.
class MCF_AI_EscortLeadAction : MCF_AI_SubjectOrderActionBase
{
	override bool CanBeShownScript(IEntity user)
	{
		return m_Control && IsSubdued() && m_Control.GetOrder() != MCF_ESubjectOrder.LEAD;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Walk in front of me";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !m_Control)
			return;

		m_Control.Order(MCF_ESubjectOrder.LEAD, pUserEntity);
	}
}

//! "As you were" -- lifts whatever standing order is on somebody.
class MCF_AI_ClearOrderAction : MCF_AI_SubjectOrderActionBase
{
	override bool CanBeShownScript(IEntity user)
	{
		return m_Control && m_Control.GetOrder() != MCF_ESubjectOrder.NONE;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Wait here";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer() || !m_Control)
			return;

		m_Control.ClearOrder();
	}
}
