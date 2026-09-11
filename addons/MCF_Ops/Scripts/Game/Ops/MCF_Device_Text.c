//! The small text rules a device screen needs, in one place.
//!
//! WHY THIS EXISTS. The handset grew these as private methods on its own menu.
//! The laptop needs every one of them and means exactly the same thing by each,
//! and two screens that answer "what is the initial of this name" differently
//! is the same class of bug as the two Game Master screens that edited the same
//! object under near-identical names (HANDOVER step 1). One place to look.
//!
//! Everything here is static and pure: it reads a string and returns one, and
//! nothing in it knows what a widget is.
class MCF_Device_Text
{
	//! What a heading written "who - what" is split on.
	//!
	//! Mission makers were writing "M. - 02:14" and "Outgoing - 0412" into
	//! headings long before anything read them apart, because that is how a
	//! person writes a line like that. A heading with no separator is all left
	//! half, which is why nothing authored before this reads differently.
	static const string HEAD_SEP = " - ";

	static string HeadPart(string heading)
	{
		int at = heading.IndexOf(HEAD_SEP);
		if (at < 0)
			return heading;

		return heading.Substring(0, at);
	}

	static string TailPart(string heading)
	{
		int at = heading.IndexOf(HEAD_SEP);
		if (at < 0)
			return "";

		int from = at + HEAD_SEP.Length();
		return heading.Substring(from, heading.Length() - from);
	}

	static string SubjectOf(string heading)
	{
		string tail = TailPart(heading);
		if (tail.IsEmpty())
			return heading;

		return tail;
	}

	static string SenderOf(string heading)
	{
		string head = HeadPart(heading);
		if (head.IsEmpty())
			return "(unknown sender)";

		return head;
	}

	//! The letter on the disc. Digits and punctuation get a dash rather than a
	//! number, because "0" as a face reads as an error.
	static string Initial(string name)
	{
		if (name.IsEmpty())
			return "?";

		string first = name.Get(0);
		int code = first.ToAscii();

		// ToUpper MUTATES AND RETURNS AN INT -- see the Enfusion lessons in
		// HANDOVER. Call it for its effect and hand back the string itself.
		if (code >= 97 && code <= 122)
		{
			first.ToUpper();
			return first;
		}

		if (code >= 65 && code <= 90)
			return first;

		return "-";
	}

	//! A colour that belongs to the name, so the same sender is the same disc
	//! every time -- which is most of what makes a list of people scannable.
	//! NOT Math.RandomInt: that is broken on wide ranges and this has to give
	//! every machine the same answer anyway.
	static int AvatarColour(string name)
	{
		int seed = 11;
		int count = name.Length();
		for (int i = 0; i < count; i++)
		{
			seed = (seed * 31 + name.Get(i).ToAscii()) % 9973;
		}

		int index = seed % 6;

		if (index == 0)
			return 0xFF8C4A3C;

		if (index == 1)
			return 0xFF3E6E52;

		if (index == 2)
			return 0xFF3A5A78;

		if (index == 3)
			return 0xFF6A5A8C;

		if (index == 4)
			return 0xFF8A6A2E;

		return 0xFF4C5560;
	}

	//! A body, as it is meant to be read.
	//!
	//! THE CONFIG PARSER DOES NOT TURN "\n" INTO A NEWLINE. A mission maker
	//! writing a paragraph break into MCF_DeviceProfiles.conf gets the two
	//! characters through to the screen, and the device prints them at the
	//! reader: "signed for nine.\n\nIf anything happens to me". Every place
	//! that shows a body has to undo that, so it lives here rather than in
	//! each of the eleven of them.
	static string Body(string text)
	{
		// `out` IS A RESERVED WORD -- it is the out-parameter marker, and this
		// is the third local called that in one day. Named for what it holds.
		string plain = text;
		plain.Replace("\\n", "\n");
		return plain;
	}

	//! One line of a body, short enough to sit under a sender.
	static string Preview(string body, int limit)
	{
		string flat = body;
		flat.Replace("\\n", " ");
		flat.Replace("\n", " ");

		if (flat.Length() > limit)
			return flat.Substring(0, limit) + "...";

		return flat;
	}

	//! Everything in a written number that a keypad could have produced, so a
	//! contact saved as "555 0148" still answers to 5550148.
	static string Digits(string text)
	{
		string kept;
		int n = text.Length();

		for (int i = 0; i < n; i++)
		{
			string ch = text.Substring(i, 1);

			if (ch == "0" || ch == "1" || ch == "2" || ch == "3" || ch == "4" || ch == "5" || ch == "6" || ch == "7" || ch == "8" || ch == "9")
				kept = kept + ch;
		}

		return kept;
	}

	//! Trims to a length without the ellipsis, for places that have no room
	//! for one -- a task button, a call-log column.
	static string Clip(string text, int limit)
	{
		if (text.Length() <= limit)
			return text;

		return text.Substring(0, limit);
	}

	static string Pad2(int value)
	{
		if (value < 10)
			return "0" + value.ToString();

		return value.ToString();
	}
}
