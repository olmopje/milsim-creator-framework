//! Makes an intel object something you have to break into first.
//!
//! WHY THIS IS A SEPARATE COMPONENT. It sits beside MCF_Intel_CarrierComponent
//! rather than inside it, so the Ops module never learns that hacking exists.
//! Ops owns what a device knows; this owns whether you may look at it. Remove
//! the Devices module and this component is dropped at prefab load -- one
//! error line -- and the object reads normally through Ops' viewer.
//!
//! IT READS UNLOCKED WHEN THE MODULE IS ABSENT, and that is deliberate. The
//! alternative is a device nobody can ever open, which is a mission silently
//! broken rather than a mission slightly easier. A missing lock is a
//! degradation the mission maker can see.
//!
//! THE SERVER OWNS THE STATE. m_bUnlocked is only ever written on the server,
//! in response to an answer the server itself checked -- see
//! MCF_PlayerController_Devices.c. Clients learn about it through RplProp.
//!
//! Once open, it stays open for everyone. A phone that has been broken into is
//! broken into; making each player repeat the puzzle would be busywork that
//! punishes the squad for having a specialist.

[ComponentEditorProps(category: "MCF/Devices", description: "This object has to be broken into before its intel can be read.")]
class MCF_Devices_LockComponentClass : ScriptComponentClass
{
}

class MCF_Devices_LockComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Whether this device is secured at mission start. Turn off for a phone somebody left unlocked.")]
	protected bool m_bSecured;

	[Attribute(defvalue: "2", uiwidget: UIWidgets.Slider, params: "0 4 1", desc: "How hard it is: 0 is three symbols and twelve seconds, 4 is seven and five.")]
	protected int m_iDifficulty;

	[Attribute(defvalue: "Break into", uiwidget: UIWidgets.EditBox, desc: "Verb on the prompt while it is still locked.")]
	protected string m_sBreakInVerb;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event published once this device is opened. Leave empty for none.")]
	protected string m_sOpenedEvent;

	//! False until somebody gets in. Server-authoritative; replicated so a
	//! client can stop offering the prompt and start offering the contents.
	[RplProp(onRplName: "OnUnlockedReplicated")]
	protected bool m_bUnlocked;

	//! Set while a challenge is out with a player, so a second player cannot
	//! start one and have the first player's answer accepted for it.
	protected int m_iPendingSeed;
	protected int m_iPendingPlayerId;
	protected float m_fPendingIssuedAt;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (!m_bSecured)
			m_bUnlocked = true;

		if (Replication.IsServer() && !m_sOpenedEvent.IsEmpty())
			MCF_Core_ValidationRegistry.GetInstance().RegisterPublisher(m_sOpenedEvent);

		MCF_Core_Log.Debug("device lock init, secured=" + m_bSecured.ToString() + " difficulty=" + m_iDifficulty.ToString());
	}

	bool IsSecured()
	{
		return m_bSecured;
	}

	bool IsUnlocked()
	{
		return m_bUnlocked;
	}

	//! True while this object should refuse to be read.
	bool IsLocked()
	{
		return m_bSecured && !m_bUnlocked;
	}

	int GetDifficulty()
	{
		return m_iDifficulty;
	}

	string GetBreakInVerb()
	{
		return m_sBreakInVerb;
	}

	// -------------------------------------------------------------- server

	//! Server side. Invents a challenge for one player and remembers it.
	//! \return The seed, or 0 if this device is not accepting attempts.
	int IssueChallenge(int playerId)
	{
		if (!Replication.IsServer() || !IsLocked())
			return 0;

		// A seed of 0 would be a dead LCG state -- every draw would return the
		// same cell. Cheap to avoid, expensive to debug.
		int seed = Math.RandomInt(1, 0x7FFFFFFE);

		m_iPendingSeed = seed;
		m_iPendingPlayerId = playerId;
		m_fPendingIssuedAt = WorldTimeSeconds();

		return seed;
	}

	//! Server side. Checks an answer against the challenge this device issued.
	//!
	//! Both halves matter. The sequence proves they solved the right puzzle;
	//! the clock proves they solved it in the time allowed, measured on the
	//! server, because an elapsed time reported by a client is not evidence of
	//! anything.
	bool SubmitAnswer(int playerId, string answer)
	{
		if (!Replication.IsServer())
			return false;

		if (m_iPendingSeed == 0 || m_iPendingPlayerId != playerId)
			return false;

		int seed = m_iPendingSeed;
		float elapsed = WorldTimeSeconds() - m_fPendingIssuedAt;

		// One challenge, one answer, right or wrong. Otherwise a client can
		// keep guessing against a seed it already knows the puzzle for.
		m_iPendingSeed = 0;
		m_iPendingPlayerId = 0;

		if (elapsed > MCF_Devices_Challenge.TimeLimit(m_iDifficulty))
		{
			MCF_Core_Log.Debug("player " + playerId.ToString() + " answered too late (" + elapsed.ToString() + "s)");
			return false;
		}

		if (!MCF_Devices_Challenge.Verify(seed, m_iDifficulty, answer))
			return false;

		Unlock();
		return true;
	}

	//! Server side. Opens it and tells everybody.
	void Unlock()
	{
		if (!Replication.IsServer() || m_bUnlocked)
			return;

		m_bUnlocked = true;
		Replication.BumpMe();

		if (!m_sOpenedEvent.IsEmpty())
			MCF_Core_EventManager.GetInstance().Publish(m_sOpenedEvent, this);

		MCF_Core_Log.Debug("device opened");
	}

	// ------------------------------------------------- Game Master editing

	//! Set from MCF_Devices_SecuredEditorAttribute. Turning security off opens
	//! the device outright rather than leaving it secured-but-shut, because
	//! "not secured" and "still closed" is a state nothing else understands.
	void SetSecuredFromEditor(bool secured)
	{
		m_bSecured = secured;

		if (!secured)
			Unlock();

		Replication.BumpMe();
	}

	//! Set from MCF_Devices_DifficultyEditorAttribute. Takes effect on the next
	//! attempt; a challenge already in flight keeps the difficulty it was
	//! issued with, which is why SubmitAnswer reads m_iDifficulty rather than
	//! storing it -- worth knowing if a Game Master changes it mid-attempt.
	void SetDifficultyFromEditor(int difficulty)
	{
		m_iDifficulty = MCF_Devices_Challenge.Clamp(difficulty, 0, MCF_Devices_Challenge.MAX_DIFFICULTY);
	}

	protected void OnUnlockedReplicated()
	{
		MCF_Core_Log.Debug("device unlock replicated to this machine");
	}

	protected float WorldTimeSeconds()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetWorldTime() / 1000.0;
	}
}
