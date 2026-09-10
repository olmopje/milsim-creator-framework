//! Game Master controls for a lockable device.
//!
//! THE CONSTRAINT, AGAIN. SCR_BaseEditorAttributeVar packs everything into one
//! vector and replicates twelve bytes: CreateInt, CreateFloat, CreateBool,
//! CreateVector, and nothing else. Both of these are therefore numbers, which
//! is fine -- neither wants to be a sentence.
//!
//! A slider-backed attribute writes a FLOAT. CreateInt round-trips as 0 here,
//! which is a silent zero rather than an error, so difficulty reads and writes
//! as a float and rounds on the way in. This is the same shape as
//! MCF_RestraintPoseEditorAttribute in the subdue module; copy that if a third
//! one is ever needed.
//!
//! These classes are registered in CORE's Configs/Editor/MCF_EditorAttributes.conf,
//! which is a manifest listing every module's attributes. With this module
//! absent the parser cannot resolve the class and skips the entry, which is the
//! intended outcome.

//! Whether the device is secured at all.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_Devices_SecuredEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_Devices_LockComponent lock = GetLock(item);
		if (!lock)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(lock.IsSecured());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_Devices_LockComponent lock = GetLock(item);
		if (!lock)
			return;

		lock.SetSecuredFromEditor(var.GetBool());
	}

	protected MCF_Devices_LockComponent GetLock(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_Devices_LockComponent.Cast(owner.FindComponent(MCF_Devices_LockComponent));
	}
}

//! How hard the break-in is. 0 is three symbols and twelve seconds, 4 is seven
//! and five -- see MCF_Devices_Challenge.c for the exact curve.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_Devices_DifficultyEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_Devices_LockComponent lock = GetLock(item);
		if (!lock)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(lock.GetDifficulty());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_Devices_LockComponent lock = GetLock(item);
		if (!lock)
			return;

		lock.SetDifficultyFromEditor(Math.Round(var.GetFloat()));
	}

	protected MCF_Devices_LockComponent GetLock(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_Devices_LockComponent.Cast(owner.FindComponent(MCF_Devices_LockComponent));
	}
}
