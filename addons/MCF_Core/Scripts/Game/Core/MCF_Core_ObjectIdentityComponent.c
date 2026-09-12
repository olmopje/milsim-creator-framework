//! Generic tag/identity component, placeable on any entity in the World
//! Editor. Gives a mission maker's chosen entity a lookup tag: other MCF
//! nodes and modules find it via MCF_Core_TagRegistry instead of holding a
//! hard entity reference (ARCHITECTURE.md section 3, "Object Identity").
//!
//! Scope: tag registration only. Event Bus subscription cleanup belongs to
//! each module's own component, in its own EOnDeactivate.

[ComponentEditorProps(category: "MCF/Core", description: "Tag/identity component -- gives this entity a mission-maker-chosen tag so other MCF nodes can find it without a hard reference.")]
class MCF_Core_ObjectIdentityComponentClass : ScriptComponentClass
{
}

class MCF_Core_ObjectIdentityComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Unique tag used by other MCF nodes/modules to reference this entity instead of a hard entity reference. Leave empty if this entity does not need to be looked up by tag.")]
	protected string m_sTag;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (!m_sTag.IsEmpty())
			MCF_Core_TagRegistry.GetInstance().Register(m_sTag, owner);
	}

	override void OnDelete(IEntity owner)
	{
		if (!m_sTag.IsEmpty())
			MCF_Core_TagRegistry.GetInstance().Unregister(m_sTag);
	}

	string GetTag()
	{
		return m_sTag;
	}

	//! For runtime-spawned entities that need a tag assigned in script.
	//! Call before EOnInit runs (i.e. right after spawning, before the
	//! entity is added to the world).
	void SetTag(string tag)
	{
		m_sTag = tag;
	}
}
