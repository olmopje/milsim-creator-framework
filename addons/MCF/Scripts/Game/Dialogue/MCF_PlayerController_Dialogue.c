//! The dialogue module's end of the player controller: talking to somebody,
//! and a Game Master writing what they say.
//!
//! See MCF_PlayerController_Core.c for why any of this lives on the player
//! controller and for the rule about chain order across these files.
//!
//! THE SHAPE IS THE ONE THE BOARD USES. The client asks; the server decides
//! and answers. A request carries an entity id and an index, never a player
//! id and never an effect -- the player is read from the controller the
//! request arrived through.

modded class SCR_PlayerController
{
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
}
