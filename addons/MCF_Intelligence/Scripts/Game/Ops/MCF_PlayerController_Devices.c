//! Ops' device authoring, on the player controller.
//!
//! See MCF_PlayerController_Core.c for why any of this lives here and for the
//! rule about chain order across these files.
//!
//! ONE REQUEST, ONE WHOLE PROFILE. The editor sends the rewritten device in a
//! single serialised string rather than a call per app or per message. Partial
//! edits would need the server to hold a draft between calls, and a draft that
//! outlives a disconnected Game Master is state nobody owns.

modded class SCR_PlayerController
{
	//! Client side. Sends a rewritten device profile for one object.
	void MCF_RequestWriteDeviceProfile(RplId targetId, string payload)
	{
		Rpc(MCF_RpcAsk_WriteDeviceProfile, targetId, payload);
	}

	//! Runs on the server. Writes a profile onto one object in the world.
	//!
	//! The object is addressed by its replication id, the same way vanilla's
	//! own editor identifies an entity it is editing: a name would be ambiguous
	//! and a position guessable, an RplId resolves to exactly one entity or to
	//! nothing.
	//!
	//! This is authoring, so it is gated on the permission to author -- the
	//! same gate MCF_RpcAsk_EditIntelObject uses, and the same single switch
	//! that closes it when the mod stops being built.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_WriteDeviceProfile(RplId targetId, string payload)
	{
		if (!Replication.IsServer())
			return;

		int playerId = GetPlayerId();

		if (!MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.EDIT))
		{
			MCF_SendMessage("You are not authorised to rewrite devices.");
			return;
		}

		// An unbounded string off the wire is somebody else's problem to have.
		// A profile of eight apps with a dozen messages each is well under
		// this; anything past it is not a profile.
		if (payload.Length() > MCF_DEVICE_PROFILE_MAX_LENGTH)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " sent an oversized device profile -- ignored");
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
			MCF_Core_Log.Warn("player " + playerId.ToString() + " tried to rewrite an object that is not a device");
			return;
		}

		// Read it back before storing it. A payload that will not deserialise
		// would replicate to every client and leave each of them silently
		// falling back to the object's own entries -- the failure this whole
		// design exists to avoid, arriving by a different door.
		MCF_Device_Profile parsed = MCF_Device_Script.Deserialize(payload);
		if (!parsed)
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " sent a device profile that could not be read back -- ignored");
			MCF_SendMessage("That did not save.");
			return;
		}

		carrier.SetProfileFromServer(payload);

		// Also kept in the library, under its own id, so a profile written on
		// one device can be assigned to another by id later. The object still
		// carries its own copy -- that is what the client reads -- but the
		// library is what makes it reusable and what survives the restart.
		if (!parsed.m_sId.IsEmpty())
			MCF_Device_Library.GetInstance().Upsert(parsed);

		MCF_Core_Log.Debug("player " + playerId.ToString() + " rewrote a device");
		MCF_SendMessage("Device updated.");
	}

	//! Characters, not novels. Eight apps of a dozen messages is roughly 6k.
	static const int MCF_DEVICE_PROFILE_MAX_LENGTH = 24000;
}
