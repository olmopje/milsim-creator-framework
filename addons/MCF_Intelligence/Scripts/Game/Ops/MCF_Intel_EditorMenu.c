//! Where a Game Master rewrites what an intel object says.
//!
//! Opened from the right-click menu on the object in Game Master, never from
//! the world -- this is authoring, not play.
//!
//! WHY THIS SCREEN EXISTS AT ALL. The vanilla editor attribute panel cannot
//! carry text: every attribute value is packed into a single `vector` and
//! replicated as 12 bytes, with factories for int, float, bool and vector and
//! nothing else. A custom attribute class does not escape that -- the value
//! still has to fit. So MCF leaves the attribute system alone for text and
//! carries it over its own RPC, where the limit does not exist. Numbers and
//! toggles can still go through the vanilla panel, and should.
//!
//! ONE PAGE AT A TIME. A letter has one page, a notebook has several, and both
//! are edited here: the list on the left is the pages, the fields on the right
//! are the page you picked. Typing is held locally until APPLY, so a Game
//! Master can rewrite three pages and commit them as one change rather than
//! sending an RPC per keystroke.

class MCF_Intel_EditorMenu : ChimeraMenuBase
{
	protected static const ResourceName ENTRY_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string W_STATUS = "Status";
	protected static const string W_ENTRY_LIST = "EntryList";
	protected static const string W_EDIT_DEVICE = "EditDevice";
	protected static const string W_EDIT_HEADING = "EditHeading";
	protected static const string W_EDIT_STAMP = "EditStamp";
	protected static const string W_EDIT_BODY = "EditBody";
	protected static const string W_BUTTON_VIEW = "ButtonView";
	protected static const string W_BUTTON_ADD = "ButtonAddEntry";
	protected static const string W_BUTTON_REMOVE = "ButtonRemoveEntry";
	protected static const string W_BUTTON_APPLY = "ButtonApply";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	protected static MCF_Intel_CarrierComponent s_PendingCarrier;
	protected static SCR_EditableEntityComponent s_PendingEditable;

	protected MCF_Intel_CarrierComponent m_Carrier;
	//! The wire identity of the object being edited.
	//!
	//! It is the EDITABLE COMPONENT, not the entity. Replication.FindItemId
	//! resolves replicated *items*, and an IEntity is not one -- passing the
	//! entity returns an invalid id and the edit silently goes nowhere.
	//! Vanilla's own attribute manager addresses edited objects exactly this
	//! way, by their SCR_EditableEntityComponent.
	protected SCR_EditableEntityComponent m_Editable;

	protected TextWidget m_wStatus;
	protected VerticalLayoutWidget m_wEntryList;

	protected SCR_EditBoxComponent m_EditDevice;
	protected SCR_EditBoxComponent m_EditHeading;
	protected SCR_EditBoxComponent m_EditStamp;
	protected SCR_EditBoxComponent m_EditBody;

	protected SCR_ButtonTextComponent m_ButtonView;
	protected SCR_ButtonTextComponent m_ButtonAdd;
	protected SCR_ButtonTextComponent m_ButtonRemove;
	protected SCR_ButtonTextComponent m_ButtonApply;
	protected SCR_ButtonTextComponent m_ButtonClose;

	protected ref array<SCR_ButtonTextComponent> m_aRowButtons = {};
	protected ref array<Widget> m_aRowWidgets = {};

	//! The working copy. Edits happen here and only reach the object on APPLY,
	//! so backing out costs nothing and a multi-page rewrite is one change.
	protected ref array<ref MCF_Intel_Entry> m_aDraft = {};
	protected string m_sDraftDevice;
	protected MCF_EIntelView m_eDraftView;
	//! The verb the object already carried. Kept so APPLY does not rename
	//! an authored action just because this screen has no field for it.
	protected string m_sDraftVerb;
	protected int m_iSelected = -1;

	static MCF_Intel_EditorMenu OpenFor(notnull MCF_Intel_CarrierComponent carrier, SCR_EditableEntityComponent editable)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return null;

		s_PendingCarrier = carrier;
		s_PendingEditable = editable;

		// OpenDialog, not OpenMenu. A dialog sits inside the Game Master
		// editor the way the attributes window does, instead of taking the
		// whole screen and hiding the object being edited. The preset's
		// ActionContext does the rest -- see chimeraMenus.conf.
		MCF_Intel_EditorMenu menu = MCF_Intel_EditorMenu.Cast(menuManager.OpenDialog(ChimeraMenuPreset.MCF_IntelEditor, DialogPriority.INFORMATIVE, 0, true));
		if (!menu)
		{
			s_PendingCarrier = null;
			s_PendingEditable = null;
		}

		return menu;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Carrier = s_PendingCarrier;
		m_Editable = s_PendingEditable;
		s_PendingCarrier = null;
		s_PendingEditable = null;

		Widget root = GetRootWidget();
		if (!root || !m_Carrier)
		{
			MCF_Core_Log.Warn("intel editor opened with nothing to edit");
			return;
		}

		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wEntryList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_ENTRY_LIST));

		m_EditDevice = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_DEVICE, root);
		m_EditHeading = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_HEADING, root);
		m_EditStamp = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_STAMP, root);
		m_EditBody = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_BODY, root);

		m_ButtonView = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_VIEW, root);
		if (m_ButtonView)
			m_ButtonView.m_OnClicked.Insert(OnViewClicked);

		m_ButtonAdd = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_ADD, root);
		if (m_ButtonAdd)
			m_ButtonAdd.m_OnClicked.Insert(OnAddClicked);

		m_ButtonRemove = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_REMOVE, root);
		if (m_ButtonRemove)
			m_ButtonRemove.m_OnClicked.Insert(OnRemoveClicked);

		m_ButtonApply = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_APPLY, root);
		if (m_ButtonApply)
			m_ButtonApply.m_OnClicked.Insert(OnApplyClicked);

		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);

		LoadDraft();
		RebuildList();
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

	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}

	// ---------------------------------------------------------------- draft

	protected void LoadDraft()
	{
		m_sDraftDevice = m_Carrier.GetDeviceName();
		m_eDraftView = m_Carrier.GetView();
		m_sDraftVerb = m_Carrier.GetActionVerb();

		array<MCF_Intel_Entry> current = {};
		m_Carrier.GetEntries(current);

		m_aDraft = {};
		foreach (MCF_Intel_Entry entry : current)
		{
			MCF_Intel_Entry copy = new MCF_Intel_Entry();
			copy.m_sHeading = entry.m_sHeading;
			copy.m_sTimestamp = entry.m_sTimestamp;
			copy.m_sBody = entry.m_sBody;
			m_aDraft.Insert(copy);
		}

		if (m_EditDevice)
			m_EditDevice.SetValue(m_sDraftDevice);

		SyncViewButton();
	}

	//! Pulls whatever is typed into the fields back into the working copy.
	//! Called before anything that changes which page is shown, so switching
	//! pages does not quietly discard what was just typed.
	protected void CaptureFields()
	{
		if (m_EditDevice)
			m_sDraftDevice = m_EditDevice.GetValue();

		if (m_iSelected < 0 || m_iSelected >= m_aDraft.Count())
			return;

		MCF_Intel_Entry entry = m_aDraft[m_iSelected];

		if (m_EditHeading)
			entry.m_sHeading = m_EditHeading.GetValue();

		if (m_EditStamp)
			entry.m_sTimestamp = m_EditStamp.GetValue();

		if (m_EditBody)
			entry.m_sBody = m_EditBody.GetValue();
	}

	protected void ShowFields(int index)
	{
		m_iSelected = index;

		if (index < 0 || index >= m_aDraft.Count())
		{
			SetValue(m_EditHeading, "");
			SetValue(m_EditStamp, "");
			SetValue(m_EditBody, "");
			return;
		}

		MCF_Intel_Entry entry = m_aDraft[index];
		SetValue(m_EditHeading, entry.m_sHeading);
		SetValue(m_EditStamp, entry.m_sTimestamp);
		SetValue(m_EditBody, entry.m_sBody);
	}

	protected void SetValue(SCR_EditBoxComponent editor, string value)
	{
		if (editor)
			editor.SetValue(value);
	}

	// ----------------------------------------------------------------- list

	protected void RebuildList()
	{
		if (!m_wEntryList)
			return;

		while (m_wEntryList.GetChildren())
		{
			m_wEntryList.GetChildren().RemoveFromHierarchy();
		}

		m_aRowButtons.Clear();
		m_aRowWidgets.Clear();

		foreach (int i, MCF_Intel_Entry entry : m_aDraft)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wEntryList);
			if (!row)
				continue;

			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!button)
				continue;

			button.SetText((i + 1).ToString() + ".  " + entry.DescribeShort());
			button.m_OnClicked.Insert(OnRowClicked);

			m_aRowButtons.Insert(button);
			m_aRowWidgets.Insert(row);
		}

		int select = m_iSelected;
		if (select < 0 || select >= m_aDraft.Count())
			select = 0;

		if (!m_aDraft.IsEmpty())
			SelectRow(select);
		else
			ShowFields(-1);

		UpdateStatus();
	}

	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aRowButtons.Find(button);
		if (index < 0)
			return;

		CaptureFields();
		SelectRow(index);

		// The list label carries the heading, so it has to be redrawn after a
		// page is edited or the index goes stale under the editor's hands.
		RefreshRowLabels();
	}

	protected void SelectRow(int index)
	{
		foreach (int i, SCR_ButtonTextComponent button : m_aRowButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		if (index >= 0 && index < m_aRowWidgets.Count())
			GetGame().GetWorkspace().SetFocusedWidget(m_aRowWidgets[index]);

		ShowFields(index);
	}

	protected void RefreshRowLabels()
	{
		foreach (int i, SCR_ButtonTextComponent button : m_aRowButtons)
		{
			if (i < m_aDraft.Count())
				button.SetText((i + 1).ToString() + ".  " + m_aDraft[i].DescribeShort());
		}
	}

	protected void UpdateStatus()
	{
		if (!m_wStatus)
			return;

		m_wStatus.SetText(m_aDraft.Count().ToString() + " page(s). Changes are not on the object until you press APPLY.");
	}

	// -------------------------------------------------------------- actions

	//! DOCUMENT and DEVICE are the only two shapes this screen can describe,
	//! and the button flips between them -- taking the action verb with it,
	//! because a device is searched and a page is read.
	//!
	//! Anything else is left exactly as it is. PAPER, NOTEPAD and MAP reach
	//! this screen (the device-shaped views do not -- see
	//! MCF_Intel_EditContextAction), and flipping one of them would land on
	//! DOCUMENT and quietly turn a notepad into a flat page. The button used
	//! to do that on a single click, while showing "VIEW: DOCUMENT" for a view
	//! that was nothing of the kind.
	protected void OnViewClicked(SCR_ButtonTextComponent button)
	{
		if (m_eDraftView == MCF_EIntelView.DOCUMENT)
		{
			m_eDraftView = MCF_EIntelView.DEVICE;
			m_sDraftVerb = "Search";
		}
		else if (m_eDraftView == MCF_EIntelView.DEVICE)
		{
			m_eDraftView = MCF_EIntelView.DOCUMENT;
			m_sDraftVerb = "Read";
		}
		else if (m_wStatus)
		{
			m_wStatus.SetText("VIEW is " + ViewName(m_eDraftView) + ". This screen edits its pages but cannot change that shape.");
			return;
		}

		SyncViewButton();
	}

	protected void SyncViewButton()
	{
		if (!m_ButtonView)
			return;

		m_ButtonView.SetText("VIEW: " + ViewName(m_eDraftView));
	}

	//! The enum's own ToString() gives the number, which is what goes over the
	//! wire and is not what a Game Master should be shown.
	protected string ViewName(MCF_EIntelView view)
	{
		switch (view)
		{
			case MCF_EIntelView.DOCUMENT: return "DOCUMENT";
			case MCF_EIntelView.DEVICE:   return "DEVICE";
			case MCF_EIntelView.MAP:      return "MAP";
			case MCF_EIntelView.PAPER:    return "PAPER";
			case MCF_EIntelView.NOTEPAD:  return "NOTEPAD";
			case MCF_EIntelView.PHONE:    return "PHONE";
			case MCF_EIntelView.LAPTOP:   return "LAPTOP";
		}

		return "UNKNOWN";
	}

	protected void OnAddClicked(SCR_ButtonTextComponent button)
	{
		CaptureFields();

		MCF_Intel_Entry entry = new MCF_Intel_Entry();
		entry.m_sHeading = "New page";
		m_aDraft.Insert(entry);

		m_iSelected = m_aDraft.Count() - 1;
		RebuildList();
	}

	protected void OnRemoveClicked(SCR_ButtonTextComponent button)
	{
		if (m_iSelected < 0 || m_iSelected >= m_aDraft.Count())
			return;

		m_aDraft.Remove(m_iSelected);

		if (m_iSelected >= m_aDraft.Count())
			m_iSelected = m_aDraft.Count() - 1;

		RebuildList();
	}

	//! Sends the whole rewritten object to the server in one go.
	protected void OnApplyClicked(SCR_ButtonTextComponent button)
	{
		CaptureFields();

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_Editable)
			return;

		// The entity is named across the wire by its replication id, the same
		// way vanilla's own editor identifies an edited entity. Sending a name
		// or a position would be guessable and ambiguous; this is neither.
		RplId targetId = Replication.FindItemId(m_Editable);
		if (targetId == RplId.Invalid())
		{
			MCF_Core_Log.Warn("intel editor cannot address that object -- it is not replicated");
			return;
		}

		controller.MCF_RequestEditIntelObject(targetId, BuildPayload());

		if (m_wStatus)
			m_wStatus.SetText("Sent. " + m_aDraft.Count().ToString() + " page(s) applied.");

		RefreshRowLabels();
	}

	//! Packs the working copy into the same wire format the carrier reads.
	protected string BuildPayload()
	{
		string result = m_sDraftDevice + "<<f>>" + m_eDraftView.ToString() + "<<f>>" + m_sDraftVerb;

		foreach (MCF_Intel_Entry entry : m_aDraft)
		{
			result = result + "<<e>>" + entry.m_sHeading + "<<f>>" + entry.m_sTimestamp + "<<f>>" + entry.m_sBody;
		}

		return result;
	}
}
