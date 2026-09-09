//! The screen you get when you read an intel object.
//!
//! ONE SCREEN, SEVERAL SKINS. A letter, a phone and a notebook are the same
//! thing drawn differently: a list of items and the selected one beside it.
//! DOCUMENT simply hides the list, because a letter with a one-row index looks
//! like a bug. Building a separate viewer per object type would have tripled
//! the work and produced three viewers that drift apart.
//!
//! The menu registration follows the same route as the operations board, which
//! is verified in a running game: `modded enum ChimeraMenuPreset` plus an
//! appended entry in MCF's GUID-override of chimeraMenus.conf. Cursor and
//! input capture come from `ActionContext "MenuContext"` on the preset --
//! nothing here calls for them.
//!
//! WHAT THIS DOES NOT DO YET: logging the intel to the operations board. That
//! is the next step and it is deliberately separate, because it is the part
//! that touches shared state and therefore has to be server-validated. Reading
//! is local; publishing is not.

class MCF_Intel_ViewerMenu : ChimeraMenuBase
{
	protected static const ResourceName ENTRY_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string W_DEVICE_NAME = "DeviceName";
	protected static const string W_ENTRY_SCROLL = "EntryScroll";
	protected static const string W_ENTRY_LIST = "EntryList";
	protected static const string W_BODY_TEXT = "BodyText";
	protected static const string W_BUTTON_LOG = "ButtonLog";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	//! The object being read. Static because the menu is created by the menu
	//! manager, which gives no way to pass an argument in -- vanilla does the
	//! same thing in SCR_FieldManualUI.Open, which opens the menu and then
	//! calls a method on the returned instance. This is that, minus a frame of
	//! blankness.
	protected static MCF_Intel_CarrierComponent s_PendingCarrier;

	protected MCF_Intel_CarrierComponent m_Carrier;

	protected Widget m_wEntryScroll;
	protected VerticalLayoutWidget m_wEntryList;
	protected RichTextWidget m_wBodyText;
	protected TextWidget m_wDeviceName;
	protected SCR_ButtonTextComponent m_ButtonLog;
	protected SCR_ButtonTextComponent m_ButtonClose;

	//! Which entry is on screen. Entering intel on the board enters the entry
	//! the reader is actually looking at, not the whole object -- a notebook
	//! with three unrelated notes should become three separate reports, not one
	//! unreadable blob.
	protected int m_iSelected = -1;

	protected ref array<MCF_Intel_Entry> m_aEntries = {};
	protected ref array<SCR_ButtonTextComponent> m_aRowButtons = {};
	protected ref array<Widget> m_aRowWidgets = {};

	//! Opens the viewer for one object.
	static MCF_Intel_ViewerMenu OpenFor(notnull MCF_Intel_CarrierComponent carrier)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return null;

		s_PendingCarrier = carrier;

		MCF_Intel_ViewerMenu menu = MCF_Intel_ViewerMenu.Cast(menuManager.OpenMenu(ChimeraMenuPreset.MCF_IntelViewer));
		if (!menu)
			s_PendingCarrier = null;

		return menu;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Carrier = s_PendingCarrier;
		s_PendingCarrier = null;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("intel viewer opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wDeviceName = TextWidget.Cast(root.FindAnyWidget(W_DEVICE_NAME));
		m_wEntryScroll = root.FindAnyWidget(W_ENTRY_SCROLL);
		m_wEntryList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_ENTRY_LIST));
		m_wBodyText = RichTextWidget.Cast(root.FindAnyWidget(W_BODY_TEXT));

		m_ButtonLog = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_LOG, root);
		if (m_ButtonLog)
			m_ButtonLog.m_OnClicked.Insert(OnLogClicked);

		// A visible way out, because Escape is not always available: in the
		// Workbench it stops the play session instead of closing the screen.
		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);

		if (!m_Carrier)
		{
			MCF_Core_Log.Warn("intel viewer opened with nothing to read");
			if (m_wBodyText)
				m_wBodyText.SetText("There is nothing here.");
			return;
		}

		Build();
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

	// --------------------------------------------------------------- build

	protected void Build()
	{
		if (m_wDeviceName)
			m_wDeviceName.SetText(m_Carrier.GetDeviceName());

		m_Carrier.GetEntries(m_aEntries);

		bool isDevice = m_Carrier.GetView() == MCF_EIntelView.DEVICE;

		// A letter is one page, so the index is noise. A phone is nothing but
		// its index.
		if (m_wEntryScroll)
			m_wEntryScroll.SetVisible(isDevice);

		if (isDevice)
			BuildList();
		else
			ShowAllAsOnePage();

		UpdateLogButton();

		MCF_Core_Log.Debug("intel viewer showing '" + m_Carrier.GetDeviceName() + "' with " + m_aEntries.Count().ToString() + " entrie(s)");
	}

	//! DEVICE: an index on the left, the selected entry on the right.
	protected void BuildList()
	{
		if (!m_wEntryList)
			return;

		m_aRowButtons.Clear();
		m_aRowWidgets.Clear();

		foreach (MCF_Intel_Entry entry : m_aEntries)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wEntryList);
			if (!row)
				continue;

			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!button)
				continue;

			button.SetText(entry.DescribeShort());
			button.m_OnClicked.Insert(OnRowClicked);

			m_aRowButtons.Insert(button);
			m_aRowWidgets.Insert(row);
		}

		if (!m_aEntries.IsEmpty())
			SelectEntry(0);
	}

	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aRowButtons.Find(button);
		if (index >= 0)
			SelectEntry(index);
	}

	protected void SelectEntry(int index)
	{
		foreach (int i, SCR_ButtonTextComponent button : m_aRowButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		if (index >= 0 && index < m_aRowWidgets.Count())
			GetGame().GetWorkspace().SetFocusedWidget(m_aRowWidgets[index]);

		if (!m_wBodyText || index < 0 || index >= m_aEntries.Count())
			return;

		m_iSelected = index;
		m_wBodyText.SetText(FormatEntry(m_aEntries[index]));
	}

	//! DOCUMENT: everything on one page, in order.
	//!
	//! Multiple entries are still allowed here -- a several-page letter, a
	//! captured order with numbered paragraphs -- they simply run on rather
	//! than becoming an index.
	protected void ShowAllAsOnePage()
	{
		if (!m_wBodyText)
			return;

		string page = "";
		foreach (int i, MCF_Intel_Entry entry : m_aEntries)
		{
			if (i > 0)
				page = page + "\n\n";

			page = page + FormatEntry(entry);
		}

		if (page.IsEmpty())
			page = "Nothing legible.";

		m_wBodyText.SetText(page);
	}

	protected string FormatEntry(notnull MCF_Intel_Entry entry)
	{
		string text = "";

		if (!entry.m_sHeading.IsEmpty())
			text = entry.m_sHeading;

		if (!entry.m_sTimestamp.IsEmpty())
		{
			if (!text.IsEmpty())
				text = text + "\n";

			text = text + entry.m_sTimestamp;
		}

		if (!entry.m_sBody.IsEmpty())
		{
			if (!text.IsEmpty())
				text = text + "\n\n";

			text = text + entry.m_sBody;
		}

		return text;
	}

	// ------------------------------------------------------------- logging

	//! Enters what is on screen into the shared store.
	//!
	//! This is the step that turns "I know something" into "the force knows
	//! something", and it is deliberately a press rather than a consequence of
	//! reading. Somebody has to decide it is worth passing up, and in a unit
	//! that decision belongs to a person, not to a trigger.
	//!
	//! A DOCUMENT enters its whole page. A DEVICE enters the entry being read,
	//! so a notebook of unrelated notes becomes several reports rather than
	//! one unreadable blob.
	protected void OnLogClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_Carrier || m_aEntries.IsEmpty())
			return;

		string source = m_Carrier.GetDeviceName();

		if (m_Carrier.GetView() == MCF_EIntelView.DEVICE)
		{
			int index = m_iSelected;
			if (index < 0 || index >= m_aEntries.Count())
				index = 0;

			MCF_Intel_Entry entry = m_aEntries[index];
			controller.MCF_RequestLogIntel(source, entry.m_sHeading, entry.m_sTimestamp, entry.m_sBody);
			return;
		}

		// DOCUMENT: one page, however many blocks it was authored in.
		MCF_Intel_Entry first = m_aEntries[0];

		string body = "";
		foreach (int i, MCF_Intel_Entry entry : m_aEntries)
		{
			if (i > 0)
				body = body + "\n\n";

			body = body + entry.m_sBody;
		}

		controller.MCF_RequestLogIntel(source, first.m_sHeading, first.m_sTimestamp, body);
	}

	//! Puts the object down. Always enabled -- a way out must never be
	//! conditional on anything.
	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}

	//! Entering intel is only possible at the operations board.
	//!
	//! Reading is not restricted -- you can go through a dead man's pockets
	//! wherever he fell. Sharing it with the force is what requires the walk
	//! back. Without this the whole intel design collapses: a scout could read
	//! a document behind enemy lines and the entire force would know it in the
	//! same second, and nothing would ever need carrying anywhere.
	//!
	//! Greyed rather than hidden, with the reason written on the button, so a
	//! player learns the rule instead of wondering where the option went.
	protected void UpdateLogButton()
	{
		if (!m_ButtonLog)
			return;

		bool atBoard = MCF_Task_BoardComponent.IsPlayerAtBoard(GetLocalPlayerId());

		m_ButtonLog.SetEnabled(atBoard);

		if (atBoard)
			m_ButtonLog.SetText("ENTER ON BOARD");
		else
			m_ButtonLog.SetText("TAKE IT TO THE BOARD");
	}

	protected int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}
}
