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
	protected static const string W_APP_LABEL_PREFIX = "AppLabel";
	protected static const string W_CLOCK_TIME = "ClockTime";
	protected static const string W_CLOCK_DATE = "ClockDate";
	protected static const string W_MODEL = "Model";
	protected static const string W_BODY = "Body";
	protected static const string W_SCREEN_AREA = "ScreenArea";

	//! The phone's own proportions, and how much of its face is glass. Two
	//! constants and a measurement are all it takes to put the controls on the
	//! display at any resolution -- see FitScreenToModel.
	protected static const float PHONE_ASPECT = 0.4698;
	//! At 0.92 x 0.94 the glass covered the phone almost edge to edge, and what
	//! was left of the body read as a glow around a panel rather than as a
	//! phone. A visible bezel is what makes it look held rather than overlaid.
	protected static const float GLASS_X = 0.86;
	protected static const float GLASS_Y = 0.90;

	//! How tall the preview box is, as a fraction of the screen.
	protected static const float PHONE_SCREEN_HEIGHT = 0.95;

	//! Half the model, in metres, from MCF_Devices_Phone.et's own dimensions:
	//! 7.0 x 14.9 x 0.9 cm, lying flat, long axis forward.
	protected static const float PHONE_HALF_X = 0.035;
	protected static const float PHONE_HALF_Z = 0.0745;

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

	//! The app name sits UNDER its tile, the way it does on a phone, so it is
	//! its own widget rather than text inside the button.
	protected ref array<TextWidget> m_aAppLabels = {};

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
	protected TextWidget m_wClockTime;
	protected TextWidget m_wClockDate;
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

	protected ItemPreviewWidget m_wModel;
	protected Widget m_wBody;
	protected int m_iFitFrame;

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
		m_wClockTime = TextWidget.Cast(root.FindAnyWidget(W_CLOCK_TIME));
		m_wClockDate = TextWidget.Cast(root.FindAnyWidget(W_CLOCK_DATE));

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
		m_aAppLabels.Clear();
		for (int i = 0; i < APP_SLOTS; i++)
		{
			SCR_ButtonTextComponent app = SCR_ButtonTextComponent.GetButtonText(W_APP_PREFIX + i.ToString(), root);
			if (!app)
				continue;

			app.m_OnClicked.Insert(OnAppClicked);
			m_aAppButtons.Insert(app);

			// Inserted even when null, so a label always shares its tile's
			// index and a missing widget cannot shift every name by one.
			m_aAppLabels.Insert(TextWidget.Cast(root.FindAnyWidget(W_APP_LABEL_PREFIX + i.ToString())));
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

		ShowModel(root);

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

	//! Puts the object's own model behind the screen.
	//!
	//! This is the vanilla inventory preview: an ItemPreviewWidget rendered by
	//! ItemPreviewManagerEntity, the same machinery that draws a rifle in the
	//! inventory. Which means the thing on screen is the actual phone, lit and
	//! shaded, and the controls sit on its glass rather than on a rectangle
	//! that resembles glass.
	//!
	//! EVERY STEP CAN FAIL AND EACH ONE FALLS BACK TO THE DRAWN PANEL. No
	//! prefab named, no world, no preview manager, no widget in this layout --
	//! all of them mean "draw the flat body instead", which is a screen that
	//! looks plainer than intended rather than a screen that is not there. The
	//! same rule as the lock: degrade where the mission maker can see it.
	protected void ShowModel(notnull Widget root)
	{
		m_wModel = ItemPreviewWidget.Cast(root.FindAnyWidget(W_MODEL));
		m_wBody = root.FindAnyWidget(W_BODY);

		if (m_wModel)
			m_wModel.SetVisible(false);

		if (m_wBody)
			m_wBody.SetVisible(true);

		if (!m_wModel || !m_Carrier)
			return;

		ResourceName prefab = m_Carrier.GetPreviewPrefab();
		if (prefab.IsEmpty())
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
		if (!manager)
		{
			MCF_Core_Log.Warn("no ItemPreviewManager in this world -- the shell falls back to the drawn panel");
			return;
		}

		manager.SetPreviewItemFromPrefab(m_wModel, prefab);

		m_wModel.SetVisible(true);

		// Not here: a widget has no screen size until the layout has been
		// through a frame, and asking during OnMenuOpen returns 0x0.
		m_iFitFrame = 0;

		if (m_wBody)
			m_wBody.SetVisible(false);
	}

	//! Sizes the preview box to the phone's own proportions, then puts the
	//! glass on it.
	//!
	//! WHY THE BOX IS RESHAPED AND NOT JUST MEASURED. The first attempt kept a
	//! box of whatever shape the layout gave it and multiplied its height by a
	//! measured "how much of it the model fills" constant. That constant is not
	//! constant: widening the box from 280 to 600 pixels, at the same height,
	//! made the rendered phone SMALLER -- 519 pixels tall became 440. Whatever
	//! the preview camera is fitting, it is not the box's height alone, so any
	//! number measured in one box shape is wrong in another.
	//!
	//! Giving the box the model's own 7.0 x 14.9 proportions removes the
	//! variable instead of trying to calibrate it: there is only one shape the
	//! box is ever in, so there is only one thing to tune -- the camera
	//! distance on the prefab, until the phone fills it.
	//!
	//! Units: GetScreenSize and GetWidth report physical pixels while FrameSlot
	//! works in the reference resolution, so DPIUnscale is what converts.
	protected void FitScreenToModel(notnull Widget root)
	{
		if (!m_wModel)
			return;

		Widget screenArea = root.FindAnyWidget(W_SCREEN_AREA);
		if (!screenArea)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float screenHeight = workspace.DPIUnscale(workspace.GetHeight());
		if (screenHeight <= 0)
			return;

		float boxH = screenHeight * PHONE_SCREEN_HEIGHT;
		float boxW = boxH * PHONE_ASPECT;

		PlaceCentred(m_wModel, boxW, boxH);
		PlaceCentred(screenArea, boxW * GLASS_X, boxH * GLASS_Y);
	}

	//! Asks the preview where the phone actually ended up, and puts the glass
	//! there.
	//!
	//! WHY THIS EXISTS AT ALL. Everything above still assumes the model fills
	//! the box it is given, and it does not -- at the camera distance the
	//! prefab asks for it fills about three quarters of it. That could be tuned
	//! by hand, but it would have to be retuned for every new model and after
	//! every change to the camera, and each round of that costs a restart.
	//!
	//! TryGetItemNodePositionInWidgetSpace answers the question directly: give
	//! it a point on the model and it returns where that point landed in the
	//! widget. Four corners of the model's own bounding box are enough to know
	//! exactly how big the phone is on screen, whatever the camera is doing.
	//!
	//! \return True if the preview answered and the glass was placed from it.
	protected bool FitScreenToPhone(notnull Widget root)
	{
		Widget screenArea = root.FindAnyWidget(W_SCREEN_AREA);
		if (!m_wModel || !screenArea)
			return false;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		float left, right, top, bottom;
		if (!NodePoint(-PHONE_HALF_X, 0, left, top))
			return false;

		float unusedY;
		if (!NodePoint(PHONE_HALF_X, 0, right, unusedY))
			return false;

		float unusedX;
		if (!NodePoint(0, PHONE_HALF_Z, unusedX, top))
			return false;

		if (!NodePoint(0, -PHONE_HALF_Z, unusedX, bottom))
			return false;

		float phoneW = right - left;
		float phoneH = bottom - top;

		// Signs depend on which way the camera looks at it; only the extent
		// matters here.
		if (phoneW < 0)
			phoneW = -phoneW;
		if (phoneH < 0)
			phoneH = -phoneH;

		float boxW, boxH;
		m_wModel.GetScreenSize(boxW, boxH);

		MCF_Core_Log.Debug("phone in widget space: " + phoneW.ToString() + "x" + phoneH.ToString()
			+ " against a box of " + boxW.ToString() + "x" + boxH.ToString());

		// A preview that has not drawn yet answers with a degenerate box.
		if (phoneW < 8 || phoneH < 8)
			return false;

		// THE ANSWER IS NOT ALWAYS IN THE SAME UNITS AS THE WIDGET, and that is
		// what this guard is for. Measured on the same build, the same phone
		// came back as 399.7 x 850.6 both in a 1101 x 615 Workbench viewport
		// and at 1920 x 1080 fullscreen -- an unchanging number, so it is in
		// the reference resolution, not in the widget's own. Fullscreen those
		// coincide and the placement is exact. In a viewport they do not, and
		// the phone is reported taller than the box that contains it, which
		// cannot be true.
		//
		// So: believe the answer only when it fits inside the box. Otherwise
		// keep the box-relative placement, which is a little generous but stays
		// on the screen. Fullscreen -- the case that ships -- gets the exact
		// one; the Workbench viewport gets the approximation.
		if (boxH > 0 && phoneH > boxH * 1.05)
		{
			MCF_Core_Log.Debug("preview answered in a different coordinate space than the widget -- keeping the box-relative fit");
			return false;
		}

		PlaceCentred(screenArea, phoneW * GLASS_X, phoneH * GLASS_Y);
		return true;
	}

	//! One point on the model, in the model's own space, as a position in the
	//! widget. The transform is the identity with the point in its last row --
	//! Enfusion passes transforms as four vectors and the fourth is the
	//! translation.
	protected bool NodePoint(float x, float z, out float outX, out float outY)
	{
		vector offset[4];
		Math3D.MatrixIdentity4(offset);
		offset[3] = Vector(x, 0, z);

		vector inWidget;
		if (!m_wModel.TryGetItemNodePositionInWidgetSpace(-1, offset, inWidget))
			return false;

		outX = inWidget[0];
		outY = inWidget[1];
		return true;
	}

	//! Pins a widget to the middle of the screen at an exact size.
	//!
	//! The anchors have to collapse to a point first. A slot whose anchors are
	//! stretched takes its size from them and ignores SetSize entirely, which
	//! fails silently and looks like the call did nothing.
	protected void PlaceCentred(notnull Widget widget, float width, float height)
	{
		FrameSlot.SetAnchor(widget, 0.5, 0.5);
		FrameSlot.SetSize(widget, width, height);
		FrameSlot.SetPos(widget, -width * 0.5, -height * 0.5);
	}

	//! Prints what the screen is actually made of, in real pixels.
	//!
	//! WHY THIS IS WORTH KEEPING. Lining the controls up with the rendered
	//! phone was done for three rounds by measuring screenshots, and it failed
	//! every time for the same reason: a screenshot arrives cropped and scaled
	//! by an unknown amount, so a size read off it is a size in unknown units.
	//! Two numbers from the game itself settle it -- and because ScreenArea's
	//! size is logged too, any later screenshot can be scaled correctly by
	//! comparing the panel in the picture against the panel in this line.
	protected void LogGeometry(notnull Widget root)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float modelW, modelH, screenW, screenH, modelX, modelY;
		m_wModel.GetScreenSize(modelW, modelH);
		m_wModel.GetScreenPos(modelX, modelY);

		Widget screenArea = root.FindAnyWidget("ScreenArea");
		if (screenArea)
			screenArea.GetScreenSize(screenW, screenH);

		MCF_Core_Log.Debug("shell geometry: workspace " + workspace.GetWidth().ToString() + "x" + workspace.GetHeight().ToString()
			+ " | Model " + modelW.ToString() + "x" + modelH.ToString() + " at " + modelX.ToString() + "," + modelY.ToString()
			+ " | ScreenArea " + screenW.ToString() + "x" + screenH.ToString());
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

			// The tile stays blank. The name goes underneath it, which is what
			// separates a home screen from a list of grey buttons.
			button.SetText("");

			if (i >= m_aAppLabels.Count() || !m_aAppLabels[i])
				continue;

			m_aAppLabels[i].SetVisible(used);

			if (used)
				m_aAppLabels[i].SetText(AppLabel(m_aApps[i]));
		}

		ShowClock(true);

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

		foreach (TextWidget label : m_aAppLabels)
		{
			if (label)
				label.SetVisible(home);
		}

		// A phone replaces its home screen rather than scrolling past it, so
		// the clock goes away with the tiles.
		ShowClock(home);

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

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_iFitFrame > 2)
			return;

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_iFitFrame++;

		// Frame one places the widgets. The measurement waits, because
		// GetScreenSize reports what the last layout pass produced -- asking in
		// the same frame reports the size from before the change, which read
		// as "the call did nothing" for two rounds.
		if (m_iFitFrame == 1)
			FitScreenToModel(root);
		else if (m_iFitFrame == 3)
		{
			// The preview has drawn by now, so it can be asked where the phone
			// really is. If it cannot answer, the box-relative guess above
			// stays, which is wrong by a known amount rather than broken.
			FitScreenToPhone(root);
			LogGeometry(root);
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

	//! The clock, from the mission's own time rather than the player's.
	protected void ShowClock(bool visible)
	{
		if (m_wClockTime)
		{
			m_wClockTime.SetVisible(visible);
			m_wClockTime.SetText(ClockText());
		}

		if (m_wClockDate)
		{
			m_wClockDate.SetVisible(visible);
			m_wClockDate.SetText(DateText());
		}

		// The device name and the clock want the same line, and only one of
		// them is worth reading at a time: the name once you are inside an app,
		// the clock on the home screen.
		if (m_wDeviceName)
			m_wDeviceName.SetVisible(!visible);
	}

	protected string ClockText()
	{
		TimeAndWeatherManagerEntity time = WorldTime();
		if (!time)
			return "";

		TimeContainer now = time.GetTime();
		if (!now)
			return "";

		return Pad(now.m_iHours) + ":" + Pad(now.m_iMinutes);
	}

	protected string DateText()
	{
		TimeAndWeatherManagerEntity time = WorldTime();
		if (!time)
			return "";

		int year, month, day;
		time.GetDate(year, month, day);

		return time.GetWeekDayString() + "  " + day.ToString() + "/" + Pad(month);
	}

	protected TimeAndWeatherManagerEntity WorldTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return null;

		return world.GetTimeAndWeatherManager();
	}

	protected string Pad(int value)
	{
		if (value < 10)
			return "0" + value.ToString();

		return value.ToString();
	}

	protected string StatusLine()
	{
		if (m_eView == MCF_EIntelView.LAPTOP)
			return "MCF          AC POWER";

		return "MCF          LTE          100%";
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
