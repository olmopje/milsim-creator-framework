//! Repeat the order the grid flashed at you.
//!
//! The original break-in game, and still the one that reads fastest: it needs
//! no explanation, and a player who fails it knows exactly why. Difficulty adds
//! steps and takes seconds away.

class MCF_Devices_Puzzle_Sequence : MCF_Devices_Puzzle
{
	//! Cells in the grid the player is shown. Three by three.
	static const int GRID_SIZE = 9;

	//! Shortest and longest a sequence gets, across the difficulty range.
	static const int MIN_STEPS = 3;
	static const int MAX_STEPS = 7;

	override string Title()
	{
		return "KEYPAD";
	}

	override string Instruction()
	{
		return "Watch the sequence, then repeat it before the timer runs out.";
	}

	override float TimeLimit(int difficulty)
	{
		return MCF_Devices_Challenge.ScaleSeconds(difficulty, 12.0, 5.0);
	}

	//! Builds the sequence a given seed and difficulty produce. Called on the
	//! server to decide the answer, and on the client to draw the puzzle --
	//! both must reach the same list or nothing works.
	void Build(int seed, int difficulty, notnull array<int> outSequence)
	{
		outSequence.Clear();

		int steps = MIN_STEPS + MCF_Devices_Challenge.Clamp(difficulty, 0, MCF_Devices_Challenge.MAX_DIFFICULTY);
		if (steps > MAX_STEPS)
			steps = MAX_STEPS;

		int state = seed;
		int previous = -1;

		for (int i = 0; i < steps; i++)
		{
			int cell = MCF_Devices_Challenge.NextRandom(state) % GRID_SIZE;

			// Never the same cell twice running. Two identical presses in a
			// row are indistinguishable from one press that registered twice,
			// and a player who loses to that learns nothing.
			if (cell == previous)
				cell = (cell + 1) % GRID_SIZE;

			outSequence.Insert(cell);
			previous = cell;
		}
	}

	override bool Verify(int seed, int difficulty, string answer)
	{
		array<int> expected = {};
		Build(seed, difficulty, expected);

		return MCF_Devices_Challenge.Encode(expected) == answer;
	}
}
