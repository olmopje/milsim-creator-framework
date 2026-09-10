//! Intel drawn as the thing it actually is: a phone in your hands with apps on
//! it, a sheet of paper, a notepad you leaf through.
//!
//! WHY THIS IS IN OPS AND NOT IN THE DEVICES MODULE. It was written there
//! first, on the assumption that a phone shell belonged with the hacking. It
//! does not. A letter that looks like paper has nothing to do with breaking
//! into anything, and nobody should have to install a hacking module to get a
//! readable letter. Presentation is a property of intel, which is what
//! MCF_EIntelView has said since the day it was written: "the kind only
//! decides how it is drawn". Devices keeps the lock and the minigame, and
//! nothing else.
//!
//! ONE CLASS, SEVERAL LAYOUTS. Each skin is its own layout and its own menu
//! preset, because a phone and a sheet of paper have nothing in common
//! visually and forcing them through one widget tree would produce something
//! that is neither. They share this class because the *behaviour* is identical
//! -- pick an item, read it, file it -- and every widget lookup below is
//! null-checked, so a skin simply omits the widgets it has no use for.
//!
//! THREE SCREENS, ONE LAYOUT. Home, list and read are the same widget tree
//! with different parts hidden, because three layouts drift apart and one does
//! not. HOME is the only screen where BACK closes the phone; everywhere else
//! it steps back one level, which is what a phone does.

class MCF_Intel_ShellMenu : ChimeraMenuBase
{
	protected static const ResourceName ROW_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string W_DEVICE_NAME = "DeviceName";
	protected static const string W_STATUS_BAR = "StatusBar";
	protected static const string W_APP_PREFIX = "App";
	protected static const string W_LIST_SCROLL = "ListScroll";
	protected static const string W_ENTRY_LIST = "EntryList";
	protected static const string W_READ_SCROLL = "ReadScroll";
	protected static const string W_READ_COLUMN = "ReadColumn";
	protected static const string W_READ_HEADING = "ReadHeading";
	protected static const string W_READ_TIMESTAMP = "ReadTimestamp";
	protected static const string W_BUTTON_BACK = "ButtonBack";
	protected static const string W_BUTTON_PREV = "ButtonPrev";
	protected static const string W_BUTTON_NEXT = "ButtonNext";
	protected static const string W_PAGE_NUMBER = "PageNumber";
	protected static const string W_BUTTON_LOG = "ButtonLog";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";
	protected static const string W_HINT = "Hint";

	protected static const int APP_SLOTS = 6;

	protected static MCF_Intel_CarrierComponent s_PendingCarrier;
	protected static MCF_EIntelView s_PendingView;

	protected MCF_Intel_CarrierComponent m_Carrier;
	protected MCF_EIntelView m_eView;

	//! Which screen is up. There are three and they are exclusive, so an int
	//! would do -- these read better at the call sites.
	protected bool m_bOnHome = true;
	protected bool m_bOnList;

	protected ref array<MCF_Intel_Entry> m_aEntries = {};

	//! The apps that actually have something in them, in enum order. An empty
	//! app is not drawn at all: a phone with a Photos icon that opens on
	//! nothing is worse than a phone with no Photos icon.
	protected ref array<int> m_aApps = {};
	protected ref array<SCR_ButtonTextComponent> m_aAppButtons = {};

	protected int m_iOpenApp = -1;
	protected int m_iOpenEntry = -1;
	protected ref array<MCF_Intel_Entry> m_aVisible = {};
	protected ref array<SCR_ButtonTextComponent> m_aRowButtons = {};

	protected Widget m_wListScroll;
	protected Widget m_wReadScroll;
	protected VerticalLayoutWidget m_wEntryList;
	protected VerticalLayoutWidget m_wReadColumn;
	protected RichTextWidget m_wReadBody;
	protected TextWidget m_wDeviceName;
	protected TextWidget m_wStatusBar;
	protected TextWidget m_wReadHeading;
	protected TextWidget m_wReadTimestamp;
	protected TextWidget m_wHint;
	protected SCR_ButtonTextComponent m_ButtonBack;
	protected SCR_ButtonTextComponent m_ButtonLog;
	protected SCR_ButtonTextComponent m_ButtonPrev;
	protected SCR_ButtonTextComponent m_ButtonNext;
	protected TextWidget m_wPageNumber;

	//! Paper and notepad have no app grid, so there is nothing to go home to:
	//! they open on the first page and turn. Decided by what the layout
	//! actually contains rather than by the view, so a new skin picks its
	//! behaviour by which widgets it has.
	protected bool m_bPageMode;
	protected SCR_ButtonTextComponent m_ButtonClose;

	//! Opens the skin this object's view calls for.
	//! \return True if a skin took it; false means the caller should fall back
	//! to the plain viewer, which is what DOCUMENT, DEVICE and MAP still use.
	static bool OpenFor(notnull MCF_Intel_CarrierComponent carrier)
	{
		MCF_EIntelView view = carrier.GetView();

		int preset = -1;
		switch (view)
		{
			case MCF_EIntelView.PAPER:   preset = ChimeraMenuPreset.MCF_IntelPaper; break;
			case MCF_EIntelView.NOTEPAD: preset = ChimeraMenuPreset.MCF_IntelNotepad; break;
			case MCF_EIntelView.PHONE:   preset = ChimeraMenuPreset.MCF_IntelDevice; break;
			case MCF_EIntelView.LAPTOP:  preset = ChimeraMenuPreset.MCF_IntelDevice; break;
		}

		if (preset < 0)
			return false;

		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return false;

		s_PendingCarrier = carrier;
		s_PendingView = view;

		if (!menuManager.OpenMenu(preset))
		{
			s_PendingCarrier = null;
			return false;
		}

		return true;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Carrier = s_PendingCarrier;
		m_eView = s_PendingView;
		s_PendingCarrier = null;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("device shell opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wDeviceName = TextWidget.Cast(root.FindAnyWidget(W_DEVICE_NAME));
		m_wStatusBar = TextWidget.Cast(root.FindAnyWidget(W_STATUS_BAR));
		m_wListScroll = root.FindAnyWidget(W_LIST_SCROLL);
		m_wReadScroll = root.FindAnyWidget(W_READ_SCROLL);
		m_wEntryList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_ENTRY_LIST));
		m_wReadColumn = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_READ_COLUMN));
		m_wReadHeading = TextWidget.Cast(root.FindAnyWidget(W_READ_HEADING));
		m_wReadTimestamp = TextWidget.Cast(root.FindAnyWidget(W_READ_TIMESTAMP));
		m_wHint = TextWidget.Cast(root.FindAnyWidget(W_HINT));

		m_ButtonBack = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_BACK, root);
		if (m_ButtonBack)
			m_ButtonBack.m_OnClicked.Insert(OnBackClicked);

		// Filing a report has to be possible from here too. A phone shell that
		// can read intel but not enter it would be a downgrade from Ops' plain
		// viewer wearing nicer clothes, and the whole claim of this module is
		// that it is a skin and costs nothing.
		m_ButtonLog = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_LOG, root);
		if (m_ButtonLog)
			m_ButtonLog.m_OnClicked.Insert(OnLogClicked);

		m_ButtonPrev = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_PREV, root);
		if (m_ButtonPrev)
			m_ButtonPrev.m_OnClicked.Insert(OnPrevClicked);

		m_ButtonNext = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_NEXT, root);
		if (m_ButtonNext)
			m_ButtonNext.m_OnClicked.Insert(OnNextClicked);

		m_wPageNumber = TextWidget.Cast(root.FindAnyWidget(W_PAGE_NUMBER));

		// Always enabled, always visible. Escape stops the play session in the
		// Workbench, so a way out cannot depend on it.
		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);

		m_aAppButtons.Clear();
		for (int i = 0; i < APP_SLOTS; i++)
		{
			SCR_ButtonTextComponent app = SCR_ButtonTextComponent.GetButtonText(W_APP_PREFIX + i.ToString(), root);
			if (!app)
				continue;

			app.m_OnClicked.Insert(OnAppClicked);
			m_aAppButtons.Insert(app);
		}

		// The body text is created rather than laid out, so that a long
		// message scrolls instead of running off the bottom of the phone.
		if (m_wReadColumn)
		{
			Widget made = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wReadColumn);
			if (made)
				made.RemoveFromHierarchy();
		}

		if (!m_Carrier)
		{
			MCF_Core_Log.Warn("device shell opened with nothing to read");
			return;
		}

		m_Carrier.GetEntries(m_aEntries);

		if (m_wDeviceName)
			m_wDeviceName.SetText(m_Carrier.GetDeviceName());

		if (m_wStatusBar)
			m_wStatusBar.SetText(StatusLine());

		m_bPageMode = m_aAppButtons.IsEmpty();

		if (m_bPageMode)
		{
			// Everything is one stack of pages. No apps, no index.
			m_aVisible.Clear();
			foreach (MCF_Intel_Entry e : m_aEntries)
			{
				m_aVisible.Insert(e);
			}

			if (m_aVisible.IsEmpty())
				SetHint("Nothing legible.");
			else
				ShowEntry(0);
		}
		else
		{
			CollectApps();
			ShowHome();
		}

		MCF_Core_Log.Debug("device shell showing '" + m_Carrier.GetDeviceName() + "' with " + m_aEntries.Count().ToString() + " entrie(s) across " + m_aApps.Count().ToString() + " app(s)");
	}

	// ----------------------------------------------------------- the apps

	//! Which apps have anything in them, in enum order.
	protected void CollectApps()
	{
		m_aApps.Clear();

		for (int app = 0; app <= MCF_EIntelApp.FILES; app++)
		{
			foreach (MCF_Intel_Entry entry : m_aEntries)
			{
				if (entry.m_eApp == app)
				{
					m_aApps.Insert(app);
					break;
				}
			}
		}
	}

	protected void ShowHome()
	{
		m_bOnHome = true;
		m_bOnList = false;
		m_iOpenApp = -1;
		m_iOpenEntry = -1;

		SetScreens(true, false, false);

		foreach (int i, SCR_ButtonTextComponent button : m_aAppButtons)
		{
			bool used = i < m_aApps.Count();
			button.GetRootWidget().SetVisible(used);

			if (used)
				button.SetText(AppLabel(m_aApps[i]));
		}

		if (m_ButtonBack)
			m_ButtonBack.SetText("LOCK");

		UpdateLogButton();
		SetHint("");
	}

	protected void OnAppClicked(SCR_ButtonTextComponent button)
	{
		int slot = m_aAppButtons.Find(button);
		if (slot < 0 || slot >= m_aApps.Count())
			return;

		ShowList(m_aApps[slot]);
	}

	// ---------------------------------------------------------- the list

	protected void ShowList(int app)
	{
		m_bOnHome = false;
		m_bOnList = true;
		m_iOpenApp = app;
		m_iOpenEntry = -1;

		SetScreens(false, true, false);

		m_aVisible.Clear();
		m_aRowButtons.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		foreach (MCF_Intel_Entry entry : m_aEntries)
		{
			if (entry.m_eApp != app)
				continue;

			m_aVisible.Insert(entry);

			if (!m_wEntryList)
				continue;

			Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wEntryList);
			if (!row)
				continue;

			SCR_ButtonTextComponent rowButton = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!rowButton)
				continue;

			rowButton.SetText(entry.DescribeShort());
			rowButton.m_OnClicked.Insert(OnRowClicked);
			m_aRowButtons.Insert(rowButton);
		}

		if (m_wDeviceName)
			m_wDeviceName.SetText(AppLabel(app));

		if (m_ButtonBack)
			m_ButtonBack.SetText("HOME");

		UpdateLogButton();
		SetHint(m_aVisible.Count().ToString() + " item(s)");
	}

	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aRowButtons.Find(button);
		if (index >= 0)
			ShowEntry(index);
	}

	// ---------------------------------------------------------- one entry

	protected void ShowEntry(int index)
	{
		if (index < 0 || index >= m_aVisible.Count())
			return;

		m_bOnHome = false;
		m_bOnList = false;

		SetScreens(false, false, true);

		m_iOpenEntry = index;
		MCF_Intel_Entry entry = m_aVisible[index];

		if (m_wReadHeading)
			m_wReadHeading.SetText(entry.m_sHeading);

		if (m_wReadTimestamp)
			m_wReadTimestamp.SetText(entry.m_sTimestamp);

		if (m_wReadColumn)
		{
			ClearChildren(m_wReadColumn);

			Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wReadColumn);
			if (row)
			{
				SCR_ButtonTextComponent bodyButton = SCR_ButtonTextComponent.FindButtonTextComponent(row);
				if (bodyButton)
				{
					bodyButton.SetText(entry.m_sBody);
					bodyButton.SetEnabled(false);
				}
			}
		}

		if (m_ButtonBack)
			m_ButtonBack.SetText("BACK");

		UpdatePageButtons();
		UpdateLogButton();
	}

	// ------------------------------------------------------------ plumbing

	protected void SetScreens(bool home, bool list, bool read)
	{
		foreach (SCR_ButtonTextComponent button : m_aAppButtons)
		{
			button.GetRootWidget().SetVisible(home);
		}

		if (m_wListScroll)
			m_wListScroll.SetVisible(list);

		if (m_wReadScroll)
			m_wReadScroll.SetVisible(read);

		if (m_wReadHeading)
			m_wReadHeading.SetVisible(read);

		if (m_wReadTimestamp)
			m_wReadTimestamp.SetVisible(read);
	}

	//! BACK steps back one level, and closes the phone from the home screen --
	//! which is what a phone does, and means the button is never dead.
	protected void OnBackClicked(SCR_ButtonTextComponent button)
	{
		if (m_bPageMode)
		{
			Close();
			return;
		}

		if (m_bOnHome)
		{
			Close();
			return;
		}

		if (m_bOnList)
		{
			ShowHome();
			return;
		}

		ShowList(m_iOpenApp);
	}

	//! Enters the open message on the board. Only ever the one being read --
	//! a phone of unrelated messages should become several reports, not one
	//! unreadable blob, which is the same rule Ops' viewer follows.
	protected void OnLogClicked(SCR_ButtonTextComponent button)
	{
		if (m_iOpenEntry < 0 || m_iOpenEntry >= m_aVisible.Count() || !m_Carrier)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		MCF_Intel_Entry entry = m_aVisible[m_iOpenEntry];
		controller.MCF_RequestLogIntel(m_Carrier.GetDeviceName(), entry.m_sHeading, entry.m_sTimestamp, entry.m_sBody);
	}

	//! Hidden unless a message is open, and greyed with the reason on it when
	//! the player is not at the board. Greyed rather than gone, so the rule
	//! gets learned instead of the option seeming to vanish.
	protected void UpdateLogButton()
	{
		if (!m_ButtonLog)
			return;

		bool reading = m_iOpenEntry >= 0;
		m_ButtonLog.GetRootWidget().SetVisible(reading);

		if (!reading)
			return;

		bool atBoard = MCF_Task_BoardComponent.IsPlayerAtBoard(LocalPlayerId());
		m_ButtonLog.SetEnabled(atBoard);

		if (atBoard)
			m_ButtonLog.SetText("LOG");
		else
			m_ButtonLog.SetText("NO NET");

		if (atBoard)
			SetHint("Enter it on the board to share it with the force.");
		else
			SetHint("Take it to the operations board to share it.");
	}

	protected void OnPrevClicked(SCR_ButtonTextComponent button)
	{
		if (m_iOpenEntry > 0)
			ShowEntry(m_iOpenEntry - 1);
	}

	protected void OnNextClicked(SCR_ButtonTextComponent button)
	{
		if (m_iOpenEntry >= 0 && m_iOpenEntry < m_aVisible.Count() - 1)
			ShowEntry(m_iOpenEntry + 1);
	}

	//! Greyed at the ends rather than hidden, so the page count stays where it
	//! is and the object does not appear to change shape as you leaf through.
	protected void UpdatePageButtons()
	{
		int pages = m_aVisible.Count();

		if (m_ButtonPrev)
			m_ButtonPrev.SetEnabled(m_iOpenEntry > 0);

		if (m_ButtonNext)
			m_ButtonNext.SetEnabled(m_iOpenEntry >= 0 && m_iOpenEntry < pages - 1);

		if (!m_wPageNumber)
			return;

		if (pages > 1)
			m_wPageNumber.SetText((m_iOpenEntry + 1).ToString() + " / " + pages.ToString());
		else
			m_wPageNumber.SetText("");
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
		OnBackClicked(null);
	}

	protected void ClearChildren(notnull Widget parent)
	{
		Widget child = parent.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			child.RemoveFromHierarchy();
			child = next;
		}
	}

	protected void SetHint(string text)
	{
		if (m_wHint)
			m_wHint.SetText(text);
	}

	protected string StatusLine()
	{
		if (m_eView == MCF_EIntelView.LAPTOP)
			return "MCF                    AC POWER";

		return "MCF                    100%";
	}

	protected string AppLabel(int app)
	{
		switch (app)
		{
			case MCF_EIntelApp.MESSAGES: return "MESSAGES";
			case MCF_EIntelApp.EMAIL:    return "MAIL";
			case MCF_EIntelApp.NOTES:    return "NOTES";
			case MCF_EIntelApp.PHOTOS:   return "PHOTOS";
			case MCF_EIntelApp.FILES:    return "FILES";
		}

		return "INBOX";
	}

	protected int LocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}
}
