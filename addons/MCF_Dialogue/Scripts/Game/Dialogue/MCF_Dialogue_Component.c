//! A person you can talk to. Put this on a civilian, a prisoner, a shopkeeper.
//!
//! This is the piece MCF was missing. Everything else was already here: the
//! trigger layer notices things, the event bus carries them, intel objects get
//! carried to a board, orders come out of it. But the most common way a real
//! mission gives players information is that somebody tells them, and until
//! now there was no way for anybody to tell them anything beyond a single
//! canned line.
//!
//! HOW IT CONNECTS TO EVERYTHING ELSE. A reply publishes an MCF event. That is
//! the only thing a reply can do to the world. Hang an Intel Source on that
//! event and the conversation produces a document, or signals a report
//! straight to a faction's board. Hang a Recipe on it and it starts a scene.
//! Hang an Objective on it and it completes a task. None of those had to know
//! that conversations exist.
//!
//! THE SERVER DECIDES EVERYTHING. The component is authored identically on
//! every machine, because it is prefab data -- but only the server reads
//! disposition, evaluates requirements, applies effects and advances the
//! conversation. A client is sent one screen at a time and sends back an index
//! into it. See MCF_Dialogue_View for why that is the shape.
//!
//! FLAGS ARE PER PERSON AND PER MISSION. Set on the component, not on the
//! player: what this man has admitted to is a fact about him. Two players who
//! talk to him in turn are talking to the same man, and the second one arrives
//! after the first one's conversation. They are not persisted across a restart
//! -- that is the same gap dropped intel objects have, and it is written down
//! rather than discovered.

[ComponentEditorProps(category: "MCF/AI", description: "Makes this person talkable: a branching conversation gated on trust and fear, whose replies publish MCF events.")]
class MCF_Dialogue_ComponentClass : ScriptComponentClass
{
}

class MCF_Dialogue_Component : ScriptComponent
{
	[Attribute(defvalue: "Civilian", uiwidget: UIWidgets.EditBox, desc: "Name shown at the top of the conversation. 'Farmer', 'Wounded soldier', 'Ahmed'.")]
	protected string m_sSpeakerName;

	[Attribute(defvalue: "Talk", uiwidget: UIWidgets.EditBox, desc: "Verb on the interaction prompt: Talk, Question, Interrogate.")]
	protected string m_sActionVerb;

	[Attribute(defvalue: "start", uiwidget: UIWidgets.EditBox, desc: "Node the conversation opens on.")]
	protected string m_sStartNodeId;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.Slider, params: "1 50 1", desc: "How close a player must be to talk, in metres. Checked on the server so a client cannot claim to be here.")]
	protected float m_fTalkRange;

	[Attribute(desc: "The conversation itself.")]
	protected ref array<ref MCF_Dialogue_Node> m_aNodes;
	//! Which library conversation this person is running, empty for nobody.
	//!
	//! REPLICATED, because the client decides whether to draw the talk prompt
	//! and therefore has to know this person is talkable at all. An RPC would
	//! reach whoever was listening at the time and miss everyone who joins or
	//! streams the character in afterwards -- the exact bug the intel carrier
	//! already had and fixed the same way.
	//!
	//! The client resolving the library also means a determined player could
	//! read every conversation out of the mod files. That is the same honest
	//! limit intel has: secrecy in the fiction, not against a datamine. What
	//! actually matters is enforced server-side -- which replies are available
	//! depends on trust, fear and flags the client never sees.
	[RplProp(onRplName: "OnConversationAssigned")]
	protected string m_sConversationId;

	//! The assigned speaker's name and verb, replicated alongside the id.
	//!
	//! WHY THESE TRAVEL AND THE NODES DO NOT. The client draws the prompt --
	//! "Talk: Farmer" -- so it needs the name. It does not need the tree, and
	//! should not have it: the whole point of sending one screen at a time is
	//! that a client cannot read ahead. A Game Master can also write a
	//! conversation mid-mission, so the library on a client is not even the
	//! same as the server's; the name has to be told, not looked up.
	[RplProp()]
	protected string m_sAssignedSpeaker;

	[RplProp()]
	protected string m_sAssignedVerb;

	//! The conversation used when this person has been subdued.
	//!
	//! A SECOND SLOT, not a flag on the first. The same farmer has a normal
	//! conversation and, if you put a rifle in his face and tie his hands, a
	//! different one -- different things said, different things sayable. One
	//! slot with a required state would force a mission maker to choose which
	//! of the two the person gets; two slots let them have both, and a person
	//! with only one still works.
	[RplProp(onRplName: "OnConversationAssigned")]
	protected string m_sInterrogationId;

	[RplProp()]
	protected string m_sInterrogationSpeaker;

	protected ref MCF_Dialogue_Conversation m_AssignedInterrogation;

	//! The resolved library entry. Rebuilt on every machine from the id.
	protected ref MCF_Dialogue_Conversation m_Assigned;


	//! Flags set so far, by name. Server-side only, like everything else that
	//! decides an outcome.
	//! Initialised inline, not in a constructor. A ScriptComponent's
	//! constructor is not free to redeclare -- the engine's own takes
	//! arguments, and a bare one fails to compile with "Overloaded function
	//! not compatible", which is a message that names the constructor rather
	//! than the reason.
	protected ref array<string> m_aFlags = {};

	//! Which node each player is currently on, so two players can hold
	//! separate positions in the same conversation without one advancing the
	//! other. Parallel arrays rather than a map because Enforce maps of
	//! int->string are more trouble than two Finds on a list this short.
	//! Whether we took this character off its behaviour tree to hold the
	//! conversation, so it is handed back exactly once.
	protected bool m_bAiSuspended;

	//! Whether we put this character into the conversational loiter stance.
	protected bool m_bLoitering;

	protected ref array<int> m_aTalkerIds = {};
	protected ref array<string> m_aTalkerNodes = {};

	//! Whether this person counts as subdued, and so is interrogated rather
	//! than talked to.
	//!
	//! COMPLIANT is enough. Somebody with their hands up is not going to walk
	//! off mid-sentence, and requiring restraint would mean carrying zip ties
	//! to ask a question.
	bool IsSubdued()
	{
		MCF_AI_DispositionComponent disposition = GetDisposition();
		if (!disposition)
			return false;

		return disposition.GetCaptiveState() >= MCF_ECaptiveState.COMPLIANT;
	}

	//! Which of the two conversations applies right now.
	protected bool UseInterrogation()
	{
		return IsSubdued() && !m_sInterrogationId.IsEmpty();
	}

	//! Server side. Sets the conversation used once somebody has been subdued.
	void SetInterrogation(string conversationId)
	{
		if (!Replication.IsServer())
			return;

		m_sInterrogationId = conversationId;
		m_AssignedInterrogation = MCF_Dialogue_Library.GetInstance().Find(conversationId);

		if (m_AssignedInterrogation)
			m_sInterrogationSpeaker = m_AssignedInterrogation.m_sSpeakerName;
		else
			m_sInterrogationSpeaker = string.Empty;

		Replication.BumpMe();

		MCF_Core_Log.Debug("'" + GetSpeakerName() + "' assigned interrogation '" + conversationId + "'");
	}

	string GetInterrogationId()
	{
		return m_sInterrogationId;
	}

	//! Server side. Gives this person a conversation out of the library, or
	//! takes it away with an empty id.
	void SetConversation(string conversationId)
	{
		if (!Replication.IsServer())
			return;

		m_sConversationId = conversationId;
		Resolve();

		// Copied out for the clients, which have no reliable library of their
		// own once a Game Master starts writing conversations at runtime.
		if (m_Assigned)
		{
			m_sAssignedSpeaker = m_Assigned.m_sSpeakerName;
			m_sAssignedVerb = m_Assigned.m_sActionVerb;
		}
		else
		{
			m_sAssignedSpeaker = string.Empty;
			m_sAssignedVerb = string.Empty;
		}

		Replication.BumpMe();

		MCF_Core_Log.Debug("'" + GetSpeakerName() + "' assigned conversation '" + conversationId + "'");
	}

	//! Which library conversation this person is running, empty for none.
	string GetConversationId()
	{
		return m_sConversationId;
	}

	protected void OnConversationAssigned()
	{
		Resolve();
	}

	protected void Resolve()
	{
		m_Assigned = MCF_Dialogue_Library.GetInstance().Find(m_sConversationId);
		m_AssignedInterrogation = MCF_Dialogue_Library.GetInstance().Find(m_sInterrogationId);
	}

	//! The library entry in play right now -- the interrogation if this person
	//! has been subdued and has one, otherwise the ordinary conversation.
	protected MCF_Dialogue_Conversation Current()
	{
		if (UseInterrogation())
			return m_AssignedInterrogation;

		return m_Assigned;
	}

	//! The library entry wins over anything authored on the prefab. A prefab
	//! can still carry its own conversation -- MCF's own talkable props do --
	//! but a person assigned one at runtime is running that one.
	string GetSpeakerName()
	{
		// The replicated copies first: on a client they are the only ones that
		// are certainly right.
		if (UseInterrogation() && !m_sInterrogationSpeaker.IsEmpty())
			return m_sInterrogationSpeaker;

		if (!m_sAssignedSpeaker.IsEmpty())
			return m_sAssignedSpeaker;

		MCF_Dialogue_Conversation current = Current();
		if (current && !current.m_sSpeakerName.IsEmpty())
			return current.m_sSpeakerName;

		return m_sSpeakerName;
	}

	//! The verb on the prompt. A subdued person is questioned, not chatted to,
	//! and the prompt says so without a mission maker having to set anything.
	string GetActionVerb()
	{
		if (UseInterrogation())
		{
			MCF_Dialogue_Conversation interrogation = Current();
			if (interrogation && !interrogation.m_sActionVerb.IsEmpty())
				return interrogation.m_sActionVerb;

			return "Interrogate";
		}

		if (!m_sAssignedVerb.IsEmpty())
			return m_sAssignedVerb;

		if (m_Assigned && !m_Assigned.m_sActionVerb.IsEmpty())
			return m_Assigned.m_sActionVerb;

		if (m_sActionVerb.IsEmpty())
			return "Talk";

		return m_sActionVerb;
	}

	protected string GetStartNodeId()
	{
		MCF_Dialogue_Conversation current = Current();

		if (current && !current.m_sStartNodeId.IsEmpty())
			return current.m_sStartNodeId;

		return m_sStartNodeId;
	}

	//! Whether there is anything to say at all.
	//!
	//! This is what keeps an untouched mission untouched: every character in
	//! the game now carries this component, and every one of them answers
	//! false here until somebody assigns a conversation. No prompt, no change.
	bool HasConversation()
	{
		// An assigned id is enough. The client is deliberately not given the
		// node tree, so it cannot check the start node exists -- and it does
		// not need to: the server checks that when the conversation opens and
		// says "They have nothing to say." if the author left it broken.
		if (UseInterrogation())
			return true;

		if (!m_sConversationId.IsEmpty())
			return true;

		return GetNode(GetStartNodeId()) != null;
	}

	float GetTalkRange()
	{
		return m_fTalkRange;
	}

	// ------------------------------------------------------------- authoring

	//! The nodes in play -- the assigned library conversation if there is one,
	//! otherwise whatever the prefab authored.
	protected array<ref MCF_Dialogue_Node> GetNodes()
	{
		MCF_Dialogue_Conversation current = Current();

		if (current && current.m_aNodes && !current.m_aNodes.IsEmpty())
			return current.m_aNodes;

		return m_aNodes;
	}

	protected MCF_Dialogue_Node GetNode(string nodeId)
	{
		if (nodeId.IsEmpty())
			return null;

		array<ref MCF_Dialogue_Node> nodes = GetNodes();
		if (!nodes)
			return null;

		foreach (MCF_Dialogue_Node node : nodes)
		{
			if (node && node.m_sId == nodeId)
				return node;
		}

		return null;
	}

	// -------------------------------------------------------- server: state

	protected string GetCurrentNodeId(int playerId)
	{
		int index = m_aTalkerIds.Find(playerId);
		if (index < 0)
			return "";

		return m_aTalkerNodes[index];
	}

	protected void SetCurrentNodeId(int playerId, string nodeId)
	{
		int index = m_aTalkerIds.Find(playerId);

		if (index < 0)
		{
			m_aTalkerIds.Insert(playerId);
			m_aTalkerNodes.Insert(nodeId);
			return;
		}

		m_aTalkerNodes[index] = nodeId;
	}

	protected void ForgetTalker(int playerId)
	{
		int index = m_aTalkerIds.Find(playerId);
		if (index < 0)
			return;

		m_aTalkerIds.Remove(index);
		m_aTalkerNodes.Remove(index);
	}

	protected MCF_AI_DispositionComponent GetDisposition()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		return MCF_AI_DispositionComponent.Cast(owner.FindComponent(MCF_AI_DispositionComponent));
	}

	// ------------------------------------------------------- server: opening

	//! Server side. Starts the conversation for one player.
	//! \return The first screen, or null if there is nothing to say.
	MCF_Dialogue_View Begin(int playerId, IEntity listener)
	{
		if (!Replication.IsServer())
			return null;

		FaceTowards(listener);

		MCF_Dialogue_Node node = GetNode(GetStartNodeId());
		if (!node)
		{
			MCF_Core_Log.Warn("dialogue on '" + GetSpeakerName() + "' has no node named '" + GetStartNodeId() + "' -- nothing to open");
			return null;
		}

		SetCurrentNodeId(playerId, node.m_sId);

		MCF_AI_DispositionComponent disposition = GetDisposition();
		if (disposition)
			MCF_Core_Log.Debug("player " + playerId.ToString() + " opens dialogue with '" + GetSpeakerName() + "', " + disposition.Describe());
		else
			MCF_Core_Log.Debug("player " + playerId.ToString() + " opens dialogue with '" + GetSpeakerName() + "' (no disposition component -- every reply is available)");

		return BuildView(node);
	}

	//! Server side. Applies a reply and returns what comes next.
	//!
	//! \param playerId Who is speaking, taken from the controller the request
	//!        arrived through and never from the wire.
	//! \param nodeId The screen the player was looking at. A reply from any
	//!        other screen is stale and refused.
	//! \param choiceIndex Position in the view that was sent.
	MCF_Dialogue_View Choose(int playerId, string nodeId, int choiceIndex, IEntity listener)
	{
		if (!Replication.IsServer())
			return null;

		// Re-aimed on every reply, not only when the conversation opened. A
		// player who sidesteps mid-conversation should not end up talking to
		// somebody's ear.
		FaceTowards(listener);

		string current = GetCurrentNodeId(playerId);
		if (current.IsEmpty())
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " replied to '" + GetSpeakerName() + "' without an open conversation");
			return null;
		}

		// Stale click. Two arrive from one double-press, and the second would
		// otherwise apply a reply from a screen that is already gone.
		if (current != nodeId)
		{
			MCF_Core_Log.Debug("player " + playerId.ToString() + " replied on node '" + nodeId + "' but is on '" + current + "' -- ignored as stale");
			return BuildView(GetNode(current));
		}

		MCF_Dialogue_Node node = GetNode(current);
		if (!node || !node.m_aChoices)
			return null;

		// The client sends an index into the view it was given, and the view
		// omits hidden replies -- so the index is rebuilt the same way here
		// rather than indexing the authored list directly.
		array<ref MCF_Dialogue_Choice> offered = {};
		CollectOffered(node, offered);

		if (choiceIndex < 0 || choiceIndex >= offered.Count())
		{
			MCF_Core_Log.Warn("player " + playerId.ToString() + " sent reply " + choiceIndex.ToString() + " which was never offered");
			return BuildView(node);
		}

		MCF_Dialogue_Choice choice = offered[choiceIndex];

		// Re-checked here and not only when the view was built. The state can
		// have moved between the two -- a grenade goes off nearby and the area
		// gets more hostile while the player is reading.
		if (!IsAvailable(choice))
		{
			MCF_Core_Log.Debug("player " + playerId.ToString() + " tried a reply they are not entitled to on '" + GetSpeakerName() + "'");
			return BuildView(node);
		}

		Apply(choice);

		if (choice.m_sNextNodeId.IsEmpty())
		{
			ForgetTalker(playerId);
			return BuildEnded();
		}

		MCF_Dialogue_Node next = GetNode(choice.m_sNextNodeId);
		if (!next)
		{
			MCF_Core_Log.Warn("dialogue on '" + GetSpeakerName() + "' points at node '" + choice.m_sNextNodeId + "' which does not exist -- ending the conversation");
			ForgetTalker(playerId);
			return BuildEnded();
		}

		SetCurrentNodeId(playerId, next.m_sId);
		return BuildView(next);
	}

	//! Turns this person to look at whoever just spoke to them.
	//!
	//! Being addressed by somebody who keeps staring at a wall reads as broken
	//! long before anybody reads a word of what they say. Only the yaw is
	//! touched: pitch and roll on a standing character are the animation
	//! system's business, and writing them produces a leaning corpse.
	//!
	//! Server side, so every machine sees the same turn through the entity's
	//! normal replication rather than each client guessing.
	protected void FaceTowards(IEntity listener)
	{
		if (!listener)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		vector delta = listener.GetOrigin() - owner.GetOrigin();
		delta[1] = 0;

		// Standing on top of each other: any yaw would be arbitrary, and the
		// snap is more jarring than leaving them as they were.
		if (delta.LengthSq() < 0.04)
			return;

		// NOT SetYawPitchRoll, and not SetTransform. Writing a character's
		// rotation directly does nothing you can see: the animation system
		// and the command handler own that transform and put it back on the
		// next frame. Tried, watched it do nothing, looked it up -- no vanilla
		// script sets a character's rotation that way either.
		//
		// The turn has to go through the animation system instead.
		// CharacterHeadingAnimComponent.AlignPosDirWS is the primitive vanilla
		// itself uses, via StartLoitering(alignToPosition: true), which is how
		// AI characters end up facing the right way when they sit down or
		// lean on something. StartLoitering is avoided here because it insists
		// on a loitering TYPE and would put the person into an animation;
		// this wants the turn and nothing else.
		// STOP THE BEHAVIOUR TREE FIRST, or the turn below is undone within a
		// second. This was the whole reason the first attempt looked like it
		// half-worked: the body did turn, and then the AI picked its next
		// behaviour and walked its heading straight back.
		//
		// StartLoitering does NOT prevent that -- it disables movement
		// *controls*, not behaviour selection, and its loiter behaviour only
		// outranks Idle. DeactivateAI is the one call in vanilla that stops
		// the tree outright; vanilla itself uses it when a character's life
		// state changes.
		//
		// It is also the honest reading of the situation: somebody who has
		// been stopped and spoken to is not getting on with their day.
		SuspendAI();

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return;

		// THE ALIGN ONLY HOLDS INSIDE A LOITER, which is why calling
		// AlignPosDirWS on its own looked like it did nothing: the turn went
		// in and the command handler put the heading back on the next frame.
		//
		// SCR_GetDisableMovementControls() is what keeps the controls off
		// while a character is turning, and it reads m_iLoiteringType -- which
		// is zero unless StartLoitering set it. So the alignment and the
		// loiter are one mechanism, not two, and taking half of it gets you a
		// turn that is immediately undone.
		//
		// LOITERING is the plain "standing about" type, which is what somebody
		// who has been stopped and spoken to is in fact doing.
		vector current[4];
		owner.GetWorldTransform(current);

		vector target[4];
		Math3D.DirectionAndUpMatrix(delta.Normalized(), current[1], target);
		target[3] = current[3];

		controller.StartLoitering(null, ELoiteringType.LOITERING, true, true, true, target);

		m_bLoitering = true;

		MCF_Core_Log.Debug("'" + GetSpeakerName() + "' turned to face the player");
	}

	//! Lets the character out of the conversational stance.
	protected void StopFacing()
	{
		if (!m_bLoitering)
			return;

		m_bLoitering = false;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return;

		// Not "fast": that is reserved for going into combat, and a person
		// who has finished a conversation should stand up out of it rather
		// than snap.
		controller.StopLoitering(false);
	}



	//! Server side. The player walked away or closed the screen.
	void End(int playerId)
	{
		if (!Replication.IsServer())
			return;

		ForgetTalker(playerId);

		// Only when the last person walks away. Two players can be talking to
		// the same man, and the first one leaving must not hand him back to
		// the AI while the second is mid-sentence.
		if (m_aTalkerIds.IsEmpty())
		{
			StopFacing();
			ResumeAI();
		}
	}

	//! Takes the character off its behaviour tree so it stays where it is and
	//! keeps facing whoever it is talking to.
	protected void SuspendAI()
	{
		if (m_bAiSuspended)
			return;

		AIControlComponent control = GetAIControl();
		if (!control)
			return;

		control.DeactivateAI();
		m_bAiSuspended = true;

		MCF_Core_Log.Debug("'" + GetSpeakerName() + "' taken off its behaviour tree for the conversation");
	}

	//! Hands the character back to its own behaviour. Anything it was doing
	//! before is decided again from scratch, which is correct -- the world has
	//! moved on while it was talking.
	protected void ResumeAI()
	{
		if (!m_bAiSuspended)
			return;

		AIControlComponent control = GetAIControl();
		if (control)
			control.ActivateAI();

		m_bAiSuspended = false;
	}

	// ------------------------------------------------------------ head look

	//! Whether this machine is currently driving the head towards its local
	//! player. Not replicated: it is a visual, and the only person who needs
	//! to see it is the one being looked at.
	protected bool m_bLookingAtLocalPlayer;

	protected CharacterAnimationComponent m_Animation;
	protected TAnimGraphVariable m_LookVariable;
	protected bool m_bLookBound;

	//! Client side. Starts the head following the local player.
	//!
	//! COPIED FROM THE TUTORIAL, which is the only place in the game where an
	//! NPC visibly watches you. Its narrative NPCs do not use the AI at all --
	//! they run SCR_NarrativeComponent, which re-issues a head IK target every
	//! fixed frame. So this is not a trick; it is how the game does it.
	//!
	//! The enable variable differs by animation graph, which is the one thing
	//! that does not carry over: the tutorial's NPCs run a narrative graph with
	//! "NarrativeLookAtIntensity", while ordinary characters run the player
	//! graph, where the equivalent is the bool "Look". Binding is guarded
	//! because a graph without it returns nothing, and the head simply will
	//! not turn rather than the whole thing failing.
	void StartLookingAtLocalPlayer()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (!m_Animation)
		{
			m_Animation = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));

			if (m_Animation)
			{
				m_LookVariable = m_Animation.BindVariableBool("Look");
				m_bLookBound = true;
			}
		}

		if (!m_Animation)
			return;

		m_bLookingAtLocalPlayer = true;

		// The event mask is set only while somebody is being looked at.
		// Every character in the game carries this component, and a fixed
		// frame on all of them for a head that is not turning would be a real
		// cost for nothing.
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
	}

	//! Client side. Lets the head go.
	void StopLookingAtLocalPlayer()
	{
		m_bLookingAtLocalPlayer = false;

		IEntity owner = GetOwner();
		if (owner)
			ClearEventMask(owner, EntityEvent.FIXEDFRAME);

		if (m_Animation && m_bLookBound)
			m_Animation.SetVariableBool(m_LookVariable, false);
	}

	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		if (!m_bLookingAtLocalPlayer || !m_Animation)
			return;

		IEntity player = EntityUtils.GetPlayer();
		if (!player)
			return;

		// Eye height rather than the origin, or they stare at your boots.
		vector target = player.GetOrigin() + "0 1.6 0";

		ChimeraCharacter character = ChimeraCharacter.Cast(player);
		if (character)
			target = character.EyePosition();

		// The IK target is in the looker's own local space.
		m_Animation.SetIKTarget("HeadLook", "HeadLook", owner.CoordToLocal(target), {0, 0, 0});

		if (m_bLookBound)
			m_Animation.SetVariableBool(m_LookVariable, true);
	}

	protected AIControlComponent GetAIControl()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		return AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
	}


	// ------------------------------------------------------ server: the rules

	//! The replies this node offers at all. Hidden ones are left out entirely
	//! so the player never learns they exist; locked ones stay in and are
	//! greyed, because seeing what you could not say is the feedback that
	//! makes trust and fear mean anything.
	protected void CollectOffered(notnull MCF_Dialogue_Node node, notnull out array<ref MCF_Dialogue_Choice> outChoices)
	{
		outChoices.Clear();

		if (!node.m_aChoices)
			return;

		foreach (MCF_Dialogue_Choice choice : node.m_aChoices)
		{
			if (!choice)
				continue;

			if (choice.m_bHideWhenLocked && !IsAvailable(choice))
				continue;

			outChoices.Insert(choice);
		}
	}

	protected bool IsAvailable(notnull MCF_Dialogue_Choice choice)
	{
		if (!choice.m_sRequiresFlag.IsEmpty() && !m_aFlags.Contains(choice.m_sRequiresFlag))
			return false;

		MCF_AI_DispositionComponent disposition = GetDisposition();

		// No disposition component means nobody authored feelings for this
		// person, so nothing is gated. Failing open is right here: a mission
		// maker who forgot the component gets a working conversation rather
		// than a silent one with every reply greyed out.
		if (!disposition)
			return true;

		if (disposition.GetTrust() < choice.m_fMinTrust)
			return false;

		if (disposition.GetFear() > choice.m_fMaxFear)
			return false;

		return true;
	}

	//! Why a reply is greyed. Authored text wins; otherwise the reason is
	//! derived, because "you cannot say that" teaches a player nothing.
	protected string LockedReason(notnull MCF_Dialogue_Choice choice)
	{
		if (!choice.m_sLockedReason.IsEmpty())
			return choice.m_sLockedReason;

		if (!choice.m_sRequiresFlag.IsEmpty() && !m_aFlags.Contains(choice.m_sRequiresFlag))
			return "There is no way to bring that up yet.";

		MCF_AI_DispositionComponent disposition = GetDisposition();
		if (!disposition)
			return "Not now.";

		if (disposition.GetFear() > choice.m_fMaxFear)
			return "They are too frightened to answer that.";

		if (disposition.GetTrust() < choice.m_fMinTrust)
			return "They do not trust you enough for that.";

		return "Not now.";
	}

	//! What a reply does. Order matters: the flag is set and the disposition
	//! moves before the event goes out, so anything listening on that event
	//! sees the world the reply has already changed.
	protected void Apply(notnull MCF_Dialogue_Choice choice)
	{
		if (!choice.m_sSetFlag.IsEmpty() && !m_aFlags.Contains(choice.m_sSetFlag))
			m_aFlags.Insert(choice.m_sSetFlag);

		MCF_AI_DispositionComponent disposition = GetDisposition();
		if (disposition)
		{
			disposition.AdjustTrust(choice.m_fTrustChange);
			disposition.AdjustFear(choice.m_fFearChange);
		}

		if (!choice.m_sPublishEvent.IsEmpty())
		{
			MCF_Core_Log.Debug("dialogue with '" + GetSpeakerName() + "' publishes '" + choice.m_sPublishEvent + "'");
			MCF_Core_EventManager.GetInstance().Publish(choice.m_sPublishEvent, this);
		}
	}

	// ------------------------------------------------------ server: the view

	protected MCF_Dialogue_View BuildView(MCF_Dialogue_Node node)
	{
		if (!node)
			return BuildEnded();

		MCF_Dialogue_View view = new MCF_Dialogue_View();
		view.m_sSpeaker = GetSpeakerName();
		view.m_sNodeId = node.m_sId;
		view.m_sText = node.m_sText;

		array<ref MCF_Dialogue_Choice> offered = {};
		CollectOffered(node, offered);

		foreach (int i, MCF_Dialogue_Choice choice : offered)
		{
			MCF_Dialogue_ChoiceView choiceView = new MCF_Dialogue_ChoiceView();
			choiceView.m_iIndex = i;
			choiceView.m_sText = choice.m_sText;
			choiceView.m_bEnabled = IsAvailable(choice);

			if (!choiceView.m_bEnabled)
				choiceView.m_sLockedReason = LockedReason(choice);

			view.m_aChoices.Insert(choiceView);
		}

		// A node with nothing sayable is the end of the road, whether the
		// author wrote it that way or every reply happens to be locked.
		view.m_bEnded = view.m_aChoices.IsEmpty();

		return view;
	}

	protected MCF_Dialogue_View BuildEnded()
	{
		MCF_Dialogue_View view = new MCF_Dialogue_View();
		view.m_sSpeaker = GetSpeakerName();
		view.m_bEnded = true;
		return view;
	}
}
