//! What every break-in puzzle has to be able to do.
//!
//! THE SERVER SIDE IS POLYMORPHIC, THE CLIENT SIDE IS NOT, and that is
//! deliberate. Everything the server needs -- how long you get, and whether
//! this answer is the right one -- is the same question for every game, so it
//! is asked through this base class and MCF_Devices_LockComponent never learns
//! which game it handed out. Drawing is the opposite: a flashing grid, a pair
//! of waveforms and a port table have nothing in common visually, and forcing
//! them through one "give me your widgets" interface would produce a contract
//! that every implementation lies about. So MCF_Devices_HackMenu switches on
//! the kind and calls each puzzle's own builders directly.
//!
//! The practical rule this leaves: ADDING A PUZZLE TOUCHES THREE PLACES. The
//! enum in MCF_Devices_Challenge.c, GetPuzzle beside it, and the panel switch
//! in the menu. Nothing else -- not the lock, not the player controller, not
//! the wire format.

class MCF_Devices_Puzzle
{
	//! Heading at the top of the screen. Says which game this is, so a player
	//! who has met it before recognises it before reading anything.
	string Title()
	{
		return "SECURED DEVICE";
	}

	//! One line telling the player what winning looks like. Written for
	//! somebody who has never seen this puzzle, because most of them have not.
	string Instruction()
	{
		return "";
	}

	//! Seconds the server will accept an answer for, at this difficulty.
	float TimeLimit(int difficulty)
	{
		return MCF_Devices_Challenge.ScaleSeconds(difficulty, 12.0, 5.0);
	}

	//! Whether this answer solves the puzzle this seed produces. Runs on the
	//! server, from the seed the server issued -- never from anything the
	//! client asserted about its own result.
	bool Verify(int seed, int difficulty, string answer)
	{
		return false;
	}
}
