//! Extends the vanilla player controller with the one thing every MCF module
//! needs from it: a private line to one player.
//!
//! WHY THE PLAYER CONTROLLER. A player controller belongs to exactly one
//! player, so an RPC sent through it with `RplRcver.Owner` reaches that player
//! and nobody else, and an RPC sent *from* it with `RplRcver.Server` arrives
//! on the server already knowing who asked.
//!
//! That matters more than it sounds. Text lines are broadcast and filtered on
//! arrival, which is fine because everyone is allowed to know a line exists.
//! Taskings are not: in milsim, a rifleman holding the commander's entire plan
//! in client memory is wrong *in the fiction*, not merely wasteful. So the
//! server decides what each player may see and sends only that. Nothing a
//! client was not entitled to ever leaves the server.
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
//! argument. Every request re-checks entitlement and state on the server
//! rather than assuming the client only asked for something it was shown.
//!
//! ON THE SPLIT ACROSS FILES. All of this was one `modded class` block until
//! 2026-09-10, deliberately: Enforce chains modded classes, and a block can
//! only see members declared in blocks earlier in the chain. Keeping one block
//! meant never having to reason about that order.
//!
//! The modules make that impossible -- an addon that is not installed cannot
//! contribute half a class -- so the chain order now matters. It is made safe
//! in one direction only: MCF_SendMessage lives in Core's block, every other
//! block calls it, and Core always loads first because every module addon
//! declares Core as a dependency. Nothing in Core's block calls anything a
//! module declares.
//!
//! If that order is ever wrong the compiler says so. This is not a failure
//! mode that can go quiet, which is why the split was acceptable at all.

modded class SCR_PlayerController
{
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
}
