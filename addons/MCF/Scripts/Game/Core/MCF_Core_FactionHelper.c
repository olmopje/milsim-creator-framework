//! Faction Alias helper (ARCHITECTURE.md 3.1) -- any MCF module that needs
//! a faction reference should resolve it through here instead of hardcoding
//! a faction key like "US" or "USSR" directly, so a scenario stays reusable
//! with different faction combinations without a rebuild.
//!
//! Wraps the confirmed native SCR_FactionAliasComponent.ResolveFactionAlias().
//! That component is expected to live on the GameMode entity (or be
//! overridden via the mission header) -- if none is present, ResolveAlias()
//! returns the input unchanged, same fallback behavior as the native method.

class MCF_Core_FactionHelper
{
	//! Resolves aliasKey to its actual FactionKey via the scenario's
	//! SCR_FactionAliasComponent, if one exists. Returns aliasKey
	//! unchanged if no alias component is found or no alias is configured
	//! for that key.
	static FactionKey ResolveAlias(FactionKey aliasKey)
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return aliasKey;

		SCR_FactionAliasComponent aliasComponent = SCR_FactionAliasComponent.Cast(gameMode.FindComponent(SCR_FactionAliasComponent));
		if (!aliasComponent)
			return aliasKey;

		return aliasComponent.ResolveFactionAlias(aliasKey);
	}
}
