//! "Spotted by player" trigger -- place on the entity/NPC that needs to be
//! seen before a scripted moment is allowed to matter (e.g. don't spring
//! an ambush reveal, don't let a debrief count intel as "delivered",
//! unless a player actually had eyes on it). Fires once a registered
//! watcher (a player-controlled entity) has this entity within range and
//! within their forward-facing view cone.
//!
//! Same distance+angle approximation as MCF_AI_ComplianceComponent and
//! MCF_Obj_ConeDetectionTriggerComponent -- not true line of sight, can't
//! tell if a wall is in the way. The cone here should be set to a wide,
//! natural field-of-view angle (wider than a weapon-aim cone), since this
//! is approximating "did you look this way," not "are you aiming at it."
//!
//! Watchers can be registered manually via RegisterWatcher(), or
//! automatically -- this component registers itself with
//! MCF_Core_AutoWatcherRegistry, which MCF_Core_GameModeComponent uses to
//! auto-add every newly spawned controllable entity (see that file for
//! the "not player-only yet" caveat).

[ComponentEditorProps(category: "MCF/Objective", description: "Fires once a registered player has this entity within view range/angle -- use to gate events on 'has actually been seen'.")]
class MCF_Obj_SpottedByPlayerComponentClass : ScriptComponentClass
{
}

class MCF_Obj_SpottedByPlayerComponent : ScriptComponent
{
	[Attribute(defvalue: "100", uiwidget: UIWidgets.EditBox, desc: "Maximum distance in metres at which a watcher can spot this entity.")]
	protected float m_fMaxRange;

	[Attribute(defvalue: "45", uiwidget: UIWidgets.EditBox, desc: "Half-angle of the watcher's view cone in degrees. Wider than a weapon-aim cone -- this approximates natural field of view, not precise aim.")]
	protected float m_fViewHalfAngleDegrees;

	[Attribute(defvalue: "MCF_Obj_PlayerSpottedTarget", uiwidget: UIWidgets.EditBox, desc: "Event name published once a watcher spots this entity.")]
	protected string m_sSpottedEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "If true, only fires once.")]
	protected bool m_bTriggerOnce;

	protected ref array<IEntity> m_aWatchers;
	protected bool m_bHasTriggered;
	protected IEntity m_Owner;
	protected ScriptInvoker m_TickInvoker;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_Owner = owner;
		m_aWatchers = new array<IEntity>();

		MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sSpottedEvent);

		// Detection is authoritative and runs on the server only. Without
		// this guard every client evaluates its own copy against its own
		// view of the world, publishes its own events and keeps its own
		// trigger-once state -- so "fires once" would mean once per machine,
		// and clients could disagree about whether it fired at all.
		// See docs/research/multiplayer-and-audience.md.
		if (!Replication.IsServer())
			return;

		m_TickInvoker = MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Core_TickCritical");
		m_TickInvoker.Insert(OnTickCritical);

		MCF_Core_AutoWatcherRegistry.GetInstance().RegisterSpottedTrigger(this);

		MCF_Core_Log.Debug("SpottedByPlayer init, range=" + m_fMaxRange.ToString() + " event=" + m_sSpottedEvent);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TickInvoker)
			m_TickInvoker.Remove(OnTickCritical);

		MCF_Core_AutoWatcherRegistry.GetInstance().UnregisterSpottedTrigger(this);
	}

	//! Adds a player-controlled entity as a potential spotter of this one.
	void RegisterWatcher(IEntity watcher)
	{
		if (watcher && m_aWatchers.Find(watcher) == -1)
			m_aWatchers.Insert(watcher);
	}

	void UnregisterWatcher(IEntity watcher)
	{
		int index = m_aWatchers.Find(watcher);
		if (index != -1)
			m_aWatchers.Remove(index);
	}

	protected void OnTickCritical(Managed payload)
	{
		if ((m_bTriggerOnce && m_bHasTriggered) || !m_Owner)
			return;

		vector ownPosition = m_Owner.GetOrigin();

		foreach (IEntity watcher : m_aWatchers)
		{
			if (!watcher)
				continue;

			vector toOwner = ownPosition - watcher.GetOrigin();
			float distance = toOwner.Length();
			if (distance > m_fMaxRange)
				continue;

			toOwner.Normalize();
			vector watcherForward = watcher.GetTransformAxis(2);
			float angleDegrees = Math.Acos(Math.Clamp(vector.Dot(watcherForward, toOwner), -1, 1)) * Math.RAD2DEG;

			if (angleDegrees <= m_fViewHalfAngleDegrees)
			{
				m_bHasTriggered = true;
				MCF_Core_Log.Debug("SpottedByPlayer FIRED, publishing " + m_sSpottedEvent);
				MCF_Core_EventManager.GetInstance().Publish(m_sSpottedEvent, this);
				return;
			}
		}
	}

	bool HasBeenSpotted()
	{
		return m_bHasTriggered;
	}
}
