//! The puzzle itself: generated from a seed, and checkable from that same seed.
//!
//! WHY A SEED AND NOT A RESULT. The obvious shape for a minigame is "client
//! plays it, client tells the server it won". That shape hands every phone in
//! the mission to anybody willing to edit a packet, and it would be the one
//! soft spot in a system where every other request is re-checked on the server
//! (see MCF_PlayerController_Ops.c). So the server invents the puzzle, keeps
//! the seed, and re-derives the answer when the client sends one back. The
//! client never says whether it won -- only what it pressed.
//!
//! That constraint rules out a whole class of minigames. Anything whose result
//! cannot be re-derived from a seed -- a reaction test, a hold-the-button bar
//! -- is decided by the client no matter how it is dressed up. Every puzzle
//! below is a thing the server can invent, hand over, and mark itself.
//!
//! WHY THE SEED ALSO PICKS WHICH PUZZLE. The seed is the only thing that
//! travels, and both machines already derive the whole puzzle from it. Letting
//! it decide the kind as well means a second puzzle costs nothing on the wire
//! and adds no server state: the server does not have to remember which game it
//! handed out, because the seed says. See MCF_Devices_LockComponent.IssueChallenge
//! for how the same device is stopped from serving the same kind twice running.
//!
//! Enforce has no seedable RNG, so this carries its own. It has to be exactly
//! reproducible on both sides, which a shared Math.RandomInt could never be.

//! Which game the player gets. NEVER REORDERED and never sent over the wire --
//! both sides derive it from the seed, so a mismatch here would be two machines
//! playing different games with no error anywhere.
enum MCF_EPuzzleKind
{
	//! Repeat the order the grid flashed at you.
	SEQUENCE,
	//! Tune your wave until it sits on top of the one you were given.
	FREQUENCY,
	//! Read the rule, open exactly the ports that match it.
	PORTS
}

class MCF_Devices_Challenge
{
	//! How many values MCF_EPuzzleKind has. Enforce cannot count an enum, so
	//! this is maintained by hand -- add a puzzle, raise this, or the new one
	//! is never drawn.
	static const int PUZZLE_COUNT = 3;

	//! Difficulty runs 0..4. A Game Master sets it with a slider, which writes
	//! a float -- see MCF_Devices_EditorAttributes.c.
	static const int MAX_DIFFICULTY = 4;

	//! Longest answer the server will even look at. An unbounded string off the
	//! wire is somebody else's problem to have; this is the longest any puzzle
	//! below can legitimately produce, with room to spare.
	static const int MAX_ANSWER_LENGTH = 24;

	//! Instances live for the session. They hold no per-attempt state -- every
	//! method takes the seed -- so one of each is enough and two players
	//! hacking two phones at once cannot tread on each other.
	protected static ref MCF_Devices_Puzzle_Sequence s_Sequence;
	protected static ref MCF_Devices_Puzzle_Frequency s_Frequency;
	protected static ref MCF_Devices_Puzzle_Ports s_Ports;

	//! Deterministic linear congruential generator. Values are the ones from
	//! the C standard's example generator; nothing here is cryptographic and
	//! nothing needs to be -- it only has to give the same answer twice.
	//!
	//! Overflow is expected and harmless: Enforce ints wrap, and the mask
	//! keeps the result positive so the modulo below cannot go negative.
	static int NextRandom(inout int state)
	{
		state = (state * 1103515245 + 12345) & 0x7FFFFFFF;
		return state;
	}

	//! Which game this seed calls for. Two draws in, not one, so that the kind
	//! is not a simple function of the seed's low bits -- the seed is also used
	//! to build the puzzle itself, and reusing the very first draw for both
	//! would correlate the two in ways that are tedious to reason about.
	static int KindFor(int seed)
	{
		int state = seed;
		NextRandom(state);
		return NextRandom(state) % PUZZLE_COUNT;
	}

	//! The puzzle a kind names. Never null for a kind KindFor can return.
	static MCF_Devices_Puzzle GetPuzzle(int kind)
	{
		if (kind == MCF_EPuzzleKind.FREQUENCY)
		{
			if (!s_Frequency)
				s_Frequency = new MCF_Devices_Puzzle_Frequency();

			return s_Frequency;
		}

		if (kind == MCF_EPuzzleKind.PORTS)
		{
			if (!s_Ports)
				s_Ports = new MCF_Devices_Puzzle_Ports();

			return s_Ports;
		}

		if (!s_Sequence)
			s_Sequence = new MCF_Devices_Puzzle_Sequence();

		return s_Sequence;
	}

	//! The puzzle this seed calls for, in one step. What both the menu and the
	//! lock actually call.
	static MCF_Devices_Puzzle PuzzleFor(int seed)
	{
		return GetPuzzle(KindFor(seed));
	}

	//! How long the server will accept an answer for. Per puzzle, because
	//! repeating six flashes and reading eight rows of a port table are not the
	//! same amount of work and one clock for both would make one of them a
	//! formality and the other unwinnable.
	static float TimeLimit(int seed, int difficulty)
	{
		return PuzzleFor(seed).TimeLimit(difficulty);
	}

	//! Compares an answer against the puzzle a seed produces.
	static bool Verify(int seed, int difficulty, string answer)
	{
		return PuzzleFor(seed).Verify(seed, difficulty, answer);
	}

	//! Answers travel as text because that is what an RPC carries cheaply and
	//! what the rest of MCF already does (see MCF_Task.Serialize). Each puzzle
	//! decides its own spelling; the only shared rule is that it is short.
	static string Encode(notnull array<int> values)
	{
		// Not named "out": Enforce reserves that for parameter direction, and
		// using it as a local is a "Broken expression (missing ';'?)" with no
		// hint as to why. Same family as `reference`.
		string encoded = "";
		foreach (int value : values)
		{
			encoded = encoded + value.ToString();
		}
		return encoded;
	}

	static int Clamp(int value, int low, int high)
	{
		if (value < low)
			return low;
		if (value > high)
			return high;
		return value;
	}

	//! Straight-line interpolation between an easy and a hard number of
	//! seconds. Every puzzle scales its clock the same way and only picks the
	//! two ends, so that "difficulty 4" means the same kind of pressure
	//! whichever game comes up.
	static float ScaleSeconds(int difficulty, float easy, float hard)
	{
		int d = Clamp(difficulty, 0, MAX_DIFFICULTY);
		float span = MAX_DIFFICULTY;
		return easy + (hard - easy) * (d / span);
	}
}
