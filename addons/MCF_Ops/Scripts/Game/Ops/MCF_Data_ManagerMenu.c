//! Where a mission maker looks at what the server is remembering, throws parts
//! of it away, and keeps starting positions to come back to.
//!
//! THE ONLY SCREEN IN MCF THAT DELETES, which shapes three decisions:
//!
//!   1. Nothing is destroyed on one click. A clear button arms first and says
//!      what will go; the second click does it. A misclick costs a second
//!      click to undo, and there is no undo for the other outcome.
//!   2. Every count comes from the server. A client cannot read the store, and
//!      a screen that guessed would show a mission maker four taskings while
//!      deleting seven.
//!   3. It says what a set IS before it offers to delete it. "Conversations"
//!      does not tell a person that a week of authored dialogue is inside.
//!
//! WHY THE SUMMARY ARRIVES THROUGH A STATIC. The reply comes back on the
//! player controller, which knows nothing about menus and must not: the same
//! shape the device shell and the planning board already use.

class MCF_Data_ManagerMenu : ChimeraMenuBase
{
	protected static const string ROW_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string SEP_SECTION = "<<s>>";
	protected static const string SEP_ROW = "<<r>>";
	protected static const string SEP_FIELD = "<<f>>";

	protected static const string W_STATUS = "Status";
	protected static const string W_SET_LIST = "SetList";
	protected static const string W_SET_DETAIL = "SetDetail";
	protected static const string W_SNAPSHOT_LIST = "SnapshotList";
	protected static const string W_EDIT_NAME = "EditSnapshotName";
	protected static const string W_HINT = "Hint";

	protected static const string W_BUTTON_CLEAR_SET = "ButtonClearSet";
	protected static const string W_BUTTON_CLEAR_ALL = "ButtonClearAll";
	protected static const string W_BUTTON_SAVE = "ButtonSnapshotSave";
	protected static const string W_BUTTON_LOAD = "ButtonSnapshotLoad";
	protected static const string W_BUTTON_DELETE = "ButtonSnapshotDelete";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	//! The one open screen, so a reply that arrives from the server can find
	//! it. Null when nothing is open, which is the normal state.
	protected static MCF_Data_ManagerMenu s_Open;

	protected VerticalLayoutWidget m_wSetList;
	protected VerticalLayoutWidget m_wSnapshotList;
	protected TextWidget m_wStatus;
	protected TextWidget m_wSetDetail;
	protected TextWidget m_wHint;
	protected SCR_EditBoxComponent m_EditName;

	protected SCR_ButtonTextComponent m_ButtonClearSet;
	protected SCR_ButtonTextComponent m_ButtonClearAll;
	protected SCR_ButtonTextComponent m_ButtonSave;
	protected SCR_ButtonTextComponent m_ButtonLoad;
	protected SCR_ButtonTextComponent m_ButtonDelete;
	protected SCR_ButtonTextComponent m_ButtonClose;

	protected ref array<ref MCF_Data_SetRow> m_aSets = {};
	protected ref array<string> m_aSnapshots = {};
	protected ref array<SCR_ButtonTextComponent> m_aSetButtons = {};
	protected ref array<SCR_ButtonTextComponent> m_aSnapshotButtons = {};

	protected int m_iSelectedSet = -1;
	protected int m_iSelectedSnapshot = -1;

	//! What is armed, if anything: "" nothing, a set id, or "all".
	protected string m_sArmed;

	static void Open()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MCF_DataManager);
	}

	//! Called from the player controller when the server answers.
	static void ApplySummary(string payload)
	{
		if (s_Open)
			s_Open.Receive(payload);
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		s_Open = this;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("mission data screen opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wSetList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_SET_LIST));
		m_wSnapshotList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_SNAPSHOT_LIST));
		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wSetDetail = TextWidget.Cast(root.FindAnyWidget(W_SET_DETAIL));
		m_wHint = TextWidget.Cast(root.FindAnyWidget(W_HINT));

		m_EditName = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_NAME, root);

		m_ButtonClearSet = Bind(W_BUTTON_CLEAR_SET, root);
		m_ButtonClearAll = Bind(W_BUTTON_CLEAR_ALL, root);
		m_ButtonSave = Bind(W_BUTTON_SAVE, root);
		m_ButtonLoad = Bind(W_BUTTON_LOAD, root);
		m_ButtonDelete = Bind(W_BUTTON_DELETE, root);
		m_ButtonClose = Bind(W_BUTTON_CLOSE, root);

		SetStatus("Asking the server what it is holding...");
		SetHint("");

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestDataSummary();
	}

	override void OnMenuClose()
	{
		if (s_Open == this)
			s_Open = null;

		super.OnMenuClose();
	}

	//! One dispatcher for every button, because Enforce refuses a method that
	//! takes a func -- the same constraint that shapes the device editor.
	protected SCR_ButtonTextComponent Bind(string name, notnull Widget root)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, root);
		if (button)
			button.m_OnClicked.Insert(OnButtonClicked);

		return button;
	}

	protected void OnButtonClicked(SCR_ButtonTextComponent button)
	{
		if (button == m_ButtonClose)
		{
			Close();
			return;
		}

		if (button == m_ButtonClearSet)
		{
			OnClearSet();
			return;
		}

		if (button == m_ButtonClearAll)
		{
			OnClearAll();
			return;
		}

		// Anything below this point is not a clearing button, so whatever was
		// armed is no longer what the person is thinking about.
		Disarm();

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		if (button == m_ButtonSave)
		{
			string name = TypedName();
			if (name.IsEmpty())
			{
				SetHint("Give the snapshot a name first.");
				return;
			}

			controller.MCF_RequestSaveSnapshot(name);
			return;
		}

		if (button == m_ButtonLoad)
		{
			if (m_iSelectedSnapshot < 0)
			{
				SetHint("Pick a snapshot to restore.");
				return;
			}

			controller.MCF_RequestLoadSnapshot(m_aSnapshots[m_iSelectedSnapshot]);
			return;
		}

		if (button == m_ButtonDelete)
		{
			if (m_iSelectedSnapshot < 0)
			{
				SetHint("Pick a snapshot to delete.");
				return;
			}

			controller.MCF_RequestDeleteSnapshot(m_aSnapshots[m_iSelectedSnapshot]);
			return;
		}

		// A row.
		int setIndex = m_aSetButtons.Find(button);
		if (setIndex >= 0)
		{
			SelectSet(setIndex);
			return;
		}

		int snapshotIndex = m_aSnapshotButtons.Find(button);
		if (snapshotIndex >= 0)
			SelectSnapshot(snapshotIndex);
	}

	// --------------------------------------------------------------- clearing

	protected void OnClearSet()
	{
		if (m_iSelectedSet < 0)
		{
			SetHint("Pick something to clear.");
			return;
		}

		MCF_Data_SetRow dataSet = m_aSets[m_iSelectedSet];

		// First click arms and says what goes. Second click does it.
		if (m_sArmed != dataSet.m_sId)
		{
			m_sArmed = dataSet.m_sId;
			m_ButtonClearSet.SetText("CONFIRM");
			SetHint("This deletes " + dataSet.m_iCount.ToString() + " " + dataSet.m_sLabel
				+ " and cannot be undone. Click again to go through with it.");
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestClearDataSet(dataSet.m_sId);

		Disarm();
	}

	protected void OnClearAll()
	{
		if (m_sArmed != "all")
		{
			m_sArmed = "all";
			m_ButtonClearAll.SetText("CONFIRM");
			SetHint("This deletes everything listed on the left and cannot be undone."
				+ " Save a snapshot first if you may want it back. Click again to go through with it.");
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestClearAllData();

		Disarm();
	}

	protected void Disarm()
	{
		if (m_sArmed.IsEmpty())
			return;

		m_sArmed = "";

		if (m_ButtonClearSet)
			m_ButtonClearSet.SetText("CLEAR SELECTED");

		if (m_ButtonClearAll)
			m_ButtonClearAll.SetText("CLEAR EVERYTHING");

		SetHint("");
	}

	// ---------------------------------------------------------------- filling

	protected void Receive(string payload)
	{
		Disarm();

		m_aSets.Clear();
		m_aSnapshots.Clear();

		array<string> halves = {};
		payload.Split(SEP_SECTION, halves, false);

		if (!halves.IsEmpty())
			ReadSets(halves[0]);

		if (halves.Count() > 1)
			ReadSnapshots(halves[1]);

		FillSets();
		FillSnapshots();

		int total;
		foreach (MCF_Data_SetRow dataSet : m_aSets)
		{
			total = total + dataSet.m_iCount;
		}

		SetStatus(total.ToString() + " entries held, across " + m_aSets.Count().ToString()
			+ " kinds. " + m_aSnapshots.Count().ToString() + " snapshot(s) saved.");
	}

	protected void ReadSets(string text)
	{
		if (text.IsEmpty())
			return;

		array<string> rows = {};
		text.Split(SEP_ROW, rows, false);

		foreach (string row : rows)
		{
			array<string> fields = {};
			row.Split(SEP_FIELD, fields, false);

			if (fields.Count() < 4)
				continue;

			MCF_Data_SetRow dataSet = new MCF_Data_SetRow();
			dataSet.m_sId = fields[0];
			dataSet.m_sLabel = fields[1];
			dataSet.m_sDescription = fields[2];
			dataSet.m_iCount = fields[3].ToInt();
			m_aSets.Insert(dataSet);
		}
	}

	protected void ReadSnapshots(string text)
	{
		if (text.IsEmpty())
			return;

		array<string> rows = {};
		text.Split(SEP_ROW, rows, false);

		foreach (string row : rows)
		{
			if (!row.IsEmpty())
				m_aSnapshots.Insert(row);
		}
	}

	protected void FillSets()
	{
		ClearList(m_wSetList, m_aSetButtons);

		if (!m_wSetList)
			return;

		foreach (MCF_Data_SetRow dataSet : m_aSets)
		{
			AddRow(m_wSetList, m_aSetButtons, dataSet.m_sLabel + "   " + dataSet.m_iCount.ToString());
		}

		if (m_aSets.IsEmpty())
		{
			SetDetail("Nothing has registered anything to store. That is a mod problem, not a mission one.");
			return;
		}

		SelectSet(0);
	}

	protected void FillSnapshots()
	{
		ClearList(m_wSnapshotList, m_aSnapshotButtons);

		if (!m_wSnapshotList)
			return;

		foreach (string name : m_aSnapshots)
		{
			AddRow(m_wSnapshotList, m_aSnapshotButtons, name);
		}

		m_iSelectedSnapshot = -1;
	}

	protected SCR_ButtonTextComponent AddRow(notnull VerticalLayoutWidget list, notnull array<SCR_ButtonTextComponent> into, string label)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, list);
		if (!row)
			return null;

		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!button)
			return null;

		button.SetText(label);
		button.m_OnClicked.Insert(OnButtonClicked);
		into.Insert(button);
		return button;
	}

	protected void ClearList(VerticalLayoutWidget list, notnull array<SCR_ButtonTextComponent> buttons)
	{
		buttons.Clear();

		if (!list)
			return;

		// RemoveFromHierarchy rather than walking siblings: the same call the
		// planning board uses, and the one that is known to work here.
		while (list.GetChildren())
		{
			list.GetChildren().RemoveFromHierarchy();
		}
	}

	protected void SelectSet(int index)
	{
		if (index < 0 || index >= m_aSets.Count())
			return;

		Disarm();
		m_iSelectedSet = index;

		foreach (int i, SCR_ButtonTextComponent button : m_aSetButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		MCF_Data_SetRow dataSet = m_aSets[index];
		SetDetail(dataSet.m_sDescription);
	}

	protected void SelectSnapshot(int index)
	{
		if (index < 0 || index >= m_aSnapshots.Count())
			return;

		m_iSelectedSnapshot = index;

		foreach (int i, SCR_ButtonTextComponent button : m_aSnapshotButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		if (m_EditName)
			m_EditName.SetValue(m_aSnapshots[index]);
	}

	// --------------------------------------------------------------- plumbing

	protected string TypedName()
	{
		if (!m_EditName)
			return "";

		return m_EditName.GetValue();
	}

	protected void SetStatus(string text)
	{
		if (m_wStatus)
			m_wStatus.SetText(text);
	}

	protected void SetDetail(string text)
	{
		if (m_wSetDetail)
			m_wSetDetail.SetText(text);
	}

	protected void SetHint(string text)
	{
		if (m_wHint)
			m_wHint.SetText(text);
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
		// Escape disarms before it closes, so a screen left armed and returned
		// to later is not one click from deleting a mission.
		if (!m_sArmed.IsEmpty())
		{
			Disarm();
			return;
		}

		Close();
	}
}

//! One line of the left-hand list, as the server described it.
class MCF_Data_SetRow
{
	string m_sId;
	string m_sLabel;
	string m_sDescription;
	int m_iCount;
}
