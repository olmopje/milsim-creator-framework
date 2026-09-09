//! The operations module's end of the player controller: the task and intel
//! transport, and every request a board can make of the server.
//!
//! See MCF_PlayerController_Core.c for why any of this lives on the player
//! controller and for the rule about chain order across these files.
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
//! argument. Every request re-checks entitlement and state on the server
//! rather than assuming the client only asked for something it was shown.

modded class SCR_PlayerController
{
	// ------------------------------------------------------- server -> owner

	//! Runs on the owning client. Applies one task into that client's mirror
	//! of the task list.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ReceiveTask(string data)
	{
		MCF_Task task = MCF_Task_Store.GetInstance().ApplySerialized(data);
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
		MCF_Task_Store.GetInstance().ClearMirror();
		MCF_PlanningBoardMenu.RefreshIfOpen();
	}

	//! Server side. Tells this controller's player to forget its task list.
	//! Always paired with a resend -- see
	//! MCF_Ops_GameModeComponent.SendTasksToPlayer.
	void MCF_ClearTasks()
	{
		Rpc(MCF_RpcDo_ClearTasks);
	}


	//! Server side. Sends one task to this controller's player.
	void MCF_SendTask(notnull MCF_Task task)
	{
		Rpc(MCF_RpcDo_ReceiveTask, task.Serialize());
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
		MCF_Task_Store store = MCF_Task_Store.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to accept unknown task '" + taskId + "'");
			MCF_SendMessage("That task no longer exists.");
			return;
		}

		// Re-check rather than trust that the client only asked for what it
		// was shown.
		if (!store.IsVisibleTo(task, playerId, MCF_Core_FactionHelper.GetPlayerFactionKey(playerId)))
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
		MCF_Task_Store store = MCF_Task_Store.GetInstance();

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

		MCF_Task task = MCF_Task_Store.GetInstance().CreateTask(title, playerId);
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

		MCF_Task_Store store = MCF_Task_Store.GetInstance();
		MCF_Task task = store.GetTask(edited.m_sId);
		if (!task)
		{
			MCF_SendMessage("That task no longer exists.");
			return;
		}

		if (!store.IsVisibleTo(task, playerId, MCF_Core_FactionHelper.GetPlayerFactionKey(playerId)))
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
		MCF_Task_Store store = MCF_Task_Store.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_SendMessage("That order no longer exists.");
			return;
		}

		if (!store.IsVisibleTo(task, playerId, MCF_Core_FactionHelper.GetPlayerFactionKey(playerId)))
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
		MCF_Task_Store store = MCF_Task_Store.GetInstance();

		MCF_Task task = store.GetTask(taskId);
		if (!task)
		{
			MCF_SendMessage("That order no longer exists.");
			return;
		}

		if (!store.IsVisibleTo(task, playerId, MCF_Core_FactionHelper.GetPlayerFactionKey(playerId)))
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
		MCF_Intel_Record record = MCF_Intel_Store.GetInstance().ApplySerialized(data);
		if (!record)
			return;

		MCF_PlanningBoardMenu.RefreshIfOpen();
	}

	//! Runs on the owning client. Drops what it thinks it knows, so the batch
	//! that follows is the whole truth rather than an addition to a stale list.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ClearIntel()
	{
		MCF_Intel_Store.GetInstance().ClearMirror();
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

		MCF_Intel_Record record = MCF_Intel_Store.GetInstance().Log(Cap(source, 64), Cap(heading, 128), Cap(timestamp, 32), Cap(body, 2000), playerId, MCF_Core_FactionHelper.GetPlayerFactionKey(playerId));
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

		MCF_Intel_Record record = MCF_Intel_Store.GetInstance().GetRecord(recordId);
		if (!record)
		{
			MCF_SendMessage("That report no longer exists.");
			return;
		}

		MCF_Task_Store store = MCF_Task_Store.GetInstance();

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

		MCF_Ops_GameModeComponent ops = MCF_Ops_GameModeComponent.Cast(gameMode.FindComponent(MCF_Ops_GameModeComponent));
		if (!ops)
		{
			MCF_Core_Log.Warn("no MCF_Ops_GameModeComponent on the game mode -- cannot refresh tasks after a change");
			return;
		}

		ops.RefreshTasksForAllPlayers();
	}

}
