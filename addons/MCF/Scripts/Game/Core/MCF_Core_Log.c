//! MCF's logging, behind a switch.
//!
//! Every node writes a line when it initialises (saying what it listens for
//! and what it publishes) and another when it fires. That trace is what makes
//! a broken chain diagnosable: reading it top to bottom shows exactly where
//! the chain stops, and in practice the answer is nearly always a mismatched
//! event name.
//!
//! It earned its keep. On 2026-09-09 four nodes were found that compiled
//! cleanly, initialised without complaint and could never fire; the logging is
//! what exposed them, and how each fix was confirmed.
//!
//! So it stays on by default while the framework is young. Turn it off for a
//! production server that does not want the chatter:
//!
//!     MCF_Core_Log.SetEnabled(false);
//!
//! Warnings and errors ignore the switch -- if something is actually wrong,
//! you want to hear about it regardless.

class MCF_Core_Log
{
	protected static const string PREFIX = "[MCF] ";

	protected static bool s_bEnabled = true;

	//! Turn the informational trace on or off. Warnings and errors are not
	//! affected.
	static void SetEnabled(bool enabled)
	{
		s_bEnabled = enabled;
	}

	static bool IsEnabled()
	{
		return s_bEnabled;
	}

	//! Normal trace: initialisation, events firing, state changes.
	static void Debug(string message)
	{
		if (s_bEnabled)
			Print(PREFIX + message);
	}

	//! Something is off but the mission can continue. Always logged.
	static void Warn(string message)
	{
		Print(PREFIX + message, LogLevel.WARNING);
	}

	//! Something is broken. Always logged.
	static void Error(string message)
	{
		Print(PREFIX + message, LogLevel.ERROR);
	}
}
