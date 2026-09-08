//! Tag -> IEntity lookup used by MCF_Core_ObjectIdentityComponent.
//!
//! Lets modules find entities by a mission-maker-chosen tag string instead
//! of holding hard entity references, keeping scenarios reusable and
//! modules decoupled from each other (ARCHITECTURE.md section 3).

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

	//! Registers entity under tag. Overwrites silently if the tag is
	//! already in use.
	void Register(string tag, IEntity entity)
	{
		if (tag.IsEmpty() || !entity)
			return;

		m_mTaggedEntities.Set(tag, entity);
	}

	//! Removes tag from the registry. Safe to call on a tag that was
	//! never registered.
	void Unregister(string tag)
	{
		if (tag.IsEmpty())
			return;

		m_mTaggedEntities.Remove(tag);
	}

	//! Returns the entity registered under tag, or null if none.
	IEntity GetByTag(string tag)
	{
		return m_mTaggedEntities.Get(tag);
	}

	bool HasTag(string tag)
	{
		return m_mTaggedEntities.Contains(tag);
	}

	//! Diagnostic helper for the debug overlay (ARCHITECTURE.md section 7).
	int GetRegisteredCount()
	{
		return m_mTaggedEntities.Count();
	}
}
