//! Gives the people standing here something to say.
//!
//! Every character in the game carries a dialogue component now, and every one
//! of them is silent until assigned a conversation. This is what assigns one:
//! place the node, name a conversation from the library, and everybody within
//! the radius becomes talkable.
//!
//! WHY A NODE AND NOT AN ATTRIBUTE ON THE PERSON. There is one prefab behind
//! every civilian in the game, and a Game Master cannot type into a script
//! attribute at runtime -- editor attributes pack into a vector and carry
//! twelve bytes, so they hold a number and never a sentence. That is the same
//! constraint that made the intel system need its own authoring screen. A node
//! placed in the world sidesteps it: the words live in the library, and what
//! the Game Master places is the decision about *who* says them.
//!
//! RADIUS, NOT A LINK. Deliberately. A mission maker drops this in a village
//! and every civilian in it can be spoken to, without wiring each one up by
//! hand -- and ambient civilians who wander in later are picked up too, if the
//! node is fired again by an event.
//!
//! DISPOSITION IS SET HERE TOO. A frightened village and a friendly one are
//! the same conversation with different numbers, so the numbers belong with
//! the placement rather than with the words.

[ComponentEditorProps(category: "MCF/AI", description: "Assigns a library conversation to everybody within a radius, at start or on an MCF event.")]
class MCF_Dialogue_AssignComponentClass : ScriptComponentClass
{
}

class MCF_Dialogue_AssignComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Conversation id from Configs/Dialogue/MCF_Conversations.conf.")]
	protected string m_sConversationId;

	[Attribute(defvalue: "15", uiwidget: UIWidgets.Slider, params: "1 200 1", desc: "Everybody within this many metres of the node is given the conversation.")]
	protected float m_fRadius;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "MCF event that triggers the assignment. Leave empty to assign once when the mission starts.")]
	protected string m_sTriggerEvent;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Assign only the first time. Turn off to pick up people who arrive later, by firing the event again.")]
	protected bool m_bOnce;

	[Attribute(defvalue: "-1", uiwidget: UIWidgets.Slider, params: "-1 100 1", desc: "Trust to give these people, or -1 to leave whatever they already have.")]
	protected float m_fTrust;

	[Attribute(defvalue: "-1", uiwidget: UIWidgets.Slider, params: "-1 100 1", desc: "Fear to give these people, or -1 to leave whatever they already have.")]
	protected float m_fFear;

	protected ScriptInvoker m_TriggerInvoker;
	protected int m_iAssignCount;

	//! Collected by the sphere query callback, which cannot return a value.
	protected ref array<IEntity> m_aFound = {};

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		// Server only. Assignment is replicated state; a client doing it would
		// diverge from the machine that decides what anybody says.
		if (!Replication.IsServer())
			return;

		if (m_sConversationId.IsEmpty())
		{
			MCF_Core_Log.Warn("dialogue assign node on '" + owner.GetName() + "' names no conversation -- it will do nothing");
			return;
		}

		if (m_sTriggerEvent.IsEmpty())
		{
			// No event means "these people can always be spoken to", which is
			// the common case: a village that is simply inhabited.
			Assign();
			return;
		}

		m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
		m_TriggerInvoker.Insert(OnTriggerEvent);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_Dialogue_AssignComponent");

		MCF_Core_Log.Debug("dialogue assign node ready, '" + m_sConversationId + "' within " + m_fRadius.ToString() + "m on '" + m_sTriggerEvent + "'");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTriggerEvent);
	}

	protected void OnTriggerEvent(Managed payload)
	{
		Assign();
	}

	//! Hands the conversation to everybody in range. Reachable from script so
	//! a scenario can force it without inventing an event.
	void Assign()
	{
		if (m_bOnce && m_iAssignCount > 0)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		BaseWorld world = owner.GetWorld();
		if (!world)
			return;

		m_aFound.Clear();
		world.QueryEntitiesBySphere(owner.GetOrigin(), m_fRadius, CollectEntity, null, EQueryEntitiesFlags.ALL);

		int given;

		foreach (IEntity found : m_aFound)
		{
			MCF_Dialogue_Component dialogue = MCF_Dialogue_Component.Cast(found.FindComponent(MCF_Dialogue_Component));
			if (!dialogue)
				continue;

			dialogue.SetConversation(m_sConversationId);

			MCF_AI_DispositionComponent disposition = MCF_AI_DispositionComponent.Cast(found.FindComponent(MCF_AI_DispositionComponent));
			if (disposition)
			{
				// -1 means "leave it alone", so a node can set the mood
				// without flattening people a previous scene already moved.
				if (m_fTrust >= 0)
					disposition.SetTrust(m_fTrust);

				if (m_fFear >= 0)
					disposition.SetFear(m_fFear);
			}

			given++;
		}

		m_iAssignCount++;

		MCF_Core_Log.Debug("dialogue assign gave '" + m_sConversationId + "' to " + given.ToString() + " person(s) within " + m_fRadius.ToString() + "m");
	}

	//! Sphere-query callback. Collects rather than filters, because the query
	//! runs over everything in the volume and deciding what counts is cheaper
	//! once, afterwards, than inside a callback that cannot report why.
	protected bool CollectEntity(IEntity entity)
	{
		if (entity)
			m_aFound.Insert(entity);

		return true;
	}

	int GetAssignCount()
	{
		return m_iAssignCount;
	}
}
