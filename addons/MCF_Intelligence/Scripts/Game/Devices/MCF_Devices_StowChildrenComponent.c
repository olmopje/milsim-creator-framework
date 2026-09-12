// Hides an item's child entities while the item is inside an inventory.
//
// WHY THIS EXISTS. The inventory hides a stored item by calling HideOwner()
// and ActivateOwner(false) on the ITEM ENTITY -- see
// SCR_UniversalInventoryStorageComponent.OnAddedToSlot. Hierarchy children are
// separate entities carrying their own MeshObject and their own visibility
// flag, and nothing in that path touches them. So the laptop's lid stayed
// drawn after the body vanished, and since a stored item is parked at the
// carrier's origin the lid appeared to float around the player's feet. The
// phone has no child entities, which is the whole reason it never showed the
// fault.
//
// InventoryItemComponent.m_OnParentSlotChangedInvoker fires whenever the item
// enters or leaves a storage slot; SCR_PlaceableInventoryItemComponent is the
// vanilla user of it. A non-null GetParent() afterwards means the item is
// inside something, which is exactly when it must not be drawn.
[EntityEditorProps(category: "MCF/Devices", description: "Hides child entities while the item is stored in an inventory.")]
class MCF_Devices_StowChildrenComponentClass : ScriptComponentClass
{
}

class MCF_Devices_StowChildrenComponent : ScriptComponent
{
	protected bool m_bHidden;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		InventoryItemComponent item = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (!item)
			return;

		item.m_OnParentSlotChangedInvoker.Insert(OnParentSlotChanged);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		InventoryItemComponent item = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (item)
			item.m_OnParentSlotChangedInvoker.Remove(OnParentSlotChanged);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnParentSlotChanged()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		SetChildrenHidden(owner.GetParent() != null);
	}

	//------------------------------------------------------------------------------------------------
	//! Mirrors the root's stowed state onto every child in the hierarchy.
	protected void SetChildrenHidden(bool hidden)
	{
		if (hidden == m_bHidden)
			return;

		m_bHidden = hidden;

		IEntity child = GetOwner().GetChildren();
		while (child)
		{
			if (hidden)
				child.ClearFlags(EntityFlags.VISIBLE, true);
			else
				child.SetFlags(EntityFlags.VISIBLE, true);

			child = child.GetSibling();
		}
	}
}
