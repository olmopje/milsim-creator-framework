//! The break-in screen. Three games behind one door.
//!
//! NOT A MENU. It was one -- MCF_Devices_HackMenu, its own preset, its own
//! window over the top of the device shell -- and that is exactly what made
//! breaking into a phone feel like leaving the phone. So the puzzle is now
//! drawn INSIDE the device's glass by whoever owns that glass: this class binds
//! to a root widget it is handed, and knows nothing about menus, presets or
//! where on the screen it ended up. MCF_Intel_ShellMenu creates the panel from
//! MCF_DeviceHack.layout into the device's ScreenArea and ticks Update().
//!
//! The client draws whatever puzzle the seed calls for and sends back what the
//! player did. It never reports success -- MCF_Devices_Challenge.c explains why
//! at length.
//!
//! ONE LAYOUT, THREE PANELS. Each game gets its own frame in the layout and all
//! but one is hidden on open. Three separate layouts would have been tidier on
//! paper and worse in practice: the title, the clock, the stop button and the
//! whole submit path are identical for all three, and three copies of them
//! would drift the moment one was fixed.
//!
//! WHICH GAME IS NOT SENT. The seed decides it, and the seed is already on both
//! machines -- see MCF_Devices_Challenge.KindFor. So nothing about the puzzle
//! travels except the seed and the difficulty, and the server does not have to
//! remember what it handed out.
//!
//! The clock on screen is a courtesy. The one that decides is on the server,
//! started when it issued the seed, because a time a client reports is not
//! evidence. That means this display can be a little pessimistic on a bad
//! connection, which is the right way round -- better to look tight and pass
//! than look comfortable and be refused.

class MCF_Devices_HackScreen
{
	protected static const string W_TITLE = "Title";
	protected static const string W_STATUS = "Status";
	protected static const string W_TIMER = "Timer";
	protected static const string W_HINT = "Hint";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";
	protected static const string W_BUTTON_SUBMIT = "ButtonSubmit";

	protected static const string W_PANEL_KEYPAD = "PanelKeypad";
	protected static const string W_PANEL_WAVE = "PanelWave";
	protected static const string W_PANEL_PORTS = "PanelPorts";

	protected static const string W_CELL_PREFIX = "Cell";
	protected static const string W_WAVE_PREFIX = "Wave";
	protected static const string W_PORT_PREFIX = "Port";
	protected static const string W_DIALS = "Dials";
	protected static const string W_RULE = "Rule";
	protected static const string W_FREQ_DOWN = "FreqDown";
	protected static const string W_FREQ_UP = "FreqUp";
	protected static const string W_GAIN_DOWN = "GainDown";
	protected static const string W_GAIN_UP = "GainUp";

	//! Seconds the sequence is shown for, per step.
	protected static const float FLASH_SECONDS = 0.45;

	//! Wave cell colours, ARGB. Four states rather than two, because the thing
	//! the player is looking for is the overlap: when every lit cell is the
	//! same colour, they are done, and that reads without any text at all.
	protected static const int COLOUR_OFF = 0x8C1A2121;
	protected static const int COLOUR_TARGET = 0xB0A8B0B0;
	protected static const int COLOUR_MINE = 0xFFE0A020;
	protected static const int COLOUR_BOTH = 0xFF30E060;

	//! Fires when the puzzle is over, however it ended -- submitted, failed or
	//! walked away from. The owner takes the panel down; this class does not
	//! know how it was put up.
	ref ScriptInvoker m_OnFinished = new ScriptInvoker();

	protected Widget m_wRoot;
	protected RplId m_DeviceId;
	protected int m_iSeed;
	protected int m_iDifficulty;
	protected int m_iKind;

	protected TextWidget m_wTitle;
	protected TextWidget m_wStatus;
	protected TextWidget m_wTimer;
	protected TextWidget m_wHint;
	protected TextWidget m_wDials;
	protected TextWidget m_wRule;
	protected Widget m_wPanelKeypad;
	protected Widget m_wPanelWave;
	protected Widget m_wPanelPorts;
	protected SCR_ButtonTextComponent m_ButtonClose;
	protected SCR_ButtonTextComponent m_ButtonSubmit;

	//! Runs for every game: the clock starts when the player may first act.
	protected bool m_bAccepting;
	protected bool m_bFinished;
	protected float m_fDeadline;

	// keypad
	protected ref array<int> m_aSequence = {};
	protected ref array<int> m_aPressed = {};
	protected ref array<SCR_ButtonTextComponent> m_aCells = {};
	protected bool m_bShowing;
	protected int m_iFlashIndex;

	// signal lock
	protected ref array<ImageWidget> m_aWaveCells = {};
	protected int m_iTargetFrequency;
	protected int m_iTargetAmplitude;
	protected int m_iMyFrequency;
	protected int m_iMyAmplitude;
	protected int m_iMaxFrequency;
	protected SCR_ButtonTextComponent m_FreqDown;
	protected SCR_ButtonTextComponent m_FreqUp;
	protected SCR_ButtonTextComponent m_GainDown;
	protected SCR_ButtonTextComponent m_GainUp;

	// port table
	protected ref array<ref MCF_Devices_Port> m_aPorts = {};
	protected ref array<SCR_ButtonTextComponent> m_aPortButtons = {};
	protected ref array<bool> m_aPortOpen = {};
	protected int m_iRule;

	//! Binds to a freshly created panel and starts the game the seed calls for.
	//!
	//! The panel must be VISIBLE when this is called.
	//! SCR_ButtonTextComponent.GetButtonText does not find a button inside a
	//! subtree marked hidden, which is why the panel is created on demand and
	//! destroyed afterwards rather than shipped hidden inside the device layout.
	void Start(notnull Widget root, RplId deviceId, int seed, int difficulty)
	{
		m_wRoot = root;
		m_DeviceId = deviceId;
		m_iSeed = seed;
		m_iDifficulty = difficulty;
		m_iKind = MCF_Devices_Challenge.KindFor(m_iSeed);

		m_wTitle = TextWidget.Cast(root.FindAnyWidget(W_TITLE));
		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wTimer = TextWidget.Cast(root.FindAnyWidget(W_TIMER));
		m_wHint = TextWidget.Cast(root.FindAnyWidget(W_HINT));
		m_wPanelKeypad = root.FindAnyWidget(W_PANEL_KEYPAD);
		m_wPanelWave = root.FindAnyWidget(W_PANEL_WAVE);
		m_wPanelPorts = root.FindAnyWidget(W_PANEL_PORTS);

		// A visible way out, because Escape is not always available: in the
		// Workbench it stops the play session instead of closing the screen.
		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);

		m_ButtonSubmit = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_SUBMIT, root);
		if (m_ButtonSubmit)
			m_ButtonSubmit.m_OnClicked.Insert(OnSubmitClicked);

		MCF_Devices_Puzzle puzzle = MCF_Devices_Challenge.GetPuzzle(m_iKind);

		// The seed is logged because it is the only thing that decides what the
		// player sees: a report of "this one was impossible" is reproducible
		// from this line alone.
		MCF_Core_Log.Debug("hack screen: seed=" + m_iSeed.ToString() + " kind=" + m_iKind.ToString() + " (" + puzzle.Title() + ")");

		if (m_wTitle)
			m_wTitle.SetText(puzzle.Title());

		if (m_wHint)
			m_wHint.SetText(puzzle.Instruction());

		ShowPanel(m_iKind);

		if (m_iKind == MCF_EPuzzleKind.FREQUENCY)
			StartFrequency(root);
		else if (m_iKind == MCF_EPuzzleKind.PORTS)
			StartPorts(root);
		else
			StartSequence(root);
	}

	//! Whether the puzzle is still running. The owner asks before ticking, and
	//! before deciding what a BACK press means.
	bool IsRunning()
	{
		return !m_bFinished;
	}

	protected void ShowPanel(int kind)
	{
		if (m_wPanelKeypad)
			m_wPanelKeypad.SetVisible(kind == MCF_EPuzzleKind.SEQUENCE);

		if (m_wPanelWave)
			m_wPanelWave.SetVisible(kind == MCF_EPuzzleKind.FREQUENCY);

		if (m_wPanelPorts)
			m_wPanelPorts.SetVisible(kind == MCF_EPuzzleKind.PORTS);

		// The keypad submits itself on the last press, so an ACCEPT button
		// there would be a button that is never the thing you press.
		if (m_ButtonSubmit)
			m_ButtonSubmit.GetRootWidget().SetVisible(kind != MCF_EPuzzleKind.SEQUENCE);
	}

	//! Ticked once a frame by whoever owns the panel.
	void Update()
	{
		if (m_bFinished)
			return;

		if (m_bShowing)
		{
			UpdateShowing();
			return;
		}

		if (!m_bAccepting)
			return;

		float left = m_fDeadline - Now();
		if (left <= 0)
		{
			Fail("OUT OF TIME");
			return;
		}

		if (m_wTimer)
			m_wTimer.SetText(FormatSeconds(left));
	}

	//! Starts the clock and lets the player act. Every game goes through here,
	//! so the on-screen clock always agrees with the puzzle's own limit.
	protected void StartAccepting(string status)
	{
		m_bShowing = false;
		m_bAccepting = true;
		m_fDeadline = Now() + MCF_Devices_Challenge.GetPuzzle(m_iKind).TimeLimit(m_iDifficulty);

		if (m_wStatus)
			m_wStatus.SetText(status);
	}

	// ====================================================== keypad: sequence

	protected void StartSequence(notnull Widget root)
	{
		m_aCells.Clear();
		for (int i = 0; i < MCF_Devices_Puzzle_Sequence.GRID_SIZE; i++)
		{
			SCR_ButtonTextComponent cell = SCR_ButtonTextComponent.GetButtonText(W_CELL_PREFIX + i.ToString(), root);
			if (!cell)
			{
				MCF_Core_Log.Warn("hack screen is missing widget '" + W_CELL_PREFIX + i.ToString() + "'");
				continue;
			}

			cell.m_OnClicked.Insert(OnCellClicked);
			m_aCells.Insert(cell);
		}

		MCF_Devices_Puzzle_Sequence sequence = MCF_Devices_Puzzle_Sequence.Cast(MCF_Devices_Challenge.GetPuzzle(MCF_EPuzzleKind.SEQUENCE));
		sequence.Build(m_iSeed, m_iDifficulty, m_aSequence);
		m_aPressed.Clear();

		StartShowing();
	}

	protected void StartShowing()
	{
		m_bShowing = true;
		m_bAccepting = false;
		m_iFlashIndex = -1;
		m_fDeadline = Now() + FLASH_SECONDS;

		SetCellsEnabled(false);

		if (m_wStatus)
			m_wStatus.SetText("WATCH");
	}

	protected void UpdateShowing()
	{
		if (Now() < m_fDeadline)
			return;

		ClearCellHighlights();

		m_iFlashIndex++;
		if (m_iFlashIndex >= m_aSequence.Count())
		{
			SetCellsEnabled(true);
			StartAccepting("REPEAT IT");
			return;
		}

		Highlight(m_aSequence[m_iFlashIndex], true);
		m_fDeadline = Now() + FLASH_SECONDS;
	}

	protected void OnCellClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bAccepting)
			return;

		int index = m_aCells.Find(button);
		if (index < 0)
			return;

		// Wrong press ends it immediately. Letting a player finish a sequence
		// they have already broken wastes their remaining seconds and tells
		// them nothing.
		int step = m_aPressed.Count();
		if (step >= m_aSequence.Count() || m_aSequence[step] != index)
		{
			Fail("WRONG");
			return;
		}

		m_aPressed.Insert(index);

		if (m_aPressed.Count() < m_aSequence.Count())
			return;

		SetCellsEnabled(false);
		Submit(MCF_Devices_Challenge.Encode(m_aPressed));
	}

	protected void SetCellsEnabled(bool enabled)
	{
		foreach (SCR_ButtonTextComponent cell : m_aCells)
		{
			cell.SetEnabled(enabled);
		}
	}

	protected void ClearCellHighlights()
	{
		foreach (SCR_ButtonTextComponent cell : m_aCells)
		{
			cell.SetToggled(false, false, false);
		}
	}

	protected void Highlight(int index, bool on)
	{
		if (index < 0 || index >= m_aCells.Count())
			return;

		m_aCells[index].SetToggled(on, false, false);
	}

	// ================================================== signal lock: frequency

	protected void StartFrequency(notnull Widget root)
	{
		MCF_Devices_Puzzle_Frequency wave = MCF_Devices_Puzzle_Frequency.Cast(MCF_Devices_Challenge.GetPuzzle(MCF_EPuzzleKind.FREQUENCY));

		m_iMaxFrequency = wave.MaxFrequency(m_iDifficulty);

		// Locals rather than the fields directly: an `out` parameter bound to a
		// member is the kind of thing that works until it does not, and this
		// costs four lines.
		int targetFrequency, targetAmplitude, myFrequency, myAmplitude;
		wave.BuildTarget(m_iSeed, m_iDifficulty, targetFrequency, targetAmplitude);
		wave.BuildStart(m_iSeed, m_iDifficulty, myFrequency, myAmplitude);

		m_iTargetFrequency = targetFrequency;
		m_iTargetAmplitude = targetAmplitude;
		m_iMyFrequency = myFrequency;
		m_iMyAmplitude = myAmplitude;

		m_wDials = TextWidget.Cast(root.FindAnyWidget(W_DIALS));

		m_aWaveCells.Clear();
		for (int row = 0; row < MCF_Devices_Puzzle_Frequency.ROWS; row++)
		{
			for (int col = 0; col < MCF_Devices_Puzzle_Frequency.COLS; col++)
			{
				// Inserted in row-major order and read back the same way, so a
				// missing widget would shift the whole grid -- hence null goes
				// in rather than being skipped.
				m_aWaveCells.Insert(ImageWidget.Cast(root.FindAnyWidget(W_WAVE_PREFIX + col.ToString() + "_" + row.ToString())));
			}
		}

		m_FreqDown = BindDial(root, W_FREQ_DOWN);
		m_FreqUp = BindDial(root, W_FREQ_UP);
		m_GainDown = BindDial(root, W_GAIN_DOWN);
		m_GainUp = BindDial(root, W_GAIN_UP);

		if (m_ButtonSubmit)
			m_ButtonSubmit.SetText("LOCK");

		DrawWave();
		StartAccepting("TUNE IT");
	}

	//! Finds one dial and points it at the single handler below.
	//!
	//! FOUR BUTTONS, ONE HANDLER, and not because it is shorter. Enforce refuses
	//! a method that takes a `func` parameter -- "func arguments are not
	//! supported in script methods" -- so a helper cannot be handed the callback
	//! to bind. Telling the buttons apart inside the handler is what is left,
	//! and it is no worse: the click already carries which button sent it.
	protected SCR_ButtonTextComponent BindDial(notnull Widget root, string name)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, root);
		if (!button)
		{
			MCF_Core_Log.Warn("hack screen is missing widget '" + name + "'");
			return null;
		}

		button.m_OnClicked.Insert(OnDialClicked);
		return button;
	}

	protected void OnDialClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bAccepting || !button)
			return;

		if (button == m_FreqDown)
			m_iMyFrequency = StepWrapped(m_iMyFrequency, -1, MCF_Devices_Puzzle_Frequency.MIN_FREQUENCY, m_iMaxFrequency);
		else if (button == m_FreqUp)
			m_iMyFrequency = StepWrapped(m_iMyFrequency, 1, MCF_Devices_Puzzle_Frequency.MIN_FREQUENCY, m_iMaxFrequency);
		else if (button == m_GainDown)
			m_iMyAmplitude = StepWrapped(m_iMyAmplitude, -1, MCF_Devices_Puzzle_Frequency.MIN_AMPLITUDE, MCF_Devices_Puzzle_Frequency.MAX_AMPLITUDE);
		else if (button == m_GainUp)
			m_iMyAmplitude = StepWrapped(m_iMyAmplitude, 1, MCF_Devices_Puzzle_Frequency.MIN_AMPLITUDE, MCF_Devices_Puzzle_Frequency.MAX_AMPLITUDE);
		else
			return;

		DrawWave();
	}

	//! Wraps rather than clamps. A dial that stops dead at the end makes the
	//! player wonder whether it is broken or whether they are at the limit.
	protected int StepWrapped(int value, int step, int low, int high)
	{
		int span = high - low + 1;
		return low + (((value - low + step) % span) + span) % span;
	}

	protected void DrawWave()
	{
		MCF_Devices_Puzzle_Frequency wave = MCF_Devices_Puzzle_Frequency.Cast(MCF_Devices_Challenge.GetPuzzle(MCF_EPuzzleKind.FREQUENCY));

		for (int col = 0; col < MCF_Devices_Puzzle_Frequency.COLS; col++)
		{
			int targetRow = wave.RowAt(col, m_iTargetFrequency, m_iTargetAmplitude);
			int myRow = wave.RowAt(col, m_iMyFrequency, m_iMyAmplitude);

			for (int row = 0; row < MCF_Devices_Puzzle_Frequency.ROWS; row++)
			{
				int index = row * MCF_Devices_Puzzle_Frequency.COLS + col;
				if (index >= m_aWaveCells.Count())
					continue;

				ImageWidget cell = m_aWaveCells[index];
				if (!cell)
					continue;

				int colour = COLOUR_OFF;
				if (row == targetRow && row == myRow)
					colour = COLOUR_BOTH;
				else if (row == targetRow)
					colour = COLOUR_TARGET;
				else if (row == myRow)
					colour = COLOUR_MINE;

				cell.SetColor(Color.FromInt(colour));
			}
		}

		if (m_wDials)
			m_wDials.SetText("FREQ " + m_iMyFrequency.ToString() + "   GAIN " + m_iMyAmplitude.ToString());
	}

	// ====================================================== port table: ports

	protected void StartPorts(notnull Widget root)
	{
		MCF_Devices_Puzzle_Ports table = MCF_Devices_Puzzle_Ports.Cast(MCF_Devices_Challenge.GetPuzzle(MCF_EPuzzleKind.PORTS));

		int rule;
		table.Build(m_iSeed, m_iDifficulty, m_aPorts, rule);
		m_iRule = rule;

		m_wRule = TextWidget.Cast(root.FindAnyWidget(W_RULE));
		if (m_wRule)
			m_wRule.SetText(table.RuleText(m_iRule));

		m_aPortButtons.Clear();
		m_aPortOpen.Clear();

		for (int i = 0; i < MCF_Devices_Puzzle_Ports.MAX_PORTS; i++)
		{
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(W_PORT_PREFIX + i.ToString(), root);
			if (!button)
				continue;

			// The table is shorter at low difficulty. Rows past the end are
			// hidden rather than blanked, so the player cannot press one and
			// wonder why nothing happened.
			bool used = i < m_aPorts.Count();
			button.GetRootWidget().SetVisible(used);

			if (!used)
				continue;

			button.SetText(table.DescribePort(m_aPorts[i]));
			button.SetToggled(false, false, false);
			button.m_OnClicked.Insert(OnPortClicked);

			m_aPortButtons.Insert(button);
			m_aPortOpen.Insert(false);
		}

		if (m_ButtonSubmit)
			m_ButtonSubmit.SetText("ACCEPT");

		StartAccepting("READ THE RULE");
	}

	//! The open/shut state is kept here rather than read back off the widget,
	//! because what gets sent has to be what this screen believes, not what a
	//! toggle happened to be showing.
	protected void OnPortClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bAccepting)
			return;

		int index = m_aPortButtons.Find(button);
		if (index < 0 || index >= m_aPortOpen.Count())
			return;

		m_aPortOpen[index] = !m_aPortOpen[index];
		button.SetToggled(m_aPortOpen[index], false, false);
	}

	// ============================================================== finishing

	protected void OnSubmitClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bAccepting)
			return;

		if (m_iKind == MCF_EPuzzleKind.FREQUENCY)
		{
			Submit(MCF_Devices_Puzzle_Frequency.EncodeAnswer(m_iMyFrequency, m_iMyAmplitude));
			return;
		}

		if (m_iKind == MCF_EPuzzleKind.PORTS)
		{
			Submit(MCF_Devices_Puzzle_Ports.EncodeAnswer(m_aPortOpen));
			return;
		}
	}

	//! Sends what the player did. Whether it was right is the server's to say --
	//! this client already knows, and is deliberately not the one asked.
	protected void Submit(string answer)
	{
		m_bAccepting = false;

		if (m_wStatus)
			m_wStatus.SetText("...");

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestDeviceAnswer(m_DeviceId, answer);

		Finish();
	}

	protected void Fail(string reason)
	{
		m_bShowing = false;
		m_bAccepting = false;
		SetCellsEnabled(false);

		if (m_wStatus)
			m_wStatus.SetText(reason);

		// Nothing is sent. The server's challenge simply goes unanswered and
		// expires, and the next attempt gets a fresh seed -- which is what
		// stops a player from learning one puzzle and retrying it.
		Finish();
	}

	//! Walking away. Same as failing, minus the message: the challenge on the
	//! server is left to expire.
	void Cancel()
	{
		m_bShowing = false;
		m_bAccepting = false;
		Finish();
	}

	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Cancel();
	}

	//! Announces the end exactly once. Submit and Fail can both be reached from
	//! inside a click handler that the owner is about to tear down, so a second
	//! announcement would arrive after the widgets are gone.
	protected void Finish()
	{
		if (m_bFinished)
			return;

		m_bFinished = true;
		m_OnFinished.Invoke();
	}

	protected float Now()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetWorldTime() / 1000.0;
	}

	protected string FormatSeconds(float seconds)
	{
		int whole = seconds;
		int tenths = (seconds - whole) * 10;
		return whole.ToString() + "." + tenths.ToString();
	}
}
