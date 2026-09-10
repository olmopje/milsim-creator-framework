//! The interaction on an intel object: read the letter, use the phone.
//!
//! Reading is a world action rather than an inventory one, deliberately. It
//! avoids inventory integration entirely, and it plays better: you kneel next
//! to the body and go through the papers where they lie. The object can still
//! be picked up and carried like any item -- that is the prefab's business,
//! not this action's.
//!
//! ONE PROMPT, NOT TWO. A locked device used to hide this action and offer
//! "Break into" instead, so you stood in front of a phone choosing between two
//! prompts, one of which only existed because the other was refused. Now the
//! screen always opens and a shut device shows a lock screen, which is what a
//! shut device does. Everything about the lock is decided inside the shell.
//!
//! Local effect only. Opening a screen affects nobody else, and nothing here
//! changes shared state. Logging the intel to the operations board is a
//! separate, server-validated step that happens from inside the viewer, and so
//! is answering a lock challenge.

class MCF_Intel_ReadAction : ScriptedUserAction
{
	protected MCF_Intel_CarrierComponent m_Carrier;

	// The lid, on devices that have one. Resolved lazily rather than in Init:
	// the child entity is not reliably attached yet when the parent's actions
	// initialise, and a null cached there would be cached forever.
	protected DoorComponent m_Lid;
	protected bool m_bLidSearched;

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
	//!
	//! A locked device is NOT empty. It has content the player cannot reach
	//! yet, and hiding the prompt would make a phone worth breaking into look
	//! like a phone worth ignoring.
	//! Hidden while the lid is shut, so you cannot reach through a closed
	//! laptop to use the screen inside it. A device with no lid is never
	//! considered shut, which keeps every existing prop behaving as before.
	protected DoorComponent FindLid()
	{
		if (m_bLidSearched)
			return m_Lid;

		m_bLidSearched = true;

		IEntity owner = GetOwner();
		if (!owner)
		{
			// Nothing to search yet -- try again next time rather than
			// remembering a "no" that was only ever "not yet".
			m_bLidSearched = false;
			return null;
		}

		IEntity child = owner.GetChildren();
		while (child)
		{
			DoorComponent door = DoorComponent.Cast(child.FindComponent(DoorComponent));
			if (door)
			{
				m_Lid = door;
				return m_Lid;
			}

			child = child.GetSibling();
		}

		return null;
	}

	protected bool IsLidClosed()
	{
		DoorComponent lid = FindLid();
		if (!lid)
			return false;

		// Same test the vanilla door action uses to decide between "Open" and
		// "Close": half open counts as open.
		return Math.AbsFloat(lid.GetControlValue()) < 0.5;
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Carrier || !m_Carrier.HasContent())
			return false;

		return !IsLidClosed();
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

		// A flat viewer has nowhere to put a lock screen, so a locked object
		// without a shell stays shut rather than spilling its contents.
		if (MCF_Devices_LockComponent.IsEntityLocked(pOwnerEntity))
		{
			MCF_Core_Log.Debug("intel object is locked and has no shell to show it in -- nothing opened");
			return;
		}

		MCF_Intel_ViewerMenu.OpenFor(m_Carrier);
	}
}
