//! What THIS player has opened on a device.
//!
//! WHY THIS IS NOT REPLICATED, AND MUST NOT BE. Two players who both pick up
//! the same phone have genuinely different answers to "have you read this".
//! A phone that marked itself read on the server the moment one player opened
//! it would steal the discovery from the second, who would find a device that
//! looks picked over before they have seen a word of it.
//!
//! So the authored side of "new" travels with the content (MCF_Device_Item's
//! m_bNew, replicated, the same for everyone) and this side stays here, on the
//! machine of the person doing the reading.
//!
//! WHERE IT LIVES. MCF_Core_PersistentStore, which already writes to
//! $profile: and already survives a restart and a mod update -- and which on a
//! client is the client's own profile, so nothing has to be sent anywhere.
//! Keys are namespaced so the mission's own data and this never meet.

class MCF_Device_ReadState
{
	//! Every key this class writes starts here, so a wipe is one call.
	protected static const string PREFIX = "dev.read.";

	//! One device's read log is keyed by THAT DEVICE, not by the profile it
	//! was built from.
	//!
	//! IT USED TO BE THE PROFILE ID, AND THAT WAS WRONG. Three phones on a
	//! table all carrying `smuggler_phone` then shared one read log: opening a
	//! message on the first marked it read on all three, which reads as the
	//! phones being one phone. They are not -- they are three objects a player
	//! can pick up separately, and each has been read or not on its own.
	//!
	//! The replication id is the one thing that names this object and no other.
	//! It is a session id: after a restart the same handset is a different
	//! number and its read log starts empty. That is the honest trade for not
	//! asking a mission maker to hand-name every prop, and a fresh session
	//! showing fresh phones is not the wrong answer.
	static string DeviceKey(MCF_Intel_CarrierComponent carrier)
	{
		if (!carrier)
			return "";

		RplId id = Replication.FindItemId(carrier);
		if (id != RplId.Invalid())
			return id.ToString();

		// Not replicated -- a single-player prop, or a device being previewed.
		// Fall back to what it is, which at least keeps one object consistent
		// with itself for as long as the screen is open.
		string name = carrier.GetDeviceName();
		if (!name.IsEmpty())
			return name;

		return "device";
	}

	static bool IsRead(string deviceKey, string itemKey)
	{
		if (deviceKey.IsEmpty() || itemKey.IsEmpty())
			return false;

		return MCF_Core_PersistentStore.GetInstance().Has(Key(deviceKey, itemKey));
	}

	//! \return True if this call is what changed it -- so the caller knows
	//! whether anything on screen needs redrawing.
	static bool MarkRead(string deviceKey, string itemKey)
	{
		if (deviceKey.IsEmpty() || itemKey.IsEmpty())
			return false;

		string key = Key(deviceKey, itemKey);
		if (MCF_Core_PersistentStore.GetInstance().Has(key))
			return false;

		MCF_Core_PersistentStore.GetInstance().Set(key, "1");
		MCF_Core_PersistentStore.GetInstance().Save();
		return true;
	}

	//! Everything on one device unread again. For a mission maker testing, and
	//! for the day a device is handed to a different player character.
	static void Forget(string deviceKey)
	{
		if (deviceKey.IsEmpty())
			return;

		MCF_Core_PersistentStore.GetInstance().RemoveByPrefix(PREFIX + deviceKey + ".");
		MCF_Core_PersistentStore.GetInstance().Save();
	}

	protected static string Key(string deviceKey, string itemKey)
	{
		return PREFIX + deviceKey + "." + itemKey;
	}
}
