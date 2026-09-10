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
	//! One line in a device list. NOT the planning board's row: that one carries
	//! Clipping False and 18pt text because the board is as wide as the screen,
	//! and on a phone it drew straight out past the side of the handset.
	protected static const ResourceName ROW_LAYOUT = "{6A1C4F0B39D34500}UI/layouts/MCF/MCF_IntelRow.layout";

	//! The break-in panel, drawn inside the device's own glass. One layout
	//! serves every skin, and it is created on demand rather than shipped
	//! inside each device layout -- see StartHack below.
	protected static const ResourceName HACK_LAYOUT = "{6A1C4F0B39D30000}UI/layouts/MCF/MCF_DeviceHack.layout";

	protected static const string W_DEVICE_NAME = "DeviceName";
	protected static const string W_STATUS_BAR = "StatusBar";
	protected static const string W_STATUS_RIGHT = "StatusRight";
	protected static const string W_TILE = "Tile";
	protected static const string W_ICON = "Icon";

	//! The base game's own icon atlas. White masks, tinted by the widget.
	protected static const ResourceName ICON_SET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	protected static const string W_APP_PREFIX = "App";
	protected static const string W_DOCK_PREFIX = "Dock";
	protected static const string W_DOCK_PANEL = "DockPanel";
	protected static const string W_CHASSIS = "Chassis";
	protected static const string W_HOME_BAR = "HomeTap";
	protected static const string W_KEYPAD = "Keypad";
	protected static const string W_KEYPAD_TITLE = "KeypadTitle";
	protected static const string W_PIP_PREFIX = "Pip";
	protected static const string W_KEY_DELETE = "KeyDel";

	//! What opens the break-in. Four digits, and deliberately a joke a player
	//! can be told by another player -- the code is not the puzzle, the puzzle
	//! is the puzzle.
	protected static const string BREAK_IN_CODE = "1337";
	protected static const int CODE_LENGTH = 4;
	protected static const string W_LIST_SCROLL = "ListScroll";
	protected static const string W_ENTRY_LIST = "EntryList";
	protected static const string W_READ_SCROLL = "ReadScroll";
	protected static const string W_READ_COLUMN = "ReadColumn";
	protected static const string W_READ_BODY = "ReadBody";
	protected static const string W_READ_IMAGE = "ReadImage";
	protected static const string W_READ_IMAGE_NOTE = "ReadImageNote";
	protected static const string W_READ_IMAGE_BUTTON = "ReadImageButton";
	protected static const string W_PHOTO_OVERLAY = "PhotoOverlay";
	protected static const string W_PHOTO_BACKDROP = "PhotoBackdrop";
	protected static const string W_PHOTO_FULL = "PhotoFull";
	protected static const string W_PHOTO_FULL_BUTTON = "PhotoFullButton";
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
	protected static const string W_LOCK_SCREEN = "LockScreen";
	protected static const string W_LOCK_TITLE = "LockTitle";
	protected static const string W_LOCK_NOTE = "LockNote";
	protected static const string W_BUTTON_UNLOCK = "ButtonUnlock";

	//! Tiles on the home screen. The layout has this many App/AppLabel pairs;
	//! the presenter decides which of them are used.
	//! Tiles on the home screen: four across, three down. The layout has this
	//! many App/AppLabel pairs; the presenter decides which of them are used.
	protected static const int APP_SLOTS = 12;

	//! Tiles in the dock. They repeat the first four apps, which is what a
	//! phone does -- the dock is the shortcut row, not a separate drawer -- and
	//! they carry no label, which is what makes it read as a dock.
	protected static const int DOCK_SLOTS = 4;


	//! A laptop lid fills less of the screen's height than a phone does, and
	//! nearly all of its face is glass -- there is no bezel worth drawing on a
	//! screen that is already only a screen.
	//! A laptop does not get its glass from a fraction of its silhouette -- see
	//! MCF_Device_Layout.SetScreenQuad. These are a shade under 1 only so the
	//! bezel shows as a hairline rather than the UI running to the very edge of
	//! the panel.
	//! The laptop is deliberately drawn LARGER than the box it is given, so it
	//! overflows and covers the preview world's sky. It also puts the screen --
	//! the only part anyone is reading -- at a size worth reading. The glass
	//! follows the projected screen quad, which is computed rather than
	//! clipped, so it tracks the model out past the box's edge.
	protected static const float LAPTOP_HEIGHT = 1.0;

	//! How wide the laptop's box is, in box-heights. NOT the model's own
	//! proportions: the box is cropped to the laptop itself so that the preview
	//! world's sky has nowhere left to show.
	//!
	//! Derived from two logged fits rather than guessed. The preview's apparent
	//! size against CameraDistanceToItem fits 0.2988 / (d + 0.10) almost
	//! exactly -- 0.6495 box-heights at d = 0.36, 0.996 at d = 0.20 -- so at
	//! d = 0.23 the LCD comes out 0.904 box-heights wide, and the lid is 1.046
	//! times the LCD across. 0.904 x 1.046 = 0.946.
	protected static const float LAPTOP_BOX_ASPECT = 0.95;

	//! ZERO, AND HERE IS WHY IT IS NOT THE FIX IT LOOKED LIKE. Shifting the
	//! preview widget down moves the clip rectangle and the picture inside it
	//! together, so whatever was cut off at the top stays cut off -- it just
	//! sits lower on the screen. The only thing that decides whether the lid
	//! fits is how big the item is drawn inside its widget, and that is
	//! CameraDistanceToItem's job. The box is the whole screen now, so there is
	//! nothing to shift into.
	protected static const float LAPTOP_MODEL_DOWN = 0;
	protected static const float LAPTOP_GLASS_X = 0.99;
	protected static const float LAPTOP_GLASS_Y = 0.99;

	//! The LCD's four corners in the model's own space, engine axes, read off
	//! the LaptopOpen_Screen material -- one quad, eight vertices, so these are
	//! the corners themselves and not an estimate of them. The panel is 0.345
	//! across and 0.195 down its own face, which is 16:9, and it leans back 28
	//! degrees from vertical.
	protected static ref array<vector> LaptopScreenQuad()
	{
		return {
			Vector(-0.1725, 0.0485, 0.1330),
			Vector( 0.1725, 0.0485, 0.1330),
			Vector(-0.1725, 0.2207, 0.2246),
			Vector( 0.1725, 0.2207, 0.2246)
		};
	}

	protected static MCF_Intel_CarrierComponent s_PendingCarrier;
	protected static MCF_EIntelView s_PendingView;

	protected MCF_Intel_CarrierComponent m_Carrier;
	protected MCF_EIntelView m_eView;

	//! Which screen is up. There are three and they are exclusive, so an int
	//! would do -- these read better at the call sites.
	protected bool m_bOnHome = true;
	protected bool m_bOnList;

	//! What is on the device and in what order. Knows nothing about widgets --
	//! see MCF_Device_Presenter.
	protected ref MCF_Device_Presenter m_Content = new MCF_Device_Presenter();

	//! The apps that actually have something in them, in enum order. An empty
	//! app is not drawn at all: a phone with a Photos icon that opens on
	//! nothing is worse than a phone with no Photos icon.
	protected ref array<MCF_Device_App> m_aApps = {};
	protected ref array<SCR_ButtonTextComponent> m_aAppButtons = {};
	protected ref array<SCR_ButtonTextComponent> m_aDockButtons = {};
	protected Widget m_wDockPanel;
	protected Widget m_wChassis;
	protected Widget m_wHomeBar;
	protected Widget m_wKeypad;
	protected TextWidget m_wKeypadTitle;
	protected ref array<Widget> m_aPips = {};
	protected ref array<SCR_ButtonTextComponent> m_aKeys = {};
	protected string m_sCode;
	protected bool m_bOnKeypad;
	protected ref MCF_Device_ImageClick m_HomeBarClick;
	//! Seconds since the home bar was last tapped. Two taps inside
	//! HOME_DOUBLE_TAP put the phone away.
	protected float m_fSinceHomeTap = 99;
	protected static const float HOME_DOUBLE_TAP = 0.45;
	protected Widget m_wScreenArea;

	//! The app name sits UNDER its tile, the way it does on a phone, so it is
	//! its own widget rather than text inside the button.
	protected ref array<TextWidget> m_aAppLabels = {};

	protected MCF_Device_App m_OpenApp;
	protected int m_iOpenEntry = -1;

	//! The picture the open item is still waiting for, empty when it is not
	//! waiting for one. Everything about the spinner hangs off this being set.
	protected string m_sPictureKey;
	protected float m_fPictureTick;
	protected float m_fPictureWaited;
	protected int m_iPictureDots;

	//! How fast the dots move, and how long the shell keeps hoping.
	//!
	//! THE DEADLINE IS NOT DECORATION. A fetch that never calls back at all --
	//! no success, no error -- would otherwise leave the spinner turning for the
	//! rest of the session, which is the worst of the three outcomes because it
	//! is the one that looks like it is still working.
	protected static const float PICTURE_DOT_SECONDS = 0.35;
	protected static const float PICTURE_GIVE_UP_SECONDS = 30.0;
	protected ref array<ref MCF_Device_Item> m_aVisible = {};
	protected ref array<SCR_ButtonTextComponent> m_aRowButtons = {};

	protected Widget m_wListScroll;
	protected Widget m_wReadScroll;
	protected VerticalLayoutWidget m_wEntryList;
	protected VerticalLayoutWidget m_wReadColumn;
	protected RichTextWidget m_wReadBody;
	protected ImageWidget m_wReadImage;
	protected Widget m_wReadImageButton;
	protected RichTextWidget m_wReadImageNote;
	protected Widget m_wPhotoOverlay;
	protected ImageWidget m_wPhotoFull;
	protected Widget m_wPhotoFullButton;

	// The lock screen. Null on any device whose layout has no lock widgets --
	// a sheet of paper, say -- and every use of these is guarded, so a layout
	// that predates this feature keeps working and simply never locks.
	protected Widget m_wLockScreen;
	protected TextWidget m_wLockTitle;
	protected RichTextWidget m_wLockNote;
	protected SCR_ButtonTextComponent m_UnlockButton;
	protected bool m_bOnLock;

	//! The break-in panel while it is up, and the game running inside it.
	protected Widget m_wHackPanel;
	protected ref MCF_Devices_HackScreen m_HackScreen;

	//! Set when the puzzle announces it is over. The teardown happens on the
	//! next tick rather than inside the announcement, because that announcement
	//! reaches here from inside a click handler on a button that lives in the
	//! panel about to be destroyed.
	protected bool m_bHackDone;

	//! The shell that has a device open on this client. A challenge coming back
	//! from the server is drawn on the device it belongs to rather than in a
	//! window of its own, and this is how the reply finds it. One at a time:
	//! menus stack, but a player only has one device in their hands.
	protected static MCF_Intel_ShellMenu s_Open;

	//! Held because a handler that is not referenced is collected, and a
	//! collected handler stops handling -- quietly, exactly like the callback
	//! that cost this feature an afternoon.
	protected ref MCF_Device_ImageClick m_PhotoClick;
	protected ref MCF_Device_ImageClick m_BackdropClick;
	protected ref MCF_Device_ImageClick m_PhotoCloseClick;

	//! The picture currently drawn, so its shape can be looked up. Empty when
	//! what is drawn came from the mod rather than from a url.
	protected string m_sPictureShown;
	protected TextWidget m_wDeviceName;
	protected TextWidget m_wStatusBar;
	protected TextWidget m_wStatusRight;
	//! Seconds since the status bar was last re-read.
	protected float m_fStatusTick;
	protected RichTextWidget m_wReadHeading;
	protected RichTextWidget m_wReadTimestamp;
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

	//! Everything about where the device sits on screen and where its glass is.
	//! Owns no intel and knows no apps -- see MCF_Device_Layout.
	protected ref MCF_Device_Layout m_Geometry;
	protected int m_iFitFrame;

	//! Set once the preview has told us where the device really is. Until then
	//! the glass is placed against the preview BOX, which is bigger than the
	//! model inside it.
	protected bool m_bFitted;

	//! How long to keep asking. A second at 60fps: long enough for a preview
	//! that is merely slow, short enough that a preview which will never answer
	//! says so in the log while the player is still looking at the screen.
	protected static const int FIT_GIVE_UP_FRAME = 60;

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
			case MCF_EIntelView.LAPTOP:  preset = ChimeraMenuPreset.MCF_IntelLaptop; break;
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
		s_Open = this;

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("device shell opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wDeviceName = TextWidget.Cast(root.FindAnyWidget(W_DEVICE_NAME));
		m_wStatusBar = TextWidget.Cast(root.FindAnyWidget(W_STATUS_BAR));
		m_wStatusRight = TextWidget.Cast(root.FindAnyWidget(W_STATUS_RIGHT));
		m_wDockPanel = root.FindAnyWidget(W_DOCK_PANEL);

		// The passcode pad. Ten digits, a delete key and four pips -- bound
		// while the pad is visible in the layout, because GetButtonText does
		// not walk into a subtree the layout marks hidden.
		m_wKeypad = root.FindAnyWidget(W_KEYPAD);
		m_wKeypadTitle = TextWidget.Cast(root.FindAnyWidget(W_KEYPAD_TITLE));

		m_aKeys.Clear();
		for (int k = 0; k < 10; k++)
		{
			SCR_ButtonTextComponent digit = SCR_ButtonTextComponent.GetButtonText("Key" + k.ToString(), root);
			if (!digit)
				continue;

			digit.m_OnClicked.Insert(OnKeyClicked);
			m_aKeys.Insert(digit);
		}

		SCR_ButtonTextComponent del = SCR_ButtonTextComponent.GetButtonText(W_KEY_DELETE, root);
		if (del)
		{
			del.m_OnClicked.Insert(OnDeleteClicked);
			m_aKeys.Insert(del);
		}

		m_aPips.Clear();
		for (int pip = 0; pip < CODE_LENGTH; pip++)
		{
			m_aPips.Insert(root.FindAnyWidget(W_PIP_PREFIX + pip.ToString()));
		}

		if (m_wKeypad)
			m_wKeypad.SetVisible(false);

		// THE HOME BAR IS THE ONLY CONTROL THE PHONE NEEDS. One tap steps back
		// -- entry to list, list to home, and nothing at all once you are home.
		// Two taps in quick succession put the phone away. The three grey
		// buttons that used to sit under the screen are gone: they were the
		// last thing on it that said "game menu" out loud.
		m_wHomeBar = root.FindAnyWidget(W_HOME_BAR);
		if (m_wHomeBar)
		{
			m_HomeBarClick = new MCF_Device_ImageClick();
			m_HomeBarClick.m_OnClicked.Insert(OnHomeBarTapped);
			m_wHomeBar.AddHandler(m_HomeBarClick);
		}
		m_wListScroll = root.FindAnyWidget(W_LIST_SCROLL);
		m_wReadScroll = root.FindAnyWidget(W_READ_SCROLL);
		m_wEntryList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_ENTRY_LIST));
		m_wReadColumn = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_READ_COLUMN));
		m_wReadBody = RichTextWidget.Cast(root.FindAnyWidget(W_READ_BODY));
		m_wReadImage = ImageWidget.Cast(root.FindAnyWidget(W_READ_IMAGE));
		m_wReadImageNote = RichTextWidget.Cast(root.FindAnyWidget(W_READ_IMAGE_NOTE));
		m_wPhotoOverlay = root.FindAnyWidget(W_PHOTO_OVERLAY);
		m_wPhotoFull = ImageWidget.Cast(root.FindAnyWidget(W_PHOTO_FULL));
		m_wPhotoFullButton = root.FindAnyWidget(W_PHOTO_FULL_BUTTON);

		m_wLockScreen = root.FindAnyWidget(W_LOCK_SCREEN);
		m_wLockTitle = TextWidget.Cast(root.FindAnyWidget(W_LOCK_TITLE));
		m_wLockNote = RichTextWidget.Cast(root.FindAnyWidget(W_LOCK_NOTE));
		// BOUND WHILE VISIBLE, THEN HIDDEN. SCR_ButtonTextComponent.GetButtonText
		// does not find a button inside a subtree marked hidden in the layout,
		// which is why the lock screen came up blank with only
		// "widget not found: SCR_ButtonTextComponent ButtonUnlock" in the log.
		// So the layout ships the lock screen visible and script hides it here,
		// after everything inside it has been found.
		m_UnlockButton = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_UNLOCK, root);
		if (m_UnlockButton)
			m_UnlockButton.m_OnClicked.Insert(OnUnlockClicked);

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		// THE BUTTON TAKES THE CLICK, NOT THE IMAGE. An ImageWidget does not
		// accept cursor input, so a handler on it is attached, never called, and
		// looks exactly like a handler that is broken. The button wraps the image
		// and both are given the same size every time a picture is drawn, so the
		// clickable area cannot drift away from what is on screen.
		m_wReadImageButton = root.FindAnyWidget(W_READ_IMAGE_BUTTON);
		if (m_wReadImageButton)
		{
			m_PhotoClick = new MCF_Device_ImageClick();
			m_PhotoClick.m_OnClicked.Insert(OnPhotoClicked);
			m_wReadImageButton.AddHandler(m_PhotoClick);
		}

		// BOTH SURFACES, because "click anywhere" has to mean anywhere. The
		// enlarged picture covers most of the screen, so a backdrop that closes
		// and a picture that does not would leave the obvious click doing
		// nothing.
		Widget backdrop = root.FindAnyWidget(W_PHOTO_BACKDROP);
		if (backdrop)
		{
			m_BackdropClick = new MCF_Device_ImageClick();
			m_BackdropClick.m_OnClicked.Insert(ClosePhoto);
			backdrop.AddHandler(m_BackdropClick);
		}

		if (m_wPhotoFullButton)
		{
			m_PhotoCloseClick = new MCF_Device_ImageClick();
			m_PhotoCloseClick.m_OnClicked.Insert(ClosePhoto);
			m_wPhotoFullButton.AddHandler(m_PhotoCloseClick);
		}

		ClosePhoto();
		m_wReadHeading = RichTextWidget.Cast(root.FindAnyWidget(W_READ_HEADING));
		m_wReadTimestamp = RichTextWidget.Cast(root.FindAnyWidget(W_READ_TIMESTAMP));
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

		m_aDockButtons.Clear();
		for (int d = 0; d < DOCK_SLOTS; d++)
		{
			SCR_ButtonTextComponent dock = SCR_ButtonTextComponent.GetButtonText(W_DOCK_PREFIX + d.ToString(), root);
			if (!dock)
				continue;

			dock.m_OnClicked.Insert(OnDockClicked);
			m_aDockButtons.Insert(dock);
		}

		if (!m_Carrier)
		{
			MCF_Core_Log.Warn("device shell opened with nothing to read");
			return;
		}

		ShowModel(root);

		m_Content.Bind(m_Carrier);
		PrefetchPictures();

		if (m_wDeviceName)
			m_wDeviceName.SetText(m_Content.DeviceName());

		RefreshStatus();

		m_bPageMode = m_aAppButtons.IsEmpty();

		if (m_bPageMode)
		{
			// Everything is one stack of pages. No apps, no index.
			m_Content.GetAllItems(m_aVisible);

			if (m_aVisible.IsEmpty())
				SetHint("Nothing legible.");
			else
				ShowEntry(0);
		}
		else
		{
			m_Content.GetApps(m_aApps);

			// A shut device shows its lock screen instead of its home screen.
			// Checked here rather than in the read action so that there is one
			// prompt in the world and the refusal happens where the player can
			// see what it is and do something about it.
			if (IsDeviceLocked())
				ShowLock();
			else
				ShowHome();
		}

		MCF_Core_Log.Debug("device shell showing '" + m_Content.DeviceName() + "' with " + m_Content.TotalItems().ToString() + " item(s) across " + m_aApps.Count().ToString() + " app(s)");
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
	//! How tall the drawn phone is, as a share of the screen, and how wide it
	//! is for that height. The art is 592 x 1220, so the ratio is its own.
	protected static const float DRAWN_PHONE_HEIGHT = 0.94;
	protected static const float DRAWN_PHONE_ASPECT = 0.4852;

	//! The bezel, as a share of the art. Everything inside it is the glass.
	protected static const float DRAWN_BEZEL_X = 0.0372;
	protected static const float DRAWN_BEZEL_Y = 0.0180;

	//! Places the drawn handset and its glass.
	//!
	//! WHY THE PHONE IS A PICTURE NOW AND NOT THE MODEL. Rendering the real
	//! model behind the UI meant measuring where it landed, in a preview whose
	//! answer arrives in a different unit than the widget it lands in, a frame
	//! or three later, sometimes never. Every fault this screen has had came
	//! from that measurement -- a glass too big for the phone, a puzzle sized
	//! before the glass existed, a handset that vanished at one window size.
	//!
	//! A drawn chassis has none of it: two rectangles from one aspect ratio,
	//! recomputed every frame, correct on the first one. The laptop still uses
	//! the model -- its screen is a quad on a mesh and there is a real reason
	//! to project it.
	protected void FitDrawnPhone()
	{
		if (m_eView != MCF_EIntelView.PHONE || !m_wChassis || !m_wScreenArea)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float screenW = workspace.GetWidth();
		float screenH = workspace.GetHeight();
		if (screenW <= 0 || screenH <= 0)
			return;

		float h = screenH * DRAWN_PHONE_HEIGHT;
		float w = h * DRAWN_PHONE_ASPECT;

		// Never wider than a third of the screen: on an ultrawide the phone
		// would otherwise grow into a monolith.
		float maxW = screenW * 0.33;
		if (w > maxW)
		{
			w = maxW;
			h = w / DRAWN_PHONE_ASPECT;
		}

		FrameSlot.SetAnchor(m_wChassis, 0.5, 0.5);
		FrameSlot.SetSize(m_wChassis, w, h);
		FrameSlot.SetPos(m_wChassis, -w * 0.5, -h * 0.5);

		float glassW = w * (1 - DRAWN_BEZEL_X * 2);
		float glassH = h * (1 - DRAWN_BEZEL_Y * 2);

		FrameSlot.SetAnchor(m_wScreenArea, 0.5, 0.5);
		FrameSlot.SetSize(m_wScreenArea, glassW, glassH);
		FrameSlot.SetPos(m_wScreenArea, -glassW * 0.5, -glassH * 0.5);

		// The home bar rides on top of the chassis, not under it. A widget
		// underneath the drawn body would be covered by it, and an image
		// cannot take a click at all -- only a button can, which is why this
		// is one. Its hit area is deliberately taller than the pill it draws:
		// a 4-pixel line is not something anyone can hit.
		if (m_wHomeBar)
		{
			float barW = glassW * 0.42;
			float barH = h * 0.040;

			FrameSlot.SetAnchor(m_wHomeBar, 0.5, 0.5);
			FrameSlot.SetSize(m_wHomeBar, barW, barH);
			FrameSlot.SetPos(m_wHomeBar, -barW * 0.5, h * 0.5 - barH - h * 0.008);
		}
	}

	protected void ShowModel(notnull Widget root)
	{
		m_wChassis = root.FindAnyWidget(W_CHASSIS);
		m_wScreenArea = root.FindAnyWidget(MCF_Device_Layout.W_SCREEN_AREA);

		// The phone is drawn, not previewed. Hide the model widget and the
		// flat fallback body with it, and let FitDrawnPhone do the placing.
		if (m_eView == MCF_EIntelView.PHONE)
		{
			Widget model = root.FindAnyWidget(MCF_Device_Layout.W_MODEL);
			if (model)
				model.SetVisible(false);

			Widget body = root.FindAnyWidget(MCF_Device_Layout.W_BODY);
			if (body)
				body.SetVisible(false);

			if (m_wChassis)
				m_wChassis.SetVisible(true);

			FitDrawnPhone();
			m_bFitted = true;
			return;
		}

		if (m_wChassis)
			m_wChassis.SetVisible(false);

		m_Geometry = new MCF_Device_Layout();

		// The shell tells the geometry what shape of thing it is drawing. A
		// laptop is landscape and sits lower on the screen than a phone does;
		// everything else about the placement is identical, which is the whole
		// point of these being two numbers rather than two classes.
		if (m_eView == MCF_EIntelView.LAPTOP)
		{
			m_Geometry.Configure(LAPTOP_HEIGHT, LAPTOP_GLASS_X, LAPTOP_GLASS_Y, 0, LAPTOP_MODEL_DOWN);
			m_Geometry.SetBoxAspect(LAPTOP_BOX_ASPECT);
			m_Geometry.SetScreenQuad(LaptopScreenQuad());
		}

		if (!m_Geometry.Attach(root, PreviewSize()))
			return;

		m_Geometry.ShowFallbackBody(true);

		if (!m_Carrier)
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

		manager.SetPreviewItemFromPrefab(m_Geometry.GetModelWidget(), prefab);
		m_Geometry.ShowFallbackBody(false);
		m_Geometry.ClearPreviewBackground();

		// WHAT THE PREVIEW WORLD ACTUALLY BUILT. The widget draws black when it
		// has nothing to draw, which looks exactly like a device whose screen
		// is simply too big -- and telling those two apart by eye cost several
		// rounds. The preview manager keeps its own entity per prefab, so ask
		// it: no entity means the prefab never built, and empty bounds mean it
		// built without geometry the camera can frame.
		IEntity previewed = manager.ResolvePreviewEntityForPrefab(prefab);
		if (!previewed)
		{
			MCF_Core_Log.Warn("the preview manager built no entity for " + prefab + " -- nothing will be drawn behind the screen");
		}
		else
		{
			vector mins, maxs;
			previewed.GetBounds(mins, maxs);
			MCF_Core_Log.Debug("preview entity bounds " + mins.ToString() + " .. " + maxs.ToString() + " for " + prefab);
		}

		// Not here: a widget has no screen size until the layout has been
		// through a frame, and asking during OnMenuOpen returns 0x0.
		m_iFitFrame = 0;
	}

	//! How big the device really is, in metres. The object says so rather than
	//! the shell assuming it -- which is the whole reason a laptop can use this
	//! screen's machinery without a line of it changing.
	protected vector PreviewSize()
	{
		if (m_Carrier)
			return m_Carrier.GetPreviewSize();

		return vector.Zero;
	}

	//! Prints what the screen is actually made of, in real pixels.
	//!
	//! WHY THIS IS WORTH KEEPING. Lining the controls up with the rendered
	//! device was done for three rounds by measuring screenshots, and it failed
	//! every time for the same reason: a screenshot arrives cropped and scaled
	//! by an unknown amount, so a size read off it is a size in unknown units.
	//! Two numbers from the game itself settle it -- and because the glass's
	//! size is logged too, any later screenshot can be scaled correctly by
	//! comparing the panel in the picture against the panel in this line.
	protected void LogGeometry(notnull Widget root)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float modelW, modelH, glassW, glassH;

		Widget model = root.FindAnyWidget(MCF_Device_Layout.W_MODEL);
		if (model)
			model.GetScreenSize(modelW, modelH);

		Widget glass = root.FindAnyWidget(MCF_Device_Layout.W_SCREEN_AREA);
		if (glass)
			glass.GetScreenSize(glassW, glassH);

		MCF_Core_Log.Debug("shell geometry: workspace " + workspace.GetWidth().ToString() + "x" + workspace.GetHeight().ToString()
			+ " | Model " + modelW.ToString() + "x" + modelH.ToString()
			+ " | ScreenArea " + glassW.ToString() + "x" + glassH.ToString());
	}

	// ----------------------------------------------------------- the apps

	protected void ShowHome()
	{
		m_bOnHome = true;
		m_bOnList = false;
		m_OpenApp = null;
		m_iOpenEntry = -1;
		StopWaitingForPicture();

		SetScreens(true, false, false);

		foreach (int i, SCR_ButtonTextComponent button : m_aAppButtons)
		{
			bool used = i < m_aApps.Count();
			button.GetRootWidget().SetVisible(used);

			// A short mark on a coloured tile, with the app's name underneath.
			// The colour belongs to the app rather than to the device, so
			// MESSAGES is the same green on every phone in the mission and a
			// player learns the grid once.
			if (used)
			{
				int kind = m_aApps[i].m_eKind;
				PaintTile(button, MCF_Device_Names.ColorFor(kind));

				// The icon if the atlas has one we are sure of, the letter
				// mark if it does not. Never both: a tile with a picture AND
				// three letters on it reads as a placeholder.
				if (SetTileIcon(button, MCF_Device_Names.IconFor(kind)))
					button.SetText("");
				else
					button.SetText(MCF_Device_Names.GlyphFor(kind));
			}
			else
			{
				button.SetText("");
				SetTileIcon(button, "");
			}

			if (i >= m_aAppLabels.Count() || !m_aAppLabels[i])
				continue;

			m_aAppLabels[i].SetVisible(used);

			if (used)
				m_aAppLabels[i].SetText(m_aApps[i].ResolveLabel());
		}

		PaintDock();
		ShowClock(true);

		if (m_ButtonBack)
			m_ButtonBack.SetText("LOCK");

		UpdateLogButton();
		SetHint("");
	}

	//! Colours one home-screen tile.
	//!
	//! The tile layout keeps the colour on a widget named "Tile", which
	//! SCR_ButtonTextComponent knows nothing about. Painting the widget named
	//! "Background" instead -- the obvious choice -- works until the first
	//! mouse-over, at which point the component repaints it from its own
	//! m_BackgroundHovered and the app's colour is gone for good.
	//!
	//! Nine tiles share the widget name. FindAnyWidget from THIS button's own
	//! root searches only this button's subtree, so they never collide.
	//! Puts one sprite from the base game's icon atlas on a tile.
	//!
	//! \return True if a sprite was actually drawn. False means the caller
	//! should fall back to the letter mark -- an unknown sprite name is not an
	//! error the player should ever see, it is a tile that quietly says TXT
	//! instead.
	protected bool SetTileIcon(notnull SCR_ButtonTextComponent button, string sprite)
	{
		Widget tileRoot = button.GetRootWidget();
		if (!tileRoot)
			return false;

		ImageWidget icon = ImageWidget.Cast(tileRoot.FindAnyWidget(W_ICON));
		if (!icon)
			return false;

		if (sprite.IsEmpty())
		{
			icon.SetVisible(false);
			return false;
		}

		bool drawn = icon.LoadImageTexture(0, sprite);
		icon.SetVisible(drawn);
		return drawn;
	}

	protected void PaintTile(notnull SCR_ButtonTextComponent button, int colour)
	{
		Widget tileRoot = button.GetRootWidget();
		if (!tileRoot)
			return;

		ImageWidget tile = ImageWidget.Cast(tileRoot.FindAnyWidget(W_TILE));
		if (!tile)
			return;

		tile.SetColor(Color.FromInt(colour));
	}

	//! The dock repeats the first four apps. A click there is the same click as
	//! on the grid tile above it -- one handler, one code path, and the dock
	//! cannot drift out of step with what the device actually carries.
	protected void OnDockClicked(SCR_ButtonTextComponent button)
	{
		int slot = m_aDockButtons.Find(button);
		if (slot < 0 || slot >= m_aApps.Count())
			return;

		ShowList(m_aApps[slot]);
	}

	//! Paints the dock from the first apps the device has, and hides any tile
	//! it cannot fill -- a dock with three apps in it is a phone with three
	//! apps, not a broken row of four.
	protected void PaintDock()
	{
		foreach (int i, SCR_ButtonTextComponent dockButton : m_aDockButtons)
		{
			bool used = i < m_aApps.Count();
			dockButton.GetRootWidget().SetVisible(used);

			if (!used)
				continue;

			int kind = m_aApps[i].m_eKind;
			PaintTile(dockButton, MCF_Device_Names.ColorFor(kind));

			if (SetTileIcon(dockButton, MCF_Device_Names.IconFor(kind)))
				dockButton.SetText("");
			else
				dockButton.SetText(MCF_Device_Names.GlyphFor(kind));
		}
	}

	protected void OnAppClicked(SCR_ButtonTextComponent button)
	{
		int slot = m_aAppButtons.Find(button);
		if (slot < 0 || slot >= m_aApps.Count())
			return;

		ShowList(m_aApps[slot]);
	}

	// ---------------------------------------------------------- the list

	//! \param app Which app to list. Null means the caller lost its place --
	//! go home rather than off the end of a null.
	protected void ShowList(MCF_Device_App app)
	{
		if (!app)
		{
			ShowHome();
			return;
		}

		m_bOnHome = false;
		m_bOnList = true;
		m_OpenApp = app;
		m_iOpenEntry = -1;
		StopWaitingForPicture();

		SetScreens(false, true, false);

		m_aRowButtons.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		// Ask what is in there, then draw it. The presenter decides whether an
		// empty app has anything to say for itself; this only puts rows on the
		// screen.
		m_Content.GetItems(app, m_aVisible);

		foreach (MCF_Device_Item entry : m_aVisible)
		{
			AddRow(entry);
		}

		if (m_wDeviceName)
			m_wDeviceName.SetText(app.ResolveLabel());

		if (m_ButtonBack)
			m_ButtonBack.SetText("HOME");

		UpdateLogButton();

		if (m_aVisible.IsEmpty())
			SetHint(app.ResolveEmptyText());
		else
			SetHint(m_aVisible.Count().ToString() + " item(s)");
	}

	protected void AddRow(notnull MCF_Device_Item entry)
	{
		if (!m_wEntryList)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wEntryList);
		if (!row)
			return;

		SCR_ButtonTextComponent rowButton = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!rowButton)
			return;

		rowButton.SetText(entry.DescribeRow());
		rowButton.m_OnClicked.Insert(OnRowClicked);
		m_aRowButtons.Insert(rowButton);
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
		MCF_Device_Item entry = m_aVisible[index];

		if (m_wReadHeading)
			m_wReadHeading.SetText(entry.m_sHeading);

		if (m_wReadTimestamp)
			m_wReadTimestamp.SetText(entry.m_sTimestamp);

		// A wrapping text widget, laid out once, rather than a row created per
		// message. The shared row layout is a button: one line, no wrapping,
		// and a sentence of any length simply ran off the side of the phone.
		if (m_wReadBody)
			m_wReadBody.SetText(entry.m_sBody);

		ShowPicture(entry);

		if (m_ButtonBack)
			m_ButtonBack.SetText("BACK");

		UpdatePageButtons();
		UpdateLogButton();
	}

	//! The photograph on an item, if it has one and if this machine has it.
	//!
	//! TWO SOURCES, IN THIS ORDER. An imported texture ships with the mod and is
	//! on every machine; a fetched one exists only where somebody fetched it.
	//! The imported one wins when both are named, because a picture that is
	//! always there beats one that usually is.
	//!
	//! NOT HAVING IT IS NORMAL. A player who cannot reach the address, or who
	//! opened the phone before the fetch finished, sees the text without the
	//! picture. That is a photograph that has not loaded, not an error, and it
	//! must not read as one.
	protected void ShowPicture(notnull MCF_Device_Item item)
	{
		StopWaitingForPicture();

		if (!m_wReadImage)
			return;

		bool shown;

		if (!item.m_sImage.IsEmpty())
			shown = m_wReadImage.LoadImageTexture(0, item.m_sImage);

		string key = item.ImageKey();

		if (!shown && !key.IsEmpty())
			shown = MCF_Device_ImageCache.Show(m_wReadImage, key);

		if (shown)
		{
			m_sPictureShown = key;
			DrawPicture();
			return;
		}

		m_wReadImage.SetVisible(false);

		if (m_wReadImageButton)
			m_wReadImageButton.SetVisible(false);

		// No picture and none coming: an ordinary item, nothing to say.
		if (key.IsEmpty())
		{
			SetPictureNote("");
			return;
		}

		int state = MCF_Device_ImageCache.StateOf(key);

		// NONE means nobody ever asked -- the prefetch at open time missed it,
		// or this item was authored while the phone was already on screen.
		// FAILED means somebody asked and it did not arrive, and opening the
		// item again is the plainest retry there is: no button to explain, and
		// it costs one request that the player asked for by looking at it.
		if (state != MCF_EImageState.LOADING)
			FetchPicture(item);

		m_sPictureKey = key;
		m_fPictureTick = 0;
		m_fPictureWaited = 0;
		m_iPictureDots = 0;
		SetPictureNote(LoadingLine());
	}

	//! Puts the picture on screen at the size the glass allows.
	//!
	//! Sized to the glass rather than to the picture: a photograph wider than
	//! the screen would push the text off the side, and one much narrower would
	//! look like a thumbnail somebody forgot to finish.
	protected void DrawPicture()
	{
		if (!m_wReadImage)
			return;

		float width = GlassWidth() * 0.88;
		float height = width / PictureAspect();

		m_wReadImage.SetSize(width, height);
		m_wReadImage.SetVisible(true);

		// The button is NOT sized here: Widget has no SetSize -- only the widget
		// types that own a size do -- so the button's slot is set to size itself
		// to its content instead, and its content is the image just sized above.
		// One number, one place, and the clickable area cannot disagree with what
		// is drawn.
		if (m_wReadImageButton)
			m_wReadImageButton.SetVisible(true);

		SetPictureNote("");
	}

	//! The drawn picture's width over its height.
	//!
	//! The cache read it out of the file's own header when it stored it. An
	//! imported texture has no entry there and neither does a picture cached
	//! before this existed, so there is a fallback -- but a photograph squashed
	//! into the wrong shape is obvious at a glance, which is why it is worth
	//! asking rather than assuming.
	protected float PictureAspect()
	{
		float aspect = MCF_Device_ImageCache.Aspect(m_sPictureShown);
		if (aspect > 0)
			return aspect;

		return 4.0 / 3.0;
	}

	//! Opens the picture over the whole screen.
	//!
	//! OVER EVERYTHING, PHONE INCLUDED. A photograph is the one thing on a device
	//! that a player actually has to study -- a number plate, a face, a map
	//! corner -- and studying it through a phone-sized window is the difference
	//! between intel and decoration.
	protected void OnPhotoClicked()
	{
		if (!m_wPhotoOverlay || !m_wPhotoFull || m_sPictureShown.IsEmpty())
			return;

		if (!MCF_Device_ImageCache.Show(m_wPhotoFull, m_sPictureShown))
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float screenW = workspace.DPIUnscale(workspace.GetWidth());
		float screenH = workspace.DPIUnscale(workspace.GetHeight());

		float aspect = PictureAspect();

		// Fit inside the screen rather than fill it: whichever side runs out
		// first decides, so nothing is cropped and nothing is stretched.
		float height = screenH * 0.80;
		float width = height * aspect;

		if (width > screenW * 0.80)
		{
			width = screenW * 0.80;
			height = width / aspect;
		}

		// The wrapper is placed, the picture inside is sized: FrameSlot works on
		// any widget, but only widgets that own a size have SetSize, and a button
		// is not one of them.
		if (m_wPhotoFullButton)
		{
			FrameSlot.SetAnchor(m_wPhotoFullButton, 0.5, 0.5);
			FrameSlot.SetSize(m_wPhotoFullButton, width, height);
			FrameSlot.SetPos(m_wPhotoFullButton, -width * 0.5, -height * 0.5);
		}

		m_wPhotoFull.SetSize(width, height);

		m_wPhotoOverlay.SetVisible(true);
	}

	protected void ClosePhoto()
	{
		if (m_wPhotoOverlay)
			m_wPhotoOverlay.SetVisible(false);
	}

	protected bool IsPhotoOpen()
	{
		return m_wPhotoOverlay && m_wPhotoOverlay.IsVisible();
	}

	//! Ticked every frame while an item with a picture is open.
	//!
	//! POLLED, NOT NOTIFIED. The fetch finishes on a callback that knows nothing
	//! about menus, and a menu that has been closed in the meantime must not be
	//! called into. Asking the cache once a frame costs a file-exists check and
	//! cannot outlive the screen that does the asking.
	protected void UpdatePicture(float tDelta)
	{
		if (m_sPictureKey.IsEmpty())
			return;

		int state = MCF_Device_ImageCache.StateOf(m_sPictureKey);

		if (state == MCF_EImageState.READY)
		{
			if (m_wReadImage && MCF_Device_ImageCache.Show(m_wReadImage, m_sPictureKey))
			{
				m_sPictureShown = m_sPictureKey;
				DrawPicture();
			}
			else
			{
				SetPictureNote("Photo unavailable");
			}

			m_sPictureKey = "";
			return;
		}

		if (state == MCF_EImageState.FAILED)
		{
			SetPictureNote("Photo unavailable");
			m_sPictureKey = "";
			return;
		}

		m_fPictureWaited = m_fPictureWaited + tDelta;
		if (m_fPictureWaited > PICTURE_GIVE_UP_SECONDS)
		{
			MCF_Core_Log.Warn("device image '" + m_sPictureKey + "' never came back after "
				+ PICTURE_GIVE_UP_SECONDS.ToString() + "s -- giving up on screen");

			SetPictureNote("Photo unavailable");
			m_sPictureKey = "";
			return;
		}

		m_fPictureTick = m_fPictureTick + tDelta;
		if (m_fPictureTick < PICTURE_DOT_SECONDS)
			return;

		m_fPictureTick = 0;
		m_iPictureDots = (m_iPictureDots + 1) % 4;
		SetPictureNote(LoadingLine());
	}

	protected void StopWaitingForPicture()
	{
		m_sPictureKey = "";
		m_sPictureShown = "";
		ClosePhoto();
		m_fPictureTick = 0;
		m_fPictureWaited = 0;
		m_iPictureDots = 0;
	}

	//! The spinner. Dots rather than a rotating glyph, because the note sits in
	//! a text column and a character that is not in the font is invisible
	//! rather than wrong -- which would put us back to an empty space meaning
	//! two different things.
	protected string LoadingLine()
	{
		string line = "Loading photo";

		for (int i = 0; i < m_iPictureDots; i++)
		{
			line = line + ".";
		}

		return line;
	}

	protected void SetPictureNote(string text)
	{
		if (!m_wReadImageNote)
			return;

		m_wReadImageNote.SetText(text);
		m_wReadImageNote.SetVisible(!text.IsEmpty());
	}

	//! Starts one item's fetch. Shares its splitting with the prefetch so there
	//! is one place a URL is taken apart.
	protected void FetchPicture(notnull MCF_Device_Item item)
	{
		MCF_Device_ImageCache.FetchUrl(item.ImageUrl());
	}

	//! Asks a picture's source to fetch itself, so it is on disk by the time
	//! somebody opens the item. Called once when the device opens, because that
	//! is the only moment there is time to spare.
	protected void PrefetchPictures()
	{
		array<ref MCF_Device_Item> items = {};
		m_Content.GetAllItems(items);

		int asked;

		foreach (MCF_Device_Item item : items)
		{
			if (item.ImageUrl().IsEmpty())
				continue;

			FetchPicture(item);
			asked++;
		}

		MCF_Core_Log.Debug("device shell: " + items.Count().ToString() + " item(s), "
			+ asked.ToString() + " with a picture url");
	}


	protected float GlassWidth()
	{
		if (m_Geometry)
			return m_Geometry.GetGlassWidth();

		return 200;
	}

	// ------------------------------------------------------------ plumbing

	// ---------------------------------------------------------------- the lock

	protected bool IsDeviceLocked()
	{
		if (!m_Carrier)
			return false;

		return MCF_Devices_LockComponent.IsEntityLocked(m_Carrier.GetOwner());
	}

	//! The screen a shut device shows. Everything else goes away, including the
	//! app tiles -- a locked phone that still lists its apps has told you what
	//! is on it, which is most of what breaking in was supposed to earn.
	//! The passcode pad, which is what a locked phone shows when you touch it.
	//!
	//! WHY A CODE IN FRONT OF THE PUZZLE. The break-in used to be one button
	//! that said BREAK IN, which told the player nothing and looked like a
	//! cheat. A keypad is what the object actually has: it can be tried, it
	//! can be wrong, and a code found somewhere else in the mission -- on a
	//! note, from a prisoner, over the radio -- now has a place to be typed.
	//! Type the right one and the lock's own challenge starts.
	protected void ShowKeypad()
	{
		m_bOnKeypad = true;
		m_bOnLock = false;
		m_sCode = "";

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		if (m_wKeypad)
			m_wKeypad.SetVisible(true);

		if (m_wKeypadTitle)
			m_wKeypadTitle.SetText("Enter passcode");

		DrawPips();
		SetHint("");
	}

	//! Four pips, filled as far as the code has been typed.
	protected void DrawPips()
	{
		foreach (int i, Widget pip : m_aPips)
		{
			if (!pip)
				continue;

			ImageWidget dot = ImageWidget.Cast(pip);
			if (!dot)
				continue;

			if (i < m_sCode.Length())
				dot.SetColor(Color.FromInt(0xFFFFFFFF));
			else
				dot.SetColor(Color.FromInt(0x59FFFFFF));
		}
	}

	protected void OnKeyClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bOnKeypad || !button)
			return;

		if (m_sCode.Length() >= CODE_LENGTH)
			return;

		int slot = m_aKeys.Find(button);
		if (slot < 0 || slot > 9)
			return;

		m_sCode = m_sCode + slot.ToString();
		DrawPips();

		if (m_sCode.Length() < CODE_LENGTH)
			return;

		if (m_sCode == BREAK_IN_CODE)
		{
			if (m_wKeypadTitle)
				m_wKeypadTitle.SetText("Accepted");

			OnUnlockClicked(null);
			return;
		}

		// Wrong. Say so, empty the pips, and let them try again -- the phone
		// is not the thing keeping score, the lock on the server is.
		if (m_wKeypadTitle)
			m_wKeypadTitle.SetText("Wrong code");

		m_sCode = "";
		DrawPips();
	}

	protected void OnDeleteClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bOnKeypad || m_sCode.IsEmpty())
			return;

		m_sCode = m_sCode.Substring(0, m_sCode.Length() - 1);
		DrawPips();
	}

	protected void ShowLock()
	{
		m_bOnLock = true;
		m_bOnHome = false;
		m_bOnList = false;
		m_OpenApp = null;
		m_iOpenEntry = -1;
		StopWaitingForPicture();

		SetScreens(false, false, false);

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(true);

		if (m_wKeypad)
			m_wKeypad.SetVisible(false);

		m_bOnKeypad = false;

		// The name belongs to an app screen's header. On the lock screen it
		// landed on top of the note, which is what made it read as a jumble.
		if (m_wDeviceName)
			m_wDeviceName.SetVisible(false);

		// The old BREAK IN button is gone: the way in is the keypad, reached
		// by touching the bar, which is where a hand goes on a phone anyway.
		if (m_UnlockButton)
			m_UnlockButton.GetRootWidget().SetVisible(false);

		MCF_Devices_LockComponent lock = MCF_Devices_LockComponent.FindOn(m_Carrier.GetOwner());

		// A locked phone still shows its clock, large, the way a phone on a
		// table does -- and the fact that it is locked belongs under it in
		// small type, not as a shout across the middle of the screen. The word
		// LOCKED was the only thing on this screen for weeks and it read as an
		// error message rather than as a phone.
		if (m_wLockTitle)
			m_wLockTitle.SetText(ClockText());

		if (m_wLockNote && lock)
			m_wLockNote.SetText(DateText() + "\n\nLocked  -  " + DescribeLock(lock.GetDifficulty()));

		if (m_UnlockButton && lock)
			m_UnlockButton.SetText(lock.GetBreakInVerb());

		SetHint("Touch the bar to unlock.");
	}

	//! What the player is up against, in words rather than a number. Naming it
	//! before the attempt is deliberate: a device that cannot be cracked in the
	//! time available should say so first, not after.
	//! Said as the BREAK-IN it is, not as the lock it sits behind.
	//!
	//! The player is not shopping for a padlock; they are deciding whether to
	//! spend the next minute on this phone while the patrol comes back. So the
	//! line names the work: how hard, and roughly how long.
	protected string DescribeLock(int difficulty)
	{
		if (difficulty <= 0)
			return "Hack: trivial, seconds";

		if (difficulty == 1)
			return "Hack: light, under a minute";

		if (difficulty == 2)
			return "Hack: signal work, a minute or two";

		if (difficulty == 3)
			return "Hack: hardened, expect a fight";

		return "Hack: hardened and alarmed";
	}

	//! Asks the server for a challenge. Nothing about the puzzle is decided
	//! here, because a client that generated its own puzzle would also know the
	//! answer to it.
	protected void OnUnlockClicked(SCR_ButtonTextComponent button)
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

		SetHint("Working...");
		controller.MCF_RequestDeviceChallenge(rpl.Id());
	}

	//! Watches for the unlock arriving. The server decides, and the answer
	//! comes back as a replicated bool with no callback of its own on this end,
	//! so the shell has to notice rather than be told. Polling a bool once a
	//! frame costs nothing and is the only thing that turns a solved puzzle
	//! into an open screen without the player closing and reopening the device.
	protected void PollLock()
	{
		if (!m_bOnLock)
			return;

		if (IsDeviceLocked())
			return;

		MCF_Core_Log.Debug("device unlocked while its screen was open -- opening it");

		m_bOnLock = false;

		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		m_Content.GetApps(m_aApps);
		ShowHome();
	}

	// ------------------------------------------------------------ the break-in

	//! Draws a challenge on the device it belongs to.
	//!
	//! WHY THIS IS A STATIC HANDOFF. The challenge is a reply from the server
	//! and arrives on the player controller, which has no idea what is on
	//! screen. It used to answer that by opening a menu of its own over the top
	//! of the device -- which is precisely what made breaking into a phone feel
	//! like putting the phone down first.
	//!
	//! \return True if a shell took it. False means nothing is open to draw on
	//! and the challenge is dropped, which is only reachable if the player
	//! closed the device between asking and being answered.
	static bool ShowChallenge(RplId deviceId, int seed, int difficulty)
	{
		if (!s_Open)
			return false;

		return s_Open.StartHack(deviceId, seed, difficulty);
	}

	//! Puts the puzzle on the device's own glass.
	//!
	//! CREATED ON DEMAND, DESTROYED AFTERWARDS, rather than shipped hidden
	//! inside every device layout. Two reasons, and the second one is the one
	//! that decided it: one layout then serves the phone, the laptop and
	//! anything added later, and SCR_ButtonTextComponent.GetButtonText will not
	//! find a button inside a subtree marked hidden -- a panel that ships
	//! hidden binds nothing at all, silently, which cost this feature's lock
	//! screen an evening already.
	protected bool StartHack(RplId deviceId, int seed, int difficulty)
	{
		Widget root = GetRootWidget();
		if (!root)
			return false;

		Widget screen = root.FindAnyWidget(MCF_Device_Layout.W_SCREEN_AREA);
		if (!screen)
		{
			MCF_Core_Log.Warn("this device skin has no " + MCF_Device_Layout.W_SCREEN_AREA + " -- nowhere to draw the break-in");
			return false;
		}

		CloseHack();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		m_wHackPanel = workspace.CreateWidgets(HACK_LAYOUT, screen);
		if (!m_wHackPanel)
		{
			MCF_Core_Log.Warn("could not create the break-in panel -- check the layout path");
			return false;
		}

		FitHackPanel();

		// The lock screen is what the player pressed to get here. The puzzle
		// replaces it rather than sitting on top of it -- and the keypad goes
		// with it, or it is still standing there when the puzzle is over and
		// the phone comes back to a passcode nobody asked for. That was the
		// glitch: a finished break-in landing on the keypad instead of home.
		if (m_wLockScreen)
			m_wLockScreen.SetVisible(false);

		if (m_wKeypad)
			m_wKeypad.SetVisible(false);

		m_bOnKeypad = false;

		m_bHackDone = false;
		m_HackScreen = new MCF_Devices_HackScreen();
		m_HackScreen.m_OnFinished.Insert(OnHackFinished);
		m_HackScreen.Start(m_wHackPanel, deviceId, seed, difficulty);

		SetHint("");
		return true;
	}

	//! The panel fills the glass, whatever shape this device's glass is.
	//!
	//! The anchors collapse to a point before the size is set. A slot whose
	//! anchors are stretched takes its size from them and ignores SetSize,
	//! silently -- see the note at the top of MCF_Device_Layout.c.
	protected void FitHackPanel()
	{
		if (!m_wHackPanel)
			return;

		// MEASURED OFF THE GLASS WIDGET ITSELF, not off the geometry object.
		// The panel is a child of ScreenArea, so the size that matters is the
		// one that widget actually has after a layout pass -- and asking the
		// widget survives a device whose 3D fit never landed, which is exactly
		// when the puzzle used to come up as a strip too small to read.
		//
		// GetScreenSize answers in physical pixels and FrameSlot works in the
		// reference resolution. Converting is not optional; see the same note
		// in MCF_Device_Layout.FitToDevice.
		float width, height;
		Widget screenArea = m_wHackPanel.GetParent();
		if (screenArea)
		{
			screenArea.GetScreenSize(width, height);

			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (workspace)
			{
				width = workspace.DPIUnscale(width);
				height = workspace.DPIUnscale(height);
			}
		}

		if (width <= 0 || height <= 0)
		{
			width = m_Geometry.GetGlassWidth();
			height = m_Geometry.GetGlassHeight();
		}

		if (width <= 0 || height <= 0)
			return;

		FrameSlot.SetAnchor(m_wHackPanel, 0.5, 0.5);
		FrameSlot.SetSize(m_wHackPanel, width, height);
		FrameSlot.SetPos(m_wHackPanel, -width * 0.5, -height * 0.5);
	}

	//! Only records that it is over. See m_bHackDone.
	protected void OnHackFinished()
	{
		m_bHackDone = true;
	}

	//! Takes the panel down and puts the device back where it was.
	//!
	//! Whether the break-in worked is the server's answer and arrives as a
	//! replicated bool a moment later, so the lock screen goes back up and
	//! PollLock takes it down again if the device opened.
	//! The puzzle is over, one way or another.
	//!
	//! IT MUST LAND ON A SCREEN. Leaving the phone on none of them -- which is
	//! what happened when the device came back unlocked -- means every flag
	//! this class keeps says "not here", and the next tap on the home bar
	//! walked off the end of the back chain into a null app. A finished
	//! break-in goes to the lock screen if it failed and to the home screen if
	//! it worked; there is no third answer.
	protected void FinishHack()
	{
		CloseHack();

		if (IsDeviceLocked())
		{
			ShowLock();
			return;
		}

		m_Content.GetApps(m_aApps);
		ShowHome();
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

	// ------------------------------------------------------------ plumbing

	protected void SetScreens(bool home, bool list, bool read)
	{
		// Any ordinary screen means the lock screen is not the one showing.
		if (m_wLockScreen && (home || list || read))
			m_wLockScreen.SetVisible(false);

		foreach (SCR_ButtonTextComponent button : m_aAppButtons)
		{
			button.GetRootWidget().SetVisible(home);
		}

		foreach (SCR_ButtonTextComponent dockButton : m_aDockButtons)
		{
			dockButton.GetRootWidget().SetVisible(home);
		}

		if (m_wDockPanel)
			m_wDockPanel.SetVisible(home);

		if (m_wKeypad && (home || list || read))
		{
			m_wKeypad.SetVisible(false);
			m_bOnKeypad = false;
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

	}

	//! BACK steps back one level, and closes the phone from the home screen --
	//! which is what a phone does, and means the button is never dead.
	//! One tap back, two taps away.
	//!
	//! The gap is measured on the menu's own tick rather than from a clock,
	//! because the tick is the only time source this screen already trusts --
	//! and a double tap that misses simply reads as two ordinary steps back,
	//! which is a harmless way to be wrong.
	protected void OnHomeBarTapped()
	{
		if (m_fSinceHomeTap < HOME_DOUBLE_TAP)
		{
			Close();
			return;
		}

		m_fSinceHomeTap = 0;

		// A locked phone answers a tap the way a locked phone does: with the
		// keypad. The code is the door; the puzzle behind it is the lock.
		if (m_bOnLock)
		{
			ShowKeypad();
			return;
		}

		if (m_bOnKeypad)
		{
			ShowLock();
			return;
		}

		// On the home screen there is nowhere behind: a single tap there does
		// nothing, and the second tap of a double is what closes.
		if (m_bOnHome)
			return;

		OnBackClicked(null);
	}

	protected void OnBackClicked(SCR_ButtonTextComponent button)
	{
		// Backing out of the puzzle returns to the lock screen, not out of the
		// device. Giving up on a break-in is not the same as putting the phone
		// away, and the player almost always wants another go.
		if (m_HackScreen && m_HackScreen.IsRunning())
		{
			m_HackScreen.Cancel();
			return;
		}

		// The photograph is a layer over the screen, not a screen of its own, so
		// backing out of it puts you where you already were rather than one step
		// further back than you asked for.
		if (IsPhotoOpen())
		{
			ClosePhoto();
			return;
		}

		if (m_bPageMode)
		{
			Close();
			return;
		}

		// There is nowhere behind a lock screen to go back to.
		if (m_bOnLock)
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

		// The last step back from an entry is its own app's list. With no app
		// open there is nothing behind but the home screen, and asking for a
		// list of nothing is how this crashed.
		if (!m_OpenApp)
		{
			ShowHome();
			return;
		}

		ShowList(m_OpenApp);
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

		MCF_Device_Item entry = m_aVisible[m_iOpenEntry];
		// The picture goes with it. A report that says "photograph attached" and
		// has none is worse than one that never mentioned it, and the address is
		// all that has to travel -- every machine fetches its own copy anyway.
		controller.MCF_RequestLogIntel(m_Carrier.GetDeviceName(), entry.m_sHeading, entry.m_sTimestamp, entry.m_sBody, entry.ImageUrl());
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
			SetHint("Log it at the board.");
		else
			SetHint("Not at the board.");
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

		// BEFORE the geometry early-out below. The fit finishes after three
		// frames and stops running; a picture can arrive seconds later, and an
		// unlock can arrive later still.
		UpdatePicture(tDelta);

		// The puzzle runs on the same tick as everything else, and is torn down
		// here rather than inside its own click handler.
		if (m_HackScreen)
		{
			if (m_bHackDone)
			{
				FinishHack();
			}
			else
			{
				// Re-fitted every frame while it is up. The glass has no size at
				// all on the frame the panel is created, and a puzzle that was
				// measured then stays that size for the rest of the break-in.
				FitHackPanel();
				m_HackScreen.Update();
			}
		}

		PollLock();

		// The drawn phone is re-placed every frame: it costs three slot calls
		// and it survives a window resize, which the measured fit never did.
		FitDrawnPhone();

		m_fSinceHomeTap = m_fSinceHomeTap + tDelta;

		// The status bar carries the mission's own clock, so it has to be
		// re-read rather than set once at open. Once a second is enough for a
		// clock that only shows minutes, and cheap enough not to think about.
		m_fStatusTick = m_fStatusTick + tDelta;
		if (m_fStatusTick >= 1.0)
		{
			m_fStatusTick = 0;
			RefreshStatus();

			if (m_bOnHome)
				ShowClock(true);
		}

		if (m_bFitted || m_iFitFrame > FIT_GIVE_UP_FRAME)
			return;

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_iFitFrame++;

		// Frame one places the widgets. The measurement waits, because
		// GetScreenSize reports what the last layout pass produced -- asking in
		// the same frame reports the size from before the change, which read
		// as "the call did nothing" for two rounds.
		if (!m_Geometry)
			return;

		if (m_iFitFrame == 1)
		{
			m_Geometry.FitToBox();
			return;
		}

		if (m_iFitFrame < 3)
			return;

		// KEEP ASKING UNTIL IT ANSWERS. This was one attempt on frame three,
		// which is enough for a phone and was not enough for the laptop: the
		// preview answered nothing, the box-relative placement stood, and the
		// glass came out about a third too big in both directions -- big enough
		// to read as a flat panel with a laptop somewhere behind it rather than
		// as a laptop's screen.
		//
		// The box-relative fit assumes the model FILLS the box it is given, and
		// it does not. Measured on the phone, where the preview does answer:
		// 400x851 of model inside a 643x1368 box, so 62% of it. That is the
		// size of the error being papered over, and it is why the fallback is
		// worth this many frames of asking.
		// Every frame until the fit lands: the preview manager takes the render
		// target over when it is ready, and whatever was set before that is
		// gone.
		m_Geometry.ClearPreviewBackground();

		if (m_Geometry.FitToDevice())
		{
			m_bFitted = true;
			FitHackPanel();
			LogGeometry(root);
			return;
		}

		if (m_iFitFrame == FIT_GIVE_UP_FRAME)
		{
			MCF_Core_Log.Warn("the preview never said where this device is, after " + FIT_GIVE_UP_FRAME.ToString()
				+ " frames -- the glass stays box-relative and will be too big for the model behind it");
			LogGeometry(root);
		}
	}

	//! Stops the server's reply arriving at a shell that is no longer on screen.
	override void OnMenuClose()
	{
		CloseHack();

		if (s_Open == this)
			s_Open = null;

		super.OnMenuClose();
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

	//! The two ends of the status bar, refreshed on a slow tick.
	//!
	//! A status bar is the cheapest authenticity a screen has, and the easiest
	//! to get wrong. Nothing in the framework reads any of this -- but a phone
	//! that says 100% on the same line as every other phone in the mission
	//! reads as a mock-up, and one that says 41% here and 88% over there reads
	//! as two phones somebody owned.
	protected void RefreshStatus()
	{
		if (m_wStatusBar)
			m_wStatusBar.SetText(ClockText());

		if (m_wStatusRight)
			m_wStatusRight.SetText(StatusRightText());
	}

	protected string StatusRightText()
	{
		if (m_eView == MCF_EIntelView.LAPTOP)
			return "ETH    AC POWER";

		// The signal and battery GLYPHS sit beside this text in the layout, so
		// the words for them would be a second helping of the same fact.
		return BatteryPercent().ToString() + "%";
	}

	//! A number that belongs to this device and to no other, and that every
	//! player gets the same answer from.
	//!
	//! Derived from what the device is called and which profile it carries,
	//! because both are authored and both replicate. NOT from Math.RandomInt,
	//! which is broken on wide ranges here (see the Enfusion lessons), and not
	//! from anything per-client: two players looking at the same phone over
	//! someone's shoulder must not see two different batteries.
	protected int DeviceSeed()
	{
		string source;
		if (m_Carrier)
			source = m_Carrier.GetDeviceName() + "/" + m_Carrier.GetProfileId();

		if (source.IsEmpty())
			source = "device";

		int seed = 17;
		int count = source.Length();
		for (int i = 0; i < count; i++)
		{
			seed = (seed * 31 + source.Get(i).ToAscii()) % 100003;
		}

		return seed;
	}

	//! Never 100, never flat. A dead phone would have to explain why it still
	//! lights up, and a full one looks like a default.
	protected int BatteryPercent()
	{
		return 11 + (DeviceSeed() % 78);
	}

	protected string SignalLabel()
	{
		int band = DeviceSeed() % 9;

		if (band == 0)
			return "NO SERVICE";

		if (band < 3)
			return "3G";

		if (band < 6)
			return "4G";

		return "LTE";
	}

	protected int LocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}
}
