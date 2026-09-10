//! The break-in screen: repeat the sequence the device shows you.
//!
//! The client draws the puzzle from a seed the server sent, and sends back the
//! order the player pressed. It never reports success -- MCF_Devices_Challenge.c
//! explains why at length.
//!
//! HOW IT PLAYS. The grid flashes the sequence once, then goes quiet and the
//! player repeats it. Get one wrong and the attempt is over; the prompt comes
//! back and the server issues a fresh seed next time. Difficulty sets both the
//! length and the clock.
//!
//! The clock on screen is a courtesy. The one that decides is on the server,
//! started when it issued the seed, because a time a client reports is not
//! evidence. That means this display can be a little pessimistic on a bad
//! connection, which is the right way round -- better to look tight and pass
//! than look comfortable and be refused.

class MCF_Devices_HackMenu : ChimeraMenuBase
{
	protected static const string W_TITLE = "Title";
	protected static const string W_STATUS = "Status";
	protected static const string W_TIMER = "Timer";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";
	protected static const string W_CELL_PREFIX = "Cell";

	//! Seconds the sequence is shown for, per step.
	protected static const float FLASH_SECONDS = 0.45;

	protected static RplId s_PendingDevice;
	protected static int s_PendingSeed;
	protected static int s_PendingDifficulty;

	protected RplId m_DeviceId;
	protected int m_iSeed;
	protected int m_iDifficulty;

	protected ref array<int> m_aSequence = {};
	protected ref array<int> m_aPressed = {};
	protected ref array<SCR_ButtonTextComponent> m_aCells = {};

	protected TextWidget m_wTitle;
	protected TextWidget m_wStatus;
	protected TextWidget m_wTimer;
	protected SCR_ButtonTextComponent m_ButtonClose;

	protected bool m_bShowing;      //!< replaying the sequence at the player
	protected bool m_bAccepting;    //!< waiting for presses
	protected int m_iFlashIndex;
	protected float m_fDeadline;

	static void OpenFor(RplId deviceId, int seed, int difficulty)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return;

		s_PendingDevice = deviceId;
		s_PendingSeed = seed;
		s_PendingDifficulty = difficulty;

		menuManager.OpenMenu(ChimeraMenuPreset.MCF_DeviceHack);
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_DeviceId = s_PendingDevice;
		m_iSeed = s_PendingSeed;
		m_iDifficulty = s_PendingDifficulty;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("hack screen opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wTitle = TextWidget.Cast(root.FindAnyWidget(W_TITLE));
		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wTimer = TextWidget.Cast(root.FindAnyWidget(W_TIMER));

		// A visible way out, because Escape is not always available: in the
		// Workbench it stops the play session instead of closing the screen.
		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);

		m_aCells.Clear();
		for (int i = 0; i < MCF_Devices_Challenge.GRID_SIZE; i++)
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

		MCF_Devices_Challenge.Build(m_iSeed, m_iDifficulty, m_aSequence);
		m_aPressed.Clear();

		if (m_wTitle)
			m_wTitle.SetText("SECURED DEVICE");

		StartShowing();
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

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

	// ------------------------------------------------------ show the answer

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
			StartAccepting();
			return;
		}

		Highlight(m_aSequence[m_iFlashIndex], true);
		m_fDeadline = Now() + FLASH_SECONDS;
	}

	// --------------------------------------------------------- take presses

	protected void StartAccepting()
	{
		m_bShowing = false;
		m_bAccepting = true;
		m_fDeadline = Now() + MCF_Devices_Challenge.TimeLimit(m_iDifficulty);

		SetCellsEnabled(true);

		if (m_wStatus)
			m_wStatus.SetText("REPEAT IT");
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

		Submit();
	}

	//! Sends what was pressed. Whether it was right is the server's to say --
	//! this client already knows, and is deliberately not the one asked.
	protected void Submit()
	{
		m_bAccepting = false;
		SetCellsEnabled(false);

		if (m_wStatus)
			m_wStatus.SetText("...");

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestDeviceAnswer(m_DeviceId, MCF_Devices_Challenge.Encode(m_aPressed));

		Close();
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
		Close();
	}

	// ------------------------------------------------------------- widgets

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

	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}

	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	override void OnMenuFocusLost()
	{
		super.OnMenuFocusLost();
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	protected void OnBack()
	{
		Close();
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
