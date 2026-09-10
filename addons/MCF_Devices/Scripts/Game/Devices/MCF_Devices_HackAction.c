//! The prompt on a locked device.
//!
//! Shown only while the thing is still locked; once it is open Ops' own read
//! action takes over (MCF_Devices_ReadGate.c is the other half of that swap).
//! The two never appear together, so a player sees one verb at a time.
//!
//! Unlike the read action this is NOT local-effect-only: it asks the server
//! for a puzzle, and the server has to answer to the right player.

class MCF_Devices_HackAction : ScriptedUserAction
{
	protected MCF_Devices_LockComponent m_Lock;
	protected MCF_Intel_CarrierComponent m_Carrier;
	protected RplComponent m_Rpl;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Lock = MCF_Devices_LockComponent.Cast(pOwnerEntity.FindComponent(MCF_Devices_LockComponent));
		m_Carrier = MCF_Intel_CarrierComponent.Cast(pOwnerEntity.FindComponent(MCF_Intel_CarrierComponent));
		m_Rpl = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));

		// Same reason the read action logs: "no prompt appeared" has several
		// causes and they are indistinguishable without a line here.
		if (!m_Lock)
			MCF_Core_Log.Warn("MCF_Devices_HackAction sits on an entity with no MCF_Devices_LockComponent -- it will never show");
		else if (!m_Rpl)
			MCF_Core_Log.Warn("a lockable device has no RplComponent -- the server cannot be told which one it is");
		else
			MCF_Core_Log.Debug("device break-in action registered");
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_Lock && m_Rpl && m_Lock.IsLocked();
	}

	override bool GetActionNameScript(out string outName)
	{
		if (!m_Lock)
			return false;

		string what = "device";
		if (m_Carrier)
			what = m_Carrier.GetDeviceName();

		outName = m_Lock.GetBreakInVerb() + ": " + what;
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_Lock || !m_Rpl)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		controller.MCF_RequestDeviceChallenge(m_Rpl.Id());
	}
}
