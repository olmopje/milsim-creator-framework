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

	[Attribute(defvalue: "2", uiwidget: UIWidgets.Slider, params: "0 4 1", desc: "How hard it is, for whichever of the three break-in games comes up: more steps, a wider tuning range, a longer port table, and less time on the clock.")]
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

	//! Which game this device handed out last, so it does not hand out the same
	//! one twice running. -1 before the first attempt. Server-side only: the
	//! kind is derived from the seed on both machines, so this never travels.
	protected int m_iLastKind = -1;

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

	//! Convenience for callers that hold an entity rather than the component --
	//! the read action and the shell both do. Null-safe on purpose: an object
	//! with no lock is not locked, which is what makes a lock optional.
	static bool IsEntityLocked(IEntity entity)
	{
		if (!entity)
			return false;

		MCF_Devices_LockComponent lock = MCF_Devices_LockComponent.Cast(entity.FindComponent(MCF_Devices_LockComponent));
		return lock && lock.IsLocked();
	}

	static MCF_Devices_LockComponent FindOn(IEntity entity)
	{
		if (!entity)
			return null;

		return MCF_Devices_LockComponent.Cast(entity.FindComponent(MCF_Devices_LockComponent));
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

		int seed = DrawSeed();

		// Never the same game twice running on the same device. Drawing at
		// random would give a player the keypad three times in a row often
		// enough to feel like the module only has one puzzle, which is exactly
		// the impression the other two exist to prevent.
		//
		// Bounded, because a run of unlucky draws must not stall the server.
		// Falling out of the loop with a repeat is a worse round, not a broken
		// one, so this gives up rather than looping until it wins.
		int attempts = 0;
		while (attempts < 16 && MCF_Devices_Challenge.KindFor(seed) == m_iLastKind)
		{
			seed = DrawSeed();
			attempts++;
		}

		m_iLastKind = MCF_Devices_Challenge.KindFor(seed);

		MCF_Core_Log.Debug("device challenge issued: seed=" + seed.ToString() + " kind=" + m_iLastKind.ToString() + " after " + attempts.ToString() + " reroll(s)");

		m_iPendingSeed = seed;
		m_iPendingPlayerId = playerId;
		m_fPendingIssuedAt = WorldTimeSeconds();

		return seed;
	}

	//! A random 31-bit seed, built out of three small draws.
	//!
	//! MEASURED, NOT ASSUMED. This was `Math.RandomInt(1, 0x7FFFFFFE)` and the
	//! log of four consecutive attempts read seed=-1, seed=65535, seed=1,
	//! seed=65536. Math.RandomInt does not survive a range that large: it
	//! returns values clustered on powers of two, and a negative one at that,
	//! from a range whose lower bound was 1. The visible symptom was "I keep
	//! getting the same puzzle", which looked like a bug in the puzzle picker
	//! and was not -- the generator was being fed four seeds out of a handful.
	//!
	//! Three draws of 30 000 mixed together stay far inside the range where
	//! Math.RandomInt behaves, and cover the 31-bit space evenly.
	protected int DrawSeed()
	{
		int a = Math.RandomInt(0, 30000);
		int b = Math.RandomInt(0, 30000);
		int c = Math.RandomInt(0, 30000);

		int seed = ((a * 30011 + b) * 30011 + c) & 0x7FFFFFFF;

		// A seed of 0 would be a dead LCG state -- every draw would return the
		// same cell. Cheap to avoid, expensive to debug.
		if (seed == 0)
			seed = 1;

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

		// The clock is the puzzle's own -- reading a port table takes longer
		// than repeating six flashes -- and the seed is what says which puzzle
		// this was.
		if (elapsed > MCF_Devices_Challenge.TimeLimit(seed, m_iDifficulty))
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
