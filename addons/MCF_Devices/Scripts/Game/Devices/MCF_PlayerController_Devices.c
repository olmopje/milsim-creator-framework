//! The devices module's end of the player controller: breaking into things.
//!
//! See MCF_PlayerController_Core.c for why any of this lives on the player
//! controller and for the rule about chain order across these files.
//!
//! THE SERVER DECIDES, THE CLIENT ONLY PRESSES. The request carries an entity
//! and, later, a sequence of presses. It never carries "I won". The server
//! invents the puzzle, keeps the seed, times it on its own clock, and re-
//! derives the answer -- see MCF_Devices_Challenge.c for why that shape was
//! chosen over the obvious one.

modded class SCR_PlayerController
{
	//! Client side. Asks to start breaking into a device.
	void MCF_RequestDeviceChallenge(RplId deviceId)
	{
		Rpc(MCF_RpcAsk_DeviceChallenge, deviceId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DeviceChallenge(RplId deviceId)
	{
		if (!Replication.IsServer())
			return;

		MCF_Devices_LockComponent lock = MCF_ResolveDeviceLock(deviceId);
		if (!lock)
			return;

		if (!lock.IsLocked())
		{
			MCF_SendMessage("It is already open.");
			return;
		}

		int seed = lock.IssueChallenge(GetPlayerId());
		if (seed == 0)
			return;

		Rpc(MCF_RpcDo_DeviceChallenge, deviceId, seed, lock.GetDifficulty());
	}

	//! Runs on the owning client. The puzzle to draw.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_DeviceChallenge(RplId deviceId, int seed, int difficulty)
	{
		MCF_Devices_HackMenu.OpenFor(deviceId, seed, difficulty);
	}

	//! Client side. What the player pressed, in order. Not whether it was right.
	void MCF_RequestDeviceAnswer(RplId deviceId, string answer)
	{
		Rpc(MCF_RpcAsk_DeviceAnswer, deviceId, answer);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DeviceAnswer(RplId deviceId, string answer)
	{
		if (!Replication.IsServer())
			return;

		MCF_Devices_LockComponent lock = MCF_ResolveDeviceLock(deviceId);
		if (!lock)
			return;

		// The answer is capped before it is looked at. A sequence is never
		// longer than MAX_STEPS digits, and an unbounded string off the wire
		// is somebody else's problem to have.
		if (answer.Length() > MCF_Devices_Challenge.MAX_STEPS)
		{
			MCF_SendMessage("That did not work.");
			return;
		}

		if (lock.SubmitAnswer(GetPlayerId(), answer))
			MCF_SendMessage("You are in.");
		else
			MCF_SendMessage("That did not work.");
	}

	//! Resolves the device the request names, checking it is close enough to
	//! reach. A player who is not standing over it has no business opening it,
	//! whatever their client says.
	protected MCF_Devices_LockComponent MCF_ResolveDeviceLock(RplId deviceId)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(deviceId));
		if (!rpl)
			return null;

		IEntity device = rpl.GetEntity();
		if (!device)
			return null;

		IEntity user = GetControlledEntity();
		if (!user)
			return null;

		if (vector.Distance(user.GetOrigin(), device.GetOrigin()) > MCF_DEVICE_REACH)
		{
			MCF_SendMessage("Too far away.");
			return null;
		}

		return MCF_Devices_LockComponent.Cast(device.FindComponent(MCF_Devices_LockComponent));
	}

	//! Metres. Generous enough for a phone on a desk you are leaning over,
	//! short enough that it cannot be done from cover.
	static const float MCF_DEVICE_REACH = 4.0;
}
