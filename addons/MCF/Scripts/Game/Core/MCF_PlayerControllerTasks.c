//! Extends the vanilla player controller with MCF's task transport.
//!
//! This is MCF's first `modded class`. It exists here rather than on the
//! GameMode entity for one reason: **a player controller belongs to exactly
//! one player**, so an RPC sent through it with `RplRcver.Owner` reaches that
//! player and nobody else, and an RPC sent *from* it with `RplRcver.Server`
//! arrives on the server already knowing who asked.
//!
//! That matters more for tasks than it does for text. Text lines are
//! broadcast and filtered on arrival, which is fine because everyone is
//! allowed to know a line exists. Tasks are not: in milsim, a rifleman holding
//! the commander's entire plan in client memory is wrong *in the fiction*, not
//! merely wasteful. So the server decides what each player may see and sends
//! only that. Nothing a client was not entitled to ever leaves the server.
//!
//! Vanilla does the same thing the same way -- `SCR_TaskSystemNetworkComponent`
//! is a plain ScriptComponent on `SCR_PlayerController` for exactly this
//! reason.
//!
//! Routing, from the engine's own table (RplRcver.c):
//!
//!   caller server, not owner, receiver Owner  -> routed to the owning client
//!   caller server, is owner,  receiver Owner  -> direct local call (hosted)
//!   caller client, is owner,  receiver Server -> routed to the server
//!
//! ON TRUSTING THE CLIENT: the request RPCs below deliberately take no player
//! id. The server reads it from `GetPlayerId()` on the controller the RPC
//! arrived through, so a client cannot act as somebody else by editing an
//! argument. The task id is still attacker-controlled, which is why every
//! request re-checks visibility and state on the server rather than assuming
//! the client only asked for something it was shown.

modded class SCR_PlayerController
{
	// ------------------------------------------------------- server -> owner

	//! Runs on the owning client. Applies one task into that client's mirror
	//! of the task list.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ReceiveTask(string data)
	{
		MCF_Task task = MCF_Core_TaskStore.GetInstance().ApplySerialized(data);
		if (!task)
			return;

		MCF_Core_Log.Debug("client received task " + task.Describe());

		// If the board is open while this arrives, it must redraw -- a
		// planning screen showing a stale plan is worse than one showing
		// none. RefreshIfOpen does nothing when no board is open, so the
		// transport never has to know whether one is.
		MCF_PlanningBoardMenu.RefreshIfOpen();
	}

	//! Runs on the owning client. Drops everything that client thinks it
	//! knows, so the batch that follows is the whole truth rather than an
	//! addition to a stale list.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ClearTasks()
	{
		MCF_Core_TaskStore.GetInstance().ClearMirror();
		MCF_PlanningBoardMenu.RefreshIfOpen();
	}

	//! Server side. Tells this controller's player to forget its task list.
	//! Always paired with a resend -- see
	//! MCF_Core_GameModeComponent.SendTasksToPlayer.
	void MCF_ClearTasks()
	{
		Rpc(MCF_RpcDo_ClearTasks);
	}


	//! Server side. Sends one task to this controller's player.
	void MCF_SendTask(notnull MCF_Task task)
	{
		Rpc(MCF_RpcDo_ReceiveTask, task.Serialize());
	}

	//! Runs on the owning client. Short feedback for something that player
	//! just did -- "task accepted", "somebody already took that one".
	//!
	//! Separate from the MCF_Voice line system on purpose: a line is mission
	//! fiction that goes through the event bus and the line queue, this is a
	//! direct answer to a button press and should never be queued behind one.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ShowMessage(string text)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
		{
			MCF_Core_Log.Debug("no popup widget here -- not rendering: " + text);
			return;
		}

		popup.PopupMsg(text, 4);
	}

	//! Server side. Says something to this controller's player alone.
	void MCF_SendMessage(string text)
	{
		Rpc(MCF_RpcDo_ShowMessage, text);
	}

	// ------------------------------------------------------- client -> server

	//! Client side. Asks the server to assign a board task to this player.
	void MCF_RequestAcceptTask(string taskId)
	{
		Rpc(MCF_RpcAsk_AcceptTask, taskId);
	}

	//! Client side. Asks the server to put one of this player's drafts on the
	//! board where the rest of the force can see it.
	void MCF_RequestPublishTask(string taskId)
	{
		Rpc(MCF_RpcAsk_PublishTask, taskId);
	}

	//! Runs on the server. `GetPlayerId()` here is the requesting player --
	//! the controller the call arrived through -- not anything the client
	//! sent.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_AcceptTask(string taskId)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();
		MCF_Core_TaskStore store = MCF_Core_TaskStore.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to accept unknown task '" + taskId + "'");
			MCF_SendMessage("That task no longer exists.");
			return;
		}

		// Re-check rather than trust that the client only asked for what it
		// was shown.
		if (!store.IsVisibleTo(task, playerId, MCF_GetFactionKey(playerId)))
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to accept task '" + taskId + "' they cannot see");
			MCF_SendMessage("That task is not on your board.");
			return;
		}

		if (task.m_eState != MCF_ETaskState.PUBLISHED)
		{
			MCF_SendMessage("Already taken: " + task.m_sTitle);
			return;
		}

		store.AssignTask(taskId, MCF_ETaskAssignee.PLAYER, playerId.ToString());
		MCF_Core_Log.Debug("player " + playerId.ToString() + " accepted " + task.Describe());

		MCF_SendMessage("Accepted: " + task.m_sTitle);
		MCF_RefreshEveryonesTasks();
	}

	//! Runs on the server. Puts an author's own draft on the board.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_PublishTask(string taskId)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();
		MCF_Core_TaskStore store = MCF_Core_TaskStore.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_SendMessage("That task no longer exists.");
			return;
		}

		// Only the author publishes their own draft. Anything else would let
		// a player push somebody else's half-written plan to the whole force.
		if (task.m_iAuthorPlayerId != playerId)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to publish task '" + taskId + "' they did not author");
			MCF_SendMessage("That is not your task to issue.");
			return;
		}

		if (task.m_eState != MCF_ETaskState.DRAFT)
		{
			MCF_SendMessage("Already issued: " + task.m_sTitle);
			return;
		}

		store.SetState(taskId, MCF_ETaskState.PUBLISHED);
		MCF_Core_Log.Debug("player " + playerId.ToString() + " issued " + task.Describe());

		MCF_SendMessage("Issued to the force: " + task.m_sTitle);
		MCF_RefreshEveryonesTasks();
	}

	//! Client side. Asks the server to start a new order in this player's name.
	void MCF_RequestCreateTask(string title)
	{
		Rpc(MCF_RpcAsk_CreateTask, title);
	}

	//! Client side. Sends the whole edited order back in one go.
	//!
	//! The payload is a serialised MCF_Task rather than six separate string
	//! arguments, so that adding a paragraph later does not change the RPC
	//! signature and there is exactly one serialisation format in the mod.
	//! The server takes only the paragraphs out of it -- see below.
	void MCF_RequestSaveTask(notnull MCF_Task edited)
	{
		Rpc(MCF_RpcAsk_SaveTask, edited.Serialize());
	}

	//! Runs on the server. Creates a draft owned by whoever asked.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_CreateTask(string title)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		if (!MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.CREATE))
		{
			MCF_SendMessage("You are not authorised to write orders.");
			return;
		}

		if (title.IsEmpty())
			title = "Untitled order";

		MCF_Task task = MCF_Core_TaskStore.GetInstance().CreateTask(title, playerId);
		if (!task)
			return;

		MCF_Core_Log.Debug("player " + playerId.ToString() + " created " + task.Describe());

		// A draft is visible to its author alone, so only the author's board
		// changes -- but the refresh is cheap and going through the same path
		// as every other change keeps one code path instead of two.
		MCF_SendMessage("Draft started: " + title);
		MCF_RefreshEveryonesTasks();
	}

	//! Runs on the server. Applies edited paragraph text to an existing task.
	//!
	//! DELIBERATELY NARROW: only the title and the five SMEAC paragraphs are
	//! taken from the payload. State, author, assignee and id are server-owned
	//! and are never read from the wire -- otherwise a client could hand
	//! itself a task, promote its own draft, or rewrite authorship simply by
	//! editing the string it sends.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_SaveTask(string data)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		MCF_Task edited = MCF_Task.Deserialize(data);
		if (!edited)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " sent an unreadable task edit");
			return;
		}

		MCF_Core_TaskStore store = MCF_Core_TaskStore.GetInstance();
		MCF_Task task = store.GetTask(edited.m_sId);
		if (!task)
		{
			MCF_SendMessage("That task no longer exists.");
			return;
		}

		if (!store.IsVisibleTo(task, playerId, MCF_GetFactionKey(playerId)))
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to edit task '" + edited.m_sId + "' they cannot see");
			MCF_SendMessage("That task is not on your board.");
			return;
		}

		// An author may always rewrite their own order. Editing somebody
		// else's is a separate, higher permission.
		bool isAuthor = task.m_iAuthorPlayerId == playerId;
		if (!isAuthor && !MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.EDIT))
		{
			MCF_SendMessage("That is not your order to change.");
			return;
		}

		store.UpdateText(task.m_sId, edited.m_sTitle, edited.m_sSituation, edited.m_sMission, edited.m_sExecution, edited.m_sAdminLogistics, edited.m_sCommandSignal);

		MCF_Core_Log.Debug("player " + playerId.ToString() + " saved " + task.Describe());
		MCF_SendMessage("Order saved.");
		MCF_RefreshEveryonesTasks();
	}


	//! Client side. Asks to close an order out -- completed or cancelled.
	void MCF_RequestCloseTask(string taskId, MCF_ETaskState state)
	{
		Rpc(MCF_RpcAsk_SetTaskState, taskId, state);
	}

	//! Runs on the server. Closes an order out.
	//!
	//! Only COMPLETE and CANCELLED are reachable through this. It would be
	//! easy to make it a general "set any state" call and much worse: a client
	//! could then walk a task straight from DRAFT to ASSIGNED and hand itself
	//! somebody else's job, bypassing both the accept and the issue paths that
	//! exist precisely to check that.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_SetTaskState(string taskId, MCF_ETaskState state)
	{
		if (!Replication.IsServer())
			return;

		if (state != MCF_ETaskState.COMPLETE && state != MCF_ETaskState.CANCELLED)
		{
			MCF_Core_Log.Warn("player " + GetPlayerId().ToString() + " asked for state " + state.ToString() + " through the close path -- refused");
			return;
		}

		int playerId = GetPlayerId();
		MCF_Core_TaskStore store = MCF_Core_TaskStore.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_SendMessage("That order no longer exists.");
			return;
		}

		if (!store.IsVisibleTo(task, playerId, MCF_GetFactionKey(playerId)))
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to close order '" + taskId + "' they cannot see");
			return;
		}

		// An order can only be closed while it is live. Closing a draft is
		// meaningless -- delete it instead -- and re-closing something already
		// closed would quietly rewrite history.
		if (task.m_eState != MCF_ETaskState.PUBLISHED
			&& task.m_eState != MCF_ETaskState.ASSIGNED
			&& task.m_eState != MCF_ETaskState.ACKNOWLEDGED)
		{
			MCF_SendMessage("That order is not running.");
			return;
		}

		// Whoever wrote it may close it. So may anyone with authority over
		// other people's orders -- that is what a commander closing out a
		// squad's tasking is.
		bool isAuthor = task.m_iAuthorPlayerId == playerId;
		bool isAssignee = task.m_eAssigneeType == MCF_ETaskAssignee.PLAYER && task.m_sAssigneeId == playerId.ToString();

		if (!isAuthor && !isAssignee && !MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.EDIT))
		{
			MCF_SendMessage("That is not your order to close.");
			return;
		}

		store.SetState(taskId, state);
		MCF_Core_Log.Debug("player " + playerId.ToString() + " closed " + task.Describe());

		if (state == MCF_ETaskState.COMPLETE)
			MCF_SendMessage(task.GetReference() + " marked complete.");
		else
			MCF_SendMessage(task.GetReference() + " cancelled.");

		MCF_RefreshEveryonesTasks();
	}


	//! Client side. Asks to remove an order from the board for good.
	void MCF_RequestDeleteTask(string taskId)
	{
		Rpc(MCF_RpcAsk_DeleteTask, taskId);
	}

	//! Runs on the server. Removes an order permanently.
	//!
	//! DELIBERATELY REFUSES TO DELETE A RUNNING ORDER. An order that is on the
	//! board or in somebody's hands has been acted on -- people moved because
	//! of it. Erasing it would leave a squad executing a plan that, as far as
	//! the board is concerned, was never given, and it would quietly remove
	//! the record the debrief is built from. Close it out first with COMPLETE
	//! or CANCELLED, then delete it. That costs one extra press and keeps the
	//! history of the session honest.
	//!
	//! So this accepts exactly two cases: your own draft, which nobody has
	//! ever seen, and an order that has already been closed.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DeleteTask(string taskId)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();
		MCF_Core_TaskStore store = MCF_Core_TaskStore.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_SendMessage("That order no longer exists.");
			return;
		}

		if (!store.IsVisibleTo(task, playerId, MCF_GetFactionKey(playerId)))
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to delete order '" + taskId + "' they cannot see");
			return;
		}

		bool isClosed = task.m_eState == MCF_ETaskState.COMPLETE
			|| task.m_eState == MCF_ETaskState.CANCELLED
			|| task.m_eState == MCF_ETaskState.FAILED;

		bool isOwnDraft = task.m_eState == MCF_ETaskState.DRAFT && task.m_iAuthorPlayerId == playerId;

		if (!isClosed && !isOwnDraft)
		{
			MCF_SendMessage("Close " + task.GetReference() + " out before removing it.");
			return;
		}

		bool isAuthor = task.m_iAuthorPlayerId == playerId;
		if (!isAuthor && !MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.EDIT))
		{
			MCF_SendMessage("That is not your order to remove.");
			return;
		}

		string orderRef = task.GetReference();
		store.DeleteTask(taskId);

		MCF_Core_Log.Debug("player " + playerId.ToString() + " deleted " + orderRef);
		MCF_SendMessage(orderRef + " removed from the board.");
		MCF_RefreshEveryonesTasks();
	}


	// ------------------------------------------------------- intel transport

	//! Runs on the owning client. Applies one intel record into that client's
	//! mirror.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ReceiveIntel(string data)
	{
		MCF_Intel_Record record = MCF_Core_IntelStore.GetInstance().ApplySerialized(data);
		if (!record)
			return;

		MCF_PlanningBoardMenu.RefreshIfOpen();
	}

	//! Runs on the owning client. Drops what it thinks it knows, so the batch
	//! that follows is the whole truth rather than an addition to a stale list.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ClearIntel()
	{
		MCF_Core_IntelStore.GetInstance().ClearMirror();
		MCF_PlanningBoardMenu.RefreshIfOpen();
	}

	void MCF_SendIntel(notnull MCF_Intel_Record record)
	{
		Rpc(MCF_RpcDo_ReceiveIntel, record.Serialize());
	}

	void MCF_ClearIntel()
	{
		Rpc(MCF_RpcDo_ClearIntel);
	}

	//! Client side. Asks the server to enter what this player just read into
	//! the system, where the rest of the force can use it.
	void MCF_RequestLogIntel(string source, string heading, string timestamp, string body)
	{
		Rpc(MCF_RpcAsk_LogIntel, source, heading, timestamp, body);
	}

	//! Runs on the server. Enters a piece of intel into the shared store.
	//!
	//! This is the moment the whole intel design turns on: until now the
	//! content existed only on an object somebody was holding. From here it is
	//! shared, persisted, and can be the basis of an order.
	//!
	//! The text arrives from the client, which is unavoidable -- the client is
	//! the one that read the object, and a commander typing up a radio report
	//! is authoring text by definition. So it is treated as user input, not as
	//! evidence: length-capped, and attributed to the player who sent it so
	//! that anything odd has a name against it.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_LogIntel(string source, string heading, string timestamp, string body)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		if (!MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.CREATE))
		{
			MCF_SendMessage("You are not authorised to enter intel.");
			return;
		}

		if (heading.IsEmpty() && body.IsEmpty())
		{
			MCF_SendMessage("There is nothing to enter.");
			return;
		}

		// Intel has to be carried back. Reading a document in the field tells
		// you something; it does not tell the force anything until somebody
		// walks it to the command post and enters it there. Checked on the
		// server against the entity this player actually controls, so a client
		// cannot claim to be somewhere it is not.
		if (!MCF_Task_BoardComponent.IsPlayerAtBoard(playerId))
		{
			MCF_SendMessage("Bring it to the operations board to enter it.");
			return;
		}

		MCF_Intel_Record record = MCF_Core_IntelStore.GetInstance().Log(Cap(source, 64), Cap(heading, 128), Cap(timestamp, 32), Cap(body, 2000), playerId, MCF_GetFactionKey(playerId));
		if (!record)
			return;

		MCF_Core_Log.Debug("player " + playerId.ToString() + " logged " + record.Describe());
		MCF_SendMessage(record.GetReference() + " entered on the board.");

		MCF_RefreshEveryonesTasks();
	}

	//! Trims client-supplied text to something a board can display.
	//!
	//! Not a security control -- there is nothing dangerous in a long string
	//! here -- but an unbounded field is an unbounded RPC and an unreadable
	//! list, and both are easier to prevent than to notice.
	protected string Cap(string value, int limit)
	{
		if (value.Length() <= limit)
			return value;

		return value.Substring(0, limit);
	}


	//! Client side. Asks for a new draft order built on a logged report.
	void MCF_RequestCreateTaskFromIntel(string recordId)
	{
		Rpc(MCF_RpcAsk_CreateTaskFromIntel, recordId);
	}

	//! Runs on the server. Starts an order from a report.
	//!
	//! Note what is copied and what is not. The report goes into SITUATION,
	//! because that paragraph is exactly "what is known, friendly and enemy".
	//! MISSION, EXECUTION and the rest are left empty on purpose: those are
	//! the commander's judgement, and filling them in from intel would be the
	//! mod pretending the plan writes itself.
	//!
	//! The client sends only a record id, never the text. The server reads the
	//! content from its own store, so a client cannot smuggle words into an
	//! order by claiming they came from a report.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_CreateTaskFromIntel(string recordId)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		if (!MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.CREATE))
		{
			MCF_SendMessage("You are not authorised to write orders.");
			return;
		}

		MCF_Intel_Record record = MCF_Core_IntelStore.GetInstance().GetRecord(recordId);
		if (!record)
		{
			MCF_SendMessage("That report no longer exists.");
			return;
		}

		MCF_Core_TaskStore store = MCF_Core_TaskStore.GetInstance();

		MCF_Task task = store.CreateTask(record.m_sHeading, playerId);
		if (!task)
			return;

		string situation = record.m_sBody;

		// Keep the provenance in the order itself. A commander reading this
		// next week needs to know it rests on a letter found on a body, not on
		// a signals intercept.
		if (!record.m_sSource.IsEmpty())
			situation = situation + "\n\n(" + record.GetReference() + ", from: " + record.m_sSource + ")";

		store.UpdateText(task.m_sId, record.m_sHeading, situation, "", "", "", "");

		MCF_Core_Log.Debug("player " + playerId.ToString() + " drafted " + task.Describe() + " from " + record.GetReference());
		MCF_SendMessage("Draft started from " + record.GetReference() + ".");

		MCF_RefreshEveryonesTasks();
	}


	//! Client side, Game Master only in practice. Sends a rewritten intel
	//! object to the server.
	void MCF_RequestEditIntelObject(RplId targetId, string payload)
	{
		Rpc(MCF_RpcAsk_EditIntelObject, targetId, payload);
	}

	//! Runs on the server. Rewrites what an object in the world says.
	//!
	//! The object is addressed by its replication id rather than by name or
	//! position -- the same way vanilla's own editor identifies an entity it
	//! is editing. A name would be ambiguous and a position guessable; an
	//! RplId resolves to exactly one entity or to nothing.
	//!
	//! This is authoring, so it is gated on the permission to author. Today
	//! that is open to everyone, which is fine while the mod is being built
	//! and is the single switch that closes it later.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_EditIntelObject(RplId targetId, string payload)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		if (!MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.EDIT))
		{
			MCF_SendMessage("You are not authorised to rewrite intel objects.");
			return;
		}

		// The wire carries the editable component, because that is what the
		// replication tables actually hold -- an IEntity is not a replicated
		// item and asking for its id returns nothing.
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(targetId));
		if (!editable)
		{
			MCF_SendMessage("That object is no longer there.");
			return;
		}

		IEntity target = editable.GetOwner();
		if (!target)
		{
			MCF_SendMessage("That object is no longer there.");
			return;
		}

		MCF_Intel_CarrierComponent carrier = MCF_Intel_CarrierComponent.Cast(target.FindComponent(MCF_Intel_CarrierComponent));
		if (!carrier)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to rewrite an object that carries no intel");
			return;
		}

		carrier.SetContentFromServer(payload);

		MCF_Core_Log.Debug("player " + playerId.ToString() + " rewrote an intel object");
		MCF_SendMessage("Object updated.");
	}


	// ------------------------------------------------------------- helpers

	//! Server side. A state change moves a task in and out of people's view,
	//! so everyone gets a fresh copy of what they may now see.
	//!
	//! This re-sends rather than sending a delta, because a delta cannot
	//! express "you may no longer see this" without telling the client the
	//! task exists -- which is the one thing the whole design is trying to
	//! avoid. Task volumes are tiny; re-sending is cheap and honest.
	protected void MCF_RefreshEveryonesTasks()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		MCF_Core_GameModeComponent mcf = MCF_Core_GameModeComponent.Cast(gameMode.FindComponent(MCF_Core_GameModeComponent));
		if (!mcf)
		{
			MCF_Core_Log.Warn("no MCF_Core_GameModeComponent on the game mode -- cannot refresh tasks after a change");
			return;
		}

		mcf.RefreshTasksForAllPlayers();
	}

	//! \return A player's faction key, or empty if they have not picked one.
	protected string MCF_GetFactionKey(int playerId)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return string.Empty;

		Faction faction = factionManager.GetPlayerFaction(playerId);
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	// =====================================================================
	// CONVERSATIONS
	//
	// Kept in this file rather than a second `modded class SCR_PlayerController`
	// of its own. Enforce chains modded classes, so a second block would
	// probably work -- but the chain order across files is not something this
	// project controls, and MCF_SendMessage below would be visible only if the
	// dialogue block happened to land later in it. The modded enums cost a
	// round of confusion for exactly this reason; one block per class, one
	// place to look.
	//
	// THE SHAPE IS THE ONE THE BOARD USES. The client asks; the server decides
	// and answers. A request carries an entity id and an index, never a player
	// id and never an effect -- the player is read from the controller the
	// request arrived through.
	// =====================================================================

	//! Client side. Asks to start talking to someone.
	void MCF_RequestDialogueBegin(RplId speakerId)
	{
		Rpc(MCF_RpcAsk_DialogueBegin, speakerId);
	}

	//! Client side. Sends the reply the player picked.
	void MCF_RequestDialogueChoose(RplId speakerId, string nodeId, int choiceIndex)
	{
		Rpc(MCF_RpcAsk_DialogueChoose, speakerId, nodeId, choiceIndex);
	}

	//! Client side. The player walked away or closed the screen, so the server
	//! can forget where they were in the conversation.
	void MCF_RequestDialogueEnd(RplId speakerId)
	{
		Rpc(MCF_RpcAsk_DialogueEnd, speakerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DialogueBegin(RplId speakerId)
	{
		if (!Replication.IsServer())
			return;

		MCF_Dialogue_Component dialogue = MCF_ResolveDialogue(speakerId);
		if (!dialogue)
			return;

		if (!MCF_IsWithinTalkRange(dialogue))
		{
			MCF_SendMessage("Too far away to talk.");
			return;
		}

		MCF_Dialogue_View view = dialogue.Begin(GetPlayerId(), GetControlledEntity());
		if (!view)
		{
			MCF_SendMessage("They have nothing to say.");
			return;
		}

		Rpc(MCF_RpcDo_DialogueView, view.Serialize());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DialogueChoose(RplId speakerId, string nodeId, int choiceIndex)
	{
		if (!Replication.IsServer())
			return;

		MCF_Dialogue_Component dialogue = MCF_ResolveDialogue(speakerId);
		if (!dialogue)
			return;

		// Checked again on every reply, not only when the conversation opened.
		// Otherwise a player opens one and keeps talking from the far side of
		// the village.
		if (!MCF_IsWithinTalkRange(dialogue))
		{
			dialogue.End(GetPlayerId());
			MCF_SendMessage("You have walked out of earshot.");
			Rpc(MCF_RpcDo_DialogueClose);
			return;
		}

		MCF_Dialogue_View view = dialogue.Choose(GetPlayerId(), nodeId, choiceIndex, GetControlledEntity());
		if (!view)
		{
			Rpc(MCF_RpcDo_DialogueClose);
			return;
		}

		Rpc(MCF_RpcDo_DialogueView, view.Serialize());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DialogueEnd(RplId speakerId)
	{
		if (!Replication.IsServer())
			return;

		MCF_Dialogue_Component dialogue = MCF_ResolveDialogue(speakerId);
		if (dialogue)
			dialogue.End(GetPlayerId());
	}

	//! Runs on the asking client. One screenful of conversation.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_DialogueView(string data)
	{
		MCF_Dialogue_View view = MCF_Dialogue_View.Deserialize(data);
		if (!view)
		{
			MCF_Core_Log.Warn("dialogue view arrived unreadable");
			return;
		}

		MCF_Dialogue_Menu.ShowView(view);
	}

	//! Runs on the asking client. Over from the server's side -- out of range,
	//! gone, or nothing left to say.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_DialogueClose()
	{
		MCF_Dialogue_Menu.CloseIfOpen();
	}

	//! Resolves the entity a client named.
	//!
	//! HARD-WON, and repeated because it cost a full test round on the intel
	//! editor: Replication.FindItem takes and returns the replicated
	//! COMPONENT, not the entity. An IEntity handed to FindItemId silently
	//! produces an id that resolves to nothing.
	protected MCF_Dialogue_Component MCF_ResolveDialogue(RplId speakerId)
	{
		// SCR_EditableEntityComponent and not RplComponent. RplComponent is an
		// engine class that exposes no GetOwner() to script, so there is no
		// way back from it to the entity -- the compiler says so plainly, but
		// only after you have written everything around it. The editable
		// component is the route already proven by the intel editor.
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(speakerId));
		if (!editable)
		{
			MCF_SendMessage("They are no longer there.");
			return null;
		}

		IEntity speaker = editable.GetOwner();
		if (!speaker)
			return null;

		MCF_Dialogue_Component dialogue = MCF_Dialogue_Component.Cast(speaker.FindComponent(MCF_Dialogue_Component));
		if (!dialogue)
		{
			MCF_Core_Log.Warn("player " + GetPlayerId().ToString() + " addressed an entity with no dialogue on it");
			return null;
		}

		return dialogue;
	}

	//! Distance is measured from the entity this player actually controls, so
	//! a client cannot claim to be standing somewhere it is not.
	protected bool MCF_IsWithinTalkRange(notnull MCF_Dialogue_Component dialogue)
	{
		IEntity speaker = dialogue.GetOwner();
		IEntity listener = GetControlledEntity();

		if (!speaker || !listener)
			return false;

		return vector.Distance(speaker.GetOrigin(), listener.GetOrigin()) <= dialogue.GetTalkRange();
	}

	// ---------------------------------------------------------------------
	// AUTHORING CONVERSATIONS
	//
	// Free text, so it cannot go through the editor attribute panel -- that
	// packs every value into one vector and carries twelve bytes. It comes
	// over MCF's own RPC instead, exactly as intel authoring does.
	// ---------------------------------------------------------------------

	//! Client side. Saves a conversation into the shared library.
	void MCF_RequestSaveConversation(string data, RplId assignTo)
	{
		Rpc(MCF_RpcAsk_SaveConversation, data, assignTo);
	}

	//! Runs on the server. Writes the conversation and, optionally, gives it
	//! to the person it was written on.
	//!
	//! The text arrives from the client, which is unavoidable -- a Game Master
	//! typing a conversation is authoring by definition. So it is treated as
	//! user input: length-capped, and refused outright unless the sender has
	//! Game Master rights, because this writes shared, persisted state that
	//! every player in the mission will read.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_SaveConversation(string data, RplId assignTo)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		if (MCF_Task_Permissions.GetInstance().ResolveRole(playerId) != MCF_ETaskRole.COMMANDER)
		{
			MCF_SendMessage("Only a Game Master may write conversations.");
			return;
		}

		if (data.Length() > 12000)
		{
			MCF_SendMessage("That conversation is too long to store.");
			return;
		}

		MCF_Dialogue_Conversation conversation = MCF_Dialogue_Script.Deserialize(data);
		if (!conversation)
		{
			MCF_SendMessage("That conversation could not be read.");
			return;
		}

		if (!MCF_Dialogue_Library.GetInstance().Upsert(conversation))
		{
			MCF_SendMessage("The conversation was not saved.");
			return;
		}

		MCF_SendMessage("Conversation '" + conversation.m_sId + "' saved.");

		if (assignTo == RplId.Invalid())
			return;

		MCF_Dialogue_Component dialogue = MCF_ResolveDialogue(assignTo);
		if (!dialogue)
			return;

		dialogue.SetConversation(conversation.m_sId);
		MCF_SendMessage("Given to " + dialogue.GetSpeakerName() + ".");
	}

	//! Client side. Asks for the whole library index, so a Game Master can see
	//! and pick from every conversation rather than only the one the person
	//! in front of them happens to be running.
	void MCF_RequestConversationList()
	{
		Rpc(MCF_RpcAsk_ConversationList);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_ConversationList()
	{
		if (!Replication.IsServer())
			return;

		if (MCF_Task_Permissions.GetInstance().ResolveRole(GetPlayerId()) != MCF_ETaskRole.COMMANDER)
			return;

		array<string> ids = {};
		MCF_Dialogue_Library.GetInstance().GetIds(ids);

		// Comma-separated, which is safe because ids are sanitised of commas
		// when they are saved. A marker says which ones ship with the mod, so
		// the editor can grey DELETE rather than failing on them.
		string packed = "";
		foreach (string id : ids)
		{
			if (!packed.IsEmpty())
				packed = packed + ",";

			if (MCF_Dialogue_Library.GetInstance().IsShipped(id))
				packed = packed + "*" + id;
			else
				packed = packed + id;
		}

		Rpc(MCF_RpcDo_ConversationList, packed);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ConversationList(string packed)
	{
		MCF_Dialogue_EditorMenu.ReceiveList(packed);
	}

	//! Client side. Deletes a conversation a Game Master wrote.
	void MCF_RequestDeleteConversation(string conversationId)
	{
		Rpc(MCF_RpcAsk_DeleteConversation, conversationId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DeleteConversation(string conversationId)
	{
		if (!Replication.IsServer())
			return;

		if (MCF_Task_Permissions.GetInstance().ResolveRole(GetPlayerId()) != MCF_ETaskRole.COMMANDER)
		{
			MCF_SendMessage("Only a Game Master may do that.");
			return;
		}

		if (!MCF_Dialogue_Library.GetInstance().Delete(conversationId))
		{
			MCF_SendMessage("'" + conversationId + "' ships with the mod and cannot be deleted.");
			return;
		}

		MCF_SendMessage("Conversation '" + conversationId + "' deleted.");

		// Send the index back so the editor's list is right immediately.
		MCF_RpcAsk_ConversationList();
	}

	//! Client side. Asks the server for the text of a conversation, so the
	//! editor opens on what is actually stored.
	//!
	//! ASKED, NOT READ LOCALLY. A Game Master's machine has the shipped config
	//! but not conversations written this session on a dedicated server.
	//! Reading the local library would open an empty editor over somebody's
	//! existing words and then save over them.
	void MCF_RequestConversationText(string conversationId)
	{
		Rpc(MCF_RpcAsk_ConversationText, conversationId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_ConversationText(string conversationId)
	{
		if (!Replication.IsServer())
			return;

		if (MCF_Task_Permissions.GetInstance().ResolveRole(GetPlayerId()) != MCF_ETaskRole.COMMANDER)
			return;

		MCF_Dialogue_Conversation conversation = MCF_Dialogue_Library.GetInstance().Find(conversationId);
		if (!conversation)
		{
			// Answered anyway, with nothing. The editor is waiting, and an
			// unanswered request leaves it saying "Fetching..." for ever.
			Rpc(MCF_RpcDo_ConversationText, string.Empty);
			return;
		}

		Rpc(MCF_RpcDo_ConversationText, MCF_Dialogue_Script.Serialize(conversation));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ConversationText(string data)
	{
		MCF_Dialogue_EditorMenu.ReceiveConversation(data);
	}

	//! Client side. Points somebody at a conversation that already exists.
	void MCF_RequestAssignConversation(RplId speakerId, string conversationId)
	{
		Rpc(MCF_RpcAsk_AssignConversation, speakerId, conversationId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_AssignConversation(RplId speakerId, string conversationId)
	{
		if (!Replication.IsServer())
			return;

		if (MCF_Task_Permissions.GetInstance().ResolveRole(GetPlayerId()) != MCF_ETaskRole.COMMANDER)
		{
			MCF_SendMessage("Only a Game Master may do that.");
			return;
		}

		MCF_Dialogue_Component dialogue = MCF_ResolveDialogue(speakerId);
		if (!dialogue)
			return;

		dialogue.SetConversation(conversationId);
	}

	// ---------------------------------------------------------------------
	// SHOUTING
	// ---------------------------------------------------------------------

	//! Client side. The player shouted.
	//!
	//! Only the fact and whether the weapon was up travel. Who heard it is the
	//! server's to work out -- a client that sent a list of who should obey
	//! would be deciding the outcome.
	void MCF_RequestShout(int shout, bool weaponRaised)
	{
		Rpc(MCF_RpcAsk_Shout, shout, weaponRaised);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_Shout(int shout, bool weaponRaised)
	{
		if (!Replication.IsServer())
			return;

		IEntity shouter = GetControlledEntity();
		if (!shouter)
			return;

		// Re-read on the server rather than trusting the flag. The client
		// sends it so the server need not guess at a value that changes
		// between the key press and the packet arriving, but a client that
		// lies about it would be buying the threat bonus for free.
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(shouter.FindComponent(SCR_CharacterControllerComponent));
		bool raised = weaponRaised;

		if (controller)
			raised = controller.IsWeaponRaised();

		int obeyed = MCF_AI_Shout.Resolve(shouter, shout, raised);

		if (obeyed > 0)
			MCF_SendMessage(obeyed.ToString() + " did as they were told.");
	}
}
