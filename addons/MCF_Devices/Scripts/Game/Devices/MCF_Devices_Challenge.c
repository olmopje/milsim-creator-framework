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
//! -- is decided by the client no matter how it is dressed up. What is here is
//! a sequence: the server picks an order, the player repeats it, the server
//! compares. Simple, and honest about who decides.
//!
//! Enforce has no seedable RNG, so this carries its own. It has to be exactly
//! reproducible on both sides, which a shared Math.RandomInt could never be.

class MCF_Devices_Challenge
{
	//! Cells in the grid the player is shown. Three by three.
	static const int GRID_SIZE = 9;

	//! Shortest and longest a sequence gets, across the difficulty range.
	static const int MIN_STEPS = 3;
	static const int MAX_STEPS = 7;

	//! Seconds allowed, at the easiest and hardest setting. Checked on the
	//! server against its own clock, never against a time the client reports.
	static const float MAX_SECONDS_EASY = 12.0;
	static const float MAX_SECONDS_HARD = 5.0;

	//! Difficulty runs 0..4. A Game Master sets it with a slider, which writes
	//! a float -- see MCF_Devices_EditorAttributes.c.
	static const int MAX_DIFFICULTY = 4;

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

	//! Builds the sequence a given seed and difficulty produce. Called on the
	//! server to decide the answer, and on the client to draw the puzzle --
	//! both must reach the same list or nothing works.
	static void Build(int seed, int difficulty, notnull out array<int> outSequence)
	{
		outSequence.Clear();

		int steps = MIN_STEPS + Clamp(difficulty, 0, MAX_DIFFICULTY);
		if (steps > MAX_STEPS)
			steps = MAX_STEPS;

		int state = seed;
		int previous = -1;

		for (int i = 0; i < steps; i++)
		{
			int cell = NextRandom(state) % GRID_SIZE;

			// Never the same cell twice running. Two identical presses in a
			// row are indistinguishable from one press that registered twice,
			// and a player who loses to that learns nothing.
			if (cell == previous)
				cell = (cell + 1) % GRID_SIZE;

			outSequence.Insert(cell);
			previous = cell;
		}
	}

	//! How long the server will accept an answer for.
	static float TimeLimit(int difficulty)
	{
		int d = Clamp(difficulty, 0, MAX_DIFFICULTY);
		float t = MAX_DIFFICULTY;
		return MAX_SECONDS_EASY + (MAX_SECONDS_HARD - MAX_SECONDS_EASY) * (d / t);
	}

	//! Compares an answer against the sequence a seed produces.
	static bool Verify(int seed, int difficulty, string answer)
	{
		array<int> expected = {};
		Build(seed, difficulty, expected);

		return Encode(expected) == answer;
	}

	//! Sequences travel as text because that is what an RPC carries cheaply
	//! and what the rest of MCF already does (see MCF_Task.Serialize).
	static string Encode(notnull array<int> sequence)
	{
		string out = "";
		foreach (int cell : sequence)
		{
			out = out + cell.ToString();
		}
		return out;
	}

	static int Clamp(int value, int low, int high)
	{
		if (value < low)
			return low;
		if (value > high)
			return high;
		return value;
	}
}
