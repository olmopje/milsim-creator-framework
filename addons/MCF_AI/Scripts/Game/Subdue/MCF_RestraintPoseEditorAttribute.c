//! Game Master attribute for the restrained pose.
//!
//! Lives with the subdue module rather than with the dialogue attributes it
//! used to share a file with: the pose belongs to
//! MCF_AI_SubjectControlComponent, and a module must not name a class from a
//! module it does not depend on. Dialogue depends on AI, not on Subdue.
//!
//! It is registered in Configs/Editor/MCF_EditorAttributes.conf like the rest.
//! With the subdue module absent the entry names a class the parser cannot
//! resolve and is skipped, which is the intended outcome -- see
//! docs/architecture/STRUCTURE.md.

//! Which clip in the restrained-pose graph plays.
//!
//! EXISTS ONLY TO BE HUNTED WITH. The narrative graph holds a hundred
//! variants and nothing anywhere says which number is which -- the clips are
//! binary and the graph is binary. So the number is a slider, and finding the
//! right pose is a matter of restraining somebody and dialling until it looks
//! right, which takes a minute and no recompiles.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_RestraintPoseEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_AI_SubjectControlComponent control = GetControl(item);
		if (!control)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(control.GetPoseVariant());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_AI_SubjectControlComponent control = GetControl(item);
		if (!control)
			return;

		control.SetPoseVariant(Math.Round(var.GetFloat()));
	}

	protected MCF_AI_SubjectControlComponent GetControl(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_AI_SubjectControlComponent.Cast(owner.FindComponent(MCF_AI_SubjectControlComponent));
	}
}
