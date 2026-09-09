//! Who is allowed to do what with a task.
//!
//! The seam has been here since the board was built, because retrofitting one
//! later means touching every RPC handler and every button in the UI.
//! Everything that asks "may this player do this" asks here, so switching the
//! answer on is a change to this one file.
//!
//! WHAT CHANGED: ResolveRole is real now. It reads command standing out of
//! vanilla, so a mission needs no MCF-specific setup to have a chain of
//! command -- a faction commander is the commander, a group leader is the
//! squad leader, everyone else a soldier. The master switch below is still
//! permissive, deliberately: roles are resolved and logged first so they can
//! be watched being right, and only then enforced. Getting that order wrong
//! means debugging a locked-out player instead of reading a line of log.
//!
//! THE ENFORCEMENT POINT IS THE SERVER. The UI calls these to decide whether
//! to grey a button out, which is a courtesy to the player. The request
//! handlers in MCF_PlayerControllerTasks.c call them again before changing
//! anything, which is the part that actually matters. A client that lies
//! about its role gets nowhere, because the server never asks the client what
//! its role is -- it resolves it from the player id the RPC arrived with.
//!
//! WHERE ROLES COME FROM, in order of preference:
//!
//!   1. Derived from vanilla -- implemented below. Needs no setup.
//!   2. MCF's own Game Master slotting -- fixed units, roles and loadouts a
//!      player picks at join, with the GM marking which squads carry command
//!      rights. Not built.
//!   3. Unit-defined ranks, so a unit can add tiers of its own instead of
//!      living with three. This is why the permission table has to end up as
//!      data a unit can edit rather than a switch statement.
//!
//! See docs/research/command-center-design.md.

//! Coarse command tiers. Deliberately few: these are about *authority over
//! the plan*, not about job (a medic and a rifleman have the same authority).
//!
//! WARNING: if these values are ever persisted, they become append-only for
//! the same reason MCF_ETaskState did. They are not persisted today.
enum MCF_ETaskRole
{
	SOLDIER,
	SQUAD_LEADER,
	COMMANDER
}

//! What someone wants to do with a task.
enum MCF_ETaskAction
{
	//! See it on the board at all.
	READ,
	//! Take an unclaimed task for yourself.
	ACCEPT,
	//! Change its text.
	EDIT,
	//! Write a new one.
	CREATE,
	//! Put a draft on the board for the force to see.
	PUBLISH
}

class MCF_Task_Permissions
{
	private static ref MCF_Task_Permissions s_Instance;

	//! Turn this off and the role checks below start biting. Kept as one
	//! switch so the whole restriction can be tried and reverted in a session
	//! without a rebuild.
	protected bool m_bEveryoneMayDoEverything = true;

	static MCF_Task_Permissions GetInstance()
	{
		if (!s_Instance)
			s_Instance = new MCF_Task_Permissions();
		return s_Instance;
	}

	//! The single question the rest of MCF asks.
	//! \param playerId Who is asking. On the server this comes from the
	//!        controller the request arrived through, never from the wire.
	//! \return Whether they may.
	bool Can(int playerId, MCF_ETaskAction action)
	{
		if (m_bEveryoneMayDoEverything)
			return true;

		return IsAllowed(ResolveRole(playerId), action);
	}

	//! What tier this player holds, read out of vanilla's own command state.
	//!
	//! ORDER MATTERS. A Game Master outranks everything, because someone
	//! running the mission has to be able to fix the board when the in-fiction
	//! chain of command has been shot. Faction commander next, then group
	//! leader. A player who is both is the commander.
	MCF_ETaskRole ResolveRole(int playerId)
	{
		if (playerId <= 0)
			return MCF_ETaskRole.SOLDIER;

		if (HasGameMasterRights(playerId))
			return MCF_ETaskRole.COMMANDER;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
		{
			SCR_Faction faction = SCR_Faction.Cast(factionManager.GetPlayerFaction(playerId));

			// SCR_Faction.AI_COMMANDER_ID is 0, which IsPlayerCommander
			// already excludes for any real player id -- but a player id of 0
			// would otherwise read as "the AI commander is me", so the guard
			// at the top of this method is load-bearing.
			if (faction && faction.IsPlayerCommander(playerId))
				return MCF_ETaskRole.COMMANDER;
		}

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (groups)
		{
			SCR_AIGroup group = groups.GetPlayerGroup(playerId);
			if (group && group.IsPlayerLeader(playerId))
				return MCF_ETaskRole.SQUAD_LEADER;
		}

		return MCF_ETaskRole.SOLDIER;
	}

	//! Whether this player is running the mission rather than fighting in it.
	//!
	//! The per-player editor map lives on the server only -- SCR_EditorManagerCore
	//! early-returns out of creating managers on a client. A client therefore
	//! can only answer this about itself, which is all a client needs it for:
	//! deciding whether to grey out its own buttons. The server, which is the
	//! side that actually enforces anything, has the full map.
	protected bool HasGameMasterRights(int playerId)
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

	//! For display. The board shows this so a player can see what the system
	//! thinks they are, which is how a wrong answer gets noticed early.
	static string RoleLabel(MCF_ETaskRole role)
	{
		switch (role)
		{
			case MCF_ETaskRole.COMMANDER:    return "COMMAND";
			case MCF_ETaskRole.SQUAD_LEADER: return "SECTION COMD";
		}

		return "SOLDIER";
	}

	//! The table. It is a table rather than scattered checks precisely because
	//! units are meant to be able to redefine it later -- at which point this
	//! reads from a config instead of being written here, and the shape of the
	//! question does not change.
	protected bool IsAllowed(MCF_ETaskRole role, MCF_ETaskAction action)
	{
		switch (action)
		{
			// Reading the board is what a board is for. Which *tasks* a
			// player sees is a separate question, answered per task by
			// MCF_Core_TaskStore.IsVisibleTo -- this only says whether they
			// may open the thing at all.
			case MCF_ETaskAction.READ:
				return true;

			// Taking a job off the board is something any soldier does.
			case MCF_ETaskAction.ACCEPT:
				return true;

			// Writing and issuing orders is command work.
			case MCF_ETaskAction.CREATE:
			case MCF_ETaskAction.PUBLISH:
				return role >= MCF_ETaskRole.SQUAD_LEADER;

			// Editing someone else's order is the commander's business.
			// Note this is the blunt version: authorship is checked
			// separately, so an author editing their own draft is handled
			// there rather than here.
			case MCF_ETaskAction.EDIT:
				return role >= MCF_ETaskRole.COMMANDER;
		}

		return false;
	}
}
