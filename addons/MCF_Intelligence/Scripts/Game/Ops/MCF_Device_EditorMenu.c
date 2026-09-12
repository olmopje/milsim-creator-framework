//! Where a Game Master writes what is on a device.
//!
//! Three columns in the order the work happens: pick an app, pick an item in
//! it, write the item. Nothing is hidden behind a mode.
//!
//! IT EDITS A DRAFT, NOT THE DEVICE. Every change lands in m_Draft, and APPLY
//! sends the whole thing in one request. Partial edits would need the server to
//! hold a draft between calls, and a draft that outlives a Game Master who
//! disconnected is state nobody owns. It also means REVERT is free: throw the
//! draft away and read the device again.
//!
//! WHERE THE TEXT GOES. Not into an editor attribute --
//! SCR_BaseEditorAttributeVar packs every value into one vector and replicates
//! twelve bytes, so it can carry a number and never a sentence. The words go
//! over MCF's own RPC to the server, which writes them onto the object as
//! replicated state. MCF_Intel_CarrierComponent.m_sProfileOverride says why
//! they live on the object rather than in the library.
//!
//! ON READING FIELDS BACK. An edit box only surrenders its text when it is
//! asked, so every action that moves the selection captures the fields first.
//! Forgetting one is how a Game Master loses a sentence they just typed, with
//! no error and no way to tell it happened -- hence CaptureFields at the top of
//! every handler that changes what is selected.

class MCF_Device_EditorMenu : ChimeraMenuBase
{
	protected static const ResourceName ROW_LAYOUT = "{6A1C4F0B39D34500}UI/layouts/MCF/MCF_IntelRow.layout";

	protected static const string W_TITLE = "Title";
	protected static const string W_STATUS = "Status";
	protected static const string W_HINT = "Hint";
	protected static const string W_APP_LIST = "AppList";
	protected static const string W_ITEM_LIST = "ItemList";
	protected static const string W_EDIT_ID = "EditId";
	protected static const string W_EDIT_DEVICE = "EditDevice";
	protected static const string W_EDIT_APP_LABEL = "EditAppLabel";
	protected static const string W_EDIT_HEADING = "EditHeading";
	protected static const string W_EDIT_STAMP = "EditStamp";
	protected static const string W_EDIT_BODY = "EditBody";
	protected static const string W_EDIT_IMAGE = "EditImage";
	protected static const string W_EDIT_IMAGE_URL = "EditImageUrl";
	protected static const string W_BUTTON_LOAD = "ButtonLoad";
	protected static const string W_BUTTON_RESET = "ButtonReset";
	protected static const string W_BUTTON_APP_ADD = "ButtonAppAdd";
	protected static const string W_BUTTON_APP_REMOVE = "ButtonAppRemove";
	protected static const string W_BUTTON_APP_UP = "ButtonAppUp";
	protected static const string W_BUTTON_APP_DOWN = "ButtonAppDown";
	protected static const string W_BUTTON_APP_KIND = "ButtonAppKind";
	protected static const string W_BUTTON_ITEM_ADD = "ButtonItemAdd";
	protected static const string W_BUTTON_ITEM_REMOVE = "ButtonItemRemove";
	protected static const string W_BUTTON_APPLY = "ButtonApply";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	//! The kinds the KIND button cycles through, in the order it cycles them.
	protected static const int KIND_COUNT = 8;

	protected static MCF_Intel_CarrierComponent s_PendingCarrier;
	protected static SCR_EditableEntityComponent s_PendingEditable;

	protected MCF_Intel_CarrierComponent m_Carrier;
	protected SCR_EditableEntityComponent m_Editable;

	protected ref MCF_Device_Profile m_Draft;
	protected int m_iApp = -1;
	protected int m_iItem = -1;

	//! Where in the library's list LOAD NEXT is. Cycling rather than a dropdown
	//! because a dropdown is a widget with its own behaviour to get right, and
	//! there are rarely more than a handful of profiles.
	protected int m_iLibraryIndex = -1;

	protected TextWidget m_wStatus;
	protected TextWidget m_wHint;
	protected VerticalLayoutWidget m_wAppList;
	protected VerticalLayoutWidget m_wItemList;

	protected SCR_EditBoxComponent m_EditId;
	protected SCR_EditBoxComponent m_EditDevice;
	protected SCR_EditBoxComponent m_EditAppLabel;
	protected SCR_EditBoxComponent m_EditHeading;
	protected SCR_EditBoxComponent m_EditStamp;
	protected SCR_EditBoxComponent m_EditBody;
	protected SCR_EditBoxComponent m_EditImage;
	protected SCR_EditBoxComponent m_EditImageUrl;

	protected SCR_ButtonTextComponent m_ButtonKind;
	protected SCR_ButtonTextComponent m_ButtonLoad;
	protected SCR_ButtonTextComponent m_ButtonRevert;
	protected SCR_ButtonTextComponent m_ButtonAppAdd;
	protected SCR_ButtonTextComponent m_ButtonAppRemove;
	protected SCR_ButtonTextComponent m_ButtonAppUp;
	protected SCR_ButtonTextComponent m_ButtonAppDown;
	protected SCR_ButtonTextComponent m_ButtonItemAdd;
	protected SCR_ButtonTextComponent m_ButtonItemRemove;
	protected SCR_ButtonTextComponent m_ButtonApply;
	protected SCR_ButtonTextComponent m_ButtonClose;
	protected ref array<SCR_ButtonTextComponent> m_aAppRows = {};
	protected ref array<SCR_ButtonTextComponent> m_aItemRows = {};

	static MCF_Device_EditorMenu OpenFor(notnull MCF_Intel_CarrierComponent carrier, SCR_EditableEntityComponent editable)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return null;

		s_PendingCarrier = carrier;
		s_PendingEditable = editable;

		return MCF_Device_EditorMenu.Cast(menuManager.OpenMenu(ChimeraMenuPreset.MCF_DeviceEditor));
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Carrier = s_PendingCarrier;
		m_Editable = s_PendingEditable;
		s_PendingCarrier = null;
		s_PendingEditable = null;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("device editor opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wHint = TextWidget.Cast(root.FindAnyWidget(W_HINT));
		m_wAppList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_APP_LIST));
		m_wItemList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_ITEM_LIST));

		m_EditId = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_ID, root);
		m_EditDevice = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_DEVICE, root);
		m_EditAppLabel = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_APP_LABEL, root);
		m_EditHeading = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_HEADING, root);
		m_EditStamp = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_STAMP, root);
		m_EditBody = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_BODY, root);
		m_EditImage = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_IMAGE, root);
		m_EditImageUrl = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_IMAGE_URL, root);

		m_ButtonLoad = Bind(root, W_BUTTON_LOAD);
		m_ButtonRevert = Bind(root, W_BUTTON_RESET);
		m_ButtonAppAdd = Bind(root, W_BUTTON_APP_ADD);
		m_ButtonAppRemove = Bind(root, W_BUTTON_APP_REMOVE);
		m_ButtonAppUp = Bind(root, W_BUTTON_APP_UP);
		m_ButtonAppDown = Bind(root, W_BUTTON_APP_DOWN);
		m_ButtonItemAdd = Bind(root, W_BUTTON_ITEM_ADD);
		m_ButtonItemRemove = Bind(root, W_BUTTON_ITEM_REMOVE);
		m_ButtonApply = Bind(root, W_BUTTON_APPLY);
		m_ButtonClose = Bind(root, W_BUTTON_CLOSE);
		m_ButtonKind = Bind(root, W_BUTTON_APP_KIND);

		LoadDraftFromDevice();
	}

	//! Every button here shares one handler and is told apart by which
	//! component was clicked.
	//!
	//! NOT A STYLE CHOICE. Enforce refuses a method that takes a `func`
	//! parameter -- "func arguments are not supported in script methods" -- so
	//! a helper cannot be handed the callback to bind, and eleven near-identical
	//! two-line handlers is worse than one that dispatches.
	protected SCR_ButtonTextComponent Bind(notnull Widget root, string name)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, root);
		if (!button)
		{
			MCF_Core_Log.Warn("device editor is missing widget '" + name + "'");
			return null;
		}

		button.m_OnClicked.Insert(OnButtonClicked);
		return button;
	}

	protected void OnButtonClicked(SCR_ButtonTextComponent button)
	{
		if (!button)
			return;

		if (button == m_ButtonClose)
		{
			Close();
			return;
		}

		if (button == m_ButtonLoad)
			OnLoadNext();
		else if (button == m_ButtonRevert)
			LoadDraftFromDevice();
		else if (button == m_ButtonKind)
			OnKindClicked();
		else if (button == m_ButtonAppAdd)
			OnAppAdd();
		else if (button == m_ButtonAppRemove)
			OnAppRemove();
		else if (button == m_ButtonAppUp)
			MoveApp(-1);
		else if (button == m_ButtonAppDown)
			MoveApp(1);
		else if (button == m_ButtonItemAdd)
			OnItemAdd();
		else if (button == m_ButtonItemRemove)
			OnItemRemove();
		else if (button == m_ButtonApply)
			OnApply();
	}

	// ------------------------------------------------------------ the draft

	//! Reads the device into a draft.
	//!
	//! Through the presenter, so the editor sees exactly what a player would --
	//! including the profile synthesised from an object's own entries, which is
	//! what makes "edit a device that has never been authored" work without a
	//! separate blank-slate path.
	protected void LoadDraftFromDevice()
	{
		MCF_Device_Presenter presenter = new MCF_Device_Presenter();
		presenter.Bind(m_Carrier);

		m_Draft = new MCF_Device_Profile();
		m_Draft.m_sId = MCF_Device_Library.SanitiseId(presenter.DeviceName());
		m_Draft.m_sDeviceName = presenter.DeviceName();
		m_Draft.m_aApps = {};

		array<MCF_Device_App> apps = {};
		presenter.GetApps(apps);

		foreach (MCF_Device_App app : apps)
		{
			m_Draft.m_aApps.Insert(CopyApp(app, presenter));
		}

		m_iApp = -1;
		m_iItem = -1;

		SetValue(m_EditId, m_Draft.m_sId);
		SetValue(m_EditDevice, m_Draft.m_sDeviceName);

		RebuildApps();
		Status("Loaded from the device.");
	}

	//! A deep copy, because the draft must not be the profile the world is
	//! using: an edit that took effect while it was still being typed would be
	//! impossible to undo and impossible to explain.
	protected MCF_Device_App CopyApp(notnull MCF_Device_App source, notnull MCF_Device_Presenter presenter)
	{
		MCF_Device_App copy = new MCF_Device_App();
		copy.m_eKind = source.m_eKind;
		copy.m_sLabel = source.m_sLabel;
		copy.m_sEmptyText = source.m_sEmptyText;
		copy.m_aItems = {};

		array<ref MCF_Device_Item> items = {};
		presenter.GetItems(source, items);

		foreach (MCF_Device_Item item : items)
		{
			MCF_Device_Item itemCopy = new MCF_Device_Item();
			itemCopy.m_sHeading = item.m_sHeading;
			itemCopy.m_sTimestamp = item.m_sTimestamp;
			itemCopy.m_sBody = item.m_sBody;
			itemCopy.m_sImage = item.m_sImage;
			itemCopy.m_sImageUrl = item.m_sImageUrl;
			copy.m_aItems.Insert(itemCopy);
		}

		return copy;
	}

	//! Pulls what has been typed into the draft.
	//!
	//! Called before anything that changes the selection. An edit box only
	//! gives up its text when asked, so a handler that forgets this loses
	//! whatever was typed since the last one, silently.
	protected void CaptureFields()
	{
		if (!m_Draft)
			return;

		if (m_EditId)
			m_Draft.m_sId = MCF_Device_Library.SanitiseId(m_EditId.GetValue());

		if (m_EditDevice)
			m_Draft.m_sDeviceName = m_EditDevice.GetValue();

		MCF_Device_App app = CurrentApp();
		if (app && m_EditAppLabel)
			app.m_sLabel = m_EditAppLabel.GetValue();

		MCF_Device_Item item = CurrentItem();
		if (!item)
			return;

		if (m_EditHeading)
			item.m_sHeading = m_EditHeading.GetValue();

		if (m_EditStamp)
			item.m_sTimestamp = m_EditStamp.GetValue();

		if (m_EditBody)
			item.m_sBody = m_EditBody.GetValue();

		if (m_EditImage)
			item.m_sImage = m_EditImage.GetValue();

		if (m_EditImageUrl)
			item.m_sImageUrl = m_EditImageUrl.GetValue();
	}

	protected MCF_Device_App CurrentApp()
	{
		if (!m_Draft || !m_Draft.m_aApps || m_iApp < 0 || m_iApp >= m_Draft.m_aApps.Count())
			return null;

		return m_Draft.m_aApps[m_iApp];
	}

	protected MCF_Device_Item CurrentItem()
	{
		MCF_Device_App app = CurrentApp();
		if (!app || !app.m_aItems || m_iItem < 0 || m_iItem >= app.m_aItems.Count())
			return null;

		return app.m_aItems[m_iItem];
	}

	// ------------------------------------------------------------- the lists

	protected void RebuildApps()
	{
		m_aAppRows.Clear();

		if (m_wAppList)
			ClearChildren(m_wAppList);

		if (!m_Draft || !m_Draft.m_aApps)
			return;

		foreach (int i, MCF_Device_App app : m_Draft.m_aApps)
		{
			string mark = "  ";
			if (i == m_iApp)
				mark = "> ";

			int count;
			if (app.m_aItems)
				count = app.m_aItems.Count();

			AddRow(m_wAppList, m_aAppRows, mark + app.ResolveLabel() + "  (" + count.ToString() + ")");
		}

		ShowAppFields();
		RebuildItems();
	}

	protected void RebuildItems()
	{
		m_aItemRows.Clear();

		if (m_wItemList)
			ClearChildren(m_wItemList);

		MCF_Device_App app = CurrentApp();
		if (!app || !app.m_aItems)
		{
			ShowItemFields();
			return;
		}

		foreach (int i, MCF_Device_Item item : app.m_aItems)
		{
			string mark = "  ";
			if (i == m_iItem)
				mark = "> ";

			AddRow(m_wItemList, m_aItemRows, mark + item.DescribeShort());
		}

		ShowItemFields();
	}

	protected void AddRow(VerticalLayoutWidget parent, notnull array<SCR_ButtonTextComponent> into, string label)
	{
		if (!parent)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, parent);
		if (!row)
			return;

		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!button)
			return;

		button.SetText(label);
		button.m_OnClicked.Insert(OnRowClicked);
		into.Insert(button);
	}

	//! One handler for both lists, told apart by which array the component is
	//! in. See the note on Bind about why there is not one handler per list.
	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		CaptureFields();

		int appIndex = m_aAppRows.Find(button);
		if (appIndex >= 0)
		{
			m_iApp = appIndex;
			m_iItem = -1;
			RebuildApps();
			return;
		}

		int itemIndex = m_aItemRows.Find(button);
		if (itemIndex >= 0)
		{
			m_iItem = itemIndex;
			RebuildItems();
		}
	}

	protected void ShowAppFields()
	{
		MCF_Device_App app = CurrentApp();

		if (app)
			SetValue(m_EditAppLabel, app.m_sLabel);
		else
			SetValue(m_EditAppLabel, "");

		if (!m_ButtonKind)
			return;

		if (app)
			m_ButtonKind.SetText(MCF_Device_Names.LabelFor(app.m_eKind));
		else
			m_ButtonKind.SetText("-");
	}

	protected void ShowItemFields()
	{
		MCF_Device_Item item = CurrentItem();

		if (!item)
		{
			SetValue(m_EditHeading, "");
			SetValue(m_EditStamp, "");
			SetValue(m_EditBody, "");
			SetValue(m_EditImage, "");
			SetValue(m_EditImageUrl, "");
			return;
		}

		SetValue(m_EditHeading, item.m_sHeading);
		SetValue(m_EditStamp, item.m_sTimestamp);
		SetValue(m_EditBody, item.m_sBody);
		SetValue(m_EditImage, item.m_sImage);
		SetValue(m_EditImageUrl, item.m_sImageUrl);
	}

	protected void SetValue(SCR_EditBoxComponent editor, string value)
	{
		if (editor)
			editor.SetValue(value);
	}

	// ------------------------------------------------------------- the buttons

	protected void OnKindClicked()
	{
		CaptureFields();

		MCF_Device_App app = CurrentApp();
		if (!app)
			return;

		// Cycles MESSAGES..SETTINGS and wraps. GENERAL is skipped: it means
		// "no app named", which is not something to choose on purpose.
		int kind = app.m_eKind + 1;
		if (kind > KIND_COUNT)
			kind = MCF_EIntelApp.MESSAGES;

		app.m_eKind = kind;
		RebuildApps();
	}

	protected void OnAppAdd()
	{
		CaptureFields();

		if (!m_Draft)
			return;

		if (!m_Draft.m_aApps)
			m_Draft.m_aApps = {};

		MCF_Device_App app = new MCF_Device_App();
		app.m_eKind = MCF_EIntelApp.MESSAGES;
		app.m_aItems = {};
		m_Draft.m_aApps.Insert(app);

		m_iApp = m_Draft.m_aApps.Count() - 1;
		m_iItem = -1;
		RebuildApps();
	}

	protected void OnAppRemove()
	{
		CaptureFields();

		if (!CurrentApp())
			return;

		m_Draft.m_aApps.Remove(m_iApp);

		if (m_iApp >= m_Draft.m_aApps.Count())
			m_iApp = m_Draft.m_aApps.Count() - 1;

		m_iItem = -1;
		RebuildApps();
	}

	//! The order of the apps is the home screen, so this is a real edit and not
	//! a convenience.
	protected void MoveApp(int direction)
	{
		CaptureFields();

		if (!CurrentApp())
			return;

		int target = m_iApp + direction;
		if (target < 0 || target >= m_Draft.m_aApps.Count())
			return;

		MCF_Device_App moved = m_Draft.m_aApps[m_iApp];
		m_Draft.m_aApps[m_iApp] = m_Draft.m_aApps[target];
		m_Draft.m_aApps[target] = moved;

		m_iApp = target;
		RebuildApps();
	}

	protected void OnItemAdd()
	{
		CaptureFields();

		MCF_Device_App app = CurrentApp();
		if (!app)
		{
			Status("Pick an app first.");
			return;
		}

		if (!app.m_aItems)
			app.m_aItems = {};

		MCF_Device_Item item = new MCF_Device_Item();
		item.m_sHeading = "New item";
		app.m_aItems.Insert(item);

		m_iItem = app.m_aItems.Count() - 1;
		RebuildApps();
	}

	protected void OnItemRemove()
	{
		CaptureFields();

		MCF_Device_App app = CurrentApp();
		if (!app || !CurrentItem())
			return;

		app.m_aItems.Remove(m_iItem);

		if (m_iItem >= app.m_aItems.Count())
			m_iItem = app.m_aItems.Count() - 1;

		RebuildApps();
	}

	//! Steps through the library and loads one in as the draft.
	//!
	//! It does NOT assign it: the draft is what APPLY sends, so loading a
	//! library profile is "start from this one", and the Game Master can change
	//! its id before applying to keep the original intact.
	protected void OnLoadNext()
	{
		CaptureFields();

		array<string> ids = {};
		MCF_Device_Library.GetInstance().GetIds(ids);

		if (ids.IsEmpty())
		{
			Status("The library is empty.");
			return;
		}

		m_iLibraryIndex++;
		if (m_iLibraryIndex >= ids.Count())
			m_iLibraryIndex = 0;

		MCF_Device_Profile source = MCF_Device_Library.GetInstance().Find(ids[m_iLibraryIndex]);
		if (!source)
			return;

		// Through the serialiser, which is a deep copy that costs nothing extra
		// and is already tested. Editing the library's own object would change
		// it for every device using it, before APPLY was ever pressed.
		m_Draft = MCF_Device_Script.Deserialize(MCF_Device_Script.Serialize(source));

		m_iApp = -1;
		m_iItem = -1;

		SetValue(m_EditId, m_Draft.m_sId);
		SetValue(m_EditDevice, m_Draft.m_sDeviceName);

		RebuildApps();
		Status("Loaded '" + m_Draft.m_sId + "' from the library.");
	}

	//! Sends the whole rewritten device in one request.
	protected void OnApply()
	{
		CaptureFields();

		if (!m_Draft)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_Editable)
		{
			Status("Cannot reach the server.");
			return;
		}

		// The object is named across the wire by its replication id, the same
		// way vanilla's own editor identifies an edited entity. A name would be
		// ambiguous and a position guessable; this is neither.
		RplId targetId = Replication.FindItemId(m_Editable);
		if (targetId == RplId.Invalid())
		{
			Status("That object is not replicated and cannot be edited.");
			return;
		}

		controller.MCF_RequestWriteDeviceProfile(targetId, MCF_Device_Script.Serialize(m_Draft));

		int apps;
		if (m_Draft.m_aApps)
			apps = m_Draft.m_aApps.Count();

		Status("Sent. " + apps.ToString() + " app(s) applied.");
	}

	// ------------------------------------------------------------- plumbing

	protected void Status(string text)
	{
		if (m_wStatus)
			m_wStatus.SetText(text);
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
}
