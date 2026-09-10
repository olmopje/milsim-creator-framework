//! Tune your wave until it sits on top of the one you were given.
//!
//! Two waves are drawn into ONE grid rather than two, because the whole point
//! is the moment they coincide, and two stacked graphs make the player compare
//! shapes instead of watching them merge. A cell lit by the target only, by
//! yours only, or by both gets a different colour -- so "solved" is the screen
//! going one colour, which needs no reading at all.
//!
//! THE PLAYER CAN SEE THE TARGET, AND THAT IS FINE. Both machines derive the
//! whole puzzle from the seed, so a modified client could always read the
//! answer -- that is equally true of the keypad, whose sequence is flashed at
//! the player anyway. What the server guarantees is narrower and is the thing
//! that matters: you cannot be let in without sending the right answer inside
//! the time the server itself measured. See MCF_Devices_Challenge.c.

class MCF_Devices_Puzzle_Frequency : MCF_Devices_Puzzle
{
	//! The display grid. Odd row count so there is a true centre line for the
	//! wave to cross.
	static const int COLS = 16;
	static const int ROWS = 9;
	static const int CENTRE_ROW = 4;

	//! Amplitude runs 1..4, which is exactly what fits above and below the
	//! centre line. Any wider and the wave would clip, which looks like a bug
	//! rather than a difficulty.
	static const int MIN_AMPLITUDE = 1;
	static const int MAX_AMPLITUDE = 4;

	//! Frequency range at the easiest and hardest setting. More choices is the
	//! difficulty here: the tuning itself never gets harder, the space you are
	//! searching does.
	static const int MIN_FREQUENCY = 1;
	static const int MAX_FREQUENCY_EASY = 4;
	static const int MAX_FREQUENCY_HARD = 8;

	override string Title()
	{
		return "SIGNAL LOCK";
	}

	override string Instruction()
	{
		return "Tune frequency and gain until your trace covers the reference, then lock.";
	}

	//! Longer than the keypad. This one is a search rather than a recall, and a
	//! keypad clock would make it a coin flip.
	override float TimeLimit(int difficulty)
	{
		return MCF_Devices_Challenge.ScaleSeconds(difficulty, 30.0, 16.0);
	}

	//! The highest frequency in play at this difficulty. Both sides need this
	//! to agree or the player's dial and the server's answer live in different
	//! ranges.
	int MaxFrequency(int difficulty)
	{
		int d = MCF_Devices_Challenge.Clamp(difficulty, 0, MCF_Devices_Challenge.MAX_DIFFICULTY);
		float span = MCF_Devices_Challenge.MAX_DIFFICULTY;
		float f = MAX_FREQUENCY_EASY + (MAX_FREQUENCY_HARD - MAX_FREQUENCY_EASY) * (d / span);
		return Math.Round(f);
	}

	//! The wave the player has to reach.
	void BuildTarget(int seed, int difficulty, out int frequency, out int amplitude)
	{
		int state = seed;
		int maxF = MaxFrequency(difficulty);

		frequency = MIN_FREQUENCY + MCF_Devices_Challenge.NextRandom(state) % (maxF - MIN_FREQUENCY + 1);
		amplitude = MIN_AMPLITUDE + MCF_Devices_Challenge.NextRandom(state) % (MAX_AMPLITUDE - MIN_AMPLITUDE + 1);
	}

	//! Where the player's dials start. Never on the answer, and never one nudge
	//! away from it -- a puzzle that solves itself on the first press is not a
	//! puzzle, and one that is already solved when it opens looks broken.
	void BuildStart(int seed, int difficulty, out int frequency, out int amplitude)
	{
		int targetF, targetA;
		BuildTarget(seed, difficulty, targetF, targetA);

		int maxF = MaxFrequency(difficulty);
		int state = seed;

		// Three draws in, so the start is not visibly related to the target.
		MCF_Devices_Challenge.NextRandom(state);
		MCF_Devices_Challenge.NextRandom(state);

		frequency = MIN_FREQUENCY + MCF_Devices_Challenge.NextRandom(state) % (maxF - MIN_FREQUENCY + 1);
		amplitude = MIN_AMPLITUDE + MCF_Devices_Challenge.NextRandom(state) % (MAX_AMPLITUDE - MIN_AMPLITUDE + 1);

		// Walk it off the answer if the draw landed on or beside it. Wrapping
		// inside the range keeps it legal whichever end it started at.
		int guard = 0;
		while (guard < MAX_FREQUENCY_HARD && AbsInt(frequency - targetF) < 2)
		{
			frequency = MIN_FREQUENCY + ((frequency - MIN_FREQUENCY + 2) % (maxF - MIN_FREQUENCY + 1));
			guard++;
		}

		if (amplitude == targetA)
			amplitude = MIN_AMPLITUDE + ((amplitude - MIN_AMPLITUDE + 2) % (MAX_AMPLITUDE - MIN_AMPLITUDE + 1));
	}

	//! Which row the trace occupies in this column. One sample per column, so a
	//! high frequency reads as a dense zigzag rather than a smooth curve --
	//! which is what a 16-column display can honestly show, and it stays
	//! readable because the two traces alias identically.
	int RowAt(int col, int frequency, int amplitude)
	{
		float phase = 2.0 * Math.PI * frequency * col / COLS;
		float offset = amplitude * Math.Sin(phase);
		int row = CENTRE_ROW - Math.Round(offset);

		return MCF_Devices_Challenge.Clamp(row, 0, ROWS - 1);
	}

	//! What the client sends: the two dial positions, nothing else. Not "I
	//! matched it" -- the server decides that by building the target itself.
	static string EncodeAnswer(int frequency, int amplitude)
	{
		return frequency.ToString() + "," + amplitude.ToString();
	}

	override bool Verify(int seed, int difficulty, string answer)
	{
		int frequency, amplitude;
		BuildTarget(seed, difficulty, frequency, amplitude);

		return EncodeAnswer(frequency, amplitude) == answer;
	}

	protected int AbsInt(int value)
	{
		if (value < 0)
			return -value;

		return value;
	}
}
