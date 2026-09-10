//! The interaction on an intel object: read the letter, search the phone.
//!
//! Reading is a world action rather than an inventory one, deliberately. It
//! avoids inventory integration entirely, and it plays better: you kneel next
//! to the body and go through the papers where they lie. The object can still
//! be picked up and carried like any item -- that is the prefab's business,
//! not this action's.
//!
//! Local effect only. Opening a screen affects nobody else, and nothing here
//! changes shared state. Logging the intel to the operations board is a
//! separate, server-validated step that happens from inside the viewer.

class MCF_Intel_ReadAction : ScriptedUserAction
{
	protected MCF_Intel_CarrierComponent m_Carrier;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Carrier = MCF_Intel_CarrierComponent.Cast(pOwnerEntity.FindComponent(MCF_Intel_CarrierComponent));

		// Logged because "the prompt does not appear" has two entirely
		// different causes -- never registered on the entity, or registered
		// and the UI declining to show it -- and without this line there is no
		// way to tell them apart.
		if (m_Carrier)
			MCF_Core_Log.Debug("intel action registered on '" + m_Carrier.GetDeviceName() + "'");
		else
			MCF_Core_Log.Warn("MCF_Intel_ReadAction sits on an entity with no MCF_Intel_CarrierComponent -- it will never show");
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//! Hidden rather than greyed when the object holds nothing. An empty
	//! prompt on a prop is worse than no prompt: it promises something.
	override bool CanBeShownScript(IEntity user)
	{
		return m_Carrier && m_Carrier.HasContent();
	}

	override bool GetActionNameScript(out string outName)
	{
		if (!m_Carrier)
			return false;

		outName = m_Carrier.GetActionVerb() + ": " + m_Carrier.GetDeviceName();
		return true;
	}

	//! Opens whichever skin the object's view asks for, and falls back to the
	//! plain viewer when it asks for none. DOCUMENT, DEVICE and MAP still get
	//! the flat index-and-body screen, which is the right shape for them and
	//! the honest default for anything new.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_Carrier)
			return;

		if (MCF_Intel_ShellMenu.OpenFor(m_Carrier))
			return;

		MCF_Intel_ViewerMenu.OpenFor(m_Carrier);
	}
}
