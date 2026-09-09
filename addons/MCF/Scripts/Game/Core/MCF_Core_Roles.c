//! Who is in the chain of command.
//!
//! WHY THIS IS CORE AND NOT OPS. This started life inside
//! MCF_Task_Permissions, which made it look like a property of the operations
//! board. It is not. It reads command standing out of vanilla -- Game Master
//! rights, faction commander, group leader -- and answers a question any
//! module can have: is this player running the mission, or fighting in it?
//! The dialogue module asks it before letting somebody write a conversation,
//! and it should not have to depend on the task board to do so.
//!
//! What stayed behind in MCF_Task_Permissions is the part that really is about
//! tasks: which of READ/ACCEPT/EDIT/CREATE/PUBLISH each tier may do.
//!
//! WHERE ROLES COME FROM, in order of preference:
//!
//!   1. Derived from vanilla -- implemented below. Needs no setup.
//!   2. MCF's own Game Master slotting -- fixed units, roles and loadouts a
//!      player picks at join, with the GM marking which squads carry command
//!      rights. Not built.
//!   3. Unit-defined ranks, so a unit can add tiers of its own instead of
//!      living with three.
//!
//! THE ENFORCEMENT POINT IS THE SERVER. A UI may call this to grey a button
//! out, which is a courtesy to the player. Every request handler calls it
//! again before changing anything, which is the part that matters. A client
//! that lies about its role gets nowhere, because the server never asks the
//! client what its role is -- it resolves it from the player id the RPC
//! arrived with.
//!
//! See docs/research/command-center-design.md.

//! Coarse command tiers. Deliberately few: these are about *authority over
//! the plan*, not about job (a medic and a rifleman have the same authority).
//!
//! WARNING: if these values are ever persisted, they become append-only for
//! the same reason MCF_ETaskState did. They are not persisted today.
enum MCF_ERole
{
	SOLDIER,
	SQUAD_LEADER,
	COMMANDER
}

class MCF_Core_Roles
{
	private static ref MCF_Core_Roles s_Instance;

	static MCF_Core_Roles GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Core_Roles();
		return s_Instance;
	}

	//! What tier this player holds, read out of vanilla's own command state.
	//!
	//! ORDER MATTERS. A Game Master outranks everything, because someone
	//! running the mission has to be able to fix things when the in-fiction
	//! chain of command has been shot. Faction commander next, then group
	//! leader. A player who is both is the commander.
	MCF_ERole ResolveRole(int playerId)
	{
		if (playerId <= 0)
			return MCF_ERole.SOLDIER;

		if (HasGameMasterRights(playerId))
			return MCF_ERole.COMMANDER;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
		{
			SCR_Faction faction = SCR_Faction.Cast(factionManager.GetPlayerFaction(playerId));

			// SCR_Faction.AI_COMMANDER_ID is 0, which IsPlayerCommander
			// already excludes for any real player id -- but a player id of 0
			// would otherwise read as "the AI commander is me", so the guard
			// at the top of this method is load-bearing.
			if (faction && faction.IsPlayerCommander(playerId))
				return MCF_ERole.COMMANDER;
		}

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (groups)
		{
			SCR_AIGroup group = groups.GetPlayerGroup(playerId);
			if (group && group.IsPlayerLeader(playerId))
				return MCF_ERole.SQUAD_LEADER;
		}

		return MCF_ERole.SOLDIER;
	}

	//! Whether this player is running the mission rather than fighting in it.
	//!
	//! The per-player editor map lives on the server only -- SCR_EditorManagerCore
	//! early-returns out of creating managers on a client. A client therefore
	//! can only answer this about itself, which is all a client needs it for:
	//! deciding whether to grey out its own buttons. The server, which is the
	//! side that actually enforces anything, has the full map.
	bool HasGameMasterRights(int playerId)
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager(playerId);

		if (!editorManager && playerId == SCR_PlayerController.GetLocalPlayerId())
			editorManager = core.GetEditorManager();

		if (!editorManager)
			return false;

		// IsLimited() means the player has an editor but none of the modes
		// that carry real Game Master powers -- a spectator, say. Note this
		// asks about rights, not about whether the editor is open right now:
		// a commander who closed the GM screen to walk to the board keeps
		// their authority.
		return !editorManager.IsLimited();
	}

	//! For display. A screen shows this so a player can see what the system
	//! thinks they are, which is how a wrong answer gets noticed early.
	static string RoleLabel(MCF_ERole role)
	{
		switch (role)
		{
			case MCF_ERole.COMMANDER:    return "COMMAND";
			case MCF_ERole.SQUAD_LEADER: return "SECTION COMD";
		}

		return "SOLDIER";
	}
}
