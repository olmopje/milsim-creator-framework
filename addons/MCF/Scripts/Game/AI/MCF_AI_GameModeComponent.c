//! The AI module's end of the game mode: hostility decay.
//!
//! Core used to carry the two hostility attributes and call
//! MCF_Hostility_Manager itself. The settings belong with the module that acts
//! on them, so they moved here and Core no longer names the class.
//!
//! Decay runs on every machine that has this component, exactly as before --
//! the hostility manager drives itself off the tick manager and holds no
//! authoritative state that a client could disagree about.

[ComponentEditorProps(category: "MCF/AI", description: "Optional automatic decay of hostility over time.")]
class MCF_AI_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_AI_GameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enable automatic Hostility decay.")]
	protected bool m_bEnableHostilityDecay;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Hostility decay rate per second, if enabled above.")]
	protected float m_fHostilityDecayRate;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		MCF_Core_EventManager.GetInstance().GetInvoker(MCF_Core_GameModeComponent.EVENT_MISSION_START).Insert(OnMissionStart);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(MCF_Core_GameModeComponent.EVENT_MISSION_START, "MCF_AI_GameModeComponent (start hostility decay)");
	}

	override void OnDelete(IEntity owner)
	{
		MCF_Core_EventManager.GetInstance().GetInvoker(MCF_Core_GameModeComponent.EVENT_MISSION_START).Remove(OnMissionStart);
		super.OnDelete(owner);
	}

	protected void OnMissionStart(Managed payload)
	{
		if (m_bEnableHostilityDecay)
			MCF_Hostility_Manager.GetInstance().StartAutoDecay(m_fHostilityDecayRate);
	}
}
