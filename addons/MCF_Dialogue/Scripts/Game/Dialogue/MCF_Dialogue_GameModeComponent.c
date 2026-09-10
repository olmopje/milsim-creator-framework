//! The dialogue module's end of the game mode: bringing back the conversations
//! a Game Master wrote in an earlier session.
//!
//! Core used to call MCF_Dialogue_Library.LoadRuntime() itself, which is a
//! dependency in the wrong direction -- Core has to be installable without the
//! dialogue module. It listens for Core's store-ready event instead.
//!
//! Loaded here rather than at library construction because the conversations
//! live in the same $profile: store as the tasks and the intel: text authored
//! during play, held by the server, and expected to still be there tomorrow.
//! None of it can be read before Core has opened that store.

[ComponentEditorProps(category: "MCF/Dialogue", description: "Restores conversations written in earlier sessions once the persistent store is up.")]
class MCF_Dialogue_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_Dialogue_GameModeComponent : SCR_BaseGameModeComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		MCF_Core_EventManager.GetInstance().GetInvoker(MCF_Core_GameModeComponent.EVENT_STORE_READY).Insert(OnPersistentStoreReady);

		// Declared here for the same reason the loading is: Core must be
		// installable without this module, so this module says what it owns.
		MCF_Core_DataSets.Register("conversations", "Conversations",
			"What Game Masters wrote for people to say, and what you can say back.",
			"conversations", "conversation.", "");

		MCF_Core_DataSets.GetOnReloaded().Insert(ReloadFromStore);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(MCF_Core_GameModeComponent.EVENT_STORE_READY, "MCF_Dialogue_GameModeComponent (restore authored conversations)");
	}

	override void OnDelete(IEntity owner)
	{
		MCF_Core_EventManager.GetInstance().GetInvoker(MCF_Core_GameModeComponent.EVENT_STORE_READY).Remove(OnPersistentStoreReady);
		MCF_Core_DataSets.GetOnReloaded().Remove(ReloadFromStore);
		super.OnDelete(owner);
	}

	//! Server only -- Core publishes this inside its own IsServer guard.
	protected void OnPersistentStoreReady(Managed payload)
	{
		MCF_Dialogue_Library.GetInstance().LoadRuntime();
	}

	//! The store changed underneath us -- a set was cleared, or a snapshot
	//! restored. Conversations are read from the library on demand, so there is
	//! nothing to tell the clients: reloading is the whole job.
	protected void ReloadFromStore()
	{
		if (!Replication.IsServer())
			return;

		MCF_Dialogue_Library.GetInstance().LoadRuntime();
	}
}
