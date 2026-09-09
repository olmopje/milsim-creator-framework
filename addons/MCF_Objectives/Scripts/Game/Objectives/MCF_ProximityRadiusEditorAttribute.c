//! Editor Attribute exposing MCF_Obj_ProximityTriggerComponent's detection
//! radius in the Game Master "Edit" panel.
//!
//! Follows the vanilla SCR_ExplosiveFuzeTimerAttribute pattern: cast the
//! edited item to SCR_EditableEntityComponent, find our component on its
//! owner entity, and read/write the value directly through its accessors.
//! Returning null from ReadVariable is how this attribute opts out for any
//! entity that doesn't have an MCF_Obj_ProximityTriggerComponent -- it is
//! never shown for those items.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_ProximityRadiusEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_Obj_ProximityTriggerComponent comp = GetTriggerComponent(item);
		if (!comp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(comp.GetRadius());
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_Obj_ProximityTriggerComponent comp = GetTriggerComponent(item);
		if (!comp)
			return;

		comp.SetRadius(var.GetFloat());
	}

	//------------------------------------------------------------------------------------------------
	protected MCF_Obj_ProximityTriggerComponent GetTriggerComponent(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		return MCF_Obj_ProximityTriggerComponent.Cast(owner.FindComponent(MCF_Obj_ProximityTriggerComponent));
	}
}
