//! Managing the persistent store, on the player controller.
//!
//! See MCF_PlayerController_Core.c for why any of this lives here and for the
//! rule about chain order across these files.
//!
//! EVERYTHING HERE IS SERVER WORK. The store is a file next to the server, not
//! state a client holds, so a client can only ask. Every one of these is gated
//! on the permission to author, and every one of them logs who asked -- these
//! are the only calls in MCF that destroy data on purpose, and if one of them
//! ever fires unexpectedly the log has to name a person.
//!
//! THE SUMMARY GOES BACK AS ONE STRING. A client needs to know how many
//! entries each set holds and which snapshots exist, and neither is knowable
//! on the client. Sending it as one serialised reply rather than a call per set
//! keeps the round trips at one, which matters because the screen opens on it.

modded class SCR_PlayerController
{
	//! Between sets, and between the sets half and the snapshots half.
	protected static const string MCF_DATA_SEP_SECTION = "<<s>>";
	protected static const string MCF_DATA_SEP_ROW = "<<r>>";
	protected static const string MCF_DATA_SEP_FIELD = "<<f>>";

	//! A name is a filename. This is generous for a label and far short of
	//! anything worth worrying about on the wire.
	protected static const int MCF_SNAPSHOT_NAME_MAX = 64;

	// ------------------------------------------------------- client -> server

	//! Client side. Asks what is in the store and what snapshots exist.
	void MCF_RequestDataSummary()
	{
		Rpc(MCF_RpcAsk_DataSummary);
	}

	void MCF_RequestClearDataSet(string setId)
	{
		Rpc(MCF_RpcAsk_ClearDataSet, setId);
	}

	void MCF_RequestClearAllData()
	{
		Rpc(MCF_RpcAsk_ClearAllData);
	}

	void MCF_RequestSaveSnapshot(string name)
	{
		Rpc(MCF_RpcAsk_SaveSnapshot, name);
	}

	void MCF_RequestLoadSnapshot(string name)
	{
		Rpc(MCF_RpcAsk_LoadSnapshot, name);
	}

	void MCF_RequestDeleteSnapshot(string name)
	{
		Rpc(MCF_RpcAsk_DeleteSnapshot, name);
	}

	// ------------------------------------------------------------- the server

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DataSummary()
	{
		if (!Replication.IsServer())
			return;

		// Reading is not destructive, but it does describe the whole mission's
		// state, so it is gated the same way the writing is.
		if (!MCF_DataAllowed("read the mission's data"))
			return;

		Rpc(MCF_RpcDo_ReceiveDataSummary, MCF_BuildDataSummary());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_ClearDataSet(string setId)
	{
		if (!Replication.IsServer())
			return;

		if (!MCF_DataAllowed("clear the mission's data"))
			return;

		MCF_Core_DataSet dataSet = MCF_Core_DataSets.Find(setId);
		if (!dataSet)
		{
			MCF_Core_Log.Warn("player " + GetPlayerId().ToString() + " asked to clear unknown data set '" + setId + "'");
			return;
		}

		int before = dataSet.Count();
		MCF_Core_DataSets.Clear(setId);

		MCF_Core_Log.Debug("player " + GetPlayerId().ToString() + " cleared '" + setId + "' (" + before.ToString() + " entries)");
		MCF_SendMessage(dataSet.m_sLabel + " cleared: " + before.ToString() + " entries gone.");

		Rpc(MCF_RpcDo_ReceiveDataSummary, MCF_BuildDataSummary());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_ClearAllData()
	{
		if (!Replication.IsServer())
			return;

		if (!MCF_DataAllowed("clear the mission's data"))
			return;

		int removed = MCF_Core_DataSets.ClearAll();

		MCF_Core_Log.Debug("player " + GetPlayerId().ToString() + " cleared every data set (" + removed.ToString() + " keys)");
		MCF_SendMessage("Mission data cleared.");

		Rpc(MCF_RpcDo_ReceiveDataSummary, MCF_BuildDataSummary());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_SaveSnapshot(string name)
	{
		if (!Replication.IsServer())
			return;

		if (!MCF_DataAllowed("save a snapshot"))
			return;

		if (name.Length() > MCF_SNAPSHOT_NAME_MAX)
		{
			MCF_SendMessage("That name is too long.");
			return;
		}

		if (MCF_Core_DataSnapshots.Save(name))
			MCF_SendMessage("Saved snapshot: " + name);
		else
			MCF_SendMessage("That snapshot did not save.");

		Rpc(MCF_RpcDo_ReceiveDataSummary, MCF_BuildDataSummary());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_LoadSnapshot(string name)
	{
		if (!Replication.IsServer())
			return;

		if (!MCF_DataAllowed("restore a snapshot"))
			return;

		if (MCF_Core_DataSnapshots.Load(name))
		{
			MCF_Core_Log.Debug("player " + GetPlayerId().ToString() + " restored snapshot '" + name + "'");
			MCF_SendMessage("Restored snapshot: " + name);
		}
		else
		{
			MCF_SendMessage("That snapshot could not be restored.");
		}

		Rpc(MCF_RpcDo_ReceiveDataSummary, MCF_BuildDataSummary());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void MCF_RpcAsk_DeleteSnapshot(string name)
	{
		if (!Replication.IsServer())
			return;

		if (!MCF_DataAllowed("delete a snapshot"))
			return;

		MCF_Core_DataSnapshots.Delete(name);
		MCF_SendMessage("Deleted snapshot: " + name);

		Rpc(MCF_RpcDo_ReceiveDataSummary, MCF_BuildDataSummary());
	}

	// ------------------------------------------------------- server -> client

	//! Runs on the owning client. Hands the screen what only the server knows.
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void MCF_RpcDo_ReceiveDataSummary(string payload)
	{
		MCF_Data_ManagerMenu.ApplySummary(payload);
	}

	// ------------------------------------------------------------- internals

	protected bool MCF_DataAllowed(string what)
	{
		if (MCF_Task_Permissions.GetInstance().Can(GetPlayerId(), MCF_ETaskAction.EDIT))
			return true;

		MCF_Core_Log.Warn("player " + GetPlayerId().ToString() + " tried to " + what + " without permission");
		MCF_SendMessage("You are not authorised to do that.");
		return false;
	}

	//! "<sets><<s>><snapshots>", sets as id/label/description/count rows.
	protected string MCF_BuildDataSummary()
	{
		string sets = "";

		array<MCF_Core_DataSet> all = {};
		MCF_Core_DataSets.GetAll(all);

		foreach (int i, MCF_Core_DataSet dataSet : all)
		{
			if (i > 0)
				sets = sets + MCF_DATA_SEP_ROW;

			sets = sets + dataSet.m_sId
				+ MCF_DATA_SEP_FIELD + dataSet.m_sLabel
				+ MCF_DATA_SEP_FIELD + dataSet.m_sDescription
				+ MCF_DATA_SEP_FIELD + dataSet.Count().ToString();
		}

		string snapshots = "";

		array<string> names = {};
		MCF_Core_DataSnapshots.List(names);

		foreach (int j, string name : names)
		{
			if (j > 0)
				snapshots = snapshots + MCF_DATA_SEP_ROW;

			snapshots = snapshots + name;
		}

		return sets + MCF_DATA_SEP_SECTION + snapshots;
	}
}
