//! The laptop, as a desktop.
//!
//! WHY A SECOND MENU CLASS AND NOT A FOURTH SKIN ON THE FIRST. The handset's
//! shell shows one screen at a time and everything in it is written that way:
//! one open app, one open entry, one back button that means "up". A desktop has
//! none of those things. Ten windows can be open at once, each with its own
//! selection, and "back" is not a concept. Bolting that onto a 100 KB menu that
//! already works would have produced a third mode inside every method on it.
//!
//! WHAT IS SHARED IS THE PART WORTH SHARING: the content. MCF_Device_Presenter,
//! MCF_Device_ReadState, MCF_Device_Script, MCF_Device_Text, the row layout and
//! the bubble layout are all the handset's, unchanged. A device profile does
//! not know what it is being drawn on, and that is the whole design.
//!
//! GEOMETRY. Everything is a fraction of the desktop, and the desktop is placed
//! every frame at 16:10 -- a laptop is landscape, and that ratio is the only
//! real difference between this and the handset. Window boxes are the one
//! exception: absolute reference units, because a window that can be dragged
//! has no anchor by definition.
class MCF_Desktop_ShellMenu : ChimeraMenuBase
{
	protected static const ResourceName ROW_LAYOUT = "{6A1C4F0B39D35600}UI/layouts/MCF/MCF_PhoneRow.layout";
	protected static const ResourceName BUBBLE_LAYOUT = "{6A1C4F0B39D3564F}UI/layouts/MCF/MCF_PhoneBubble.layout";
	protected static const ResourceName HACK_LAYOUT = "{6A1C4F0B39D30000}UI/layouts/MCF/MCF_DeviceHack.layout";

	//! A folder's own icon, for the one row in a file list that is not a
	//! document.
	protected static const ResourceName FOLDER_ICON = "{6A1C4F0B39D35502}UI/images/MCF_Desktop/icon_folder.edds";

	//! A laptop screen. Not 16:9: every machine this is meant to look like is
	//! 16:10, and the difference is visible.
	protected static const float SCREEN_ASPECT = 1.60;
	protected static const float SCREEN_SHARE = 0.94;

	//! How many window frames the layout carries. One per app kind plus the
	//! terminal; a device with two contact books gets one contacts window,
	//! which is correct.
	protected static const int WINDOWS = 13;

	//! How many of those are apps the device has, rather than editors a file
	//! opens into. The launcher and the pins only ever offer these.
	protected static const int APP_WINDOWS = 10;

	//! The panel's share of the desktop's height, matching the layout.
	protected static const float PANEL_SHARE = 0.0560;

	//! The title bar's share of a window, matching the layout.
	protected static const float BAR_SHARE = 0.072;

	//! The passcode. The same four digits the handset takes, because a player
	//! who has cracked one device in a mission should not have to learn a
	//! second convention for the next one.
	protected static const string BREAK_IN_CODE = "1337";

	protected static const int PINS = 6;
	protected static const int LAUNCHER_SLOTS = 12;
	protected static const int PHOTO_TILES = 12;
	protected static const int LOCK_NOTES = 3;

	//! Which slot shows which app. Index is the window slot; the value is an
	//! MCF_EIntelApp, or -1 for a window that is not an app at all.
	protected static const string PANE_LIST = "list";
	protected static const string PANE_MAIL = "mail";
	protected static const string PANE_CHAT = "chat";
	protected static const string PANE_CONTACTS = "contacts";
	protected static const string PANE_PHOTOS = "photos";
	protected static const string PANE_TERM = "term";
	protected static const string PANE_FILES = "files";
	protected static const string PANE_TEXT = "text";
	protected static const string PANE_SHEET = "sheet";
	protected static const string PANE_DOC = "doc";

	//! The grid the spreadsheet draws, matching the layout.
	protected static const int SHEET_COLS = 6;
	protected static const int SHEET_ROWS = 13;

	protected static const ResourceName DESKTOP_ROW_LAYOUT = "{6A1C4F0B39D3566F}UI/layouts/MCF/MCF_DesktopRow.layout";
	protected static const ResourceName DOC_ICON = "{6A1C4F0B39D3550C}UI/images/MCF_Desktop/icon_doc.edds";
	protected static const ResourceName SHEET_ICON = "{6A1C4F0B39D35509}UI/images/MCF_Desktop/icon_settings.edds";
	protected static const ResourceName PHOTO_ICON = "{6A1C4F0B39D35506}UI/images/MCF_Desktop/icon_photos.edds";

	// -------------------------------------------------------------- the state

	protected static MCF_Desktop_ShellMenu s_Open;
	protected static MCF_Intel_CarrierComponent s_PendingCarrier;
	protected static SCR_EditableEntityComponent s_PendingEditable;
	protected static bool s_bPendingAuthor;

	protected MCF_Intel_CarrierComponent m_Carrier;
	protected SCR_EditableEntityComponent m_Editable;
	protected bool m_bAuthor;

	protected ref MCF_Device_Presenter m_Content = new MCF_Device_Presenter();
	protected ref MCF_Device_Profile m_Draft;

	protected Widget m_wScreen;
	protected Widget m_wPanel;
	protected Widget m_wLauncher;
	protected Widget m_wLockScreen;
	protected Widget m_wEditor;
	protected Widget m_wDragCatch;
	protected Widget m_wCtxMenu;
	protected ref MCF_Desktop_Menu m_DeskMenuHandler;

	protected float m_fScreenW;
	protected float m_fScreenH;

	protected ref array<ref MCF_Desktop_Window> m_aWindows = {};
	protected ref array<ref MCF_Desktop_Drag> m_aDragHandlers = {};
	protected ref MCF_Desktop_Catch m_CatchHandler;

	//! The order windows were last brought forward in, newest last. It is what
	//! decides which one a close falls back to, and it is cheaper than asking
	//! ten widgets what their Z order is.
	protected ref array<int> m_aStack = {};
	protected int m_iZ = 10;

	protected int m_iDragSlot = -1;
	protected float m_fGrabX;
	protected float m_fGrabY;
	protected float m_fDragFromX;
	protected float m_fDragFromY;

	protected SCR_ButtonTextComponent m_Kick;
	protected ref array<SCR_ButtonTextComponent> m_aDeskIcons = {};
	protected ref array<int> m_aDeskSlots = {};
	protected bool m_bLauncherOpen;
	protected ref array<SCR_ButtonTextComponent> m_aPins = {};
	protected ref array<int> m_aPinSlots = {};
	protected ref array<SCR_ButtonTextComponent> m_aTasks = {};
	protected ref array<int> m_aTaskSlots = {};
	protected ref array<SCR_ButtonTextComponent> m_aLauncherApps = {};
	protected ref array<int> m_aLauncherSlots = {};

	protected TextWidget m_wTrayClock;
	protected TextWidget m_wTrayDate;
	protected TextWidget m_wTrayPct;
	protected TextWidget m_wLockClock;
	protected TextWidget m_wLockDate;
	protected TextWidget m_wLockMsg;
	protected EditBoxWidget m_LockField;
	protected SCR_ButtonTextComponent m_LockGo;
	protected SCR_ButtonTextComponent m_LauncherLock;

	protected bool m_bOnLock;
	protected float m_fStatusTick;

	protected Widget m_wHackPanel;
	protected ref MCF_Devices_HackScreen m_HackScreen;
	protected bool m_bHackDone;

	//! Author mode: which window and which field the editor was opened from.
	protected TextWidget m_wEditorTitle;
	protected EditBoxWidget m_EditorField;
	protected SCR_ButtonTextComponent m_EditorSave;
	protected SCR_ButtonTextComponent m_EditorCancel;
	protected string m_sEditKind;
	protected int m_iEditSlot = -1;
	protected MCF_Device_Item m_EditItem;

	// ================================================================== opening

	//! Opens the laptop for a player.
	static bool OpenFor(notnull MCF_Intel_CarrierComponent carrier)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return false;

		s_PendingCarrier = carrier;

		if (!menuManager.OpenMenu(ChimeraMenuPreset.MCF_IntelDesktop))
		{
			s_PendingCarrier = null;
			s_PendingEditable = null;
			s_bPendingAuthor = false;
			return false;
		}

		return true;
	}

	//! Opens it the way a Game Master edits it: the desktop itself, with the
	//! toolbars switched on and no lock screen in the way. Same argument as the
	//! handset's -- a second screen that edits the same object under a similar
	//! name is the confusion that cost a day once already.
	static bool OpenForAuthor(notnull MCF_Intel_CarrierComponent carrier, SCR_EditableEntityComponent editable)
	{
		s_PendingEditable = editable;
		s_bPendingAuthor = true;

		if (OpenFor(carrier))
			return true;

		s_PendingEditable = null;
		s_bPendingAuthor = false;
		return false;
	}

	//! The server's answer to a break-in request, drawn on the desktop it
	//! belongs to. The player controller tries the handset first and falls
	//! through to here, so one reply serves both shells.
	static bool ShowChallenge(RplId deviceId, int seed, int difficulty)
	{
		if (!s_Open)
			return false;

		return s_Open.StartHack(deviceId, seed, difficulty);
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Carrier = s_PendingCarrier;
		m_Editable = s_PendingEditable;
		m_bAuthor = s_bPendingAuthor;
		s_PendingCarrier = null;
		s_PendingEditable = null;
		s_bPendingAuthor = false;
		s_Open = this;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("desktop shell opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wScreen = root.FindAnyWidget("Screen");
		m_wPanel = root.FindAnyWidget("Panel");
		m_wLauncher = root.FindAnyWidget("Launcher");
		m_wLockScreen = root.FindAnyWidget("LockScreen");
		m_wEditor = root.FindAnyWidget("EditorPane");

		if (m_Carrier)
			m_Content.Bind(m_Carrier);

		BindWindows(root);
		BindPanel(root);
		BindLauncher(root);
		BindLock(root);
		BindEditor(root);
		BindDragCatch(root);
		BindContextMenu(root);

		// The panel and the overlays sit above every window whatever a window's
		// Z order becomes. A taskbar a window can be dragged on top of is not a
		// taskbar.
		if (m_wPanel)
			m_wPanel.SetZOrder(5000);

		if (m_wLauncher)
			m_wLauncher.SetZOrder(6000);

		if (m_wLockScreen)
			m_wLockScreen.SetZOrder(8000);

		if (m_wEditor)
			m_wEditor.SetZOrder(8500);

		FitScreen();
		BindApps();

		if (!m_bAuthor && IsDeviceLocked())
		{
			ShowLock();
		}
		else
		{
			if (m_wLockScreen)
				m_wLockScreen.SetVisible(false);

			ShowDesktopChrome(true);
			OpenDefaults();
		}

		RefreshStatus();
	}

	override void OnMenuClose()
	{
		CloseHack();

		if (s_Open == this)
			s_Open = null;

		super.OnMenuClose();
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		// The desktop is re-placed every frame: three slot calls, correct on
		// the first one, and it survives a window resize -- which is the whole
		// lesson of the handset's abandoned 3D preview.
		FitScreen();

		UpdateDrag();

		if (m_HackScreen)
		{
			if (m_bHackDone)
			{
				FinishHack();
			}
			else
			{
				FitHackPanel();
				m_HackScreen.Update();
			}
		}

		PollLock();

		// Once a second is enough for a clock that shows minutes, and cheap
		// enough not to think about.
		m_fStatusTick = m_fStatusTick + tDelta;
		if (m_fStatusTick >= 1.0)
		{
			m_fStatusTick = 0;
			RefreshStatus();
		}
	}

	// ================================================================ geometry

	//! Places the desktop, 16:10, centred, as large as the window allows.
	protected void FitScreen()
	{
		if (!m_wScreen)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float wide = workspace.GetWidth();
		float tall = workspace.GetHeight();
		if (wide <= 0 || tall <= 0)
			return;

		float h = tall * SCREEN_SHARE;
		float w = h * SCREEN_ASPECT;

		float maxW = wide * SCREEN_SHARE;
		if (w > maxW)
		{
			w = maxW;
			h = w / SCREEN_ASPECT;
		}

		FrameSlot.SetAnchor(m_wScreen, 0.5, 0.5);
		FrameSlot.SetSize(m_wScreen, w, h);
		FrameSlot.SetPos(m_wScreen, -w * 0.5, -h * 0.5);

		bool changed = w != m_fScreenW || h != m_fScreenH;
		m_fScreenW = w;
		m_fScreenH = h;

		// A window's box is in absolute units, so a resized desktop has to be
		// told about it or every open window keeps the size it had on the old
		// one and half of them end up off the edge.
		if (changed)
			ReplaceAllWindows();
	}

	protected void ReplaceAllWindows()
	{
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (!win || !win.m_bOpen)
				continue;

			if (win.m_bMax)
			{
				MaximiseBox(win);
			}
			else
			{
				win.m_fW = win.m_fWantW * m_fScreenW;
				win.m_fH = win.m_fWantH * m_fScreenH;
				ClampWindow(win);
			}

			PlaceWindow(win);
		}
	}

	protected void PlaceWindow(notnull MCF_Desktop_Window win)
	{
		if (!win.m_wRoot)
			return;

		FrameSlot.SetAnchor(win.m_wRoot, 0, 0);
		FrameSlot.SetSize(win.m_wRoot, win.m_fW, win.m_fH);
		FrameSlot.SetPos(win.m_wRoot, win.m_fX, win.m_fY);
	}

	protected void MaximiseBox(notnull MCF_Desktop_Window win)
	{
		win.m_fX = 0;
		win.m_fY = 0;
		win.m_fW = m_fScreenW;
		win.m_fH = m_fScreenH * (1 - PANEL_SHARE);
	}

	//! Keeps a window somewhere a hand can still reach its title bar.
	//!
	//! Deliberately loose: most of a window may hang off the right or the
	//! bottom, because a person parking one half-off the screen is doing that
	//! on purpose. What is not allowed is the title bar leaving, because then
	//! there is nothing left to drag it back by.
	protected void ClampWindow(notnull MCF_Desktop_Window win)
	{
		float barH = win.m_fH * BAR_SHARE;
		float floor = m_fScreenH * (1 - PANEL_SHARE) - barH;

		if (win.m_fX < -win.m_fW + 80)
			win.m_fX = -win.m_fW + 80;

		if (win.m_fX > m_fScreenW - 80)
			win.m_fX = m_fScreenW - 80;

		if (win.m_fY < 0)
			win.m_fY = 0;

		if (win.m_fY > floor)
			win.m_fY = floor;
	}

	// ================================================================ dragging

	//! A title bar went down. Everything after this happens on the tick.
	//!
	//! ENFUSION HAS NO OnMouseMove AND NO MOUSE CAPTURE -- see
	//! MCF_Desktop_Drag. The press records where the pointer and the window
	//! were, and the per-frame poll does the moving.
	void BeginDrag(int slot, int x, int y)
	{
		if (slot < 0 || slot >= m_aWindows.Count())
			return;

		MCF_Desktop_Window win = m_aWindows[slot];
		if (!win || !win.m_bOpen || win.m_bMin)
			return;

		FocusWindow(slot);

		// A maximised window is not dragged; it is restored first, the way
		// every desktop does it.
		if (win.m_bMax)
			return;

		float mx, my;
		if (!MousePoint(mx, my))
			return;

		m_iDragSlot = slot;
		m_fGrabX = mx;
		m_fGrabY = my;
		m_fDragFromX = win.m_fX;
		m_fDragFromY = win.m_fY;

		if (m_wDragCatch)
		{
			m_wDragCatch.SetVisible(true);
			m_wDragCatch.SetZOrder(7500);
		}
	}

	void EndDrag()
	{
		m_iDragSlot = -1;

		if (m_wDragCatch)
			m_wDragCatch.SetVisible(false);
	}

	protected void UpdateDrag()
	{
		if (m_iDragSlot < 0)
			return;

		if (m_iDragSlot >= m_aWindows.Count())
		{
			EndDrag();
			return;
		}

		MCF_Desktop_Window win = m_aWindows[m_iDragSlot];
		if (!win || !win.m_bOpen || win.m_bMin || win.m_bMax)
		{
			EndDrag();
			return;
		}

		float mx, my;
		if (!MousePoint(mx, my))
			return;

		win.m_fX = m_fDragFromX + (mx - m_fGrabX);
		win.m_fY = m_fDragFromY + (my - m_fGrabY);
		ClampWindow(win);
		PlaceWindow(win);
	}

	//! The pointer, in the units a FrameSlot understands.
	//!
	//! GetMousePos ANSWERS IN PHYSICAL PIXELS and FrameSlot works in the
	//! reference resolution. Feeding one to the other unconverted is the same
	//! mistake that made the handset come out narrow, in a place where it would
	//! instead make a window fly away from the cursor at anything but 100% DPI.
	protected bool MousePoint(out float x, out float y)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		int px, py;
		WidgetManager.GetMousePos(px, py);

		x = workspace.DPIUnscale(px);
		y = workspace.DPIUnscale(py);
		return true;
	}

	// ================================================================= windows

	//! The window table. Slot, title, which app kind it draws, which pane shape
	//! was generated into it, and how big it wants to be as a share of the
	//! desktop -- a window measured in pixels is a postage stamp on one machine
	//! and full screen on another.
	protected void BindWindows(notnull Widget root)
	{
		m_aWindows.Clear();
		m_aDragHandlers.Clear();

		AddWindow(root, 0, "Files", MCF_EIntelApp.FILES, PANE_FILES, 0.611, 0.578);
		AddWindow(root, 1, "Mail", MCF_EIntelApp.EMAIL, PANE_MAIL, 0.604, 0.606);
		AddWindow(root, 2, "Messages", MCF_EIntelApp.MESSAGES, PANE_CHAT, 0.521, 0.561);
		AddWindow(root, 3, "Contacts", MCF_EIntelApp.CONTACTS, PANE_CONTACTS, 0.479, 0.517);
		AddWindow(root, 4, "Photos", MCF_EIntelApp.PHOTOS, PANE_PHOTOS, 0.507, 0.561);
		AddWindow(root, 5, "Notes", MCF_EIntelApp.NOTES, PANE_LIST, 0.444, 0.511);
		AddWindow(root, 6, "Terminal", -1, PANE_TERM, 0.472, 0.444);
		AddWindow(root, 7, "Settings", MCF_EIntelApp.SETTINGS, PANE_LIST, 0.458, 0.489);
		AddWindow(root, 8, "Calls", MCF_EIntelApp.CALLS, PANE_LIST, 0.458, 0.489);
		AddWindow(root, 9, "Archive", MCF_EIntelApp.GENERAL, PANE_LIST, 0.458, 0.489);

		// The editors. Kind -1, because nothing in a device profile mentions
		// them: they are what a file opens into, not something the device has.
		AddWindow(root, 10, "Text editor", -1, PANE_TEXT, 0.528, 0.578);
		AddWindow(root, 11, "Spreadsheet", -1, PANE_SHEET, 0.625, 0.622);
		AddWindow(root, 12, "Document", -1, PANE_DOC, 0.542, 0.644);
	}

	protected void AddWindow(notnull Widget root, int slot, string title, int kind, string pane, float wantW, float wantH)
	{
		MCF_Desktop_Window win = new MCF_Desktop_Window();
		win.m_iSlot = slot;
		win.m_eKind = kind;
		win.m_sPane = pane;
		win.m_fWantW = wantW;
		win.m_fWantH = wantH;

		string p = win.Prefix();
		win.m_wRoot = root.FindAnyWidget(p);
		win.m_wBody = root.FindAnyWidget(p + "Body");

		win.m_Bar = SCR_ButtonTextComponent.GetButtonText(p + "Bar", root);
		win.m_Min = SCR_ButtonTextComponent.GetButtonText(p + "Min", root);
		win.m_Max = SCR_ButtonTextComponent.GetButtonText(p + "Max", root);
		win.m_Close = SCR_ButtonTextComponent.GetButtonText(p + "Close", root);

		if (win.m_Bar)
		{
			win.m_Bar.m_OnClicked.Insert(OnBarClicked);

			// The drag lives on the widget, not on the component: the component
			// reports a completed click and a drag is the opposite of that.
			MCF_Desktop_Drag drag = new MCF_Desktop_Drag(this, slot);
			win.m_Bar.GetRootWidget().AddHandler(drag);
			m_aDragHandlers.Insert(drag);
		}

		if (win.m_Min)
			win.m_Min.m_OnClicked.Insert(OnMinClicked);

		if (win.m_Max)
			win.m_Max.m_OnClicked.Insert(OnMaxClicked);

		if (win.m_Close)
			win.m_Close.m_OnClicked.Insert(OnCloseClicked);

		TextWidget titleText = TextWidget.Cast(root.FindAnyWidget(p + "Title"));
		if (titleText)
			titleText.SetText(title);

		BindToolbar(root, win);

		if (win.m_wRoot)
			win.m_wRoot.SetVisible(false);

		m_aWindows.Insert(win);
	}

	//! Binds each window to the app the open device actually has.
	//!
	//! A device with no photographs has no photo window, and the launcher has
	//! no tile for one. A desktop that offers an app which opens onto nothing
	//! is a desktop that has told the player a lie about what they found.
	protected void BindApps()
	{
		array<MCF_Device_App> apps = {};
		m_Content.GetApps(apps);

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			win.m_App = null;

			if (win.m_eKind < 0)
				continue;

			foreach (MCF_Device_App app : apps)
			{
				if (app.m_eKind == win.m_eKind)
				{
					win.m_App = app;
					break;
				}
			}
		}

		PaintPins();
		PaintLauncher();
	}

	//! What a laptop opens onto when it is unlocked: the file manager, and the
	//! app with something new in it if there is one. A desktop that comes up
	//! empty says nothing about the person who owned it.
	protected void OpenDefaults()
	{
		OpenWindow(0);

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (!win.m_App)
				continue;

			if (m_Content.UnreadCount(win.m_App) > 0)
			{
				OpenWindow(win.m_iSlot);
				return;
			}
		}
	}

	protected bool OpenWindow(int slot)
	{
		if (slot < 0 || slot >= m_aWindows.Count())
			return false;

		MCF_Desktop_Window win = m_aWindows[slot];
		if (!win || !win.m_wRoot)
			return false;

		// The terminal is the one window with no app behind it.
		if (win.m_eKind >= 0 && !win.m_App)
			return false;

		if (win.m_bOpen)
		{
			if (win.m_bMin)
			{
				win.m_bMin = false;
				win.m_wRoot.SetVisible(true);
			}

			FocusWindow(slot);
			return true;
		}

		win.m_bOpen = true;
		win.m_bMin = false;
		win.m_bMax = false;
		win.m_wRoot.SetVisible(true);

		win.m_fW = win.m_fWantW * m_fScreenW;
		win.m_fH = win.m_fWantH * m_fScreenH;

		// Cascaded off however many are already up, so two windows opened back
		// to back do not land exactly on top of one another.
		int already = OpenCount() - 1;
		if (already < 0)
			already = 0;

		win.m_fX = m_fScreenW * (0.055 + already * 0.026);
		win.m_fY = m_fScreenH * (0.045 + already * 0.036);
		ClampWindow(win);
		PlaceWindow(win);

		FillWindow(win);
		FocusWindow(slot);
		PaintTasks();
		return true;
	}

	protected void CloseWindow(int slot)
	{
		if (slot < 0 || slot >= m_aWindows.Count())
			return;

		MCF_Desktop_Window win = m_aWindows[slot];
		if (!win)
			return;

		win.m_bOpen = false;
		win.m_bMin = false;
		win.m_iOpenEntry = -1;

		if (win.m_wRoot)
			win.m_wRoot.SetVisible(false);

		int at = m_aStack.Find(slot);
		if (at >= 0)
			m_aStack.Remove(at);

		if (m_iDragSlot == slot)
			EndDrag();

		PaintTasks();

		// Hand focus to whatever was in front before it.
		if (!m_aStack.IsEmpty())
			FocusWindow(m_aStack[m_aStack.Count() - 1]);
	}

	protected void MinimiseWindow(int slot)
	{
		MCF_Desktop_Window win = WindowAt(slot);
		if (!win || !win.m_bOpen)
			return;

		win.m_bMin = true;

		if (win.m_wRoot)
			win.m_wRoot.SetVisible(false);

		if (m_iDragSlot == slot)
			EndDrag();

		PaintTasks();
	}

	protected void ToggleMaximise(int slot)
	{
		MCF_Desktop_Window win = WindowAt(slot);
		if (!win || !win.m_bOpen)
			return;

		if (win.m_bMax)
		{
			win.m_bMax = false;
			win.m_fX = win.m_fPrevX;
			win.m_fY = win.m_fPrevY;
			win.m_fW = win.m_fPrevW;
			win.m_fH = win.m_fPrevH;
		}
		else
		{
			win.m_fPrevX = win.m_fX;
			win.m_fPrevY = win.m_fY;
			win.m_fPrevW = win.m_fW;
			win.m_fPrevH = win.m_fH;
			win.m_bMax = true;
			MaximiseBox(win);
		}

		ClampWindow(win);
		PlaceWindow(win);
		FocusWindow(slot);
	}

	//! Brings a window to the front.
	//!
	//! SetZOrder EXISTS and is the direct call; the re-parenting trick other
	//! engines need is not required here. The counter only ever goes up, which
	//! is fine for a screen nobody keeps open for a million clicks.
	protected void FocusWindow(int slot)
	{
		MCF_Desktop_Window win = WindowAt(slot);
		if (!win || !win.m_bOpen)
			return;

		m_iZ = m_iZ + 1;

		if (win.m_wRoot)
			win.m_wRoot.SetZOrder(m_iZ);

		int at = m_aStack.Find(slot);
		if (at >= 0)
			m_aStack.Remove(at);

		m_aStack.Insert(slot);
		PaintTasks();
	}

	protected MCF_Desktop_Window WindowAt(int slot)
	{
		if (slot < 0 || slot >= m_aWindows.Count())
			return null;

		return m_aWindows[slot];
	}

	protected int OpenCount()
	{
		int n;
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (win && win.m_bOpen)
				n++;
		}

		return n;
	}

	protected int FrontSlot()
	{
		if (m_aStack.IsEmpty())
			return -1;

		return m_aStack[m_aStack.Count() - 1];
	}

	// ------------------------------------------------------- chrome handlers

	protected void OnBarClicked(SCR_ButtonTextComponent button)
	{
		int slot = SlotOfBar(button);
		if (slot >= 0)
			FocusWindow(slot);
	}

	protected void OnMinClicked(SCR_ButtonTextComponent button)
	{
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (win && win.m_Min == button)
			{
				MinimiseWindow(win.m_iSlot);
				return;
			}
		}
	}

	protected void OnMaxClicked(SCR_ButtonTextComponent button)
	{
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (win && win.m_Max == button)
			{
				ToggleMaximise(win.m_iSlot);
				return;
			}
		}
	}

	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (win && win.m_Close == button)
			{
				CloseWindow(win.m_iSlot);
				return;
			}
		}
	}

	protected int SlotOfBar(SCR_ButtonTextComponent button)
	{
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (win && win.m_Bar == button)
				return win.m_iSlot;
		}

		return -1;
	}

	// ================================================================ the panel

	//! The taskbar has ten buttons and the device has thirteen windows, but
	//! three of them are editors and nobody has ten apps open at once.
	protected void BindPanel(notnull Widget root)
	{
		m_Kick = SCR_ButtonTextComponent.GetButtonText("Kick", root);
		if (m_Kick)
			m_Kick.m_OnClicked.Insert(OnKickClicked);

		m_aPins.Clear();
		m_aPinSlots.Clear();
		for (int i = 0; i < PINS; i++)
		{
			SCR_ButtonTextComponent pin = SCR_ButtonTextComponent.GetButtonText("Pin" + i.ToString(), root);
			m_aPins.Insert(pin);
			m_aPinSlots.Insert(-1);

			if (pin)
				pin.m_OnClicked.Insert(OnPinClicked);
		}

		m_aTasks.Clear();
		m_aTaskSlots.Clear();
		for (int t = 0; t < APP_WINDOWS; t++)
		{
			SCR_ButtonTextComponent task = SCR_ButtonTextComponent.GetButtonText("Task" + t.ToString(), root);
			m_aTasks.Insert(task);
			m_aTaskSlots.Insert(-1);

			if (task)
			{
				task.m_OnClicked.Insert(OnTaskClicked);
				task.GetRootWidget().SetVisible(false);
			}

			Widget lit = root.FindAnyWidget("TaskLit" + t.ToString());
			if (lit)
				lit.SetVisible(false);
		}

		// The three shortcuts on the wallpaper. They are not decoration: a
		// desktop whose icons do nothing is the tell that the whole screen is
		// a picture of a desktop.
		m_aDeskIcons.Clear();
		m_aDeskSlots.Clear();

		array<string> deskNames = {"DeskHome", "DeskDocs", "DeskFile"};

		for (int d = 0; d < deskNames.Count(); d++)
		{
			SCR_ButtonTextComponent icon = SCR_ButtonTextComponent.GetButtonText(deskNames[d], root);
			m_aDeskIcons.Insert(icon);
			m_aDeskSlots.Insert(d);

			if (icon)
				icon.m_OnClicked.Insert(OnDeskIconClicked);
		}

		m_wTrayClock = TextWidget.Cast(root.FindAnyWidget("TrayClock"));
		m_wTrayDate = TextWidget.Cast(root.FindAnyWidget("TrayDate"));
		m_wTrayPct = TextWidget.Cast(root.FindAnyWidget("TrayPct"));
	}

	//! The launchers pinned to the panel: the apps this device actually has,
	//! in the order a person reaches for them, up to six.
	protected void PaintPins()
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		array<int> wanted = {1, 2, 0, 4, 6, 7};
		int filled;

		for (int i = 0; i < m_aPins.Count(); i++)
		{
			m_aPinSlots[i] = -1;

			SCR_ButtonTextComponent pin = m_aPins[i];
			if (pin)
				pin.GetRootWidget().SetVisible(false);

			SetPinBadge(root, i, 0);
		}

		foreach (int slot : wanted)
		{
			if (filled >= m_aPins.Count())
				break;

			MCF_Desktop_Window win = WindowAt(slot);
			if (!win)
				continue;

			if (win.m_eKind >= 0 && !win.m_App)
				continue;

			SCR_ButtonTextComponent pin = m_aPins[filled];
			if (!pin)
			{
				filled++;
				continue;
			}

			m_aPinSlots[filled] = slot;
			pin.GetRootWidget().SetVisible(true);

			ImageWidget icon = ImageWidget.Cast(root.FindAnyWidget("Pin" + filled.ToString() + "Icon"));
			if (icon)
				icon.LoadImageTexture(0, IconFor(slot));

			int unread;
			if (win.m_App)
				unread = m_Content.UnreadCount(win.m_App);

			SetPinBadge(root, filled, unread);
			filled++;
		}
	}

	protected void SetPinBadge(notnull Widget root, int index, int count)
	{
		Widget badge = root.FindAnyWidget("PinBadge" + index.ToString());
		TextWidget text = TextWidget.Cast(root.FindAnyWidget("PinBadgeText" + index.ToString()));

		bool show = count > 0 && m_aPinSlots[index] >= 0;

		if (badge)
			badge.SetVisible(show);

		if (text)
		{
			text.SetVisible(show);

			if (show)
			{
				if (count > 9)
					text.SetText("9+");
				else
					text.SetText(count.ToString());
			}
		}
	}

	//! One task button per open window, in the order they were opened.
	protected void PaintTasks()
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		int front = FrontSlot();
		int used;

		for (int i = 0; i < m_aTasks.Count(); i++)
		{
			m_aTaskSlots[i] = -1;
		}

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (!win || !win.m_bOpen)
				continue;

			if (used >= m_aTasks.Count())
				break;

			SCR_ButtonTextComponent task = m_aTasks[used];
			if (!task)
			{
				used++;
				continue;
			}

			m_aTaskSlots[used] = win.m_iSlot;
			task.GetRootWidget().SetVisible(true);

			ImageWidget icon = ImageWidget.Cast(root.FindAnyWidget("Task" + used.ToString() + "Icon"));
			if (icon)
				icon.LoadImageTexture(0, IconFor(win.m_iSlot));

			TextWidget label = TextWidget.Cast(root.FindAnyWidget("Task" + used.ToString() + "Text"));
			if (label)
				label.SetText(MCF_Device_Text.Clip(TitleOf(win), 8));

			// The lit bar under a task button is what says which window the
			// keyboard and the eye are on. A minimised one keeps its button
			// and loses its bar.
			Widget lit = root.FindAnyWidget("TaskLit" + used.ToString());
			if (lit)
				lit.SetVisible(win.m_iSlot == front && !win.m_bMin);

			used++;
		}

		for (int rest = used; rest < m_aTasks.Count(); rest++)
		{
			SCR_ButtonTextComponent task = m_aTasks[rest];
			if (task)
				task.GetRootWidget().SetVisible(false);

			Widget lit = root.FindAnyWidget("TaskLit" + rest.ToString());
			if (lit)
				lit.SetVisible(false);
		}
	}

	protected void OnTaskClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aTasks.Find(button);
		if (index < 0 || index >= m_aTaskSlots.Count())
			return;

		int slot = m_aTaskSlots[index];
		if (slot < 0)
			return;

		MCF_Desktop_Window win = WindowAt(slot);
		if (!win)
			return;

		// The third state a taskbar button has: click the one already in front
		// and it goes away. Anything else brings it forward.
		if (win.m_bMin)
		{
			win.m_bMin = false;

			if (win.m_wRoot)
				win.m_wRoot.SetVisible(true);

			FocusWindow(slot);
			return;
		}

		if (FrontSlot() == slot)
		{
			MinimiseWindow(slot);
			return;
		}

		FocusWindow(slot);
	}

	//! The three shortcuts on the wallpaper, and each lands somewhere of its
	//! own. They all opened the same flat list before, because before there was
	//! no such thing as a place.
	protected void OnDeskIconClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aDeskIcons.Find(button);
		if (index < 0)
			return;

		CloseContextMenu();

		if (index == 0)
		{
			OpenFolder("", "");
			return;
		}

		if (index == 1)
		{
			OpenFolder("Documents", "");
			return;
		}

		// A document on the desktop opens in its editor, the way it does on a
		// machine -- not in the file manager with the file highlighted.
		MCF_Desktop_Window files = WindowAt(0);
		if (!files || !files.m_App)
			return;

		array<ref MCF_Device_Item> all = {};
		m_Content.GetItems(files.m_App, all);

		foreach (MCF_Device_Item item : all)
		{
			if (MCF_Device_Text.LeafOf(item.m_sHeading) == "readme.txt")
			{
				OpenDocument(item);
				return;
			}
		}
	}

	//! Everything that belongs to a logged-in session.
	//!
	//! NOT LEFT TO Z ORDER. The lock screen is declared above the panel and the
	//! icons and is given a higher Z than either, and on the first build they
	//! still drew straight through it -- so the lock screen does not cover the
	//! desktop, it empties it. That is also what a locked machine actually
	//! looks like: no taskbar, no shortcuts, nothing but the way in.
	protected void ShowDesktopChrome(bool on)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		if (m_wPanel)
			m_wPanel.SetVisible(on);

		array<string> parts = {"DeskHome", "DeskHomeArt", "DeskHomeLabel",
		                       "DeskDocs", "DeskDocsArt", "DeskDocsLabel",
		                       "DeskFile", "DeskFileArt", "DeskFileLabel"};

		foreach (string part : parts)
		{
			ShowWidget(root, part, on);
		}
	}

	protected void OnPinClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aPins.Find(button);
		if (index < 0 || index >= m_aPinSlots.Count())
			return;

		int slot = m_aPinSlots[index];
		if (slot >= 0)
			OpenWindow(slot);

		CloseLauncher();
	}

	// ============================================================= the launcher

	protected void BindLauncher(notnull Widget root)
	{
		m_aLauncherApps.Clear();
		m_aLauncherSlots.Clear();

		for (int i = 0; i < LAUNCHER_SLOTS; i++)
		{
			SCR_ButtonTextComponent app = SCR_ButtonTextComponent.GetButtonText("LApp" + i.ToString(), root);
			m_aLauncherApps.Insert(app);
			m_aLauncherSlots.Insert(-1);

			if (app)
				app.m_OnClicked.Insert(OnLauncherAppClicked);
		}

		m_LauncherLock = SCR_ButtonTextComponent.GetButtonText("LLock", root);
		if (m_LauncherLock)
			m_LauncherLock.m_OnClicked.Insert(OnLauncherLockClicked);

		if (m_wLauncher)
			m_wLauncher.SetVisible(false);

		m_bLauncherOpen = false;
	}

	protected void PaintLauncher()
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		TextWidget user = TextWidget.Cast(root.FindAnyWidget("LUser"));
		if (user)
			user.SetText(UserName());

		TextWidget face = TextWidget.Cast(root.FindAnyWidget("LFaceText"));
		if (face)
			face.SetText(MCF_Device_Text.Initial(UserName()));

		int filled;

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (filled >= m_aLauncherApps.Count())
				break;

			// An editor is not an application on this device; it is where a
			// file goes when you open it. Offering "Spreadsheet" in a launcher
			// with nothing to put in it is a menu entry that opens a blank.
			if (win.m_iSlot >= APP_WINDOWS)
				continue;

			if (win.m_eKind >= 0 && !win.m_App)
				continue;

			SCR_ButtonTextComponent app = m_aLauncherApps[filled];
			if (!app)
			{
				filled++;
				continue;
			}

			m_aLauncherSlots[filled] = win.m_iSlot;
			app.GetRootWidget().SetVisible(true);

			ImageWidget icon = ImageWidget.Cast(root.FindAnyWidget("LIcon" + filled.ToString()));
			if (icon)
				icon.LoadImageTexture(0, IconFor(win.m_iSlot));

			TextWidget label = TextWidget.Cast(root.FindAnyWidget("LLabel" + filled.ToString()));
			if (label)
				label.SetText(TitleOf(win));

			filled++;
		}

		for (int rest = filled; rest < m_aLauncherApps.Count(); rest++)
		{
			m_aLauncherSlots[rest] = -1;

			SCR_ButtonTextComponent app = m_aLauncherApps[rest];
			if (app)
				app.GetRootWidget().SetVisible(false);

			Widget icon = root.FindAnyWidget("LIcon" + rest.ToString());
			if (icon)
				icon.SetVisible(false);

			Widget label = root.FindAnyWidget("LLabel" + rest.ToString());
			if (label)
				label.SetVisible(false);
		}
	}

	protected void OnKickClicked(SCR_ButtonTextComponent button)
	{
		if (m_bLauncherOpen)
		{
			CloseLauncher();
			return;
		}

		m_bLauncherOpen = true;

		if (m_wLauncher)
			m_wLauncher.SetVisible(true);
	}

	protected void CloseLauncher()
	{
		m_bLauncherOpen = false;

		if (m_wLauncher)
			m_wLauncher.SetVisible(false);
	}

	protected void OnLauncherAppClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aLauncherApps.Find(button);
		if (index < 0 || index >= m_aLauncherSlots.Count())
			return;

		int slot = m_aLauncherSlots[index];
		if (slot >= 0)
			OpenWindow(slot);

		CloseLauncher();
	}

	protected void OnLauncherLockClicked(SCR_ButtonTextComponent button)
	{
		CloseLauncher();
		ShowLock();
	}

	// =============================================================== the status

	protected void RefreshStatus()
	{
		string clock = ClockText();
		string date = DateText();

		if (m_wTrayClock)
			m_wTrayClock.SetText(clock);

		if (m_wTrayDate)
			m_wTrayDate.SetText(date);

		if (m_wTrayPct)
			m_wTrayPct.SetText(BatteryPercent().ToString() + "%");

		if (m_wLockClock)
			m_wLockClock.SetText(clock);

		if (m_wLockDate)
			m_wLockDate.SetText(date);
	}

	//! The mission's own clock, not the player's. Re-read every second: set
	//! once at open it drifts away from the world the longer the screen stays
	//! up, which is the bug the handset's lock screen had.
	protected string ClockText()
	{
		TimeAndWeatherManagerEntity time = WorldTime();
		if (!time)
			return "";

		TimeContainer now = time.GetTime();
		if (!now)
			return "";

		return MCF_Device_Text.Pad2(now.m_iHours) + ":" + MCF_Device_Text.Pad2(now.m_iMinutes);
	}

	protected string DateText()
	{
		TimeAndWeatherManagerEntity time = WorldTime();
		if (!time)
			return "";

		int year, month, day;
		time.GetDate(year, month, day);

		return time.GetWeekDayString() + "  " + day.ToString() + "/" + MCF_Device_Text.Pad2(month);
	}

	protected TimeAndWeatherManagerEntity WorldTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return null;

		return world.GetTimeAndWeatherManager();
	}

	//! A charge that belongs to this device and to no other, and that every
	//! player gets the same answer from. Derived from what it is called, not
	//! from a random number -- two players comparing notes on the same laptop
	//! must not read different figures off it.
	protected int BatteryPercent()
	{
		string name = m_Content.DeviceName();
		int seed = 7;

		for (int i = 0, n = name.Length(); i < n; i++)
		{
			seed = (seed * 31 + name.Get(i).ToAscii()) % 9973;
		}

		return 34 + (seed % 61);
	}

	protected string UserName()
	{
		string name = m_Content.DeviceName();
		if (name.IsEmpty())
			return "user";

		return name;
	}

	protected string TitleOf(notnull MCF_Desktop_Window win)
	{
		if (win.m_App)
			return win.m_App.ResolveLabel();

		if (win.m_Doc)
			return MCF_Device_Text.LeafOf(win.m_Doc.m_sHeading);

		if (win.m_sPane == PANE_TEXT)
			return "Text editor";

		if (win.m_sPane == PANE_SHEET)
			return "Spreadsheet";

		if (win.m_sPane == PANE_DOC)
			return "Document";

		return "Terminal";
	}

	protected ResourceName IconFor(int slot)
	{
		if (slot == 0) return "{6A1C4F0B39D35501}UI/images/MCF_Desktop/icon_files.edds";
		if (slot == 1) return "{6A1C4F0B39D35503}UI/images/MCF_Desktop/icon_mail.edds";
		if (slot == 2) return "{6A1C4F0B39D35504}UI/images/MCF_Desktop/icon_messages.edds";
		if (slot == 3) return "{6A1C4F0B39D35505}UI/images/MCF_Desktop/icon_contacts.edds";
		if (slot == 4) return "{6A1C4F0B39D35506}UI/images/MCF_Desktop/icon_photos.edds";
		if (slot == 5) return "{6A1C4F0B39D35507}UI/images/MCF_Desktop/icon_notes.edds";
		if (slot == 6) return "{6A1C4F0B39D35508}UI/images/MCF_Desktop/icon_terminal.edds";
		if (slot == 7) return "{6A1C4F0B39D35509}UI/images/MCF_Desktop/icon_settings.edds";
		if (slot == 8) return "{6A1C4F0B39D3550A}UI/images/MCF_Desktop/icon_phone.edds";

		return "{6A1C4F0B39D3550C}UI/images/MCF_Desktop/icon_doc.edds";
	}

	// ============================================================== the content

	//! Fills a window from the app behind it.
	//!
	//! ONE SWITCH, ON THE PANE SHAPE AND NOT ON THE APP KIND, so that two kinds
	//! can share a pane without sharing a window -- which is what lets files,
	//! notes, settings, calls and the archive be five windows and one layout.
	protected void FillWindow(notnull MCF_Desktop_Window win)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();

		TextWidget head = TextWidget.Cast(root.FindAnyWidget(p + "Head"));
		if (head)
			head.SetText(TitleOf(win));

		ShowToolbar(root, win);

		if (win.m_sPane == PANE_TERM)
		{
			FillTerminal(win);
			return;
		}

		if (win.m_sPane == PANE_FILES)
		{
			FillFiles(win);
			return;
		}

		if (win.m_sPane == PANE_TEXT || win.m_sPane == PANE_SHEET || win.m_sPane == PANE_DOC)
		{
			FillEditor(win);
			return;
		}

		win.m_aFolders.Clear();
		win.m_aVisible.Clear();

		if (win.m_eKind == MCF_EIntelApp.FILES)
		{
			ListFolder(win);
		}
		else if (win.m_App)
		{
			m_Content.GetItems(win.m_App, win.m_aVisible);
		}

		PaintRows(win, -1);
		ShowWhere(win);

		if (win.m_aVisible.IsEmpty())
		{
			ClearReader(win);
			return;
		}

		ShowEntry(win, 0);
	}

	//! Rebuilds one window's list.
	protected void PaintRows(notnull MCF_Desktop_Window win, int selected)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		if (win.m_sPane == PANE_PHOTOS)
		{
			PaintPhotoGrid(win, selected);
			return;
		}

		Widget list = root.FindAnyWidget(win.Prefix() + "EntryList");
		if (!list)
			return;

		win.m_aRows.Clear();
		ClearChildren(list);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		// FOLDERS FIRST, then files. The row index a click reports is an index
		// into this combined list, which is why OnRowClicked subtracts the
		// folder count before it goes looking for an item.
		foreach (string folder : win.m_aFolders)
		{
			Widget crumb = workspace.CreateWidgets(ROW_LAYOUT, list);
			if (!crumb)
				continue;

			SCR_ButtonTextComponent stepper = SCR_ButtonTextComponent.FindButtonTextComponent(crumb);
			if (!stepper)
				continue;

			stepper.m_OnClicked.Insert(OnRowClicked);
			win.m_aRows.Insert(stepper);
			stepper.SetToggled(false, false, false);
			FillFolderRow(crumb, folder);
		}

		for (int i = 0; i < win.m_aVisible.Count(); i++)
		{
			Widget row = workspace.CreateWidgets(ROW_LAYOUT, list);
			if (!row)
				continue;

			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!button)
				continue;

			button.m_OnClicked.Insert(OnRowClicked);
			win.m_aRows.Insert(button);

			button.SetToggled(i == selected, false, false);
			FillRow(win, row, win.m_aVisible[i], i == selected);
		}
	}

	//! What one row says, in the terms of the app it sits in. The handset's
	//! rules exactly -- a sender for a message, a subject for a mail, a number
	//! under a name for a contact -- because they were right there too.
	protected void FillRow(notnull MCF_Desktop_Window win, notnull Widget row, notnull MCF_Device_Item entry, bool selected)
	{
		int kind = win.m_eKind;

		TextWidget line1 = TextWidget.Cast(row.FindAnyWidget("Line1"));
		RichTextWidget line2 = RichTextWidget.Cast(row.FindAnyWidget("Line2"));
		TextWidget right = TextWidget.Cast(row.FindAnyWidget("Right"));
		ImageWidget avatar = ImageWidget.Cast(row.FindAnyWidget("Avatar"));
		TextWidget avatarText = TextWidget.Cast(row.FindAnyWidget("AvatarText"));
		Widget dot = row.FindAnyWidget("Unread");

		string title = entry.m_sHeading;
		if (title.IsEmpty())
			title = "(no subject)";

		string second = entry.m_sBody;
		string trailing = entry.m_sTimestamp;
		bool showAvatar = true;

		if (kind == MCF_EIntelApp.MESSAGES)
		{
			title = MCF_Device_Text.HeadPart(title);
		}
		else if (kind == MCF_EIntelApp.EMAIL)
		{
			title = MCF_Device_Text.SubjectOf(entry.m_sHeading);
		}
		else if (kind == MCF_EIntelApp.CONTACTS)
		{
			second = entry.m_sTimestamp;
			trailing = "";
		}
		else if (kind == MCF_EIntelApp.FILES)
		{
			// The heading is the whole path; a row in a folder shows the name.
			title = MCF_Device_Text.LeafOf(entry.m_sHeading);
			showAvatar = false;
		}
		else if (kind == MCF_EIntelApp.SETTINGS || kind == MCF_EIntelApp.NOTES)
		{
			showAvatar = false;
		}

		if (line1)
			line1.SetText(title);

		if (line2)
		{
			line2.SetVisible(!second.IsEmpty());
			line2.SetText(MCF_Device_Text.Preview(second, 40));
		}

		if (right)
			right.SetText(trailing);

		// NOT SetVisible(false) ON THE DISC. The disc is what gives this row
		// its height -- 40 units plus the overlay's padding -- and a hidden
		// widget contributes nothing to a layout. Hiding it collapsed the row
		// to the height of one line, and the title and the preview drew on top
		// of each other in every app that has no faces in it. Transparent
		// keeps the space and shows nothing, which is what was wanted.
		if (avatar)
		{
			avatar.SetVisible(true);

			if (showAvatar)
				avatar.SetColor(Color.FromInt(MCF_Device_Text.AvatarColour(title)));
			else
				avatar.SetColor(Color.FromInt(0x00000000));
		}

		if (avatarText)
		{
			avatarText.SetVisible(showAvatar);

			if (showAvatar)
				avatarText.SetText(MCF_Device_Text.Initial(title));
		}

		// Without an avatar the disc's column would sit empty. Alignment
		// padding, not FrameSlot: everything in this row is placed by
		// alignment, and mixing the two puts a widget at coordinates its parent
		// does not use.
		if (!showAvatar)
		{
			if (line1)
				AlignableSlot.SetPadding(line1, 14, 2, 62, 0);

			if (line2)
				AlignableSlot.SetPadding(line2, 14, 0, 26, 2);

			Widget divider = row.FindAnyWidget("Divider");
			if (divider)
				AlignableSlot.SetPadding(divider, 14, 0, 0, -11);
		}

		if (dot)
			dot.SetVisible(m_Content.IsUnread(entry));

		// The selected row is painted by SetToggled in PaintRows, NOT by
		// reaching into the row and colouring its Background. That widget
		// belongs to SCR_ButtonTextComponent -- it is the one thing the
		// component tints -- and writing to it from outside fights the hover
		// state for ownership of the same pixel.
	}

	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (!win || !win.m_bOpen)
				continue;

			int index = win.m_aRows.Find(button);
			if (index < 0)
				continue;

			FocusWindow(win.m_iSlot);

			// The first rows are folders, and opening one is going into it.
			if (index < win.m_aFolders.Count())
			{
				EnterFolder(win, win.m_aFolders[index]);
				return;
			}

			ShowEntry(win, index - win.m_aFolders.Count());
			return;
		}
	}

	//! Opens one item in its window's reader.
	protected void ShowEntry(notnull MCF_Desktop_Window win, int index)
	{
		if (index < 0 || index >= win.m_aVisible.Count())
			return;

		MCF_Device_Item entry = win.m_aVisible[index];
		win.m_iOpenEntry = index;

		// Opening it is what reads it -- not hovering, not scrolling past, and
		// only on this machine. See MCF_Device_ReadState for why the server is
		// deliberately not told.
		bool changed = m_Content.MarkRead(entry);

		if (win.m_sPane == PANE_MAIL)
			ShowMail(win, entry);
		else if (win.m_sPane == PANE_CHAT)
			ShowChat(win, entry);
		else if (win.m_sPane == PANE_CONTACTS)
			ShowContact(win, entry);
		else if (win.m_sPane == PANE_PHOTOS)
			ShowPhoto(win, entry);
		else
			ShowDocument(win, entry);

		PaintRows(win, index);

		if (changed)
		{
			PaintPins();
			DrawLockNotes();
		}
	}

	protected void ClearReader(notnull MCF_Desktop_Window win)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();
		win.m_iOpenEntry = -1;

		SetText(root, p + "ReadHeading", "");
		SetText(root, p + "ReadStamp", "");
		SetText(root, p + "ReadBody", "");
		SetText(root, p + "From", "");
		SetText(root, p + "Date", "");
		SetText(root, p + "Subject", "");
		SetText(root, p + "Who", "");
		SetText(root, p + "When", "");
		SetText(root, p + "Name", "");
		SetText(root, p + "Number", "");
		SetText(root, p + "Note", "");

		string empty = "Nothing here.";
		if (win.m_App)
			empty = win.m_App.ResolveEmptyText();

		SetText(root, p + "Hint", empty);
		SetText(root, p + "GridHint", empty);
	}

	// ----------------------------------------------------------- the readers

	protected void ShowDocument(notnull MCF_Desktop_Window win, notnull MCF_Device_Item entry)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();

		string shown = entry.m_sHeading;
		if (win.m_eKind == MCF_EIntelApp.FILES)
			shown = MCF_Device_Text.LeafOf(shown);

		SetText(root, p + "ReadHeading", shown);
		SetText(root, p + "ReadStamp", entry.m_sTimestamp);
		SetText(root, p + "ReadBody", MCF_Device_Text.Body(entry.m_sBody));
		SetText(root, p + "Hint", "");
	}

	protected void ShowMail(notnull MCF_Desktop_Window win, notnull MCF_Device_Item entry)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();
		SetText(root, p + "FromLabel", "FROM");
		SetText(root, p + "From", MCF_Device_Text.SenderOf(entry.m_sHeading));
		SetText(root, p + "Date", entry.m_sTimestamp);
		SetText(root, p + "Subject", MCF_Device_Text.SubjectOf(entry.m_sHeading));
		SetText(root, p + "ReadBody", MCF_Device_Text.Body(entry.m_sBody));
	}

	protected void ShowChat(notnull MCF_Desktop_Window win, notnull MCF_Device_Item entry)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();
		string who = MCF_Device_Text.SenderOf(entry.m_sHeading);

		SetText(root, p + "Who", who);

		string clock = MCF_Device_Text.TailPart(entry.m_sHeading);
		string when = entry.m_sTimestamp;
		if (!clock.IsEmpty())
		{
			if (when.IsEmpty())
				when = clock;
			else
				when = when + "   " + clock;
		}

		SetText(root, p + "When", when);
		SetText(root, p + "Initial", MCF_Device_Text.Initial(who));

		ImageWidget disc = ImageWidget.Cast(root.FindAnyWidget(p + "Avatar"));
		if (disc)
			disc.SetColor(Color.FromInt(MCF_Device_Text.AvatarColour(who)));

		DrawBubbles(win, entry);
	}

	//! One message becomes a thread. The handset's format, unchanged: every
	//! line of the body is a bubble, and a line starting with ">" came from
	//! whoever owns the device and sits on the right.
	protected void DrawBubbles(notnull MCF_Desktop_Window win, notnull MCF_Device_Item entry)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		Widget column = root.FindAnyWidget(win.Prefix() + "Thread");
		if (!column)
			return;

		ClearChildren(column);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		string said = MCF_Device_Text.Body(entry.m_sBody);

		array<string> lines = {};
		said.Split("\n", lines, true);

		foreach (string line : lines)
		{
			string trimmed = MCF_Device_Script.Trim(line);
			if (trimmed.IsEmpty())
				continue;

			bool mine = trimmed.StartsWith(">");

			if (mine)
			{
				trimmed = trimmed.Substring(1, trimmed.Length() - 1);
				trimmed = MCF_Device_Script.Trim(trimmed);
			}

			if (trimmed.IsEmpty())
				continue;

			Widget bubble = workspace.CreateWidgets(BUBBLE_LAYOUT, column);
			if (!bubble)
				continue;

			Widget left = bubble.FindAnyWidget("Left");
			Widget right = bubble.FindAnyWidget("Right");

			if (left)
				left.SetVisible(!mine);

			if (right)
				right.SetVisible(mine);

			RichTextWidget target;
			if (mine)
				target = RichTextWidget.Cast(bubble.FindAnyWidget("RightText"));
			else
				target = RichTextWidget.Cast(bubble.FindAnyWidget("LeftText"));

			if (target)
				target.SetText(trimmed);
		}
	}

	protected void ShowContact(notnull MCF_Desktop_Window win, notnull MCF_Device_Item entry)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();
		string who = entry.m_sHeading;
		if (who.IsEmpty())
			who = "Unnamed";

		ImageWidget disc = ImageWidget.Cast(root.FindAnyWidget(p + "Big"));
		if (disc)
			disc.SetColor(Color.FromInt(MCF_Device_Text.AvatarColour(who)));

		SetText(root, p + "BigText", MCF_Device_Text.Initial(who));
		SetText(root, p + "Name", who);

		if (entry.m_sTimestamp.IsEmpty())
			SetText(root, p + "Number", "No number saved");
		else
			SetText(root, p + "Number", entry.m_sTimestamp);

		if (entry.m_sBody.IsEmpty())
			SetText(root, p + "NoteLabel", "");
		else
			SetText(root, p + "NoteLabel", "NOTE");

		SetText(root, p + "Note", MCF_Device_Text.Body(entry.m_sBody));

		// A CALL button on a contact with no number is a control that cannot do
		// anything, and a device that offers it is lying about what it knows.
		SCR_ButtonTextComponent call = SCR_ButtonTextComponent.GetButtonText(p + "Call", root);
		if (call)
			call.GetRootWidget().SetVisible(!entry.m_sTimestamp.IsEmpty());
	}

	//! The photo grid. A photograph's thumbnail is its own title, which is why
	//! this app is the one that never gets a list.
	protected void PaintPhotoGrid(notnull MCF_Desktop_Window win, int selected)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();

		for (int i = 0; i < PHOTO_TILES; i++)
		{
			string tag = i.ToString();
			bool has = i < win.m_aVisible.Count();

			SCR_ButtonTextComponent tile = SCR_ButtonTextComponent.GetButtonText(p + "Tile" + tag, root);
			if (tile)
				tile.GetRootWidget().SetVisible(has);

			ImageWidget shot = ImageWidget.Cast(root.FindAnyWidget(p + "Shot" + tag));
			TextWidget cap = TextWidget.Cast(root.FindAnyWidget(p + "Cap" + tag));

			if (!has)
			{
				if (shot)
					shot.SetVisible(false);

				if (cap)
					cap.SetVisible(false);

				continue;
			}

			MCF_Device_Item item = win.m_aVisible[i];
			bool drawn;

			if (shot)
			{
				if (!item.m_sImage.IsEmpty())
					drawn = shot.LoadImageTexture(0, item.m_sImage);

				string key = item.ImageKey();
				if (!drawn && !key.IsEmpty())
					drawn = MCF_Device_ImageCache.Show(shot, key);

				shot.SetVisible(drawn);
			}

			// A tile with nothing on it yet is not empty: it is the one worth
			// opening, because opening an item is what asks for the picture.
			if (cap)
			{
				cap.SetVisible(!drawn);
				cap.SetText(MCF_Device_Text.Clip(item.m_sHeading, 22));
			}
		}

		if (win.m_aVisible.Count() > PHOTO_TILES)
			SetText(root, p + "GridHint", (win.m_aVisible.Count() - PHOTO_TILES).ToString() + " more not shown");
		else if (!win.m_aVisible.IsEmpty())
			SetText(root, p + "GridHint", "");
	}

	protected void ShowPhoto(notnull MCF_Desktop_Window win, notnull MCF_Device_Item entry)
	{
		// The grid already draws what a photo has. Opening one is what marks it
		// read and what asks the cache to fetch it, which ShowEntry did before
		// it got here -- so all that is left is to draw the tiles again with
		// whatever arrived.
		PaintPhotoGrid(win, win.m_iOpenEntry);
	}

	protected void OnPhotoTileClicked(SCR_ButtonTextComponent button)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (!win || !win.m_bOpen || win.m_sPane != PANE_PHOTOS)
				continue;

			for (int i = 0; i < PHOTO_TILES; i++)
			{
				SCR_ButtonTextComponent tile = SCR_ButtonTextComponent.GetButtonText(win.Prefix() + "Tile" + i.ToString(), root);
				if (tile != button)
					continue;

				FocusWindow(win.m_iSlot);
				ShowEntry(win, i);
				return;
			}
		}
	}

	// ------------------------------------------------------------- the terminal

	//! A replayed session, not a shell.
	//!
	//! THE ONE APP A PHONE CANNOT HAVE, and most of the reason a laptop is
	//! worth building as its own thing. It is also the most useful screen on
	//! the device for a player: a file list says what is there, and a shell
	//! history says what somebody did about it -- including what they deleted.
	//!
	//! Everything in it is read off the device's own apps, so a terminal never
	//! disagrees with the file manager sitting next to it.
	protected void FillTerminal(notnull MCF_Desktop_Window win)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string user = "user";
		string host = m_Content.DeviceName();
		host.ToLower();
		host.Replace(" ", "-");

		if (host.IsEmpty())
			host = "device";

		string prompt = "<color rgba=\"#7FCF8AFF\">" + user + "@" + host + "</color>:<color rgba=\"#7FAFE0FF\">~</color>$ ";
		string dump = prompt + "ls\n";

		array<MCF_Device_App> apps = {};
		m_Content.GetApps(apps);

		string listing;
		foreach (MCF_Device_App app : apps)
		{
			array<ref MCF_Device_Item> items = {};
			m_Content.GetItems(app, items);

			foreach (MCF_Device_Item item : items)
			{
				if (app.m_eKind != MCF_EIntelApp.FILES && app.m_eKind != MCF_EIntelApp.NOTES)
					continue;

				if (!listing.IsEmpty())
					listing = listing + "   ";

				listing = listing + FileNameOf(item.m_sHeading);
			}
		}

		if (listing.IsEmpty())
			listing = "(empty)";

		dump = dump + listing + "\n\n";
		dump = dump + prompt + "whoami\n" + user + "\n\n";
		dump = dump + prompt + "uptime\n" + " up " + ClockText() + ",  1 user,  load average: 0.14 0.09 0.05\n\n";
		dump = dump + prompt + "history | tail -n 4\n";
		dump = dump + "  511  scp " + FirstFileName() + " drop:/srv/incoming\n";
		dump = dump + "  512  <color rgba=\"#E68A2EFF\">shred -u</color> notes/old.txt\n";
		dump = dump + "  513  ssh " + host + "-office\n";
		dump = dump + "  514  history | tail -n 4\n\n";
		dump = dump + prompt;

		SetText(root, win.Prefix() + "Term", dump);
	}

	//! A heading, as a file name. Mission makers write "Draft, not sent"; a
	//! shell listing that says that is a shell listing nobody believes.
	protected string FileNameOf(string heading)
	{
		string name = MCF_Device_Text.Clip(heading, 22);
		name.ToLower();
		name.Replace(" ", "_");
		name.Replace(",", "");
		name.Replace("'", "");

		if (name.IndexOf(".") < 0)
			name = name + ".txt";

		return name;
	}

	protected string FirstFileName()
	{
		array<MCF_Device_App> apps = {};
		m_Content.GetApps(apps);

		foreach (MCF_Device_App app : apps)
		{
			if (app.m_eKind != MCF_EIntelApp.FILES)
				continue;

			array<ref MCF_Device_Item> items = {};
			m_Content.GetItems(app, items);

			if (!items.IsEmpty())
				return FileNameOf(items[0].m_sHeading);
		}

		return "archive.tar";
	}

	// ====================================================== the file manager

	//! The tree on the left, and the folder's contents on the right.
	//!
	//! A FILE MANAGER'S TWO HALVES ARE NOT A LIST AND A READER. The left half
	//! is where you are -- every folder on the device, indented -- and the
	//! right half is what is in the folder you picked. The first build put the
	//! folders on top of the files in one column and a reading pane beside it,
	//! which is a message app with a breadcrumb, not a file manager.
	protected void FillFiles(notnull MCF_Desktop_Window win)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		string p = win.Prefix();

		array<ref MCF_Device_Item> all = {};
		if (win.m_App)
			m_Content.GetItems(win.m_App, all);

		// ---- every folder on the device, in order, so the tree can be drawn
		win.m_aFolders.Clear();
		win.m_aFolders.Insert("");

		foreach (MCF_Device_Item item : all)
		{
			string folder = item.m_sHeading;

			if (!MCF_Device_Text.IsFolderMark(folder))
				folder = MCF_Device_Text.FolderOf(folder);
			else
				folder = folder.Substring(0, folder.Length() - 1);

			if (folder.IsEmpty())
				continue;

			// Every folder along the way, not just the last one: a file at
			// a/b/c.txt proves that both a and a/b exist.
			string walk;
			array<string> parts = {};
			folder.Split(MCF_Device_Text.PATH_SEP, parts, true);

			foreach (string part : parts)
			{
				walk = MCF_Device_Text.Join(walk, part);

				if (win.m_aFolders.Find(walk) < 0)
					win.m_aFolders.Insert(walk);
			}
		}

		SortPaths(win.m_aFolders);

		// ---- the files in the folder that is open
		win.m_aVisible.Clear();

		foreach (MCF_Device_Item candidate : all)
		{
			if (MCF_Device_Text.IsFolderMark(candidate.m_sHeading))
				continue;

			if (MCF_Device_Text.FolderOf(candidate.m_sHeading) == win.m_sPath)
				win.m_aVisible.Insert(candidate);
		}

		// ---- draw the tree
		win.m_aTree.Clear();
		Widget tree = root.FindAnyWidget(p + "TreeList");

		if (tree)
		{
			ClearChildren(tree);

			foreach (string path : win.m_aFolders)
			{
				Widget node = workspace.CreateWidgets(DESKTOP_ROW_LAYOUT, tree);
				if (!node)
					continue;

				SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(node);
				if (!button)
					continue;

				button.m_OnClicked.Insert(OnTreeClicked);
				win.m_aTree.Insert(button);
				button.SetToggled(path == win.m_sPath, false, false);

				string name = "This device";
				if (!path.IsEmpty())
					name = MCF_Device_Text.LeafOf(path);

				FillDeskRow(node, name, "", FOLDER_ICON, Depth(path));
			}
		}

		// ---- and the files
		win.m_aRows.Clear();
		Widget list = root.FindAnyWidget(p + "FileList");

		if (list)
		{
			ClearChildren(list);

			foreach (MCF_Device_Item file : win.m_aVisible)
			{
				Widget row = workspace.CreateWidgets(DESKTOP_ROW_LAYOUT, list);
				if (!row)
					continue;

				SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
				if (!button)
					continue;

				button.m_OnClicked.Insert(OnFileClicked);
				win.m_aRows.Insert(button);
				button.SetToggled(false, false, false);

				FillDeskRow(row, MCF_Device_Text.LeafOf(file.m_sHeading),
					file.m_sTimestamp, IconForFile(file.m_sHeading), 0);
			}
		}

		ShowWhere(win);

		string empty = "This folder is empty.";
		if (!win.m_aVisible.IsEmpty())
			empty = "";

		SetText(root, p + "Hint", empty);
	}

	//! How deep a path sits, for the tree's indent.
	protected int Depth(string path)
	{
		if (path.IsEmpty())
			return 0;

		array<string> parts = {};
		path.Split(MCF_Device_Text.PATH_SEP, parts, true);
		return parts.Count();
	}

	//! Alphabetical, so a tree does not reshuffle itself between two openings
	//! of the same device. An insertion sort: these lists are a handful long.
	protected void SortPaths(notnull array<string> paths)
	{
		for (int i = 1; i < paths.Count(); i++)
		{
			string held = paths[i];
			int j = i - 1;

			while (j >= 0 && paths[j] > held)
			{
				paths[j + 1] = paths[j];
				j--;
			}

			paths[j + 1] = held;
		}
	}

	protected void FillDeskRow(notnull Widget row, string name, string right, ResourceName icon, int depth)
	{
		TextWidget label = TextWidget.Cast(row.FindAnyWidget("Label"));
		TextWidget trailing = TextWidget.Cast(row.FindAnyWidget("Right"));
		ImageWidget art = ImageWidget.Cast(row.FindAnyWidget("Icon"));

		int indent = depth * 14;

		if (art)
		{
			art.SetVisible(true);
			art.LoadImageTexture(0, icon);
			AlignableSlot.SetPadding(art, 10 + indent, 0, 0, 0);
		}

		if (label)
		{
			label.SetText(name);
			AlignableSlot.SetPadding(label, 36 + indent, 0, 110, 0);
		}

		if (trailing)
			trailing.SetText(right);
	}

	//! Which icon a name deserves. A file manager where everything is the same
	//! grey page tells the player nothing they could not read anyway.
	protected ResourceName IconForFile(string path)
	{
		string ext = MCF_Device_Text.ExtOf(path);

		if (ext == "xls" || ext == "xlsx" || ext == "csv")
			return SHEET_ICON;

		if (ext == "jpg" || ext == "jpeg" || ext == "png")
			return PHOTO_ICON;

		return DOC_ICON;
	}

	protected void OnTreeClicked(SCR_ButtonTextComponent button)
	{
		MCF_Desktop_Window win = WindowAt(0);
		if (!win)
			return;

		int index = win.m_aTree.Find(button);
		if (index < 0 || index >= win.m_aFolders.Count())
			return;

		FocusWindow(0);
		win.m_sPath = win.m_aFolders[index];
		FillFiles(win);
	}

	protected void OnFileClicked(SCR_ButtonTextComponent button)
	{
		MCF_Desktop_Window win = WindowAt(0);
		if (!win)
			return;

		int index = win.m_aRows.Find(button);
		if (index < 0 || index >= win.m_aVisible.Count())
			return;

		// Remembered, so that Rename and Delete on the right-click menu have
		// something to act on -- a file manager's selection IS its context.
		win.m_iOpenEntry = index;

		foreach (SCR_ButtonTextComponent row : win.m_aRows)
		{
			row.SetToggled(row == button, false, false);
		}

		OpenDocument(win.m_aVisible[index]);
	}

	// =========================================================== the editors

	//! Opens a file in the editor its extension calls for.
	//!
	//! THE EXTENSION IS THE WHOLE RULE, which is how a real machine decides and
	//! how a mission maker would expect it to. Nothing else about an item says
	//! what kind of thing it is, and nothing else should have to: name a file
	//! ledger.xlsx and it opens in a grid.
	protected void OpenDocument(MCF_Device_Item item)
	{
		if (!item)
			return;

		int slot = MCF_Device_Text.EditorSlot(item.m_sHeading);

		MCF_Desktop_Window win = WindowAt(slot);
		if (!win)
			return;

		win.m_Doc = item;

		if (!OpenWindow(slot))
			return;

		FillWindow(win);

		if (m_Content.MarkRead(item))
		{
			PaintPins();
			DrawLockNotes();
		}
	}

	//! The editors take one item, not an app, so their title is the file.
	protected void FillEditor(notnull MCF_Desktop_Window win)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();

		if (!win.m_Doc)
		{
			SetText(root, p + "Head", TitleOf(win));
			return;
		}

		string name = MCF_Device_Text.LeafOf(win.m_Doc.m_sHeading);
		SetText(root, p + "Head", name);

		TextWidget bar = TextWidget.Cast(root.FindAnyWidget(p + "Title"));
		if (bar)
			bar.SetText(name);

		string body = MCF_Device_Text.Body(win.m_Doc.m_sBody);

		if (win.m_sPane == PANE_SHEET)
		{
			FillSheet(win, body);
			return;
		}

		if (win.m_sPane == PANE_DOC)
		{
			SetText(root, p + "Stamp", win.m_Doc.m_sTimestamp);
			SetText(root, p + "Body", body);
			return;
		}

		// The plain editor. The gutter is one widget carrying every number,
		// which lines up with the body for free as long as both use the same
		// line spacing -- and costs one widget instead of forty.
		array<string> lines = {};
		body.Split("\n", lines, false);

		string numbers;
		int count = lines.Count();
		if (count < 1)
			count = 1;

		for (int i = 1; i <= count; i++)
		{
			if (i > 1)
				numbers = numbers + "\n";

			numbers = numbers + i.ToString();
		}

		SetText(root, p + "Gutter", numbers);
		SetText(root, p + "Body", body);
	}

	//! A comma-separated body, in a grid.
	//!
	//! FIRST LINE IS THE HEADER, and it is written into the row that would
	//! otherwise be row 1 -- which is exactly where a person putting a table
	//! into a spreadsheet puts it. A mission maker types
	//! "Crate,Weight,Signed" and gets a spreadsheet; nothing new to learn.
	protected void FillSheet(notnull MCF_Desktop_Window win, string body)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		string p = win.Prefix();

		array<string> lines = {};
		body.Split("\n", lines, true);

		for (int r = 0; r < SHEET_ROWS; r++)
		{
			array<string> cells = {};

			if (r < lines.Count())
				lines[r].Split(",", cells, false);

			for (int c = 0; c < SHEET_COLS; c++)
			{
				string value;

				if (c < cells.Count())
					value = MCF_Device_Script.Trim(cells[c]);

				SetText(root, p + "Cell" + r.ToString() + "_" + c.ToString(), value);
			}
		}
	}

	// ========================================================== the file tree

	//! What the file manager is looking at, and everything above it.
	//!
	//! FOLDERS ARE NOT IN THE DATA MODEL. A file's path is its heading -- see
	//! MCF_Device_Text.FolderOf -- so the tree is derived here, every time, out
	//! of the paths the app happens to hold. Nothing is stored twice and there
	//! is no second structure to keep in step with the first.
	protected void ListFolder(notnull MCF_Desktop_Window win)
	{
		win.m_aFolders.Clear();
		win.m_aVisible.Clear();

		array<ref MCF_Device_Item> all = {};
		if (win.m_App)
			m_Content.GetItems(win.m_App, all);

		// Somewhere to go back to, always first, the way every file manager
		// ever written puts it.
		if (!win.m_sPath.IsEmpty())
			win.m_aFolders.Insert("..");

		foreach (MCF_Device_Item item : all)
		{
			string path = item.m_sHeading;

			if (!MCF_Device_Text.Under(path, win.m_sPath))
				continue;

			// A child folder announces itself by having something deeper than
			// this level, or by being an empty-folder mark sitting right here.
			string deeper = MCF_Device_Text.NextSegment(path, win.m_sPath);

			if (!deeper.IsEmpty())
			{
				if (win.m_aFolders.Find(deeper) < 0)
					win.m_aFolders.Insert(deeper);

				continue;
			}

			if (MCF_Device_Text.IsFolderMark(path))
			{
				string leaf = MCF_Device_Text.LeafOf(path);

				if (!leaf.IsEmpty() && win.m_aFolders.Find(leaf) < 0)
					win.m_aFolders.Insert(leaf);

				continue;
			}

			if (MCF_Device_Text.FolderOf(path) == win.m_sPath)
				win.m_aVisible.Insert(item);
		}
	}

	//! Opens the file manager at a folder.
	//!
	//! The desktop shortcuts all used to land in the same place, because there
	//! was no such thing as a place: Home and Documents were the same flat list
	//! twice. They are a location now, and so is the desktop itself.
	protected void OpenFolder(string path, string select)
	{
		MCF_Desktop_Window win = WindowAt(0);
		if (!win)
			return;

		win.m_sPath = path;

		if (!OpenWindow(0))
			return;

		// OpenWindow fills it at whatever path it already had; re-fill now that
		// the path is set.
		FillWindow(win);
	}

	protected void EnterFolder(notnull MCF_Desktop_Window win, string name)
	{
		if (name == "..")
			win.m_sPath = MCF_Device_Text.FolderOf(win.m_sPath);
		else
			win.m_sPath = MCF_Device_Text.Join(win.m_sPath, name);

		FillWindow(win);
	}

	//! The location line above the list. A file manager that does not say where
	//! it is is a list of names.
	protected void ShowWhere(notnull MCF_Desktop_Window win)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		if (win.m_eKind != MCF_EIntelApp.FILES)
		{
			SetText(root, win.Prefix() + "Where", "");
			return;
		}

		string where = "/";
		if (!win.m_sPath.IsEmpty())
			where = "/" + win.m_sPath;

		SetText(root, win.Prefix() + "Where", where + "      " + win.m_aVisible.Count().ToString() + " items");
	}

	//! A folder, drawn in the same row the files use.
	protected void FillFolderRow(notnull Widget row, string name)
	{
		TextWidget line1 = TextWidget.Cast(row.FindAnyWidget("Line1"));
		RichTextWidget line2 = RichTextWidget.Cast(row.FindAnyWidget("Line2"));
		TextWidget right = TextWidget.Cast(row.FindAnyWidget("Right"));
		ImageWidget avatar = ImageWidget.Cast(row.FindAnyWidget("Avatar"));
		TextWidget avatarText = TextWidget.Cast(row.FindAnyWidget("AvatarText"));
		Widget dot = row.FindAnyWidget("Unread");

		if (line1)
			line1.SetText(name);

		if (line2)
		{
			// A folder has no preview line, and the row keeps its height from
			// Sizer either way.
			line2.SetVisible(false);
			line2.SetText("");
		}

		if (right)
			right.SetText("");

		// The disc carries the folder's own icon rather than a letter: it is
		// the one row in this list that is not a document, and a player should
		// be able to see that without reading it.
		if (avatar)
		{
			avatar.SetVisible(true);
			avatar.SetColor(Color.FromInt(0xFFFFFFFF));
			avatar.LoadImageTexture(0, FOLDER_ICON);
		}

		if (avatarText)
			avatarText.SetVisible(false);

		if (dot)
			dot.SetVisible(false);
	}

	// ====================================================== the context menu

	//! What a right-click on the wallpaper offers.
	//!
	//! IT ACTS ON THE FOLDER IN FRONT. If a file manager window is open, a new
	//! file lands in whatever folder that window is showing; otherwise it lands
	//! on the desktop, which is the root folder -- on a real machine those are
	//! the same place, and making them the same here is what lets a Game Master
	//! right-click the wallpaper, make a file, and then find it in the window.
	void OpenContextMenu()
	{
		if (!m_bAuthor || !m_wCtxMenu)
			return;

		float mx, my;
		if (!MousePoint(mx, my))
			return;

		// The pointer is in workspace units and the menu's slot is relative to
		// the desktop, so the desktop's own corner comes off first.
		float left = mx - (GetGame().GetWorkspace().GetWidth() - m_fScreenW) * 0.5;
		float top = my - (GetGame().GetWorkspace().GetHeight() - m_fScreenH) * 0.5;

		float w = m_fScreenW * 0.150;
		float h = m_fScreenH * 0.190;

		if (left + w > m_fScreenW)
			left = m_fScreenW - w;

		if (top + h > m_fScreenH * (1 - PANEL_SHARE))
			top = m_fScreenH * (1 - PANEL_SHARE) - h;

		if (left < 0)
			left = 0;

		if (top < 0)
			top = 0;

		FrameSlot.SetAnchor(m_wCtxMenu, 0, 0);
		FrameSlot.SetSize(m_wCtxMenu, w, h);
		FrameSlot.SetPos(m_wCtxMenu, left, top);

		m_wCtxMenu.SetZOrder(7000);
		m_wCtxMenu.SetVisible(true);
	}

	void CloseContextMenu()
	{
		if (m_wCtxMenu)
			m_wCtxMenu.SetVisible(false);
	}

	//! The folder a new thing goes into.
	protected string TargetFolder()
	{
		MCF_Desktop_Window win = WindowAt(0);

		if (win && win.m_bOpen && !win.m_bMin)
			return win.m_sPath;

		return "";
	}

	protected void OnCtxNewFile(SCR_ButtonTextComponent button)
	{
		CloseContextMenu();
		MakeFileItem(false);
	}

	protected void OnCtxNewFolder(SCR_ButtonTextComponent button)
	{
		CloseContextMenu();
		MakeFileItem(true);
	}

	//! Creates a file or a folder in the front file manager's folder.
	//!
	//! A FOLDER IS A HEADING THAT ENDS IN A SLASH and nothing else. It exists
	//! so that an empty folder can exist at all; the moment a file is put in
	//! one, the folder is implied by that file's own path and the mark is
	//! redundant but harmless.
	protected void MakeFileItem(bool folder)
	{
		MCF_Desktop_Window win = WindowAt(0);
		if (!win)
			return;

		MCF_Device_App app = DraftApp(win);
		if (!app)
			return;

		string where = TargetFolder();
		string name = "new_file.txt";

		if (folder)
			name = "New folder";

		MCF_Device_Item item = new MCF_Device_Item();
		item.m_sHeading = MCF_Device_Text.Join(where, name);

		if (folder)
			item.m_sHeading = item.m_sHeading + MCF_Device_Text.PATH_SEP;
		else
			item.m_sBody = "";

		app.m_aItems.Insert(item);
		win.m_App = app;
		win.m_sPath = where;

		OpenWindow(0);
		FillWindow(win);

		// Straight into the name, because "new_file.txt" is not a name anybody
		// wants and the only reason to make one is to call it something.
		ShowEditor(win, "path", "Name", item.m_sHeading, item);
	}

	protected void OnCtxRename(SCR_ButtonTextComponent button)
	{
		CloseContextMenu();

		MCF_Desktop_Window win = WindowAt(0);
		if (!win || win.m_iOpenEntry < 0 || win.m_iOpenEntry >= win.m_aVisible.Count())
			return;

		MCF_Device_Item item = win.m_aVisible[win.m_iOpenEntry];
		ShowEditor(win, "path", "Name", item.m_sHeading, item);
	}

	protected void OnCtxDelete(SCR_ButtonTextComponent button)
	{
		CloseContextMenu();

		MCF_Desktop_Window win = WindowAt(0);
		if (!win || win.m_iOpenEntry < 0 || win.m_iOpenEntry >= win.m_aVisible.Count())
			return;

		MCF_Device_App app = DraftApp(win);
		if (!app)
			return;

		MCF_Device_Item doomed = win.m_aVisible[win.m_iOpenEntry];

		for (int i = 0; i < app.m_aItems.Count(); i++)
		{
			if (app.m_aItems[i].m_sHeading == doomed.m_sHeading)
			{
				app.m_aItems.Remove(i);
				break;
			}
		}

		win.m_App = app;
		FillWindow(win);
	}

	// ============================================================== the author

	//! The Game Master's row, above each list.
	//!
	//! ON THE DESKTOP THIS IS A TOOLBAR AND NOT ROWS IN THE LIST. The handset
	//! had no room for anything else; a window has a header with empty in it,
	//! and a verb that changes the mission should not have to be scrolled to.
	protected void BindToolbar(notnull Widget root, notnull MCF_Desktop_Window win)
	{
		string p = win.Prefix();

		SCR_ButtonTextComponent add = SCR_ButtonTextComponent.GetButtonText(p + "Add", root);
		if (add)
			add.m_OnClicked.Insert(OnAuthorAdd);

		SCR_ButtonTextComponent edit = SCR_ButtonTextComponent.GetButtonText(p + "Edit", root);
		if (edit)
			edit.m_OnClicked.Insert(OnAuthorEdit);

		SCR_ButtonTextComponent del = SCR_ButtonTextComponent.GetButtonText(p + "Del", root);
		if (del)
			del.m_OnClicked.Insert(OnAuthorDelete);

		SCR_ButtonTextComponent save = SCR_ButtonTextComponent.GetButtonText(p + "Save", root);
		if (save)
			save.m_OnClicked.Insert(OnAuthorSave);

		// The photo windows have tiles rather than rows, and their own handler.
		if (win.m_sPane == PANE_PHOTOS)
		{
			for (int i = 0; i < PHOTO_TILES; i++)
			{
				SCR_ButtonTextComponent tile = SCR_ButtonTextComponent.GetButtonText(p + "Tile" + i.ToString(), root);
				if (tile)
					tile.m_OnClicked.Insert(OnPhotoTileClicked);
			}
		}

		SCR_ButtonTextComponent call = SCR_ButtonTextComponent.GetButtonText(p + "Call", root);
		if (call)
			call.GetRootWidget().SetVisible(false);
	}

	protected void ShowToolbar(notnull Widget root, notnull MCF_Desktop_Window win)
	{
		string p = win.Prefix();
		bool on = m_bAuthor && win.m_eKind >= 0;

		ShowButton(root, p + "Add", on);
		ShowButton(root, p + "Edit", on);
		ShowButton(root, p + "Del", on);
		ShowButton(root, p + "Save", on);
	}

	protected void ShowButton(notnull Widget root, string name, bool visible)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, root);
		if (button)
			button.GetRootWidget().SetVisible(visible);
	}

	//! Which window a toolbar button belongs to.
	protected MCF_Desktop_Window WindowOfTool(SCR_ButtonTextComponent button, string suffix)
	{
		Widget root = GetRootWidget();
		if (!root)
			return null;

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (!win)
				continue;

			SCR_ButtonTextComponent candidate = SCR_ButtonTextComponent.GetButtonText(win.Prefix() + suffix, root);
			if (candidate == button)
				return win;
		}

		return null;
	}

	//! The draft this Game Master is editing.
	//!
	//! A COPY WITH ITS ID CLEARED. The server upserts any profile arriving with
	//! an id straight into the shared library, which is what made editing one
	//! handset change every handset in the mission carrying the same profile.
	//! The same trap is waiting here and the same answer defuses it.
	protected MCF_Device_Profile Draft()
	{
		if (m_Draft)
			return m_Draft;

		MCF_Device_Profile source = m_Content.GetProfile();
		if (!source)
			return null;

		m_Draft = MCF_Device_Script.Deserialize(MCF_Device_Script.Serialize(source));
		if (!m_Draft)
			return null;

		m_Draft.m_sId = "";
		m_Content.SetProfile(m_Draft);
		return m_Draft;
	}

	//! The app inside the draft that matches a window, so an edit lands on the
	//! copy being edited and not on the profile that is being displayed.
	protected MCF_Device_App DraftApp(notnull MCF_Desktop_Window win)
	{
		MCF_Device_Profile draft = Draft();
		if (!draft)
			return null;

		foreach (MCF_Device_App app : draft.m_aApps)
		{
			if (app.m_eKind == win.m_eKind)
				return app;
		}

		MCF_Device_App fresh = new MCF_Device_App();
		fresh.m_eKind = win.m_eKind;
		draft.m_aApps.Insert(fresh);
		return fresh;
	}

	protected void OnAuthorAdd(SCR_ButtonTextComponent button)
	{
		MCF_Desktop_Window win = WindowOfTool(button, "Add");
		if (!win)
			return;

		MCF_Device_App app = DraftApp(win);
		if (!app)
			return;

		MCF_Device_Item item = new MCF_Device_Item();
		item.m_sHeading = "New item";
		item.m_bNew = true;
		app.m_aItems.Insert(item);

		win.m_App = app;
		FillWindow(win);
		ShowEntry(win, app.m_aItems.Count() - 1);
		ShowEditor(win, "heading", "Title", item.m_sHeading, item);
	}

	protected void OnAuthorEdit(SCR_ButtonTextComponent button)
	{
		MCF_Desktop_Window win = WindowOfTool(button, "Edit");
		if (!win || win.m_iOpenEntry < 0 || win.m_iOpenEntry >= win.m_aVisible.Count())
			return;

		MCF_Device_Item item = win.m_aVisible[win.m_iOpenEntry];
		ShowEditor(win, "heading", "Title", item.m_sHeading, item);
	}

	protected void OnAuthorDelete(SCR_ButtonTextComponent button)
	{
		MCF_Desktop_Window win = WindowOfTool(button, "Del");
		if (!win || win.m_iOpenEntry < 0 || win.m_iOpenEntry >= win.m_aVisible.Count())
			return;

		MCF_Device_App app = DraftApp(win);
		if (!app)
			return;

		MCF_Device_Item doomed = win.m_aVisible[win.m_iOpenEntry];

		for (int i = 0; i < app.m_aItems.Count(); i++)
		{
			if (app.m_aItems[i].m_sHeading == doomed.m_sHeading && app.m_aItems[i].m_sBody == doomed.m_sBody)
			{
				app.m_aItems.Remove(i);
				break;
			}
		}

		win.m_App = app;
		FillWindow(win);
	}

	protected void OnAuthorSave(SCR_ButtonTextComponent button)
	{
		SendDraft();
	}

	//! Sends the draft to the server, which writes it onto the object and
	//! replicates it to everyone -- the same route the handset uses.
	protected void SendDraft()
	{
		if (!m_Draft)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_Editable)
			return;

		RplId targetId = Replication.FindItemId(m_Editable);
		if (targetId == RplId.Invalid())
		{
			MCF_Core_Log.Warn("that object is not replicated and cannot be edited");
			return;
		}

		controller.MCF_RequestWriteDeviceProfile(targetId, MCF_Device_Script.Serialize(m_Draft));
	}

	// ------------------------------------------------------------- the editor

	protected void BindEditor(notnull Widget root)
	{
		m_wEditorTitle = TextWidget.Cast(root.FindAnyWidget("EditorTitle"));
		m_EditorField = EditBoxWidget.Cast(root.FindAnyWidget("EditorField"));

		m_EditorSave = SCR_ButtonTextComponent.GetButtonText("EditorSave", root);
		if (m_EditorSave)
			m_EditorSave.m_OnClicked.Insert(OnEditorSave);

		m_EditorCancel = SCR_ButtonTextComponent.GetButtonText("EditorCancel", root);
		if (m_EditorCancel)
			m_EditorCancel.m_OnClicked.Insert(OnEditorCancel);

		if (m_wEditor)
			m_wEditor.SetVisible(false);
	}

	//! One field at a time, full screen. A window is wide but a field that
	//! shares a window with the list it is editing is a field somebody types
	//! into while looking at the wrong thing.
	protected void ShowEditor(notnull MCF_Desktop_Window win, string kind, string title, string value, MCF_Device_Item item)
	{
		m_sEditKind = kind;
		m_iEditSlot = win.m_iSlot;
		m_EditItem = item;

		if (m_wEditorTitle)
			m_wEditorTitle.SetText(title);

		if (m_EditorField)
			m_EditorField.SetText(value);

		if (m_wEditor)
			m_wEditor.SetVisible(true);
	}

	protected void OnEditorSave(SCR_ButtonTextComponent button)
	{
		if (!m_EditorField || !m_EditItem)
		{
			OnEditorCancel(null);
			return;
		}

		string value = m_EditorField.GetText();

		if (m_sEditKind == "path")
		{
			// A rename that keeps the folder: the Game Master typed a name, not
			// a path, unless they deliberately typed one.
			m_EditItem.m_sHeading = value;
			OnEditorCancel(null);

			MCF_Desktop_Window renamed = WindowAt(m_iEditSlot);
			if (renamed)
			{
				renamed.m_App = DraftApp(renamed);
				FillWindow(renamed);
			}

			return;
		}

		if (m_sEditKind == "heading")
			m_EditItem.m_sHeading = value;
		else if (m_sEditKind == "stamp")
			m_EditItem.m_sTimestamp = value;
		else if (m_sEditKind == "body")
			m_EditItem.m_sBody = value;

		MCF_Desktop_Window win = WindowAt(m_iEditSlot);

		// Three fields, in the order a person fills them in, each one handing
		// over to the next. A form with three boxes would need a window twice
		// this wide and would still be three fields.
		if (m_sEditKind == "heading" && win)
		{
			ShowEditor(win, "stamp", "Date or time", m_EditItem.m_sTimestamp, m_EditItem);
			return;
		}

		if (m_sEditKind == "stamp" && win)
		{
			ShowEditor(win, "body", "Text", m_EditItem.m_sBody, m_EditItem);
			return;
		}

		OnEditorCancel(null);

		if (win)
		{
			win.m_App = DraftApp(win);
			FillWindow(win);
		}
	}

	protected void OnEditorCancel(SCR_ButtonTextComponent button)
	{
		m_sEditKind = "";
		m_EditItem = null;
		m_iEditSlot = -1;

		if (m_wEditor)
			m_wEditor.SetVisible(false);
	}

	// ============================================================== the lock

	protected void BindLock(notnull Widget root)
	{
		m_wLockClock = TextWidget.Cast(root.FindAnyWidget("LockClock"));
		m_wLockDate = TextWidget.Cast(root.FindAnyWidget("LockDate"));
		m_wLockMsg = TextWidget.Cast(root.FindAnyWidget("LockMsg"));
		m_LockField = EditBoxWidget.Cast(root.FindAnyWidget("LockField"));

		m_LockGo = SCR_ButtonTextComponent.GetButtonText("LockGo", root);
		if (m_LockGo)
			m_LockGo.m_OnClicked.Insert(OnLockGo);
	}

	protected void BindDragCatch(notnull Widget root)
	{
		m_wDragCatch = root.FindAnyWidget("DragCatch");
		if (!m_wDragCatch)
			return;

		m_CatchHandler = new MCF_Desktop_Catch(this);
		m_wDragCatch.AddHandler(m_CatchHandler);
		m_wDragCatch.SetVisible(false);
	}

	protected void BindContextMenu(notnull Widget root)
	{
		m_wCtxMenu = root.FindAnyWidget("CtxMenu");
		if (m_wCtxMenu)
			m_wCtxMenu.SetVisible(false);

		// The wallpaper cannot take a click -- an ImageWidget never does -- so
		// a transparent button sits over it, behind every window and the panel.
		Widget desk = root.FindAnyWidget("DeskClick");
		if (desk)
		{
			m_DeskMenuHandler = new MCF_Desktop_Menu(this);
			desk.AddHandler(m_DeskMenuHandler);
		}

		SCR_ButtonTextComponent newFile = SCR_ButtonTextComponent.GetButtonText("CtxItem0", root);
		if (newFile)
			newFile.m_OnClicked.Insert(OnCtxNewFile);

		SCR_ButtonTextComponent newFolder = SCR_ButtonTextComponent.GetButtonText("CtxItem1", root);
		if (newFolder)
			newFolder.m_OnClicked.Insert(OnCtxNewFolder);

		SCR_ButtonTextComponent rename = SCR_ButtonTextComponent.GetButtonText("CtxItem2", root);
		if (rename)
			rename.m_OnClicked.Insert(OnCtxRename);

		SCR_ButtonTextComponent drop = SCR_ButtonTextComponent.GetButtonText("CtxItem3", root);
		if (drop)
			drop.m_OnClicked.Insert(OnCtxDelete);
	}

	protected bool IsDeviceLocked()
	{
		if (!m_Carrier)
			return false;

		MCF_Devices_LockComponent lock = MCF_Devices_LockComponent.FindOn(m_Carrier.GetOwner());
		if (!lock)
			return false;

		return lock.IsLocked();
	}

	protected void ShowLock()
	{
		m_bOnLock = true;

		foreach (MCF_Desktop_Window win : m_aWindows)
		{
			if (win && win.m_bOpen)
				CloseWindow(win.m_iSlot);
		}

		CloseLauncher();
		ShowDesktopChrome(false);

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(true);

		Widget root = GetRootWidget();
		if (root)
		{
			SetText(root, "LockUser", UserName());

			string host = m_Content.DeviceName();
			host.ToLower();
			host.Replace(" ", "-");
			SetText(root, "LockHost", host);
			SetText(root, "LockFaceText", MCF_Device_Text.Initial(UserName()));
		}

		if (m_LockField)
			m_LockField.SetText("");

		if (m_wLockMsg)
			m_wLockMsg.SetText("");

		DrawLockNotes();
		RefreshStatus();
	}

	//! What a locked device is prepared to say about itself: who wrote, and
	//! when. NEVER the body -- a locked laptop that shows the message has given
	//! away what breaking into it was for.
	protected void DrawLockNotes()
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		array<MCF_Device_Item> items = {};
		array<int> kinds = {};
		m_Content.UnreadItems(LOCK_NOTES, items, kinds);

		for (int i = 0; i < LOCK_NOTES; i++)
		{
			bool has = i < items.Count();
			string tag = i.ToString();

			ShowWidget(root, "Note" + tag + "Card", has);
			ShowWidget(root, "Note" + tag + "Icon", has);
			ShowWidget(root, "Note" + tag + "Title", has);
			ShowWidget(root, "Note" + tag + "Who", has);
			ShowWidget(root, "Note" + tag + "When", has);

			if (!has)
				continue;

			MCF_Device_Item item = items[i];

			SetText(root, "Note" + tag + "Title", MCF_Device_Names.LabelFor(kinds[i]));
			SetText(root, "Note" + tag + "Who", MCF_Device_Text.SenderOf(item.m_sHeading));
			SetText(root, "Note" + tag + "When", item.m_sTimestamp);
		}
	}

	//! The password field. 1337 is the way in, the same four digits the handset
	//! takes -- a player who has cracked one device in a mission should not
	//! have to learn a second convention for the next one.
	protected void OnLockGo(SCR_ButtonTextComponent button)
	{
		if (!m_LockField)
			return;

		string typed = MCF_Device_Script.Trim(m_LockField.GetText());

		if (typed != BREAK_IN_CODE)
		{
			if (m_wLockMsg)
				m_wLockMsg.SetText("Wrong password.");

			m_LockField.SetText("");
			return;
		}

		if (m_wLockMsg)
			m_wLockMsg.SetText("Accepted. Working...");

		RequestChallenge();
	}

	protected void RequestChallenge()
	{
		if (!m_Carrier)
			return;

		IEntity owner = m_Carrier.GetOwner();
		if (!owner)
			return;

		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
		{
			MCF_Core_Log.Warn("device has a lock but no RplComponent -- cannot ask the server for a challenge");
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		controller.MCF_RequestDeviceChallenge(rpl.Id());
	}

	//! Watches for the unlock arriving. The server decides, and the answer
	//! comes back as a change of state rather than as a reply to a question.
	protected void PollLock()
	{
		if (!m_bOnLock)
			return;

		if (IsDeviceLocked())
			return;

		m_bOnLock = false;

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		ShowDesktopChrome(true);
		BindApps();
		OpenDefaults();
	}

	// ============================================================ the break-in

	protected bool StartHack(RplId deviceId, int seed, int difficulty)
	{
		if (!m_wScreen)
			return false;

		CloseHack();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		m_wHackPanel = workspace.CreateWidgets(HACK_LAYOUT, m_wScreen);
		if (!m_wHackPanel)
		{
			MCF_Core_Log.Warn("could not create the break-in panel -- check the layout path");
			return false;
		}

		// Above the lock screen it replaces, and above every window.
		m_wHackPanel.SetZOrder(8200);
		FitHackPanel();

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		m_bHackDone = false;
		m_HackScreen = new MCF_Devices_HackScreen();
		m_HackScreen.m_OnFinished.Insert(OnHackFinished);
		m_HackScreen.Start(m_wHackPanel, deviceId, seed, difficulty);
		return true;
	}

	//! The puzzle is drawn on a slice of the desktop rather than all of it.
	//!
	//! It was built for a handset and it is portrait: stretched across a 16:10
	//! screen every one of its rows would be a metre wide and unreadable. A
	//! column down the middle is what it wants, and the desktop has room to
	//! spare on both sides.
	protected void FitHackPanel()
	{
		if (!m_wHackPanel || m_fScreenH <= 0)
			return;

		float h = m_fScreenH * 0.94;
		float w = h * 0.62;

		float maxW = m_fScreenW * 0.46;
		if (w > maxW)
		{
			w = maxW;
			h = w / 0.62;
		}

		FrameSlot.SetAnchor(m_wHackPanel, 0, 0);
		FrameSlot.SetSize(m_wHackPanel, w, h);
		FrameSlot.SetPos(m_wHackPanel, (m_fScreenW - w) * 0.5, (m_fScreenH - h) * 0.5);
	}

	protected void OnHackFinished()
	{
		// Torn down on the tick rather than here: the puzzle is still inside
		// its own click handler at this point, and deleting the widget it is
		// standing on is how that ends badly.
		m_bHackDone = true;
	}

	protected void FinishHack()
	{
		CloseHack();

		if (IsDeviceLocked())
		{
			ShowLock();
			return;
		}

		m_bOnLock = false;

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		ShowDesktopChrome(true);
		BindApps();
		OpenDefaults();
	}

	protected void CloseHack()
	{
		m_HackScreen = null;
		m_bHackDone = false;

		if (m_wHackPanel)
		{
			m_wHackPanel.RemoveFromHierarchy();
			m_wHackPanel = null;
		}
	}

	// =============================================================== plumbing

	protected void SetText(notnull Widget root, string name, string value)
	{
		Widget found = root.FindAnyWidget(name);
		if (!found)
			return;

		TextWidget text = TextWidget.Cast(found);
		if (text)
		{
			text.SetText(value);
			return;
		}

		RichTextWidget rich = RichTextWidget.Cast(found);
		if (rich)
			rich.SetText(value);
	}

	protected void ShowWidget(notnull Widget root, string name, bool visible)
	{
		Widget found = root.FindAnyWidget(name);
		if (found)
			found.SetVisible(visible);
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
}
