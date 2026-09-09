//! How one person feels about you. Two numbers, because two is enough.
//!
//! TRUST is whether they think talking to you will end well for them. It goes
//! up when you keep your word and treat them decently, down when you do not.
//!
//! FEAR is how frightened they are right now. High fear does not make someone
//! hostile -- it makes them useless: they agree with everything, tell you what
//! they think you want, and give you nothing. That is the point. A player who
//! gets what they want by frightening a civilian has learned the wrong lesson
//! about this framework.
//!
//! WHY NOT ONE "FRIENDLY" NUMBER. Because the two come apart, and the scenes
//! worth playing live in the gap. A man who trusts you and is terrified of the
//! people outside is a different conversation from a man who is calm and
//! thinks you are the problem. One number cannot tell those apart.
//!
//! FEAR IS PARTLY THE ROOM, NOT THE PERSON. Reported fear is this person's own
//! fear plus a share of the hostility in the area they live in, read live from
//! MCF_Hostility_Manager. So the compliance system already wired up -- where
//! ordering civilians around at gunpoint without cause raises area hostility --
//! now has a consequence a player can hear: the whole village gets harder to
//! talk to. Nothing needed connecting for that; it falls out of reading the
//! number at the moment it is asked for rather than caching it at spawn.
//!
//! SERVER STATE, NOT REPLICATED. The values never leave the server. What a
//! client needs -- which replies are available and why -- is computed on the
//! server and sent as part of the conversation. Replicating the numbers would
//! hand every client a readout of exactly how to play each civilian, which is
//! a worse game and a bigger wire for no gain.

//! What has been done to this person, as opposed to how they feel about it.
//!
//! ORDERED ON PURPOSE. Everything that gates on this asks "at least
//! COMPLIANT", so the values must stay in increasing severity and must never
//! be reordered. If they are ever persisted they become append-only, for the
//! same reason MCF_ETaskState did.
enum MCF_ECaptiveState
{
	//! Going about their day.
	FREE,
	//! Hands up at gunpoint. Cooperating because they have to, which is not
	//! the same as cooperating.
	COMPLIANT,
	//! Restrained. Not going anywhere, and not going to be talked round
	//! either.
	RESTRAINED
}

[ComponentEditorProps(category: "MCF/AI", description: "How this person feels about the players: trust and fear. Read by conversations.")]
class MCF_AI_DispositionComponentClass : ScriptComponentClass
{
}

class MCF_AI_DispositionComponent : ScriptComponent
{
	[Attribute(defvalue: "50", uiwidget: UIWidgets.Slider, params: "0 100 1", desc: "How far they trust the players to begin with. 50 is a stranger who has no reason to think either way.")]
	protected float m_fTrust;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, params: "0 100 1", desc: "How frightened they are to begin with, before anything the area adds.")]
	protected float m_fFear;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Hostility area this person lives in. Hostility there raises their fear. Leave empty to ignore the area entirely.")]
	protected string m_sAreaKey;

	[Attribute(defvalue: "0.5", uiwidget: UIWidgets.Slider, params: "0 1 0.05", desc: "How much of the area's hostility becomes this person's fear. 0.5 means a village at 80 hostility adds 40 fear.")]
	protected float m_fAreaFearShare;

	//! What has been done to this person.
	//!
	//! REPLICATED, because the client decides which interaction prompt to draw
	//! -- "Talk" or "Interrogate" -- and cannot ask the server first without
	//! the prompt flickering.
	//!
	//! IT LIVES HERE rather than in a component of its own. Every character in
	//! the game carries this one already, and a third script component on
	//! every soldier, civilian and corpse is a real runtime cost for a single
	//! enum. The concern is admittedly not "feelings" -- it is situation --
	//! but the alternative is worse.
	[RplProp()]
	protected MCF_ECaptiveState m_eCaptiveState;

	MCF_ECaptiveState GetCaptiveState()
	{
		return m_eCaptiveState;
	}

	//! Server side.
	void SetCaptiveState(MCF_ECaptiveState state)
	{
		if (!Replication.IsServer() || m_eCaptiveState == state)
			return;

		m_eCaptiveState = state;
		Replication.BumpMe();

		MCF_Core_Log.Debug("captive state now " + state.ToString() + " (" + Describe() + ")");
	}

	//! \return Trust, 0-100. Entirely personal: nothing in the world moves it
	//! except what players do to this person.
	float GetTrust()
	{
		return Math.Clamp(m_fTrust, 0, 100);
	}

	//! \return Fear, 0-100, including the share this person takes from how
	//! hostile their area currently is.
	float GetFear()
	{
		float fear = m_fFear;

		if (!m_sAreaKey.IsEmpty() && m_fAreaFearShare > 0)
			fear = fear + MCF_Hostility_Manager.GetInstance().GetHostility(m_sAreaKey) * m_fAreaFearShare;

		return Math.Clamp(fear, 0, 100);
	}

	//! Server side. Both are no-ops elsewhere: a client changing its own copy
	//! would diverge from the machine that actually decides what this person
	//! will say.
	void AdjustTrust(float delta)
	{
		if (!Replication.IsServer() || delta == 0)
			return;

		m_fTrust = Math.Clamp(m_fTrust + delta, 0, 100);
	}

	void AdjustFear(float delta)
	{
		if (!Replication.IsServer() || delta == 0)
			return;

		// Only the personal share moves. The area's contribution is the
		// area's business and is undone by the hostility system's own decay,
		// not by being nice to one man.
		m_fFear = Math.Clamp(m_fFear + delta, 0, 100);
	}

	//! The personal share of fear, without what the area contributes.
	//!
	//! Exists for the Game Master panel, which must show and write the value a
	//! person can actually be given -- showing the combined number in a field
	//! that only writes half of it would read as the slider not working.
	float GetPersonalFear()
	{
		return Math.Clamp(m_fFear, 0, 100);
	}

	//! Server side. Sets the personal values outright, for a scene that
	//! declares how these people feel rather than nudging them.
	//!
	//! Fear set here is the personal share only; what the area contributes is
	//! still added on top when GetFear is asked. A node that sets fear to 0 in
	//! a village at 80 hostility has not made anybody calm, and should not
	//! appear to have.
	void SetTrust(float value)
	{
		if (!Replication.IsServer())
			return;

		m_fTrust = Math.Clamp(value, 0, 100);
	}

	void SetFear(float value)
	{
		if (!Replication.IsServer())
			return;

		m_fFear = Math.Clamp(value, 0, 100);
	}

	//! For logs. Both numbers and the area, because a conversation that
	//! refuses a reply is otherwise impossible to explain after the fact.
	string Describe()
	{
		string text = "trust=" + GetTrust().ToString() + " fear=" + GetFear().ToString();

		if (!m_sAreaKey.IsEmpty())
			text = text + " (area '" + m_sAreaKey + "' at " + MCF_Hostility_Manager.GetInstance().GetHostility(m_sAreaKey).ToString() + ")";

		return text;
	}
}
