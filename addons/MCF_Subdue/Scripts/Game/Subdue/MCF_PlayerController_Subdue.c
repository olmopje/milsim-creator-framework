//! The subdue module's end of the player controller: shouting at people.
//!
//! See MCF_PlayerController_Core.c for why any of this lives on the player
//! controller and for the rule about chain order across these files.
//!
//! Only the fact of the shout and whether the weapon was up travel. Who heard
//! it is the server's to work out -- a client that sent a list of who should
//! obey would be deciding the outcome.

modded class SCR_PlayerController
{
	//! Client side. The player shouted.
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
