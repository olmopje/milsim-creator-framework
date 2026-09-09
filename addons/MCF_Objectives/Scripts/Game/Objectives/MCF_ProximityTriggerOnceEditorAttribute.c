//! Editor Attribute exposing MCF_Obj_ProximityTriggerComponent's "trigger
//! once" flag in the Game Master "Edit" panel. Same pattern as
//! MCF_ProximityRadiusEditorAttribute, see that file for the general
//! explanation of how MCF hooks into the Editor Attributes system.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_ProximityTriggerOnceEditorAttribute : SCR_BaseEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		MCF_Obj_ProximityTriggerComponent comp = GetTriggerComponent(item);
		if (!comp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(comp.GetTriggerOnce());
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		MCF_Obj_ProximityTriggerComponent comp = GetTriggerComponent(item);
		if (!comp)
			return;

		comp.SetTriggerOnce(var.GetBool());
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
