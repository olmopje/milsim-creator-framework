//! MCF's own game-mode-extending component. Add this to your GameMode
//! entity to have MCF's managers start themselves automatically instead
//! of needing a manual kickoff call.
//!
//! Extends SCR_BaseGameModeComponent, which every vanilla game mode
//! (Conflict, Combat Ops, etc.) already supports adding components to,
//! confirmed via OnGameModeStart() -- "Called on every machine when game
//! mode starts."
//!
//! Also overrides the confirmed native OnControllableSpawned(IEntity) hook
//! to auto-register every newly spawned controllable entity with
//! MCF_Core_AutoWatcherRegistry, so Proximity/SpottedByPlayer triggers
//! work without a mission maker having to manually register watchers.
//! Note: this fires for AI-controlled entities too, not only players --
//! see MCF_Core_AutoWatcherRegistry's file header for that caveat.

[ComponentEditorProps(category: "MCF/Core", description: "Starts MCF's managers (AAR, Hostility decay) automatically when the game mode starts, and auto-registers spawned entities with proximity/spotted triggers.")]
class MCF_Core_GameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class MCF_Core_GameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Start the AAR/Debrief manager listening automatically.")]
	protected bool m_bEnableAAR;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Enable automatic Hostility decay.")]
	protected bool m_bEnableHostilityDecay;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.EditBox, desc: "Hostility decay rate per second, if enabled above.")]
	protected float m_fHostilityDecayRate;

	override void OnGameModeStart()
	{
		if (m_bEnableAAR)
			MCF_AAR_DebriefManager.GetInstance().StartListening();

		if (m_bEnableHostilityDecay)
			MCF_Hostility_Manager.GetInstance().StartAutoDecay(m_fHostilityDecayRate);
	}

	override void OnControllableSpawned(IEntity entity)
	{
		MCF_Core_AutoWatcherRegistry.GetInstance().NotifyNewControllable(entity);
	}
}
