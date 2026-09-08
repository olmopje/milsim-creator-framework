// MCF_Core_TagRegistry
//
// Lightweight tag -> IEntity lookup used by MCF_Core_ObjectIdentityComponent.
// This is the "resolver" side of the Eden-init-box equivalent described in
// ARCHITECTURE.md section 3: modules find entities by a mission-maker-chosen
// tag string instead of hard entity references, which keeps scenarios
// reusable and keeps modules decoupled from each other.
//
// NOT YET TESTED end-to-end in Workbench.

class MCF_Core_TagRegistry
{
	private static ref MCF_Core_TagRegistry s_Instance;

	protected ref map<string, IEntity> m_mTaggedEntities;

	void MCF_Core_TagRegistry()
	{
		m_mTaggedEntities = new map<string, IEntity>();
	}

	static MCF_Core_TagRegistry GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_TagRegistry();
		return s_Instance;
	}

	//! Registers entity under tag. Overwrites silently if the tag is already
	//! in use -- duplicate tags are a scenario-authoring mistake, not
	//! something this registry tries to prevent. A future validation pass
	//! (ARCHITECTURE.md 3.1, "Validatie-pass bij missie-init") is the right
	//! place to warn about that, not this low-level registry.
	void Register(string tag, IEntity entity)
	{
		if (tag.IsEmpty() || !entity)
			return;

		m_mTaggedEntities.Set(tag, entity);
	}

	//! Removes tag from the registry, if present. Safe to call on a tag that
	//! was never registered or was already removed.
	void Unregister(string tag)
	{
		if (tag.IsEmpty())
			return;

		m_mTaggedEntities.Remove(tag);
	}

	//! Returns the entity registered under tag, or null if no entity is
	//! currently registered under that tag (never registered, or its owner
	//! already unregistered it, e.g. on deactivation).
	IEntity GetByTag(string tag)
	{
		return m_mTaggedEntities.Get(tag);
	}

	bool HasTag(string tag)
	{
		return m_mTaggedEntities.Contains(tag);
	}

	//! Diagnostic helper for the debug overlay (ARCHITECTURE.md section 7.7).
	int GetRegisteredCount()
	{
		return m_mTaggedEntities.Count();
	}
}
