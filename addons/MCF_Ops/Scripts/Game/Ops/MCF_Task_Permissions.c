//! Who is allowed to do what with a task.
//!
//! The seam has been here since the board was built, because retrofitting one
//! later means touching every RPC handler and every button in the UI.
//! Everything that asks "may this player do this" asks here, so switching the
//! answer on is a change to this one file.
//!
//! WHAT LIVES WHERE. Resolving a player's command tier is not a task question
//! -- the dialogue module needs the same answer before letting somebody write
//! a conversation -- so it moved to MCF_Core_Roles in Core. What is left here
//! is the part that really is about tasks: the table saying which tier may do
//! which of READ/ACCEPT/EDIT/CREATE/PUBLISH.
//!
//! The master switch below is still permissive, deliberately: roles are
//! resolved and logged first so they can be watched being right, and only then
//! enforced. Getting that order wrong means debugging a locked-out player
//! instead of reading a line of log.
//!
//! THE ENFORCEMENT POINT IS THE SERVER. The UI calls these to decide whether
//! to grey a button out, which is a courtesy to the player. The request
//! handlers in MCF_PlayerController_Ops.c call them again before changing
//! anything, which is the part that actually matters.

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

	//! The single question the board asks.
	//! \param playerId Who is asking. On the server this comes from the
	//!        controller the request arrived through, never from the wire.
	//! \return Whether they may.
	bool Can(int playerId, MCF_ETaskAction action)
	{
		if (m_bEveryoneMayDoEverything)
			return true;

		return IsAllowed(MCF_Core_Roles.GetInstance().ResolveRole(playerId), action);
	}

	//! The table. It is a table rather than scattered checks precisely because
	//! units are meant to be able to redefine it later -- at which point this
	//! reads from a config instead of being written here, and the shape of the
	//! question does not change.
	protected bool IsAllowed(MCF_ERole role, MCF_ETaskAction action)
	{
		switch (action)
		{
			// Reading the board is what a board is for. Which *tasks* a
			// player sees is a separate question, answered per task by
			// MCF_Task_Store.IsVisibleTo -- this only says whether they
			// may open the thing at all.
			case MCF_ETaskAction.READ:
				return true;

			// Taking a job off the board is something any soldier does.
			case MCF_ETaskAction.ACCEPT:
				return true;

			// Writing and issuing orders is command work.
			case MCF_ETaskAction.CREATE:
			case MCF_ETaskAction.PUBLISH:
				return role >= MCF_ERole.SQUAD_LEADER;

			// Editing someone else's order is the commander's business.
			// Note this is the blunt version: authorship is checked
			// separately, so an author editing their own draft is handled
			// there rather than here.
			case MCF_ETaskAction.EDIT:
				return role >= MCF_ERole.COMMANDER;
		}

		return false;
	}
}
