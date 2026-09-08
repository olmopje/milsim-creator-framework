//! Drives the Tick Manager automatically every frame -- the missing
//! piece that lets MCF_Core_TickCritical/TickCosmetic actually fire on
//! their own, instead of needing a manual Update() call from somewhere.
//!
//! Place on the same entity as (or link to) an MCF_Core_TickManagerComponent.

[ComponentEditorProps(category: "MCF/Core", description: "Drives MCF_Core_TickManagerComponent automatically every frame.")]
class MCF_Core_GameLoopComponentClass : ScriptComponentClass
{
}

class MCF_Core_GameLoopComponent : ScriptComponent
{
	protected MCF_Core_TickManagerComponent m_TickManager;

	override void EOnInit(IEntity owner)
	{
		m_TickManager = MCF_Core_TickManagerComponent.Cast(owner.FindComponent(MCF_Core_TickManagerComponent));
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_TickManager)
			m_TickManager.Update(timeSlice);
	}
}
