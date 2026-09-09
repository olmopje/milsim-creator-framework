//! Telling a subdued person where to be: walk with me, or stay back.
//!
//! WHY A COMPONENT AND NOT JUST ACTIONS. Both orders are standing states that
//! have to be maintained, not one-off commands. A person told to follow has to
//! be re-pointed as you walk; a person told to stay back has to be pushed away
//! again every time they drift in. Vanilla's own pieces are both momentary --
//! a move behaviour snapshots its destination in its constructor, and the
//! avoid behaviour is a half-second nudge -- so something has to hold the
//! order and re-issue it. That is this.
//!
//! WHAT IS BORROWED AND WHAT IS BUILT. Following borrows
//! SCR_AIMoveIndividuallyBehavior, the same behaviour vanilla's own commanding
//! menu ends up using, raised to player priority so it outranks the AI
//! deciding to go and fight. Standing off borrows
//! SCR_AIAvoidCharacterBehavior, which sits at priority 1180 -- second highest
//! in the game -- and is what happens when a player physically bumps into an
//! AI. Neither was written for this; both do exactly the right thing when
//! re-issued.
//!
//! WHAT DOES NOT EXIST, stated so nobody looks for it: there is no vanilla way
//! to make an AI walk AHEAD of you. Nothing in the game leads. Marching a
//! prisoner in front of you means computing a spot in front of the escort and
//! sending them to it, which is what LEAD does below -- built, not borrowed,
//! and it will look less natural than following because of it.
//!
//! SERVER ONLY. Everything here drives an AI, which only the server does.

enum MCF_ESubjectOrder
{
	//! Left alone.
	NONE,
	//! Walking with you, behind.
	FOLLOW,
	//! Marched in front of you.
	LEAD,
	//! Kept at arm's length.
	STAND_OFF
}

[ComponentEditorProps(category: "MCF/AI", description: "Holds a standing order for a subdued person: follow, lead, or keep back.")]
class MCF_AI_SubjectControlComponentClass : ScriptComponentClass
{
}

class MCF_AI_SubjectControlComponent : ScriptComponent
{
	[Attribute(defvalue: "6", uiwidget: UIWidgets.Slider, params: "2 30 1", desc: "How far back a follower trails, in metres.")]
	protected float m_fFollowDistance;

	[Attribute(defvalue: "4", uiwidget: UIWidgets.Slider, params: "2 20 1", desc: "How far ahead a led prisoner is marched, in metres.")]
	protected float m_fLeadDistance;

	[Attribute(defvalue: "8", uiwidget: UIWidgets.Slider, params: "3 40 1", desc: "How far back somebody told to keep their distance is pushed.")]
	protected float m_fStandOffDistance;

	[Attribute(defvalue: "3", uiwidget: UIWidgets.Slider, params: "1 20 1", desc: "How far the escort must move before the subject is re-pointed. Smaller is smoother and costs more.")]
	protected float m_fRepointDistance;

	[Attribute(defvalue: "0.04", uiwidget: UIWidgets.Slider, params: "0 1 0.01", desc: "Chance per second that an unrestrained escortee tries to break away, at maximum nerve.")]
	protected float m_fEscapeChancePerSecond;

	[Attribute(defvalue: "MCF_AI_SubjectEscaped", uiwidget: UIWidgets.EditBox, desc: "Event published when somebody breaks away. Hang a Recipe or an alarm on it.")]
	protected string m_sEscapeEvent;

	//! How often the order is checked. One second is enough for a walking
	//! pace and cheap enough to leave running on several prisoners at once.
	//! How often the order is checked.
	//!
	//! Marching needs a fast tick, because each combat-move request lasts a
	//! little under a second and the next has to arrive before it expires or
	//! the prisoner stops between steps. Following does not -- it pathfinds,
	//! and re-issuing that often would be the stutter all over again.
	protected static const int TICK_MS = 500;

	//! How long each marching step lasts. Deliberately longer than the tick
	//! so the steps overlap and the walk is continuous.
	protected static const float MARCH_STEP_S = 1.5;

	//! WARNING, PAID FOR IN A CRASH: do NOT point this at
	//! anims/workspaces/narrative/narrative_npc_main.agr. It holds the clips
	//! we want -- including arms_back, the parade-rest pose that reads as
	//! bound wrists from behind -- but mounting it takes the Workbench down
	//! with a native crash and no script frames in the log.
	//!
	//! The reason is visible in what vanilla itself attaches: the officer
	//! mission's graph is 475 BYTES. The narrative graph is 23 KB and is a
	//! complete character graph in its own right. PreAnim_SetAttachment mounts
	//! a SUB-graph, not a replacement, and a whole graph is not one.
	//!
	//! So a restrained pose needs a small purpose-built graph containing one
	//! clip. Empty is the safe default: no pose, no error, restrain works.

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Animation graph (.agr) for the restrained pose. Empty means no pose at all -- which breaks nothing.")]
	protected ResourceName m_sPoseGraph;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Animation instance (.asi) that goes with the graph above.")]
	protected ResourceName m_sPoseGraphInstance;

	[Attribute(defvalue: "CMD_Narrative", uiwidget: UIWidgets.EditBox, desc: "Command inside the graph that starts the pose. CMD_Narrative and CMD_Gesture_NPC are the two the narrative graph offers.")]
	protected string m_sPoseCommand;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Value passed with that command.")]
	protected int m_iPoseCommandValue;

	[Attribute(defvalue: "TalkVariant", uiwidget: UIWidgets.EditBox, desc: "Variable that selects WHICH clip in the graph plays. TalkVariant is the narrative graph's selector, 1 to 100.")]
	protected string m_sPoseVariable;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.Slider, params: "0 100 1", desc: "Which clip. Nobody knows which number is which without looking -- dial it in Game Master and watch.")]
	protected int m_iPoseVariant;

	protected bool m_bInPose;

	//! Server side. Used by the Game Master slider to hunt for the right clip
	//! without a recompile per guess.
	void SetPoseVariant(int variant)
	{
		if (!Replication.IsServer())
			return;

		m_iPoseVariant = variant;

		// Re-enter so the change is visible immediately rather than on the
		// next person restrained.
		if (m_bInPose)
		{
			LeaveRestrainedPose();
			EnterRestrainedPose();
		}
	}

	int GetPoseVariant()
	{
		return m_iPoseVariant;
	}

	protected MCF_ESubjectOrder m_eOrder;
	protected IEntity m_Commander;
	protected vector m_vLastIssuedFrom;
	protected bool m_bTicking;

	//! The behaviour currently pushed into the AI, kept so it can be failed
	//! when the order is lifted rather than left to run itself out.
	protected ref AIActionBase m_CurrentAction;

	//! The running march, kept so it can be re-pointed instead of rebuilt.
	protected ref MCF_AI_MarchBehavior m_March;

	MCF_ESubjectOrder GetOrder()
	{
		return m_eOrder;
	}

	IEntity GetCommander()
	{
		return m_Commander;
	}

	// --------------------------------------------------------------- orders

	//! Server side. Puts a standing order on this person.
	void Order(MCF_ESubjectOrder order, IEntity commander)
	{
		if (!Replication.IsServer())
			return;

		m_eOrder = order;
		m_Commander = commander;

		CancelCurrentAction();

		if (order == MCF_ESubjectOrder.NONE || !commander)
		{
			m_eOrder = MCF_ESubjectOrder.NONE;
			m_Commander = null;
			StopTicking();
			return;
		}

		// The AI has to be running to path anywhere. A conversation may have
		// switched it off; escorting is the opposite situation.
		AIControlComponent control = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		if (control)
			control.ActivateAI();

		// And out of the restrained pose, or they cannot walk at all:
		// loitering disables movement controls by design. The pose is for
		// somebody standing there restrained, not for somebody being marched.
		LeaveRestrainedPose();

		// Forced, so the first order takes effect immediately instead of
		// waiting for the escort to walk far enough to trip the re-point.
		Apply(true);
		StartTicking();

		MCF_Core_Log.Debug("subject ordered to " + order.ToString());
	}

	void ClearOrder()
	{
		Order(MCF_ESubjectOrder.NONE, null);
	}

	protected void StartTicking()
	{
		if (m_bTicking)
			return;

		m_bTicking = true;
		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	protected void StopTicking()
	{
		if (!m_bTicking)
			return;

		m_bTicking = false;
		GetGame().GetCallqueue().Remove(Tick);
	}

	override void OnDelete(IEntity owner)
	{
		StopTicking();
	}

	// ----------------------------------------------------------------- tick

	protected void Tick()
	{
		if (m_eOrder == MCF_ESubjectOrder.NONE)
		{
			StopTicking();
			return;
		}

		// The escort disconnected, died, or otherwise stopped existing. A
		// prisoner left following a ghost would walk to wherever they last
		// stood and wait there for ever.
		if (!m_Commander)
		{
			ClearOrder();
			return;
		}

		if (TryEscape())
			return;

		Apply(false);
	}

	//! \param forced Re-issue even if the escort has barely moved.
	protected void Apply(bool forced)
	{
		if (m_eOrder == MCF_ESubjectOrder.STAND_OFF)
		{
			ApplyStandOff();
			return;
		}

		vector commanderPos = m_Commander.GetOrigin();

		SCR_AIUtilityComponent utility = GetUtility();
		if (!utility)
			return;

		// MARCHING AND FOLLOWING ARE DIFFERENT MECHANISMS, not the same one
		// with a different destination. A follower is going somewhere and
		// pathfinds; somebody being marched is being steered, step by step,
		// and pathfinding to a point four metres away is what made the first
		// attempt shuffle -- arrive, stop, get told again, start.
		if (m_eOrder == MCF_ESubjectOrder.LEAD)
		{
			// THE MARCH IS KEPT ALIVE, not rebuilt. Failing the behaviour and
			// adding a fresh one every tick is a stop and a start twice a
			// second, however well the individual steps overlap -- that was
			// the stutter.
			if (m_March && utility.HasActionOfType(MCF_AI_MarchBehavior))
			{
				m_March.Retarget(LeadPosition(), CommanderPace());
				return;
			}

			CancelCurrentAction();

			m_March = new MCF_AI_MarchBehavior(utility, null, LeadPosition(), MARCH_STEP_S);
			m_March.Retarget(LeadPosition(), CommanderPace());

			utility.AddAction(m_March);
			m_CurrentAction = m_March;
			return;
		}


		// Following: pathfind, and only re-issue once the escort has actually
		// gone somewhere, or the path restarts constantly.
		if (!forced && vector.Distance(commanderPos, m_vLastIssuedFrom) < m_fRepointDistance)
			return;

		m_vLastIssuedFrom = commanderPos;

		CancelCurrentAction();

		// Priority above PRIORITY_BEHAVIOR_ATTACK_SELECTED (90) and at player
		// level, which is what vanilla's own commanding uses to make an order
		// outrank the AI's own judgement.
		SCR_AIMoveIndividuallyBehavior move = new SCR_AIMoveIndividuallyBehavior(
			utility,
			null,
			commanderPos,
			SCR_AIActionBase.PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY + SCR_AIActionBase.PRIORITY_LEVEL_PLAYER,
			SCR_AIActionBase.PRIORITY_LEVEL_PLAYER,
			null,
			m_fFollowDistance);

		utility.AddAction(move);
		m_CurrentAction = move;
	}

	//! How fast the escort is actually going, as a movement type the prisoner
	//! can be given.
	//!
	//! Read from velocity rather than from an input flag, because what matters
	//! is how fast the man in front has to walk to stay in front -- not which
	//! key the escort is holding.
	protected EMovementType CommanderPace()
	{
		if (!m_Commander)
			return EMovementType.WALK;

		Physics physics = m_Commander.GetPhysics();
		if (!physics)
			return EMovementType.WALK;

		vector velocity = physics.GetVelocity();
		velocity[1] = 0;

		// Walking pace in this game is around 2 m/s; anything above that and a
		// prisoner kept at a walk drops behind and then has to sprint to catch
		// up, which is the most obvious way this stops looking like an escort.
		if (velocity.Length() > 2.2)
			return EMovementType.RUN;

		return EMovementType.WALK;
	}

	//! A spot in front of the escort, so a prisoner is marched rather than
	//! trailed. Built here because nothing in vanilla leads.
	protected vector LeadPosition()
	{
		vector transform[4];
		m_Commander.GetWorldTransform(transform);

		// Column 2 is forward.
		vector forward = transform[2];
		forward[1] = 0;
		forward.Normalize();

		return transform[3] + forward * m_fLeadDistance;
	}

	//! Pushes somebody away when they have drifted too close.
	//!
	//! The behaviour borrowed here lasts half a second by design -- it exists
	//! for a player bumping into an AI -- so holding a distance means issuing
	//! it again whenever they come back inside it.
	protected void ApplyStandOff()
	{
		float distance = vector.Distance(m_Commander.GetOrigin(), GetOwner().GetOrigin());
		if (distance > m_fStandOffDistance)
			return;

		SCR_AIUtilityComponent utility = GetUtility();
		if (!utility)
			return;

		// Vanilla's own de-duplication idiom: do not stack a second nudge on
		// top of one still running.
		if (utility.HasActionOfType(SCR_AIAvoidCharacterBehavior))
			return;

		vector velocity = vector.Zero;
		Physics physics = m_Commander.GetPhysics();
		if (physics)
			velocity = physics.GetVelocity();

		SCR_AIAvoidCharacterBehavior avoid = new SCR_AIAvoidCharacterBehavior(utility, null, m_Commander.GetOrigin(), velocity);
		utility.AddAction(avoid);
	}

	// ----------------------------------------------------------- restrained

	//! Server side. Puts a restrained person into the hands-behind-the-back
	//! pose, if one has been authored.
	//!
	//! WHY THIS EXISTS AS A HOOK RATHER THAN A POSE. Arma Reforger ships no
	//! restrained, bound or surrendering animation -- the whole set is seven
	//! gestures (point, stop, follow, move, get in twice, salute) and six
	//! loiter poses (sit, lean twice, smoke, loiter, pushups). Nothing in the
	//! game shows two characters touching either, which is why the drag mods
	//! animate the DRAGGER's arm with IK and leave the dragged man limp.
	//!
	//! The closest shipped clip is anims/anm/Tutorial/arms_back.anm -- an
	//! instructor's parade rest, hands clasped behind the back. From behind,
	//! which is exactly where an escort stands, that reads as bound wrists.
	//! It still needs a graph to play it, and graphs are built by hand in the
	//! Animation Editor, so the two paths below are left empty until somebody
	//! makes one. Empty means no pose and no error.
	//!
	//! THE ROUTE IS VANILLA'S OWN. ELoiteringType.CUSTOM with
	//! SCR_LoiterCustomAnimData mounts an outside graph onto a character, and
	//! the officer in the tutorial mission runs on exactly this. It is also
	//! already replicated, so the pose reaches other players without any work.
	void EnterRestrainedPose()
	{
		if (!Replication.IsServer() || m_bInPose)
			return;

		if (m_sPoseGraph.IsEmpty())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return;

		CharacterAnimationComponent animation = controller.GetAnimationComponent();
		if (!animation)
			return;

		// The selector variable is set BEFORE the loiter starts, which is the
		// order vanilla's own officer-mission animation uses. Set afterwards
		// it arrives a frame late and the graph has already picked a clip.
		TAnimGraphVariable variable = -1;
		if (!m_sPoseVariable.IsEmpty())
		{
			variable = animation.BindVariableInt(m_sPoseVariable);
			animation.SetVariableInt(variable, m_iPoseVariant);
		}

		SCR_LoiterCustomAnimData data = SCR_LoiterCustomAnimData.CreateInstance(
			animation.BindCommand(m_sPoseCommand),
			m_iPoseCommandValue,
			graphName: m_sPoseGraph,
			graphInstanceName: m_sPoseGraphInstance,
			controlVariableID: variable,
			controlVariableValue: m_iPoseVariant,
			controlVariableName: m_sPoseVariable);

		vector transform[4];
		owner.GetWorldTransform(transform);

		// The seventh argument is what stops a prisoner standing up by tapping
		// sprint. Loitering cancels itself on fire, sprint, raise weapon, ADS
		// or reload unless input is disabled -- which is fine for a man having
		// a cigarette and useless for one in handcuffs.
		controller.StartLoitering(null, ELoiteringType.CUSTOM, true, true, true, transform, true, data);

		m_bInPose = true;
		MCF_Core_Log.Debug("restrained pose entered");
	}

	//! Server side. Lets them stand normally again.
	void LeaveRestrainedPose()
	{
		if (!m_bInPose)
			return;

		m_bInPose = false;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
		if (controller)
			controller.StopLoitering(false);
	}

	// --------------------------------------------------------------- escape

	//! Whether an unrestrained escortee makes a break for it.
	//!
	//! RESTRAINED PEOPLE DO NOT ESCAPE. That is the entire point of tying
	//! somebody's hands, and a mechanic where it happens anyway would make the
	//! restraint pointless.
	//!
	//! Nerve is the inverse of fear and rises with distrust: somebody
	//! terrified of you does not run, and somebody who trusts you has no
	//! reason to. The most likely runner is the calm man who thinks you are
	//! the problem -- which is the same gap between the two numbers that makes
	//! conversations worth having.
	protected bool TryEscape()
	{
		if (m_eOrder != MCF_ESubjectOrder.FOLLOW && m_eOrder != MCF_ESubjectOrder.LEAD)
			return false;

		if (m_fEscapeChancePerSecond <= 0)
			return false;

		MCF_AI_DispositionComponent disposition = MCF_AI_DispositionComponent.Cast(GetOwner().FindComponent(MCF_AI_DispositionComponent));
		if (!disposition)
			return false;

		if (disposition.GetCaptiveState() == MCF_ECaptiveState.RESTRAINED)
			return false;

		float nerve = (100 - disposition.GetFear()) * 0.01;
		float defiance = (100 - disposition.GetTrust()) * 0.01;

		if (Math.RandomFloat01() > m_fEscapeChancePerSecond * nerve * defiance)
			return false;

		MCF_Core_Log.Debug("subject broke away (" + disposition.Describe() + ")");

		disposition.SetCaptiveState(MCF_ECaptiveState.FREE);
		ClearOrder();

		if (!m_sEscapeEvent.IsEmpty())
			MCF_Core_EventManager.GetInstance().Publish(m_sEscapeEvent, this);

		return true;
	}

	// -------------------------------------------------------------- helpers

	//! Entity -> agent -> utility. There is no single vanilla helper for the
	//! whole hop; SCR_AIUtils covers the first half only.
	protected SCR_AIUtilityComponent GetUtility()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		AIControlComponent control = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!control)
			return null;

		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			return null;

		return SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
	}

	protected void CancelCurrentAction()
	{
		m_March = null;

		if (!m_CurrentAction)
			return;

		m_CurrentAction.Fail();
		m_CurrentAction = null;
	}
}
