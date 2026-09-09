//! Registry of mission-maker-placed safe fallback points, used by
//! MCF_AI_CommandWatchdogComponent instead of a geometry/navmesh query we
//! couldn't confirm without guessing. The mission maker places one near
//! known problem spots (e.g. next to a vehicle door where AI tends to
//! get stuck); the Watchdog picks the nearest registered one when it
//! needs to force-correct a stuck entity.

class MCF_AI_FallbackPointRegistry
{
	private static ref MCF_AI_FallbackPointRegistry s_Instance;

	protected ref array<IEntity> m_aPoints;

	void MCF_AI_FallbackPointRegistry()
	{
		m_aPoints = new array<IEntity>();
	}

	static MCF_AI_FallbackPointRegistry GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_AI_FallbackPointRegistry();
		return s_Instance;
	}

	void Register(IEntity point)
	{
		if (point && m_aPoints.Find(point) == -1)
			m_aPoints.Insert(point);
	}

	void Unregister(IEntity point)
	{
		int index = m_aPoints.Find(point);
		if (index != -1)
			m_aPoints.Remove(index);
	}

	//! Returns the registered point closest to fromPosition, or null if
	//! none are registered.
	IEntity FindNearest(vector fromPosition)
	{
		IEntity nearest = null;
		float nearestDistance = -1;

		foreach (IEntity point : m_aPoints)
		{
			if (!point)
				continue;

			float distance = vector.Distance(fromPosition, point.GetOrigin());
			if (nearestDistance < 0 || distance < nearestDistance)
			{
				nearestDistance = distance;
				nearest = point;
			}
		}

		return nearest;
	}

	int GetCount()
	{
		return m_aPoints.Count();
	}
}

//! Placeable marker -- put one of these near a spot known to be safe for
//! AI to stand (e.g. next to a vehicle door). Registers itself
//! automatically on placement.
[ComponentEditorProps(category: "MCF/AI", description: "Marks a location as a safe fallback point for the AI Command Watchdog to use.")]
class MCF_AI_SafeFallbackPointComponentClass : ScriptComponentClass
{
}

class MCF_AI_SafeFallbackPointComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		MCF_AI_FallbackPointRegistry.GetInstance().Register(owner);
	}

	override void OnDelete(IEntity owner)
	{
		MCF_AI_FallbackPointRegistry.GetInstance().Unregister(owner);
	}
}
