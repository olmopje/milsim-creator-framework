//! Configurable budgets (ARCHITECTURE.md 7.5) -- caps the number of
//! active instances per named category (e.g. "AIGroup", "InfraNode"), so
//! an enthusiastic mission maker cannot bring the server to its knees.
//! Categories are free-form strings, not a fixed enum, so new categories
//! don't need a code change.

class MCF_Core_BudgetManager
{
	private static ref MCF_Core_BudgetManager s_Instance;

	protected ref map<string, int> m_mMaxByCategory;
	protected ref map<string, int> m_mCurrentByCategory;

	void MCF_Core_BudgetManager()
	{
		m_mMaxByCategory = new map<string, int>();
		m_mCurrentByCategory = new map<string, int>();
	}

	static MCF_Core_BudgetManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_BudgetManager();
		return s_Instance;
	}

	//! Sets (or overwrites) the maximum for category. A category with no
	//! configured limit is treated as unlimited by TryReserve().
	void SetLimit(string category, int max)
	{
		m_mMaxByCategory.Set(category, max);
	}

	//! Attempts to reserve one slot in category. Returns false if the
	//! category is at its configured limit; always succeeds for a
	//! category with no configured limit.
	bool TryReserve(string category)
	{
		if (!m_mMaxByCategory.Contains(category))
			return true;

		int current = 0;
		if (m_mCurrentByCategory.Contains(category))
			current = m_mCurrentByCategory.Get(category);

		if (current >= m_mMaxByCategory.Get(category))
			return false;

		m_mCurrentByCategory.Set(category, current + 1);
		return true;
	}

	//! Releases one previously reserved slot in category.
	void Release(string category)
	{
		if (!m_mCurrentByCategory.Contains(category))
			return;

		int current = m_mCurrentByCategory.Get(category);
		if (current > 0)
			m_mCurrentByCategory.Set(category, current - 1);
	}

	int GetCurrent(string category)
	{
		if (!m_mCurrentByCategory.Contains(category))
			return 0;
		return m_mCurrentByCategory.Get(category);
	}

	int GetLimit(string category)
	{
		if (!m_mMaxByCategory.Contains(category))
			return -1; // -1 = unlimited
		return m_mMaxByCategory.Get(category);
	}
}

//! Placeable config node -- lets a mission maker set budget limits
//! without touching code. Entries are "category:max" strings, same
//! encoding style as MCF_React_RecipeComponent's steps.
[ComponentEditorProps(category: "MCF/Core", description: "Sets budget limits per category (e.g. \"AIGroup:20\").")]
class MCF_Core_BudgetConfigComponentClass : ScriptComponentClass
{
}

class MCF_Core_BudgetConfigComponent : ScriptComponent
{
	[Attribute(desc: "Budget entries, formatted as \"category:max\", e.g. \"AIGroup:20\".")]
	protected ref array<string> m_aLimits;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (!m_aLimits)
			return;

		foreach (string entry : m_aLimits)
		{
			array<string> parts = new array<string>();
			entry.Split(":", parts, false);
			if (parts.Count() < 2)
				continue;

			MCF_Core_BudgetManager.GetInstance().SetLimit(parts[0], parts[1].ToInt());
		}
	}
}
