//! UserAction that triggers MCF_Interact_HintComponent.Interact() on the
//! owning entity -- gives mission makers a real "press to talk" prompt
//! for the interaction hint system (ARCHITECTURE.md 5.6) without writing
//! any script themselves. Place on the same entity as (or a child of) an
//! entity that has an MCF_Interact_HintComponent.

class MCF_Interact_TalkAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		MCF_Interact_HintComponent hint = MCF_Interact_HintComponent.Cast(pOwnerEntity.FindComponent(MCF_Interact_HintComponent));
		if (hint)
			hint.Interact();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		IEntity owner = GetOwner();
		if (!owner)
			return false;

		return owner.FindComponent(MCF_Interact_HintComponent) != null;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Talk";
		return true;
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
}
