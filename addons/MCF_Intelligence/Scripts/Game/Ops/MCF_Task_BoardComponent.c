//! Marks an entity as an operations board, and lets anything ask whether a
//! player is standing at one.
//!
//! WHY THIS EXISTS. Intel is meant to travel: a rifleman finds a letter, and
//! the force does not know what it says until somebody carries it back and a
//! commander enters it. The first version let a player log intel straight from
//! the reading screen, anywhere on the map -- which quietly deleted the entire
//! mechanic. You could read a document behind enemy lines and the whole force
//! knew it instantly.
//!
//! The board is the command post. Entering intel means being at it. That one
//! rule restores the journey without any inventory plumbing: pick the object
//! up, walk it back, enter it there.
//!
//! Registered on BOTH sides on purpose. The client uses it to grey the button
//! out, which is a courtesy; the server uses it to refuse the request, which
//! is the part that counts. A client that lies about where it is stands still
//! gets nowhere, because the server measures from the entity it controls.

[ComponentEditorProps(category: "MCF/Core", description: "Marks this entity as an operations board -- the place where intel can be entered into the system.")]
class MCF_Task_BoardComponentClass : ScriptComponentClass
{
}

class MCF_Task_BoardComponent : ScriptComponent
{
	[Attribute(defvalue: "6", uiwidget: UIWidgets.EditBox, desc: "How close a player must be, in metres, to enter intel at this board.")]
	protected float m_fUseRange;

	//! Every board currently in the world, on this machine.
	protected static ref array<MCF_Task_BoardComponent> s_aBoards = {};

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		s_aBoards.Insert(this);
		MCF_Core_Log.Debug("operations board registered, use range " + m_fUseRange.ToString() + "m");
	}

	override void OnDelete(IEntity owner)
	{
		int index = s_aBoards.Find(this);
		if (index >= 0)
			s_aBoards.Remove(index);
	}

	float GetUseRange()
	{
		return m_fUseRange;
	}

	//! \return Whether a position is within reach of any board.
	static bool IsAtBoard(vector position)
	{
		foreach (MCF_Task_BoardComponent board : s_aBoards)
		{
			if (!board)
				continue;

			IEntity owner = board.GetOwner();
			if (!owner)
				continue;

			if (vector.Distance(owner.GetOrigin(), position) <= board.GetUseRange())
				return true;
		}

		return false;
	}

	//! \return Whether a player is standing at a board. Works on either side:
	//! the server resolves the controlled entity from the player id, and a
	//! client resolves its own.
	static bool IsPlayerAtBoard(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		IEntity controlled = playerManager.GetPlayerControlledEntity(playerId);
		if (!controlled)
			return false;

		return IsAtBoard(controlled.GetOrigin());
	}

	static int GetBoardCount()
	{
		return s_aBoards.Count();
	}
}
