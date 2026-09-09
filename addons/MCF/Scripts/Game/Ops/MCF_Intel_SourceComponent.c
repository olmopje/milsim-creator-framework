//! Turns something that happened into something the force can find out.
//!
//! This is the join between MCF's two halves. The trigger layer already
//! notices things -- a patrol walks into a zone, a POI reports, a body is
//! spotted -- and publishes events. The command centre already handles what a
//! force knows: intel objects lie in the world, a player carries one to the
//! board, a commander logs it, an order comes out of it. Until now nothing
//! connected the two, so intel had to be placed by hand before the mission
//! started and could never be a consequence of what players actually did.
//!
//! A Game Master's triggers are the mission's tasks; the intel those triggers
//! produce is what an in-game commander writes orders from. This component is
//! that sentence, made real.
//!
//! TWO MODES, AND THE DIFFERENCE MATTERS.
//!
//!   DROP spawns a physical object -- a letter, a phone, a map. Somebody has
//!   to find it, pick it up and walk it to the board. Nothing reaches the
//!   force until they do. This is the default, and it is the one that keeps
//!   the design honest: the carrying is the interesting part, and a mode that
//!   skips it is a mode that quietly deletes the mechanic.
//!
//!   SIGNAL writes straight to the board for one faction. No object, no
//!   carrying. This is for intel that genuinely arrives by wire -- a radio
//!   intercept, a report from a friendly unit, higher command passing
//!   something down. Used for anything a player could plausibly have walked
//!   to, it makes the whole intel system pointless, so it asks for a faction
//!   explicitly rather than defaulting to one.
//!
//! ONE-SHOT BY DEFAULT. A proximity trigger fires every time somebody walks
//! through it. Intel that re-spawns on every pass leaves a pile of identical
//! letters on the ground and a board full of duplicate reports, which reads
//! as a bug even when it is exactly what was configured.

enum MCF_EIntelSourceMode
{
	//! Spawn an object somebody has to find and carry.
	DROP,
	//! Enter it on the board directly, for one faction.
	SIGNAL
}

[ComponentEditorProps(category: "MCF/Intel", description: "Produces intel when an MCF event fires: drops a document in the world, or signals it straight to a faction's board.")]
class MCF_Intel_SourceComponentClass : ScriptComponentClass
{
}

class MCF_Intel_SourceComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that produces this intel. Leave empty and nothing will ever fire it.")]
	protected string m_sTriggerEvent;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EIntelSourceMode), desc: "DROP puts an object in the world that somebody must carry to the board. SIGNAL enters it on a faction's board directly.")]
	protected MCF_EIntelSourceMode m_eMode;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Produce only the first time the event fires. Leave on unless you really want one piece of intel per trigger crossing.")]
	protected bool m_bOnce;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "DROP: the object to spawn. Needs an MCF_Intel_CarrierComponent on it if the text below is to be used.")]
	protected ResourceName m_sPrefab;

	[Attribute(defvalue: "0 0 0", uiwidget: UIWidgets.EditBox, desc: "DROP: where the object appears, relative to this entity. Use it to put the drop on a table rather than inside it.")]
	protected vector m_vSpawnOffset;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "SIGNAL: which faction learns this. Faction key, e.g. US or USSR. Required -- intel with no side goes to everyone.")]
	protected string m_sFactionKey;

	[Attribute(defvalue: "Document", uiwidget: UIWidgets.EditBox, desc: "What this came off: 'Handwritten letter', 'Radio intercept', 'Captured map'. Provenance is half of what makes intel judgeable.")]
	protected string m_sSource;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Subject line. Leave the text fields empty in DROP mode to keep whatever the prefab already says.")]
	protected string m_sHeading;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "When the source says it happened. Free text, as a person would write it.")]
	protected string m_sTimestamp;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "The content itself.")]
	protected string m_sBody;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event published once this has produced its intel, so a chain can continue. Optional.")]
	protected string m_sProducedEvent;

	protected ScriptInvoker m_TriggerInvoker;
	protected int m_iProduceCount;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		// Server only, like every other node that reacts to the event bus.
		// Spawning on a client would put an object in the world that no other
		// machine knows about, and logging on a client is refused by the store
		// anyway.
		if (!Replication.IsServer())
			return;

		if (m_sTriggerEvent.IsEmpty())
		{
			MCF_Core_Log.Warn("IntelSource on '" + owner.GetName() + "' has no trigger event -- it will never produce anything");
			return;
		}

		m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
		m_TriggerInvoker.Insert(OnTriggerEvent);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_Intel_SourceComponent");

		if (!m_sProducedEvent.IsEmpty())
			MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sProducedEvent);

		MCF_Core_Log.Debug("IntelSource init, mode=" + m_eMode.ToString() + " triggered by '" + m_sTriggerEvent + "'");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTriggerEvent);
	}

	protected void OnTriggerEvent(Managed payload)
	{
		Produce();
	}

	//! Makes the intel exist. Reachable from script as well as from the bus,
	//! so a scenario can force a drop without inventing an event for it.
	void Produce()
	{
		if (m_bOnce && m_iProduceCount > 0)
			return;

		bool produced;

		if (m_eMode == MCF_EIntelSourceMode.SIGNAL)
			produced = Signal();
		else
			produced = Drop();

		if (!produced)
			return;

		m_iProduceCount++;

		if (!m_sProducedEvent.IsEmpty())
			MCF_Core_EventManager.GetInstance().Publish(m_sProducedEvent, this);
	}

	// ------------------------------------------------------------------ DROP

	//! Spawns the object and, if this node was given text, writes that text
	//! onto it. \return Whether anything was actually put in the world.
	protected bool Drop()
	{
		if (m_sPrefab.IsEmpty())
		{
			MCF_Core_Log.Warn("IntelSource in DROP mode has no prefab set -- nothing to drop");
			return false;
		}

		Resource resource = Resource.Load(m_sPrefab);
		if (!resource || !resource.IsValid())
		{
			MCF_Core_Log.Warn("IntelSource could not load prefab '" + m_sPrefab + "'");
			return false;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return false;

		vector transform[4];
		owner.GetWorldTransform(transform);
		transform[3] = transform[3] + m_vSpawnOffset;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;

		IEntity spawned = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), params);
		if (!spawned)
		{
			MCF_Core_Log.Warn("IntelSource failed to spawn '" + m_sPrefab + "'");
			return false;
		}

		// Text is optional here. Left empty, the object keeps whatever the
		// prefab already says -- which is the right behaviour when a mission
		// maker has built a specific letter and just wants it to appear.
		if (!m_sHeading.IsEmpty() || !m_sBody.IsEmpty())
		{
			MCF_Intel_CarrierComponent carrier = MCF_Intel_CarrierComponent.Cast(spawned.FindComponent(MCF_Intel_CarrierComponent));

			if (carrier)
				carrier.SetSinglePageFromServer(m_sSource, m_sHeading, m_sTimestamp, m_sBody);
			else
				MCF_Core_Log.Warn("IntelSource wrote text for '" + m_sPrefab + "' but that prefab has no MCF_Intel_CarrierComponent -- the object dropped, the words did not");
		}

		MCF_Core_Log.Debug("IntelSource dropped '" + m_sPrefab + "' at " + transform[3].ToString());
		return true;
	}

	// ---------------------------------------------------------------- SIGNAL

	//! Enters the intel on one faction's board without anybody carrying it.
	//! \return Whether a record was created.
	protected bool Signal()
	{
		if (m_sFactionKey.IsEmpty())
		{
			// Refused rather than defaulted. A record with no faction is
			// visible to everyone, so guessing here would hand the enemy the
			// intel and look like a filtering bug rather than a missing
			// attribute.
			MCF_Core_Log.Warn("IntelSource in SIGNAL mode has no faction key -- refusing to log intel that everyone would see");
			return false;
		}

		if (m_sHeading.IsEmpty() && m_sBody.IsEmpty())
		{
			MCF_Core_Log.Warn("IntelSource in SIGNAL mode has nothing to say -- set a heading or a body");
			return false;
		}

		// Player id 0: nobody logged this, it arrived. The board shows the
		// source line, which is what a reader actually judges it by.
		MCF_Intel_Record record = MCF_Intel_Store.GetInstance().Log(m_sSource, m_sHeading, m_sTimestamp, m_sBody, 0, m_sFactionKey);

		if (!record)
			return false;

		MCF_Core_Log.Debug("IntelSource signalled " + record.Describe() + " to " + m_sFactionKey);

		RefreshEveryone();
		return true;
	}

	//! Pushes the stores out again so the new record reaches the boards that
	//! are entitled to it. Reuses the same refresh the player controller uses
	//! after a change, so there is one path and not two that can drift.
	protected void RefreshEveryone()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		MCF_Ops_GameModeComponent ops = MCF_Ops_GameModeComponent.Cast(gameMode.FindComponent(MCF_Ops_GameModeComponent));
		if (!ops)
		{
			MCF_Core_Log.Warn("no MCF_Ops_GameModeComponent on the game mode -- signalled intel will not reach anyone until the next refresh");
			return;
		}

		ops.RefreshTasksForAllPlayers();
	}

	int GetProduceCount()
	{
		return m_iProduceCount;
	}
}
