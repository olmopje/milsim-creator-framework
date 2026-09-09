//! Shouting at people: the player's end of it.
//!
//! WHY INPUT AND NOT A USER ACTION ON A PERSON. Shouting "get on the ground"
//! is something you do to a room, not to an individual you are hovering over.
//! A list of orders on one civilian reads as a menu; a key that everyone
//! within earshot has to answer for themselves reads as the thing it is.
//!
//! ADDING A KEY IS NOT SUPPORTED IN THE USUAL SENSE. Every input action in the
//! game lives in one 241 KB file, Configs/System/chimeraInputCommon.conf, and
//! no shipped mod anywhere on this machine adds one. What made it possible is
//! that vanilla itself uses the config system's append operator inside that
//! file -- `ActionRefs +{ "MenuCalibrateMotionControl" }` on a context -- so a
//! GUID override carrying only `Contexts +{ ... }` adds to the set rather than
//! replacing it, exactly as MCF's chimeraMenus.conf override adds a menu
//! without losing the other fifty-eight.
//!
//! THE CONTEXT HAS TO BE RE-ACTIVATED EVERY FRAME. MCF_CharacterContext is
//! declared with Flags 0x2, which is what vanilla's own conditional contexts
//! use, and those are activated from OnPrepareControls each frame. A context
//! that is never activated is a key that never fires, silently.
//!
//! THIS FILE IS DELIBERATELY THIN. It proves the key arrives and nothing else.
//! What a shout does to the people who hear it is a separate problem and does
//! not belong in the input layer.

modded class SCR_CharacterControllerComponent
{
	protected static const string MCF_CONTEXT = "MCF_CharacterContext";
	protected static const string MCF_ACTION_SURRENDER = "MCF_ShoutSurrender";
	protected static const string MCF_ACTION_STAY_BACK = "MCF_ShoutStayBack";

	//! Listeners are added when the local player takes control and removed
	//! when they lose it, which is the pattern vanilla uses for every
	//! gameplay keybind on this class.
	override protected void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		super.OnControlledByPlayer(owner, controlled);

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		if (controlled && owner == SCR_PlayerController.GetLocalControlledEntity())
		{
			input.AddActionListener(MCF_ACTION_SURRENDER, EActionTrigger.DOWN, MCF_OnShoutSurrender);
			input.AddActionListener(MCF_ACTION_STAY_BACK, EActionTrigger.DOWN, MCF_OnShoutStayBack);

			MCF_Core_Log.Debug("shout keys bound");
			return;
		}

		input.RemoveActionListener(MCF_ACTION_SURRENDER, EActionTrigger.DOWN, MCF_OnShoutSurrender);
		input.RemoveActionListener(MCF_ACTION_STAY_BACK, EActionTrigger.DOWN, MCF_OnShoutStayBack);
	}

	//! Keeps MCF's context alive. Without this the actions exist and never
	//! fire, with nothing anywhere to say why.
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		super.OnPrepareControls(owner, am, dt, player);

		if (player && am)
			am.ActivateContext(MCF_CONTEXT);
	}

	protected void MCF_OnShoutSurrender(float value = 0.0, EActionTrigger trigger = 0)
	{
		MCF_Shout(MCF_EShout.SURRENDER);
	}

	protected void MCF_OnShoutStayBack(float value = 0.0, EActionTrigger trigger = 0)
	{
		MCF_Shout(MCF_EShout.STAY_BACK);
	}

	//! Makes the noise here and asks the server to work out who heard it.
	//!
	//! THE SOUND AND THE GESTURE ARE NOT VANILLA'S IDEA OF A SURRENDER CALL,
	//! because the game does not have one. The whole Sounds tree was searched:
	//! the speech categories are Actions, Combat, Movement, Reports and
	//! Confirmations, and the character voices hold breathing, pain and effort
	//! and no words at all. There is no civilian, warning, challenge or detain
	//! line anywhere.
	//!
	//! SOUND_CP_STOP is "Halt!", shipped for ordering your own squad to stop,
	//! and gesture 2 is the raised fist that goes with it -- vanilla pairs
	//! exactly those two in Configs/Commanding/Commands.conf. Close enough to
	//! read correctly at a distance, and real audio rather than silence. A
	//! proper "get on the ground" needs its own samples and an .acp bank; the
	//! call below is the only line that would change.
	protected void MCF_Shout(MCF_EShout shout)
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		SCR_CommunicationSoundComponent voice = SCR_CommunicationSoundComponent.Cast(owner.FindComponent(SCR_CommunicationSoundComponent));
		if (voice)
			voice.SoundEventPriority("SOUND_CP_STOP", 50);

		// The raised fist. Skipped while aiming down sights, the way vanilla
		// skips command gestures, because the arm cannot do both.
		if (!IsWeaponADS())
			TryStartCharacterGesture(ECharacterGestures.COMMAND_STOP, 2000);

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestShout(shout, IsWeaponRaised());
	}
}
