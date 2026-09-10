//! Hides Ops' read action while a device is still locked.
//!
//! WHY A MODDED CLASS AND NOT A CHANGE TO OPS. Ops must not know that hacking
//! exists -- that is the whole reason the lock is a separate component. But
//! something has to stop the plain "Read" prompt appearing on a phone nobody
//! has broken into yet, and Ops owns that prompt.
//!
//! `modded class` is exactly the tool for this: the behaviour lives entirely
//! in this module, and when this module is absent the block simply is not
//! there and Ops' action behaves as it always did -- which is the graceful
//! failure described in MCF_Devices_LockComponent.c.
//!
//! The device prefab therefore carries BOTH actions: this one's counterpart
//! MCF_Devices_HackAction while locked, and Ops' MCF_Intel_ReadAction after.

modded class MCF_Intel_ReadAction
{
	protected MCF_Devices_LockComponent m_MCF_Lock;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		m_MCF_Lock = MCF_Devices_LockComponent.Cast(pOwnerEntity.FindComponent(MCF_Devices_LockComponent));
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (m_MCF_Lock && m_MCF_Lock.IsLocked())
			return false;

		return super.CanBeShownScript(user);
	}
}
