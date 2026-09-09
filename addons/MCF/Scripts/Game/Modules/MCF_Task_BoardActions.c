//! The interaction on the MCF task board -- the physical thing a player walks
//! up to in the world to reach the planning screen.
//!
//! WHY A PROP AND NOT A HOTKEY: milsim units want the plan to live somewhere.
//! A clipboard in a pocket, a whiteboard in the briefing tent, a map table in
//! the CP. Reaching the plan should mean going to where the plan is. This is
//! the first physical surface of that idea; it is on a table because a table
//! is a model that already exists, and the model can change without any of
//! this changing.
//!
//! There used to be three actions here -- read, accept, issue -- each doing
//! its work through popup text. That was replaced by one action opening a real
//! screen, because a popup cannot show a five-paragraph order, cannot let a
//! commander pick which task to act on, and cannot show intel next to the plan
//! it produced. The task logic those three actions drove did not go away; it
//! moved behind the board UI, where it belongs.
//!
//! HOW THIS RUNS: a user action runs on the machine of the player performing
//! it, which is why HasLocalEffectOnlyScript returns true. Opening a screen is
//! a purely local act. Everything the screen then does that changes shared
//! state goes to the server as a request through SCR_PlayerController, and the
//! server re-checks it -- see MCF_PlayerControllerTasks.c.
//!
//! HARD-WON PREFAB DETAIL: the action will register fine and still never
//! appear if the UserActionContext has no position. An empty `Position {}`
//! leaves the interaction with no anchor in the world, and no radius fixes
//! that. The vanilla arsenal box -- also a static prop with no bones in its
//! model -- solves it with `Position PointInfo { Offset 0 0.464 0 }`. The
//! board does the same at table-top height. Do not remove that Offset.

class MCF_Task_OpenBoardAction : ScriptedUserAction
{
	//! Opening a screen affects only the player who opened it.
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//! Logged because "the prompt does not appear" has two entirely different
	//! causes -- never registered on the entity, or registered and the UI
	//! declining to show it -- and without this line there is no way to tell
	//! them apart. That distinction cost a full debugging round once.
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		MCF_Core_Log.Debug("board action registered: OpenPlanningBoard");
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Enter planning board";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		MCF_PlanningBoardMenu.Open();
	}
}
