//! Read the rule, open exactly the ports that match it.
//!
//! The one puzzle here that is not about hands. The keypad tests recall and the
//! signal lock tests search; this one tests reading a table under a clock,
//! which is the failure mode a hurried player actually has. It is also the only
//! one where a wrong press is recoverable -- you toggle it back -- so the
//! pressure comes from the timer rather than from a single fumble.
//!
//! EXACTLY, not "at least". Opening a port the rule does not cover is as wrong
//! as missing one, because the interesting mistake is the port you opened
//! without checking.

//! One row of the table.
class MCF_Devices_Port
{
	int m_iNumber;
	//! 0 is TCP, 1 is UDP. An int rather than an enum because it is derived
	//! straight from the generator and never leaves this file.
	int m_iProtocol;
	int m_iResponse;
}

class MCF_Devices_Puzzle_Ports : MCF_Devices_Puzzle
{
	//! Rows the layout has. The table never shows more than this; difficulty
	//! decides how many of them are in play.
	static const int MAX_PORTS = 10;
	static const int MIN_PORTS = 6;

	//! Rules available at low and at high difficulty. The last two need a
	//! second look at each row rather than one, which is the whole increase.
	static const int RULES_EASY = 4;
	static const int RULES_HARD = 6;

	static const int PROTO_TCP = 0;
	static const int PROTO_UDP = 1;

	override string Title()
	{
		return "PORT TABLE";
	}

	override string Instruction()
	{
		return "Open exactly the ports the rule covers -- no more, no fewer -- then accept.";
	}

	//! The most generous clock of the three. Reading ten rows against a rule is
	//! slower than anything the other two ask for, and a short clock here would
	//! not make it harder, only luckier.
	override float TimeLimit(int difficulty)
	{
		return MCF_Devices_Challenge.ScaleSeconds(difficulty, 40.0, 22.0);
	}

	int PortCount(int difficulty)
	{
		int d = MCF_Devices_Challenge.Clamp(difficulty, 0, MCF_Devices_Challenge.MAX_DIFFICULTY);
		float span = MCF_Devices_Challenge.MAX_DIFFICULTY;
		float count = MIN_PORTS + (MAX_PORTS - MIN_PORTS) * (d / span);
		return Math.Round(count);
	}

	int RuleCount(int difficulty)
	{
		if (difficulty >= 2)
			return RULES_HARD;

		return RULES_EASY;
	}

	//! Builds the table and the rule. Deterministic from the seed, including
	//! the nudge below, so the server and the client always read the same rows.
	void Build(int seed, int difficulty, notnull array<ref MCF_Devices_Port> outPorts, out int outRule)
	{
		outPorts.Clear();

		int state = seed;
		int count = PortCount(difficulty);

		for (int i = 0; i < count; i++)
		{
			MCF_Devices_Port port = new MCF_Devices_Port();
			port.m_iNumber = 1024 + MCF_Devices_Challenge.NextRandom(state) % 8976;
			port.m_iProtocol = MCF_Devices_Challenge.NextRandom(state) % 2;
			port.m_iResponse = MCF_Devices_Challenge.NextRandom(state) % 100;
			outPorts.Insert(port);
		}

		outRule = MCF_Devices_Challenge.NextRandom(state) % RuleCount(difficulty);

		// A rule that covers every row, or none of them, is not a puzzle: the
		// player either presses everything or presses nothing and cannot tell
		// which of the two it was. Walk one row's response until the table is
		// mixed. Bounded, and identical on both machines because nothing here
		// touches the generator again.
		int guard = 0;
		while (guard < 100 && IsDegenerate(outPorts, outRule))
		{
			MCF_Devices_Port first = outPorts[0];
			first.m_iResponse = (first.m_iResponse + 1) % 100;
			first.m_iProtocol = (first.m_iProtocol + 1) % 2;
			// The number moves too, or rule 5 -- which reads the number and
			// nothing else -- could never be walked out of a degenerate table.
			first.m_iNumber = 1024 + ((first.m_iNumber - 1024 + 1) % 8976);
			guard++;
		}
	}

	protected bool IsDegenerate(notnull array<ref MCF_Devices_Port> ports, int rule)
	{
		int matched = 0;
		foreach (MCF_Devices_Port port : ports)
		{
			if (Matches(port, rule))
				matched++;
		}

		return matched == 0 || matched == ports.Count();
	}

	//! Whether one row is covered by one rule. The single place the rules are
	//! actually defined -- RuleText below only describes what this decides.
	bool Matches(notnull MCF_Devices_Port port, int rule)
	{
		switch (rule)
		{
			case 0: return port.m_iResponse % 2 == 0;
			case 1: return port.m_iResponse % 2 == 1;
			case 2: return port.m_iResponse >= 50;
			case 3: return port.m_iProtocol == PROTO_UDP;
			case 4: return port.m_iProtocol == PROTO_TCP && port.m_iResponse % 2 == 1;
			case 5: return port.m_iNumber % 2 == 0;
		}

		return false;
	}

	//! What the player is told. Must say exactly what Matches does, in words --
	//! a puzzle whose rule text and rule disagree is unwinnable and looks like
	//! bad luck rather than a bug.
	string RuleText(int rule)
	{
		switch (rule)
		{
			case 0: return "OPEN EVERY PORT WHOSE RESPONSE IS EVEN";
			case 1: return "OPEN EVERY PORT WHOSE RESPONSE IS ODD";
			case 2: return "OPEN EVERY PORT WHOSE RESPONSE IS 50 OR ABOVE";
			case 3: return "OPEN EVERY UDP PORT";
			case 4: return "OPEN EVERY TCP PORT WHOSE RESPONSE IS ODD";
			case 5: return "OPEN EVERY PORT WITH AN EVEN PORT NUMBER";
		}

		return "OPEN NOTHING";
	}

	string ProtocolName(int protocol)
	{
		if (protocol == PROTO_UDP)
			return "UDP";

		return "TCP";
	}

	//! One row as the player sees it.
	string DescribePort(notnull MCF_Devices_Port port)
	{
		return port.m_iNumber.ToString() + "   " + ProtocolName(port.m_iProtocol) + "   RESP " + port.m_iResponse.ToString();
	}

	//! One character per row, in table order: 1 open, 0 shut. Positional rather
	//! than a list of port numbers, so the answer's length is fixed and a
	//! malformed one is obvious before it is parsed.
	static string EncodeAnswer(notnull array<bool> opened)
	{
		string encoded = "";
		foreach (bool isOpen : opened)
		{
			if (isOpen)
				encoded = encoded + "1";
			else
				encoded = encoded + "0";
		}

		return encoded;
	}

	override bool Verify(int seed, int difficulty, string answer)
	{
		array<ref MCF_Devices_Port> ports = {};
		int rule;
		Build(seed, difficulty, ports, rule);

		array<bool> expected = {};
		foreach (MCF_Devices_Port port : ports)
		{
			expected.Insert(Matches(port, rule));
		}

		return EncodeAnswer(expected) == answer;
	}
}
