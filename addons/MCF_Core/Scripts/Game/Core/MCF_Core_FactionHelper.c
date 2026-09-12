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

	//! \return The player's faction key, or empty if they have not picked one
	//! yet.
	//!
	//! At registration time this is normally empty: faction is chosen at
	//! spawn, which happens well after a player registers. Anything
	//! faction-scoped therefore cannot be delivered on join, and needs a
	//! re-send once the player picks a side -- which is what
	//! MCF_Core_PlayerFactionChanged is for.
	static string GetPlayerFactionKey(int playerId)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return string.Empty;

		Faction faction = factionManager.GetPlayerFaction(playerId);
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}
}
