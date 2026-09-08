// MCF_Core_ObjectIdentityComponent
//
// Generic tag/identity component, placeable on any entity in the World
// Editor. This is the Eden-init-box equivalent described in ARCHITECTURE.md
// section 3 ("Object Identity"): a mission maker gives an entity a tag
// string, and other MCF nodes/modules look that entity up via
// MCF_Core_TagRegistry instead of holding a hard entity reference. Keeps
// scenarios reusable and keeps modules decoupled from each other.
//
// SCOPE NOTE, corrected from an earlier design draft: this component does
// NOT do generic Event Bus subscription cleanup. An earlier plan tried to
// centralise that here, but Enforce Script has no generic "function
// reference" parameter type to make that work cleanly (see the design note
// in MCF_Core_EventManager.c). Event Bus cleanup instead belongs on each
// module's OWN component, in its OWN EOnDeactivate, calling
// MCF_Core_EventManager.GetInstance().GetInvoker(eventName).Remove(itsOwnMethod)
// directly -- every ScriptComponent already gets that hook for free, so
// there was never a need to route it through here. This component's only
// job is the tag registry.
//
// NOT YET TESTED end-to-end in Workbench.

[ComponentEditorProps(category: "MCF/Core", description: "Tag/identity component -- gives this entity a mission-maker-chosen tag so other MCF nodes can find it without a hard reference.")]
class MCF_Core_ObjectIdentityComponentClass : ScriptComponentClass
{
}

class MCF_Core_ObjectIdentityComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Unique tag used by other MCF nodes/modules to reference this entity instead of a hard entity reference. Leave empty if this entity does not need to be looked up by tag.")]
	protected string m_sTag;

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

	//! Read-only accessor -- the tag is set by the mission maker in the
	//! editor (or, for entities spawned at runtime, via SetTag() below
	//! before the entity is registered anywhere).
	string GetTag()
	{
		return m_sTag;
	}

	//! For runtime-spawned entities that need a tag assigned in script
	//! rather than authored in the World Editor. Does NOT re-register with
	//! MCF_Core_TagRegistry automatically if the entity is already
	//! initialized -- call this before EOnInit runs (i.e. right after
	//! spawning, before the entity is added to the world), or handle
	//! registration manually if changing an already-active entity's tag.
	void SetTag(string tag)
	{
		m_sTag = tag;
	}
}
