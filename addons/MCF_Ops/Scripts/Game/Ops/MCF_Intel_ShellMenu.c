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

	//! The phone's own row: a disc, two lines and a small right-hand note.
	protected static const ResourceName PHONE_ROW_LAYOUT = "{6A1C4F0B39D35600}UI/layouts/MCF/MCF_PhoneRow.layout";

	//! The break-in panel, drawn inside the device's own glass. One layout
	//! serves every skin, and it is created on demand rather than shipped
	//! inside each device layout -- see StartHack below.
	protected static const ResourceName HACK_LAYOUT = "{6A1C4F0B39D30000}UI/layouts/MCF/MCF_DeviceHack.layout";

	protected static const string W_DEVICE_NAME = "DeviceName";
	protected static const string W_STATUS_BAR = "StatusBar";
	protected static const string W_STATUS_RIGHT = "StatusRight";
	protected static const string W_TILE = "Tile";
	protected static const string W_ICON = "Icon";
	protected static const string W_BADGE = "Badge";
	protected static const string W_BADGE_TEXT = "BadgeText";
	protected static const string W_UNREAD_DOT = "Unread";

	//! The base game's own icon atlas. White masks, tinted by the widget.
	protected static const ResourceName ICON_SET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	protected static const string W_APP_PREFIX = "App";
	protected static const string W_DOCK_PREFIX = "Dock";
	protected static const string W_DOCK_PANEL = "DockPanel";
	protected static const string W_CHASSIS = "Chassis";
	protected static const string W_HOME_BAR = "HomeTap";
	protected static const string W_KEYPAD = "Keypad";
	protected static const string W_EDITOR = "EditorPane";
	protected static const string W_EDITOR_TITLE = "EditorTitle";
	protected static const string W_EDITOR_FIELD = "EditorField";
	protected static const string W_EDITOR_SAVE = "EditorSave";
	protected static const string W_EDITOR_CANCEL = "EditorCancel";
	protected static const string W_KEYPAD_TITLE = "KeypadTitle";
	protected static const string W_PIP_PREFIX = "Pip";
	protected static const string W_KEY_DELETE = "KeyDel";

	//! What opens the break-in. Four digits, and deliberately a joke a player
	//! can be told by another player -- the code is not the puzzle, the puzzle
	//! is the puzzle.
	protected static const string BREAK_IN_CODE = "1337";
	protected static const int CODE_LENGTH = 4;

	// The dialler's own widgets. They are NOT the passcode pad's: the pad is a
	// door in front of the phone and the dialler is a screen inside it, and a
	// single set of twelve keys shared between the two meant one of them was
	// always sitting in the other one's geometry.
	protected static const string W_DIALLER = "Dialler";
	protected static const string W_DIAL_NUMBER = "DialNumber";
	protected static const string W_DIAL_SUB = "DialSub";
	protected static const string W_DIAL_KEY_PREFIX = "DialKey";
	protected static const string W_DIAL_CALL = "DialCall";
	protected static const string W_DIAL_DELETE = "DialDel";
	protected static const string W_DIAL_RECENT_LABEL = "DialRecentLabel";

	//! How many calls fit above the keys. Three is what the screen has room for
	//! once the keypad has what it needs, and a call log is read newest-first
	//! anyway -- the rest of it is in the app, not on the dialler.
	protected static const int DIAL_RECENTS = 3;

	//! Long enough for any number a mission would write down, short enough that
	//! it still fits the display at 26pt.
	protected static const int DIAL_MAX_DIGITS = 14;

	//! The bubble row for a message thread.
	protected static const ResourceName BUBBLE_LAYOUT = "{6A1C4F0B39D3564F}UI/layouts/MCF/MCF_PhoneBubble.layout";

	protected static const string W_CHAT_PANE = "ChatPane";
	protected static const string W_CHAT_COLUMN = "ChatColumn";
	protected static const string W_MAIL_PANE = "MailPane";
	protected static const string W_CONTACT_PANE = "ContactPane";
	protected static const string W_PHOTO_GRID = "PhotoGrid";

	//! How many photographs the glass holds at once.
	protected static const int PHOTO_TILES = 12;

	//! What a line of a message body starts with when the phone's owner is the
	//! one who sent it. One character, and it is the whole chat format.
	protected static const string OUTGOING_MARK = ">";

	//! What a heading written "who - what" is split on. Mission makers were
	//! already writing this before anything read it.
	protected static const string HEAD_SEP = " - ";

	//! Cards on the lock screen. Three fit above the prompt.
	protected static const int LOCK_NOTES = 3;
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
	// The Game Master's half of a paper or notepad: an edit box in the same
	// box as each read-only widget, and a tool column beside the sheet.
	protected static const string W_NAME_EDIT = "NameEdit";
	protected static const string W_HEADING_EDIT = "HeadingEdit";
	protected static const string W_STAMP_EDIT = "StampEdit";
	protected static const string W_BODY_EDIT = "BodyEdit";
	protected static const string W_AUTHOR_NOTE = "AuthorNote";
	protected static const string W_BUTTON_TYPE = "ButtonType";
	protected static const string W_BUTTON_PAGE = "ButtonPage";
	protected static const string W_BUTTON_ADD_PAGE = "ButtonAddPage";
	protected static const string W_BUTTON_DROP_PAGE = "ButtonDropPage";
	protected static const string W_BUTTON_SAVE_INTEL = "ButtonSaveIntel";

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
	protected Widget m_wEditor;
	protected TextWidget m_wEditorTitle;
	protected EditBoxWidget m_EditorField;
	protected SCR_ButtonTextComponent m_EditorSave;
	protected SCR_ButtonTextComponent m_EditorCancel;
	protected TextWidget m_wKeypadTitle;
	protected ref array<Widget> m_aPips = {};
	protected ref array<SCR_ButtonTextComponent> m_aKeys = {};
	protected string m_sCode;

	// ------------------------------------------------------------ authoring

	//! True when a Game Master opened this screen to CHANGE the device rather
	//! than to read it. Same shell, same tiles, same navigation -- the editing
	//! is a few extra rows and one extra pane, not a second screen with a
	//! similar name. See docs/architecture/DEVICE_CONTENT.md section 9.
	protected bool m_bAuthor;

	//! The object being edited, as the wire names it.
	protected SCR_EditableEntityComponent m_Editable;

	//! A copy of the device's profile. The screen draws this while the object
	//! still carries the original, so a Game Master who changes their mind can
	//! simply close the phone.
	protected ref MCF_Device_Profile m_Draft;

	//! What the editor pane is currently editing, and where to put it back.
	protected string m_sEditKind;
	protected MCF_Device_Item m_EditItem;
	protected MCF_Device_Item m_FormItem;
	protected bool m_bOnForm;

	protected static SCR_EditableEntityComponent s_PendingEditable;
	protected static bool s_bPendingAuthor;
	protected bool m_bOnKeypad;

	protected Widget m_wDialler;
	protected TextWidget m_wDialNumber;
	protected TextWidget m_wDialSub;
	protected ref array<SCR_ButtonTextComponent> m_aDialKeys = {};
	protected string m_sDialled;
	protected bool m_bOnDialler;

	protected Widget m_wChatPane;
	protected Widget m_wChatColumn;
	protected Widget m_wMailPane;
	protected Widget m_wContactPane;
	protected Widget m_wPhotoGrid;
	protected ref array<SCR_ButtonTextComponent> m_aPhotoTiles = {};
	protected SCR_ButtonTextComponent m_ContactCall;
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
	protected ref array<SCR_ButtonTextComponent> m_aAuthorButtons = {};
	protected ref array<string> m_aAuthorKinds = {};

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
	protected Widget m_wNameEdit;
	protected Widget m_wHeadingEdit;
	protected Widget m_wStampEdit;
	protected Widget m_wBodyEdit;
	protected Widget m_wAuthorNote;

	protected SCR_ButtonTextComponent m_ButtonType;
	protected SCR_ButtonTextComponent m_ButtonPageView;
	protected SCR_ButtonTextComponent m_ButtonAddPage;
	protected SCR_ButtonTextComponent m_ButtonDropPage;
	protected SCR_ButtonTextComponent m_ButtonSaveIntel;

	//! Whether the Game Master is writing on the page or looking at it.
	protected bool m_bPaperTyping;

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
	//! Opens the device the way a Game Master edits it: the phone itself, with
	//! the authoring rows switched on.
	//!
	//! WHY NOT A SEPARATE PANEL. There was one, and it was the second screen
	//! that edited the same object under a name one word different from the
	//! first. That confusion cost a day (HANDOVER step 1) and the fix was to
	//! delete the overlap, not to document it. A Game Master who edits the
	//! phone should be looking at the phone.
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

	static bool OpenFor(notnull MCF_Intel_CarrierComponent carrier)
	{
		MCF_EIntelView view = carrier.GetView();

		int preset = -1;
		switch (view)
		{
			case MCF_EIntelView.PAPER:   preset = ChimeraMenuPreset.MCF_IntelPaper; break;
			case MCF_EIntelView.NOTEPAD: preset = ChimeraMenuPreset.MCF_IntelNotepad; break;
			case MCF_EIntelView.PHONE:   preset = ChimeraMenuPreset.MCF_IntelDevice; break;
		}

		// THE LAPTOP IS NOT A SKIN ON THIS MENU ANY MORE. It is a desktop: a
		// panel, a launcher and windows that drag and stack, none of which
		// this class has a concept of. It keeps its own menu and its own
		// layout, and shares the part that was always the point -- the device
		// profile. Routed from here rather than from the caller so that there
		// is still exactly one place that answers "what opens this object".
		if (view == MCF_EIntelView.LAPTOP)
		{
			if (s_bPendingAuthor)
			{
				MCF_Intel_CarrierComponent held = carrier;
				SCR_EditableEntityComponent editable = s_PendingEditable;
				s_PendingEditable = null;
				s_bPendingAuthor = false;
				return MCF_Desktop_ShellMenu.OpenForAuthor(held, editable);
			}

			return MCF_Desktop_ShellMenu.OpenFor(carrier);
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
		m_wEditor = root.FindAnyWidget(W_EDITOR);
		m_wEditorTitle = TextWidget.Cast(root.FindAnyWidget(W_EDITOR_TITLE));
		m_EditorField = EditBoxWidget.Cast(root.FindAnyWidget(W_EDITOR_FIELD));

		m_EditorSave = SCR_ButtonTextComponent.GetButtonText(W_EDITOR_SAVE, root);
		if (m_EditorSave)
			m_EditorSave.m_OnClicked.Insert(OnEditorSave);

		m_EditorCancel = SCR_ButtonTextComponent.GetButtonText(W_EDITOR_CANCEL, root);
		if (m_EditorCancel)
			m_EditorCancel.m_OnClicked.Insert(OnEditorCancel);

		if (m_wEditor)
			m_wEditor.SetVisible(false);
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

		// Same rule as the pad above: bound while the layout still says it is
		// visible, then taken down.
		BindDialler(root);
		BindPanes(root);

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

		BindPaperAuthor(root);

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

		// AUTHOR MODE, taken from the static handover and cleared at once so a
		// later read-only open cannot inherit it.
		m_bAuthor = s_bPendingAuthor;
		m_Editable = s_PendingEditable;
		s_bPendingAuthor = false;
		s_PendingEditable = null;

		if (m_bAuthor)
		{
			// A copy, through the same serialiser the wire uses -- so what the
			// Game Master edits is exactly what can be sent, and the object
			// keeps what it has until SAVE.
			MCF_Device_Profile source = m_Content.GetProfile();
			if (source)
			{
				m_Draft = MCF_Device_Script.Deserialize(MCF_Device_Script.Serialize(source));

				if (m_Draft)
				{
					// THE DRAFT LOSES THE SHARED ID, AND THIS IS THE WHOLE
					// REASON EVERY PHONE USED TO CHANGE AT ONCE. The server
					// files any profile that arrives with an id back into the
					// library under that id -- which is what makes a profile
					// reusable, and which meant editing one handset rewrote
					// `smuggler_phone` itself and with it every other phone
					// reading that profile.
					//
					// A Game Master editing one device is editing THAT device.
					// Without an id the server writes the profile onto the
					// object and touches nothing else. Editing the shared
					// profile on purpose is a different act and deserves its
					// own row and its own warning -- see DEVICE_CONTENT.md.
					m_Draft.m_sId = "";
					m_Content.SetProfile(m_Draft);
				}
				else
				{
					MCF_Core_Log.Warn("device draft could not be built -- author mode would edit the shared profile, so it stays read-only");
					m_bAuthor = false;
				}
			}
		}

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
			//
			// A GAME MASTER DOES NOT HAVE TO BREAK INTO HIS OWN PROP. In author
			// mode the lock is content being edited, not an obstacle -- so the
			// phone opens on its home screen and the lock screen is simply one
			// of the things that can be looked at from there.
			if (IsDeviceLocked() && !m_bAuthor)
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
		m_bOnForm = false;
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

				PaintBadge(button, m_Content.UnreadCount(m_aApps[i]));
			}
			else
			{
				button.SetText("");
				SetTileIcon(button, "");
				PaintBadge(button, 0);
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

	//! The red pill on a tile, and the number in it.
	//!
	//! Hidden at zero rather than drawn empty: a badge that is always there
	//! stops meaning anything. Above nine it says 9+, because the pill is
	//! nineteen pixels across and a phone has never shown you 47.
	//! Re-reads the dots on the rows that are already on screen.
	//!
	//! Called when something has just been read, so the list behind the entry
	//! is already correct when the player steps back to it -- rebuilding the
	//! list instead would lose the scroll position, which on a phone is the
	//! difference between "I was here" and "where was I".
	protected void RefreshRowDots()
	{
		foreach (int i, SCR_ButtonTextComponent rowButton : m_aRowButtons)
		{
			if (!rowButton || i >= m_aVisible.Count())
				continue;

			Widget row = rowButton.GetRootWidget();
			if (!row)
				continue;

			Widget dot = row.FindAnyWidget(W_UNREAD_DOT);
			if (dot)
				dot.SetVisible(m_Content.IsUnread(m_aVisible[i]));
		}
	}

	protected void PaintBadge(notnull SCR_ButtonTextComponent button, int count)
	{
		Widget tileRoot = button.GetRootWidget();
		if (!tileRoot)
			return;

		Widget badge = tileRoot.FindAnyWidget(W_BADGE);
		TextWidget badgeText = TextWidget.Cast(tileRoot.FindAnyWidget(W_BADGE_TEXT));

		bool show = count > 0;

		if (badge)
			badge.SetVisible(show);

		if (!badgeText)
			return;

		badgeText.SetVisible(show);

		if (!show)
			return;

		if (count > 9)
			badgeText.SetText("9+");
		else
			badgeText.SetText(count.ToString());
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

			PaintBadge(dockButton, m_Content.UnreadCount(m_aApps[i]));
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

		// The call log is not a list on a phone -- it is the dialler, with what
		// was last rung sitting above the keys. A Game Master editing this app
		// still gets the list: you cannot edit a row you cannot see.
		if (app.m_eKind == MCF_EIntelApp.CALLS && !m_bAuthor)
		{
			ShowDialler(app);
			return;
		}

		// Photographs are their own thumbnails, so the photo app opens as a
		// grid rather than as rows. Same exception for the Game Master: the
		// list is the only place a picture's fields can be edited.
		if (app.m_eKind == MCF_EIntelApp.PHOTOS && !m_bAuthor)
		{
			ShowPhotoGrid(app);
			return;
		}

		m_bOnHome = false;
		m_bOnList = true;
		m_bOnForm = false;
		m_OpenApp = app;
		m_iOpenEntry = -1;
		StopWaitingForPicture();

		SetScreens(false, true, false);

		m_aRowButtons.Clear();
		m_aAuthorButtons.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		// Ask what is in there, then draw it. The presenter decides whether an
		// empty app has anything to say for itself; this only puts rows on the
		// screen.
		// AUTHORING ROWS FIRST. A Game Master reads this list top-down looking
		// for what they can change; a "new item" row at the bottom of a long
		// inbox is a row nobody finds.
		m_aAuthorKinds.Clear();

		if (m_bAuthor)
		{
			if (app.m_eKind == MCF_EIntelApp.SETTINGS)
				AddAuthorRow("Device name:  " + m_Content.DeviceName(), "device");

			AddAuthorRow("+  New item", "new");
		}

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

	// --------------------------------------------------------- the editor pane

	//! Opens the one-field editor over the phone.
	//!
	//! \param kind What is being edited, so SAVE knows where to put it back.
	//! \param item The item being edited, or null when it is the device itself.
	protected void ShowEditor(string kind, string title, string value, MCF_Device_Item item)
	{
		m_sEditKind = kind;
		m_EditItem = item;

		if (m_wEditorTitle)
			m_wEditorTitle.SetText(title);

		if (m_EditorField)
			m_EditorField.SetText(value);

		if (m_wEditor)
			m_wEditor.SetVisible(true);

		// Put the cursor in the field. A Game Master who has to click the box
		// before typing will click the phone behind it half the time.
		if (m_EditorField)
		{
			GetGame().GetWorkspace().SetFocusedWidget(m_EditorField);
		}
	}

	protected void HideEditor()
	{
		m_sEditKind = "";
		m_EditItem = null;

		if (m_wEditor)
			m_wEditor.SetVisible(false);
	}

	protected void OnEditorCancel(SCR_ButtonTextComponent button)
	{
		HideEditor();
	}

	//! Writes the typed value into the draft and sends the whole draft.
	//!
	//! ONE EDIT, ONE SEND. The old panel collected changes and pushed them all
	//! on APPLY, which meant a Game Master could lose ten minutes of typing by
	//! closing the wrong window. A field at a time costs one small RPC and
	//! cannot lose anything.
	protected void OnEditorSave(SCR_ButtonTextComponent button)
	{
		if (!m_EditorField || !m_Draft)
		{
			HideEditor();
			return;
		}

		string typed = MCF_Device_Script.Trim(m_EditorField.GetText());
		bool intelSide;

		if (m_sEditKind == "device")
			m_Draft.m_sDeviceName = typed;
		else if (m_sEditKind == "verb")
			intelSide = true;
		else if (m_sEditKind == "heading" && m_EditItem)
			m_EditItem.m_sHeading = typed;
		else if (m_sEditKind == "stamp" && m_EditItem)
			m_EditItem.m_sTimestamp = typed;
		else if (m_sEditKind == "body" && m_EditItem)
			m_EditItem.m_sBody = typed;
		else if (m_sEditKind == "image" && m_EditItem)
			m_EditItem.m_sImage = typed;
		else if (m_sEditKind == "url" && m_EditItem)
			m_EditItem.m_sImageUrl = typed;

		MCF_Device_Item edited = m_EditItem;
		HideEditor();

		// The prompt verb is not part of the device profile: it is the word the
		// world action says, and it lives on the carrier with the object's own
		// intel. So it goes over the intel route instead -- two payloads,
		// because they are genuinely two pieces of state.
		if (intelSide)
			SendVerb(typed);
		else
			SendDraft();

		// Redraw whatever is behind the pane, so the change is visible where it
		// was made rather than only after a trip to the home screen.
		if (m_bOnForm && edited)
			ShowItemForm(edited);
		else if (m_bOnForm)
			ShowDeviceForm();
		else if (m_bOnList && m_OpenApp)
			ShowList(m_OpenApp);
		else
			ShowHome();
	}

	//! The word the world action uses ("Search", "Read"), over the intel route.
	protected void SendVerb(string verb)
	{
		if (!m_Carrier)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_Editable)
		{
			SetHint("Cannot reach the server.");
			return;
		}

		RplId targetId = Replication.FindItemId(m_Editable);
		if (targetId == RplId.Invalid())
			return;

		// The intel payload is device name, view, verb, then the pages -- the
		// same shape MCF_Intel_EditorMenu sends. Only the verb changes here, so
		// everything else is read back off the carrier untouched.
		array<MCF_Intel_Entry> entries = {};
		m_Carrier.GetEntries(entries);

		string payload = m_Carrier.GetDeviceName() + "<<f>>" + m_Carrier.GetView().ToString() + "<<f>>" + verb;

		foreach (MCF_Intel_Entry entry : entries)
		{
			payload = payload + "<<e>>" + entry.m_sHeading + "<<f>>" + entry.m_sTimestamp + "<<f>>" + entry.m_sBody + "<<f>>" + entry.m_eApp.ToString();
		}

		controller.MCF_RequestEditIntelObject(targetId, payload);
		SetHint("Saved.");
	}

	//! Sends the draft to the server, which writes it onto the object and
	//! replicates it to everyone -- the same route the old editor used.
	protected void SendDraft()
	{
		if (!m_Draft)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !m_Editable)
		{
			SetHint("Cannot reach the server.");
			return;
		}

		RplId targetId = Replication.FindItemId(m_Editable);
		if (targetId == RplId.Invalid())
		{
			SetHint("That object is not replicated and cannot be edited.");
			return;
		}

		controller.MCF_RequestWriteDeviceProfile(targetId, MCF_Device_Script.Serialize(m_Draft));
		SetHint("Saved.");
	}

	protected void AddRow(notnull MCF_Device_Item entry)
	{
		if (!m_wEntryList)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(PHONE_ROW_LAYOUT, m_wEntryList);
		if (!row)
			return;

		SCR_ButtonTextComponent rowButton = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!rowButton)
			return;

		rowButton.m_OnClicked.Insert(OnRowClicked);
		m_aRowButtons.Insert(rowButton);

		FillRow(row, entry);
	}

	//! What one row says, in the terms of the app it is sitting in.
	//!
	//! THIS IS WHERE THE APPS STOP LOOKING ALIKE. The widgets are the same four
	//! -- a disc, a bright line, a dim line, something small on the right --
	//! but an inbox fills them with sender, preview and time; a phone book
	//! fills them with a name and nothing else; a call log puts the direction
	//! where the preview would be. Four apps out of one row, and none of them
	//! needs a layout of its own.
	protected void FillRow(notnull Widget row, notnull MCF_Device_Item entry)
	{
		int kind;
		if (m_OpenApp)
			kind = m_OpenApp.m_eKind;

		TextWidget line1 = TextWidget.Cast(row.FindAnyWidget("Line1"));
		RichTextWidget line2 = RichTextWidget.Cast(row.FindAnyWidget("Line2"));
		TextWidget right = TextWidget.Cast(row.FindAnyWidget("Right"));
		ImageWidget avatar = ImageWidget.Cast(row.FindAnyWidget("Avatar"));
		TextWidget avatarText = TextWidget.Cast(row.FindAnyWidget("AvatarText"));
		Widget dot = row.FindAnyWidget(W_UNREAD_DOT);

		string title = entry.m_sHeading;
		if (title.IsEmpty())
			title = "(no subject)";

		string second = entry.m_sBody;
		string trailing = entry.m_sTimestamp;
		bool showAvatar = true;

		if (kind == MCF_EIntelApp.MESSAGES)
		{
			// The sender, and only the sender. The time that mission makers
			// write after it ("M. - 02:14") belongs on the conversation, not
			// on a line that already has the date sitting at its right end.
			title = HeadPart(title);
		}
		else if (kind == MCF_EIntelApp.EMAIL)
		{
			// An inbox lists subjects. The sender goes on the card, where
			// there is a labelled line waiting for it.
			title = SubjectOf(entry.m_sHeading);
		}
		else if (kind == MCF_EIntelApp.CONTACTS)
		{
			// A phone book is names AGAINST NUMBERS, and it is the number that
			// makes a row read as a contact rather than as a heading with a
			// circle next to it. What is written under a contact is a note to
			// whoever owned the phone and belongs on the card; nothing sits on
			// the right, where a message list would put a time.
			second = entry.m_sTimestamp;
			trailing = "";
		}
		else if (kind == MCF_EIntelApp.SETTINGS || kind == MCF_EIntelApp.FILES)
		{
			// Settings and files are not people.
			showAvatar = false;
		}
		else if (kind == MCF_EIntelApp.NOTES)
		{
			showAvatar = false;
		}

		if (line1)
			line1.SetText(title);

		if (line2)
		{
			line2.SetVisible(!second.IsEmpty());
			line2.SetText(Preview(second));
		}

		if (right)
			right.SetText(trailing);

		// NOT SetVisible(false) -- see the note in MCF_Desktop_ShellMenu. The
		// disc is what gives the row its height, a hidden widget contributes
		// nothing to a layout, and the two text lines end up on top of each
		// other in notes, files and settings. Transparent keeps the space.
		if (avatar)
		{
			avatar.SetVisible(true);

			if (showAvatar)
				avatar.SetColor(Color.FromInt(AvatarColour(title)));
			else
				avatar.SetColor(Color.FromInt(0x00000000));
		}

		if (avatarText)
		{
			avatarText.SetVisible(showAvatar);

			if (showAvatar)
				avatarText.SetText(Initial(title));
		}

		// Without an avatar the disc's column would sit empty. The text and the
		// rule move left together: a rule that stops short of text it is not
		// separating reads as a mistake.
		//
		// AlignableSlot padding, not FrameSlot: everything in this row is
		// placed by alignment, and mixing the two is how a widget ends up at
		// coordinates its parent does not use.
		if (!showAvatar)
		{
			if (line1)
				AlignableSlot.SetPadding(line1, 14, 1, 70, 0);

			if (line2)
				AlignableSlot.SetPadding(line2, 14, 0, 40, 1);

			Widget divider = row.FindAnyWidget("Divider");
			if (divider)
				AlignableSlot.SetPadding(divider, 14, 0, 0, -9);
		}

		if (dot)
			dot.SetVisible(m_Content.IsUnread(entry));
	}

	//! One line of the body, short enough to sit under a sender.
	protected string Preview(string body)
	{
		string flat = body;
		flat.Replace("\\n", " ");
		flat.Replace("\n", " ");

		if (flat.Length() > 52)
			return flat.Substring(0, 52) + "...";

		return flat;
	}

	//! The letter on the disc. Digits and punctuation get a dot rather than a
	//! number, because "0" as a face reads as an error.
	protected string Initial(string name)
	{
		if (name.IsEmpty())
			return "?";

		string first = name.Get(0);
		int code = first.ToAscii();

		// ToUpper MUTATES AND RETURNS AN INT -- see the Enfusion lessons. Call
		// it for its effect and hand back the string itself.
		if (code >= 97 && code <= 122)
		{
			first.ToUpper();
			return first;
		}

		if (code >= 65 && code <= 90)
			return first;

		return "-";
	}

	//! A colour that belongs to the name, so the same sender is the same disc
	//! every time -- which is most of what makes a list of people scannable.
	protected int AvatarColour(string name)
	{
		int seed = 11;
		int count = name.Length();
		for (int i = 0; i < count; i++)
		{
			seed = (seed * 31 + name.Get(i).ToAscii()) % 9973;
		}

		int index = seed % 6;

		if (index == 0)
			return 0xFF8C4A3C;

		if (index == 1)
			return 0xFF3E6E52;

		if (index == 2)
			return 0xFF3A5A78;

		if (index == 3)
			return 0xFF6A5A8C;

		if (index == 4)
			return 0xFF8A6A2E;

		return 0xFF4C5560;
	}

	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aRowButtons.Find(button);
		if (index < 0)
			return;

		// In author mode a row is a thing to change, not a thing to read.
		if (m_bAuthor)
		{
			ShowItemForm(m_aVisible[index]);
			return;
		}

		ShowEntry(index);
	}

	// ---------------------------------------------------------- author forms

	//! Everything about one item, as a list of rows on the phone itself.
	//!
	//! WHY ROWS AND NOT A FORM WITH SIX BOXES. Six labelled boxes need a screen
	//! twice this wide; this glass is 250 pixels across. A list of
	//! "label — value" rows is a form that fits, reads top to bottom, and uses
	//! the same row widget everything else on the phone already uses. Tapping a
	//! row opens the one field it holds; a yes/no row flips where it stands.
	protected void ShowItemForm(MCF_Device_Item item)
	{
		if (!item)
			return;

		m_FormItem = item;
		m_bOnForm = true;
		m_bOnList = false;

		SetScreens(false, true, false);

		m_aRowButtons.Clear();
		m_aAuthorButtons.Clear();
		m_aAuthorKinds.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		if (m_wDeviceName)
			m_wDeviceName.SetText("Edit item");

		AddAuthorRow("Heading:  " + Shown(item.m_sHeading), "f.heading");
		AddAuthorRow("Timestamp:  " + Shown(item.m_sTimestamp), "f.stamp");
		AddAuthorRow("Message:  " + Shown(item.m_sBody), "f.body");
		AddAuthorRow("Shows as new:  " + YesNo(item.m_bNew), "f.new");
		AddAuthorRow("Picture (addon):  " + Shown(item.m_sImage), "f.image");
		AddAuthorRow("Picture (url):  " + Shown(item.m_sImageUrl), "f.url");
		AddAuthorRow("Delete this item", "f.delete");
		AddAuthorRow("< Back to the list", "f.back");

		SetHint("Editing this device only.");
	}

	//! The device's own settings, same shape.
	protected void ShowDeviceForm()
	{
		m_bOnForm = true;
		m_bOnList = false;
		m_FormItem = null;

		SetScreens(false, true, false);

		m_aRowButtons.Clear();
		m_aAuthorButtons.Clear();
		m_aAuthorKinds.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		if (m_wDeviceName)
			m_wDeviceName.SetText("Device");

		AddAuthorRow("Name:  " + Shown(m_Content.DeviceName()), "d.name");
		AddAuthorRow("Prompt verb:  " + Shown(CarrierVerb()), "d.verb");
		AddAuthorRow("< Back", "f.back");

		SetHint("Editing this device only.");
	}

	protected string Shown(string value)
	{
		if (value.IsEmpty())
			return "(empty)";

		if (value.Length() > 28)
			return value.Substring(0, 28) + "...";

		return value;
	}

	protected string YesNo(bool value)
	{
		if (value)
			return "YES";

		return "NO";
	}

	protected string CarrierVerb()
	{
		if (m_Carrier)
			return m_Carrier.GetActionVerb();

		return "";
	}

	//! A row that does something to the device rather than showing what is on
	//! it. Only ever added in author mode.
	protected void AddAuthorRow(string label, string kind)
	{
		if (!m_wEntryList)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wEntryList);
		if (!row)
			return;

		SCR_ButtonTextComponent rowButton = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!rowButton)
			return;

		rowButton.SetText(label);
		rowButton.m_OnClicked.Insert(OnAuthorRowClicked);
		m_aAuthorButtons.Insert(rowButton);
		m_aAuthorKinds.Insert(kind);
	}

	protected void OnAuthorRowClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aAuthorButtons.Find(button);
		if (index < 0 || index >= m_aAuthorKinds.Count())
			return;

		string kind = m_aAuthorKinds[index];

		// ---- the list's own rows
		if (kind == "device")
		{
			ShowDeviceForm();
			return;
		}

		if (kind == "new" && m_OpenApp)
		{
			MCF_Device_Item fresh = new MCF_Device_Item();
			fresh.m_sHeading = "New item";
			fresh.m_bNew = true;

			if (!m_OpenApp.m_aItems)
				m_OpenApp.m_aItems = {};

			m_OpenApp.m_aItems.Insert(fresh);
			SendDraft();
			ShowItemForm(fresh);
			return;
		}

		// ---- the device form
		if (kind == "d.name")
		{
			ShowEditor("device", "Device name", m_Content.DeviceName(), null);
			return;
		}

		if (kind == "d.verb")
		{
			ShowEditor("verb", "Prompt verb", CarrierVerb(), null);
			return;
		}

		// ---- one item's form
		if (kind == "f.back")
		{
			m_bOnForm = false;
			m_FormItem = null;

			if (m_OpenApp)
				ShowList(m_OpenApp);
			else
				ShowHome();

			return;
		}

		if (!m_FormItem)
			return;

		if (kind == "f.heading")
		{
			ShowEditor("heading", "Heading", m_FormItem.m_sHeading, m_FormItem);
			return;
		}

		if (kind == "f.stamp")
		{
			ShowEditor("stamp", "Timestamp", m_FormItem.m_sTimestamp, m_FormItem);
			return;
		}

		if (kind == "f.body")
		{
			ShowEditor("body", "Message", m_FormItem.m_sBody, m_FormItem);
			return;
		}

		if (kind == "f.image")
		{
			ShowEditor("image", "Picture, addon resource", m_FormItem.m_sImage, m_FormItem);
			return;
		}

		if (kind == "f.url")
		{
			ShowEditor("url", "Picture, base64 url", m_FormItem.m_sImageUrl, m_FormItem);
			return;
		}

		// A yes/no flips where it stands rather than opening anything: there is
		// nothing to type and a keyboard for two states is a screen too many.
		if (kind == "f.new")
		{
			m_FormItem.m_bNew = !m_FormItem.m_bNew;
			SendDraft();
			ShowItemForm(m_FormItem);
			return;
		}

		if (kind == "f.delete")
		{
			if (m_OpenApp && m_OpenApp.m_aItems)
			{
				int at = m_OpenApp.m_aItems.Find(m_FormItem);
				if (at >= 0)
					m_OpenApp.m_aItems.Remove(at);
			}

			m_FormItem = null;
			m_bOnForm = false;
			SendDraft();

			if (m_OpenApp)
				ShowList(m_OpenApp);

			return;
		}
	}


	// ---------------------------------------------------------- one entry

	protected void ShowEntry(int index)
	{
		if (index < 0 || index >= m_aVisible.Count())
			return;

		// Opening it is what reads it. Not hovering, not scrolling past -- and
		// only on this machine: see MCF_Device_ReadState for why the server is
		// deliberately not told.
		if (m_Content.MarkRead(m_aVisible[index]))
			RefreshRowDots();


		m_bOnHome = false;
		m_bOnList = false;

		// WHICH READER, BY APP. The plain heading-date-body pane below is the
		// right answer for a note, a file and a setting -- they are documents
		// and nothing else. A message is a conversation, a mail is a letter
		// with a header, and a contact is a card: three screens that exist
		// because a device where all of them look the same is a device the
		// player cannot read at a glance.
		int readerKind;
		if (m_OpenApp)
			readerKind = m_OpenApp.m_eKind;

		if (readerKind == MCF_EIntelApp.MESSAGES)
		{
			ShowChat(index);
			return;
		}

		if (readerKind == MCF_EIntelApp.EMAIL)
		{
			ShowMail(index);
			return;
		}

		if (readerKind == MCF_EIntelApp.CONTACTS)
		{
			ShowContact(index);
			return;
		}

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
			m_wReadBody.SetText(MCF_Device_Text.Body(entry.m_sBody));

		ShowPicture(entry);

		if (m_ButtonBack)
			m_ButtonBack.SetText("BACK");

		UpdatePageButtons();
		UpdateLogButton();

		// After UpdateLogButton, which shows the LOG button unconditionally --
		// a Game Master writing the page has nothing to file at the board.
		FillPaperAuthor();
		ShowPaperAuthor();
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

	// ------------------------------------------------- the per-app detail views

	//! Binds the three detail panes and the photo grid.
	//!
	//! Bound while the layout still has them visible, same as the pad and the
	//! dialler, then taken down by HidePanes.
	protected void BindPanes(notnull Widget root)
	{
		m_wChatPane = root.FindAnyWidget(W_CHAT_PANE);
		m_wChatColumn = root.FindAnyWidget(W_CHAT_COLUMN);
		m_wMailPane = root.FindAnyWidget(W_MAIL_PANE);
		m_wContactPane = root.FindAnyWidget(W_CONTACT_PANE);
		m_wPhotoGrid = root.FindAnyWidget(W_PHOTO_GRID);

		m_ContactCall = SCR_ButtonTextComponent.GetButtonText("ContactCall", root);
		if (m_ContactCall)
			m_ContactCall.m_OnClicked.Insert(OnContactCallClicked);

		m_aPhotoTiles.Clear();

		for (int t = 0; t < PHOTO_TILES; t++)
		{
			SCR_ButtonTextComponent tile = SCR_ButtonTextComponent.GetButtonText("PhotoTile" + t.ToString(), root);
			if (!tile)
				continue;

			tile.m_OnClicked.Insert(OnPhotoTileClicked);
			m_aPhotoTiles.Insert(tile);
		}

		HidePanes();
	}

	protected void HidePanes()
	{
		if (m_wChatPane)
			m_wChatPane.SetVisible(false);

		if (m_wMailPane)
			m_wMailPane.SetVisible(false);

		if (m_wContactPane)
			m_wContactPane.SetVisible(false);

		if (m_wPhotoGrid)
			m_wPhotoGrid.SetVisible(false);
	}

	//! The halves of a heading written "who - what".
	//!
	//! ONE SEPARATOR, EVERYWHERE, AND IT WAS ALREADY THERE. Mission makers were
	//! writing "M. - 02:14" and "Outgoing - 0412" into headings long before
	//! anything read them apart, because that is how a person writes a line like
	//! that. The shell reads it now: the left half is who, the right half is
	//! when or what about. A heading with no separator is all left half, which
	//! is why nothing authored before this changes how it looks.
	protected string HeadPart(string heading)
	{
		int at = heading.IndexOf(HEAD_SEP);
		if (at < 0)
			return heading;

		return heading.Substring(0, at);
	}

	protected string TailPart(string heading)
	{
		int at = heading.IndexOf(HEAD_SEP);
		if (at < 0)
			return "";

		int from = at + HEAD_SEP.Length();
		return heading.Substring(from, heading.Length() - from);
	}

	protected string SubjectOf(string heading)
	{
		string tail = TailPart(heading);
		if (tail.IsEmpty())
			return heading;

		return tail;
	}

	protected string SenderOf(string heading)
	{
		string head = HeadPart(heading);
		if (head.IsEmpty())
			return "(unknown sender)";

		return head;
	}

	//! The first app of a kind, or null. A profile is not stopped from carrying
	//! two contact books, but nothing on this device has a use for the second.
	protected MCF_Device_App FindAppOfKind(int kind)
	{
		array<MCF_Device_App> apps = {};
		m_Content.GetApps(apps);

		foreach (MCF_Device_App candidate : apps)
		{
			if (candidate.m_eKind == kind)
				return candidate;
		}

		return null;
	}

	// --------------------------------------------------------------- messages

	//! A message, read as the conversation it is.
	//!
	//! A sender across the top with their disc beside it, and the body as
	//! bubbles underneath. This is the app the whole device is judged on: it is
	//! the one every player opens first and the one they have seen a thousand
	//! times on a real handset, so a heading-and-paragraph reader here read as
	//! a mod menu no matter what the rest of the phone looked like.
	protected void ShowChat(int index)
	{
		MCF_Device_Item entry = m_aVisible[index];
		m_iOpenEntry = index;
		StopWaitingForPicture();

		SetScreens(false, false, false);

		if (m_wChatPane)
			m_wChatPane.SetVisible(true);

		Widget root = GetRootWidget();
		if (!root)
			return;

		string who = SenderOf(entry.m_sHeading);
		if (who.IsEmpty())
			who = "Unknown";

		ImageWidget disc = ImageWidget.Cast(root.FindAnyWidget("ChatAvatar"));
		if (disc)
			disc.SetColor(Color.FromInt(AvatarColour(who)));

		TextWidget initial = TextWidget.Cast(root.FindAnyWidget("ChatInitial"));
		if (initial)
			initial.SetText(Initial(who));

		TextWidget name = TextWidget.Cast(root.FindAnyWidget("ChatName"));
		if (name)
			name.SetText(who);

		TextWidget sub = TextWidget.Cast(root.FindAnyWidget("ChatSub"));
		if (sub)
			sub.SetText(WhenLine(entry));

		DrawBubbles(entry);

		if (m_wDeviceName)
			m_wDeviceName.SetVisible(false);

		if (m_ButtonBack)
			m_ButtonBack.SetText("BACK");

		UpdatePageButtons();
		UpdateLogButton();
		SetHint("");
	}

	//! The time under a sender: whatever the heading said after the separator,
	//! then the date. Either half may be missing and the line still reads.
	protected string WhenLine(notnull MCF_Device_Item entry)
	{
		string clock = TailPart(entry.m_sHeading);
		string day = entry.m_sTimestamp;

		if (clock.IsEmpty())
			return day;

		if (day.IsEmpty())
			return clock;

		return day + "   " + clock;
	}

	//! One message becomes a thread.
	//!
	//! THE AUTHORING CONVENTION IS ONE CHARACTER. Every line of the body is a
	//! bubble; a line starting with ">" is from whoever owns the phone and sits
	//! on the right, everything else came from the other end and sits on the
	//! left. That is the entire format. A mission maker who writes a
	//! conversation gets a conversation, and one who writes a paragraph gets a
	//! paragraph in a single bubble -- which is also correct, and is why every
	//! message authored before this still reads properly.
	protected void DrawBubbles(notnull MCF_Device_Item entry)
	{
		if (!m_wChatColumn)
			return;

		ClearChildren(m_wChatColumn);

		// A config file that carried the newline through as two characters
		// rather than one would otherwise put "\n" in the middle of a bubble.
		// Harmless when the parser already did the right thing.
		string said = MCF_Device_Text.Body(entry.m_sBody);

		array<string> lines = {};
		said.Split("\n", lines, true);

		foreach (string line : lines)
		{
			string trimmed = MCF_Device_Script.Trim(line);
			if (trimmed.IsEmpty())
				continue;

			bool mine = trimmed.StartsWith(OUTGOING_MARK);

			if (mine)
			{
				trimmed = trimmed.Substring(1, trimmed.Length() - 1);
				trimmed = MCF_Device_Script.Trim(trimmed);
			}

			if (trimmed.IsEmpty())
				continue;

			AddBubble(trimmed, mine);
		}
	}

	protected void AddBubble(string said, bool mine)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets(BUBBLE_LAYOUT, m_wChatColumn);
		if (!row)
			return;

		Widget left = row.FindAnyWidget("Left");
		Widget right = row.FindAnyWidget("Right");

		if (left)
			left.SetVisible(!mine);

		if (right)
			right.SetVisible(mine);

		RichTextWidget target;

		if (mine)
			target = RichTextWidget.Cast(row.FindAnyWidget("RightText"));
		else
			target = RichTextWidget.Cast(row.FindAnyWidget("LeftText"));

		if (target)
			target.SetText(said);
	}

	// ------------------------------------------------------------------- mail

	//! A mail, with the header a mail has.
	//!
	//! From, date and subject in a card, then the body in a reading column
	//! under it. The difference between this and the chat screen is the whole
	//! point of having two: a message is a conversation and a mail is a
	//! document, and a device where both look the same is a device where the
	//! player cannot tell at a glance which one they are holding.
	protected void ShowMail(int index)
	{
		MCF_Device_Item entry = m_aVisible[index];
		m_iOpenEntry = index;
		StopWaitingForPicture();

		SetScreens(false, false, false);

		if (m_wMailPane)
			m_wMailPane.SetVisible(true);

		Widget root = GetRootWidget();
		if (!root)
			return;

		TextWidget label = TextWidget.Cast(root.FindAnyWidget("MailFromLabel"));
		if (label)
			label.SetText("FROM");

		TextWidget from = TextWidget.Cast(root.FindAnyWidget("MailFrom"));
		if (from)
			from.SetText(SenderOf(entry.m_sHeading));

		TextWidget when = TextWidget.Cast(root.FindAnyWidget("MailDate"));
		if (when)
			when.SetText(entry.m_sTimestamp);

		RichTextWidget subject = RichTextWidget.Cast(root.FindAnyWidget("MailSubject"));
		if (subject)
			subject.SetText(SubjectOf(entry.m_sHeading));

		RichTextWidget body = RichTextWidget.Cast(root.FindAnyWidget("MailBody"));
		if (body)
			body.SetText(MCF_Device_Text.Body(entry.m_sBody));

		if (m_wDeviceName)
			m_wDeviceName.SetVisible(false);

		if (m_ButtonBack)
			m_ButtonBack.SetText("BACK");

		UpdatePageButtons();
		UpdateLogButton();
		SetHint("");
	}

	// --------------------------------------------------------------- contacts

	//! One person, on a card.
	//!
	//! The disc that was 40 units in the list is most of the top of the screen
	//! here, because that is the only thing a contact card has to be: a face,
	//! a name, a number you can act on, and whatever the owner of the phone
	//! wrote about them.
	protected void ShowContact(int index)
	{
		MCF_Device_Item entry = m_aVisible[index];
		m_iOpenEntry = index;
		StopWaitingForPicture();

		SetScreens(false, false, false);

		if (m_wContactPane)
			m_wContactPane.SetVisible(true);

		Widget root = GetRootWidget();
		if (!root)
			return;

		string who = entry.m_sHeading;
		if (who.IsEmpty())
			who = "Unnamed";

		ImageWidget disc = ImageWidget.Cast(root.FindAnyWidget("ContactAvatar"));
		if (disc)
			disc.SetColor(Color.FromInt(AvatarColour(who)));

		TextWidget initial = TextWidget.Cast(root.FindAnyWidget("ContactInitial"));
		if (initial)
			initial.SetText(Initial(who));

		TextWidget name = TextWidget.Cast(root.FindAnyWidget("ContactName"));
		if (name)
			name.SetText(who);

		TextWidget number = TextWidget.Cast(root.FindAnyWidget("ContactNumber"));
		if (number)
		{
			if (entry.m_sTimestamp.IsEmpty())
				number.SetText("No number saved");
			else
				number.SetText(entry.m_sTimestamp);
		}

		TextWidget noteLabel = TextWidget.Cast(root.FindAnyWidget("ContactNoteLabel"));
		if (noteLabel)
		{
			if (entry.m_sBody.IsEmpty())
				noteLabel.SetText("");
			else
				noteLabel.SetText("NOTE");
		}

		RichTextWidget note = RichTextWidget.Cast(root.FindAnyWidget("ContactNote"));
		if (note)
			note.SetText(MCF_Device_Text.Body(entry.m_sBody));

		// A CALL button on a contact with no number saved is a control that
		// cannot do anything, and a phone that offers it is lying about what it
		// knows -- which is the one thing a piece of evidence must not do.
		if (m_ContactCall)
			m_ContactCall.GetRootWidget().SetVisible(!entry.m_sTimestamp.IsEmpty());

		if (m_wDeviceName)
			m_wDeviceName.SetVisible(false);

		if (m_ButtonBack)
			m_ButtonBack.SetText("BACK");

		UpdatePageButtons();
		UpdateLogButton();
		SetHint("");
	}

	//! CALL on a card does not ring anybody: it puts the number on the dialler,
	//! which is the screen the player would have reached typing it themselves --
	//! with the name already sitting under it, which is the answer they wanted.
	protected void OnContactCallClicked(SCR_ButtonTextComponent button)
	{
		if (m_iOpenEntry < 0 || m_iOpenEntry >= m_aVisible.Count())
			return;

		string number = Digits(m_aVisible[m_iOpenEntry].m_sTimestamp);
		if (number.IsEmpty())
			return;

		MCF_Device_App calls = FindAppOfKind(MCF_EIntelApp.CALLS);
		if (!calls)
			return;

		ShowDialler(calls);

		m_sDialled = number;
		DrawDialled();
	}

	// ----------------------------------------------------------------- photos

	//! The photo app, as a grid.
	//!
	//! WHY THIS ONE IS NOT A LIST EITHER. A photograph's thumbnail is its own
	//! title. A row reading "Truck at the mill" with a coloured disc beside it
	//! tells the player strictly less than the picture does, and photos are the
	//! only app on this device where that is true.
	//!
	//! Twelve tiles is what the glass holds. A device carrying more says so at
	//! the bottom rather than dropping them silently -- an evidence device that
	//! hides evidence is worse than one that shows none.
	protected void ShowPhotoGrid(MCF_Device_App app)
	{
		m_bOnHome = false;
		m_bOnList = true;
		m_bOnForm = false;
		m_OpenApp = app;
		m_iOpenEntry = -1;
		StopWaitingForPicture();

		SetScreens(false, false, false);

		m_aRowButtons.Clear();
		m_aAuthorButtons.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		if (m_wPhotoGrid)
			m_wPhotoGrid.SetVisible(true);

		m_Content.GetItems(app, m_aVisible);

		Widget root = GetRootWidget();
		if (!root)
			return;

		for (int i = 0; i < PHOTO_TILES; i++)
		{
			SCR_ButtonTextComponent tile;
			if (i < m_aPhotoTiles.Count())
				tile = m_aPhotoTiles[i];

			if (!tile)
				continue;

			bool has = i < m_aVisible.Count();
			tile.GetRootWidget().SetVisible(has);

			if (!has)
				continue;

			MCF_Device_Item item = m_aVisible[i];

			ImageWidget shot = ImageWidget.Cast(root.FindAnyWidget("PhotoTileImage" + i.ToString()));
			TextWidget caption = TextWidget.Cast(root.FindAnyWidget("PhotoTileText" + i.ToString()));

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

			// A tile with nothing on it yet is not an empty tile: it is the one
			// worth opening, because opening an item is what asks for the
			// picture. Its heading stands in until the fetch lands.
			if (caption)
			{
				caption.SetVisible(!drawn);
				caption.SetText(item.m_sHeading);
			}
		}

		TextWidget hint = TextWidget.Cast(root.FindAnyWidget("PhotoGridHint"));
		if (hint)
		{
			if (m_aVisible.IsEmpty())
				hint.SetText(app.ResolveEmptyText());
			else if (m_aVisible.Count() > PHOTO_TILES)
				hint.SetText((m_aVisible.Count() - PHOTO_TILES).ToString() + " more not shown");
			else
				hint.SetText("");
		}

		if (m_wDeviceName)
		{
			m_wDeviceName.SetVisible(true);
			m_wDeviceName.SetText(app.ResolveLabel());
		}

		if (m_ButtonBack)
			m_ButtonBack.SetText("HOME");

		UpdateLogButton();
		SetHint("");
	}

	protected void OnPhotoTileClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aPhotoTiles.Find(button);
		if (index < 0)
			return;

		ShowEntry(index);
	}

	// ------------------------------------------------------------ the dialler

	//! Binds the dialler's keys.
	//!
	//! Called while the layout still has the pane marked visible, for the same
	//! reason the passcode pad is: GetButtonText does not walk into a subtree
	//! the layout has hidden, and twelve keys bound to nothing is a screen that
	//! looks right and does not work.
	protected void BindDialler(notnull Widget root)
	{
		m_wDialler = root.FindAnyWidget(W_DIALLER);
		m_wDialNumber = TextWidget.Cast(root.FindAnyWidget(W_DIAL_NUMBER));
		m_wDialSub = TextWidget.Cast(root.FindAnyWidget(W_DIAL_SUB));

		m_aDialKeys.Clear();

		// INDEX IS THE DIGIT. The same trick the passcode pad uses: the handler
		// asks the array where the button sits rather than reading its caption,
		// so a key's face and a key's value cannot drift apart.
		for (int k = 0; k < 10; k++)
		{
			SCR_ButtonTextComponent digit = SCR_ButtonTextComponent.GetButtonText(W_DIAL_KEY_PREFIX + k.ToString(), root);
			if (!digit)
				continue;

			digit.m_OnClicked.Insert(OnDialKeyClicked);
			m_aDialKeys.Insert(digit);
		}

		// Ten and eleven, in that order: star then hash, which is where a
		// handset puts them.
		SCR_ButtonTextComponent star = SCR_ButtonTextComponent.GetButtonText("DialKeyStar", root);
		if (star)
		{
			star.m_OnClicked.Insert(OnDialKeyClicked);
			m_aDialKeys.Insert(star);
		}

		SCR_ButtonTextComponent hash = SCR_ButtonTextComponent.GetButtonText("DialKeyHash", root);
		if (hash)
		{
			hash.m_OnClicked.Insert(OnDialKeyClicked);
			m_aDialKeys.Insert(hash);
		}

		SCR_ButtonTextComponent wipe = SCR_ButtonTextComponent.GetButtonText(W_DIAL_DELETE, root);
		if (wipe)
			wipe.m_OnClicked.Insert(OnDialDeleteClicked);

		SCR_ButtonTextComponent call = SCR_ButtonTextComponent.GetButtonText(W_DIAL_CALL, root);
		if (call)
			call.m_OnClicked.Insert(OnDialCallClicked);

		if (m_wDialler)
			m_wDialler.SetVisible(false);
	}

	//! The call app, as a phone actually presents it.
	//!
	//! WHY THIS APP IS NOT A LIST. Every other app on this device is a list of
	//! things somebody wrote down; the phone app is a machine you operate.
	//! Opening it onto rows reading "Outgoing 14:22" was the last screen on the
	//! handset that still read as a mod menu. The keys are what a player expects
	//! to see, and the last three calls are what they expect above them.
	protected void ShowDialler(MCF_Device_App app)
	{
		m_bOnHome = false;
		m_bOnList = false;
		m_bOnForm = false;
		m_OpenApp = app;
		m_iOpenEntry = -1;
		StopWaitingForPicture();

		// No list, no reader, no home. SetScreens takes the dialler down along
		// with everything else, so the flag and the pane both go back up after.
		SetScreens(false, false, false);

		m_aRowButtons.Clear();
		m_aAuthorButtons.Clear();

		if (m_wEntryList)
			ClearChildren(m_wEntryList);

		if (m_wDialler)
			m_wDialler.SetVisible(true);

		m_bOnDialler = true;

		if (m_wDeviceName)
		{
			m_wDeviceName.SetVisible(true);
			m_wDeviceName.SetText(app.ResolveLabel());
		}

		if (m_ButtonBack)
			m_ButtonBack.SetText("HOME");

		UpdateLogButton();

		m_sDialled = "";
		DrawDialled();
		DrawRecents(app);

		SetHint("");
	}

	//! The last few calls, newest first: who, and when. Rows with nothing to
	//! say are hidden rather than left blank -- three empty rules across the
	//! top of a dialler read as a screen that failed to load, and the label
	//! goes with them when there is no log at all.
	protected void DrawRecents(MCF_Device_App app)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		array<ref MCF_Device_Item> items = {};
		if (app)
			m_Content.GetItems(app, items);

		for (int i = 0; i < DIAL_RECENTS; i++)
		{
			TextWidget name = TextWidget.Cast(root.FindAnyWidget("DialRecentName" + i.ToString()));
			TextWidget meta = TextWidget.Cast(root.FindAnyWidget("DialRecentMeta" + i.ToString()));
			Widget rule = root.FindAnyWidget("DialRecentLine" + i.ToString());

			bool has = i < items.Count();

			if (name)
			{
				name.SetVisible(has);

				if (has)
					name.SetText(RecentWho(items[i]));
			}

			if (meta)
			{
				meta.SetVisible(has);

				if (has)
					meta.SetText(RecentMeta(items[i]));
			}

			if (rule)
				rule.SetVisible(has);
		}

		Widget label = root.FindAnyWidget(W_DIAL_RECENT_LABEL);
		if (label)
			label.SetVisible(!items.IsEmpty());
	}

	//! The right-hand half of a call row.
	//!
	//! A log that only says 14:22 is a clock. What makes it a call log is which
	//! way the call went, which is whatever the mission maker wrote in the body
	//! -- "Outgoing", "Missed", "12 min". Kept short: this sits in a third of
	//! the width of a handset.
	protected string RecentMeta(notnull MCF_Device_Item item)
	{
		string way = HeadPart(item.m_sHeading);
		string clock = TailPart(item.m_sHeading);

		if (clock.IsEmpty())
			return way;

		return way + "   " + clock;
	}

	//! Who the call was with. A call log entry names the direction in its
	//! heading and the person in its body ("M. - 4 min 12 s."), which is the
	//! wrong way round for a dialler: the name is what a player scans for and
	//! the direction is what they check afterwards.
	protected string RecentWho(notnull MCF_Device_Item item)
	{
		string who = HeadPart(item.m_sBody);

		if (who.IsEmpty())
			who = item.m_sHeading;

		if (who.Length() > 20)
			who = who.Substring(0, 20);

		return who;
	}

	//! What has been typed, and who it belongs to if the phone book knows.
	protected void DrawDialled()
	{
		if (m_wDialNumber)
			m_wDialNumber.SetText(m_sDialled);

		if (!m_wDialSub)
			return;

		if (m_sDialled.IsEmpty())
		{
			m_wDialSub.SetText("Enter a number");
			return;
		}

		string match = MatchContact(m_sDialled);

		if (match.IsEmpty())
			m_wDialSub.SetText("Not in contacts");
		else
			m_wDialSub.SetText(match);
	}

	//! The phone book, read the way a dialler reads it: a number that has a
	//! name gets the name.
	//!
	//! EXACT MATCH ONLY, and on digits alone so that a contact written as
	//! "555 0148" still answers to 5550148. A half-typed number that guesses at
	//! a contact is a phone lying about who is being rung, and this device is a
	//! piece of evidence before it is a convenience.
	protected string MatchContact(string number)
	{
		MCF_Device_App book = FindAppOfKind(MCF_EIntelApp.CONTACTS);
		if (!book)
			return "";

		array<ref MCF_Device_Item> people = {};
		m_Content.GetItems(book, people);

		foreach (MCF_Device_Item person : people)
		{
			if (Digits(person.m_sTimestamp) == number)
				return person.m_sHeading;
		}

		return "";
	}

	//! Everything in a written number that a keypad could have produced.
	protected string Digits(string text)
	{
		string kept;
		int n = text.Length();

		for (int i = 0; i < n; i++)
		{
			string ch = text.Substring(i, 1);

			if (ch == "0" || ch == "1" || ch == "2" || ch == "3" || ch == "4" || ch == "5" || ch == "6" || ch == "7" || ch == "8" || ch == "9")
				kept = kept + ch;
		}

		return kept;
	}

	protected void OnDialKeyClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bOnDialler || !button)
			return;

		if (m_sDialled.Length() >= DIAL_MAX_DIGITS)
			return;

		int slot = m_aDialKeys.Find(button);
		if (slot < 0)
			return;

		if (slot < 10)
			m_sDialled = m_sDialled + slot.ToString();
		else if (slot == 10)
			m_sDialled = m_sDialled + "*";
		else
			m_sDialled = m_sDialled + "#";

		DrawDialled();
	}

	protected void OnDialDeleteClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bOnDialler || m_sDialled.IsEmpty())
			return;

		m_sDialled = m_sDialled.Substring(0, m_sDialled.Length() - 1);
		DrawDialled();
	}

	//! CALL does not place one, and is not going to.
	//!
	//! There is no voice on the other end of a prop phone. What the button is
	//! for is answering the question the player actually has when they find a
	//! number on a scrap of paper: does this phone know whose it is. A name is
	//! the answer; anything else rings out.
	protected void OnDialCallClicked(SCR_ButtonTextComponent button)
	{
		if (!m_bOnDialler)
			return;

		if (m_sDialled.IsEmpty())
		{
			SetHint("No number.");
			return;
		}

		string match = MatchContact(m_sDialled);

		if (match.IsEmpty())
			SetHint("Calling " + m_sDialled + "  -  no answer.");
		else
			SetHint("Calling " + match + "  -  no answer.");
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

		DrawLockNotes();
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
	//! Notification cards on the lock screen: who wrote, and when.
	//!
	//! NEVER THE BODY. A locked phone that prints the message has handed over
	//! the reason to break into it -- the card is the hook, the break-in is the
	//! price, and putting the text on the lock screen turns the puzzle into a
	//! tax on something the player already has.
	protected void DrawLockNotes()
	{
		array<MCF_Device_Item> items = {};
		array<int> kinds = {};

		if (m_Content)
			m_Content.UnreadItems(LOCK_NOTES, items, kinds);

		for (int i = 0; i < LOCK_NOTES; i++)
		{
			bool used = i < items.Count();

			Widget card = FindLockNote(i, "Card");
			Widget icon = FindLockNote(i, "Icon");
			TextWidget title = TextWidget.Cast(FindLockNote(i, "Title"));
			TextWidget stamp = TextWidget.Cast(FindLockNote(i, "Stamp"));

			if (card)
				card.SetVisible(used);

			if (title)
				title.SetVisible(used);

			if (stamp)
				stamp.SetVisible(used);

			if (!used)
			{
				if (icon)
					icon.SetVisible(false);

				continue;
			}

			MCF_Device_Item item = items[i];

			if (title)
				title.SetText(item.m_sHeading);

			if (stamp)
				stamp.SetText(item.m_sTimestamp);

			ImageWidget picture = ImageWidget.Cast(icon);
			if (!picture)
				continue;

			string sprite = MCF_Device_Names.IconFor(kinds[i]);
			bool drawn;

			if (!sprite.IsEmpty())
				drawn = picture.LoadImageTexture(0, sprite);

			picture.SetVisible(drawn);
		}
	}

	protected Widget FindLockNote(int index, string part)
	{
		Widget root = GetRootWidget();
		if (!root)
			return null;

		return root.FindAnyWidget("Note" + index.ToString() + part);
	}

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

		// The dialler is an app screen like any other, so every other screen
		// takes it down. ShowDialler calls here first and puts it back after,
		// which is why this is unconditional rather than guarded like the pad.
		if (m_wDialler)
			m_wDialler.SetVisible(false);

		m_bOnDialler = false;

		// Same rule for the four detail panes: every screen change takes them
		// all down, and whichever one is wanted puts itself back up after.
		HidePanes();

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

		// BEFORE the list branch and before the m_OpenApp fallback below. The
		// dialler keeps m_OpenApp set, so falling through to ShowList would
		// route straight back into ShowDialler and the phone would never leave
		// the call app.
		if (m_bOnDialler)
		{
			ShowHome();
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

	// ================================ authoring on the page itself

	//! Binds the Game Master's half of a paper or notepad visual.
	//!
	//! THE EDITOR IS THE THING ITSELF. A Game Master rewriting a letter should
	//! be looking at the letter. The old form -- MCF_Intel_EditorMenu, a list
	//! and three fields -- still exists for the views that have no visual yet
	//! (DOCUMENT, MAP) and is not what these two open any more.
	protected void BindPaperAuthor(notnull Widget root)
	{
		m_wNameEdit = root.FindAnyWidget(W_NAME_EDIT);
		m_wHeadingEdit = root.FindAnyWidget(W_HEADING_EDIT);
		m_wStampEdit = root.FindAnyWidget(W_STAMP_EDIT);
		m_wBodyEdit = root.FindAnyWidget(W_BODY_EDIT);
		m_wAuthorNote = root.FindAnyWidget(W_AUTHOR_NOTE);

		m_ButtonType = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_TYPE, root);
		if (m_ButtonType)
			m_ButtonType.m_OnClicked.Insert(OnTypeClicked);

		m_ButtonPageView = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_PAGE, root);
		if (m_ButtonPageView)
			m_ButtonPageView.m_OnClicked.Insert(OnPageViewClicked);

		m_ButtonAddPage = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_ADD_PAGE, root);
		if (m_ButtonAddPage)
			m_ButtonAddPage.m_OnClicked.Insert(OnAddPageClicked);

		m_ButtonDropPage = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_DROP_PAGE, root);
		if (m_ButtonDropPage)
			m_ButtonDropPage.m_OnClicked.Insert(OnDropPageClicked);

		m_ButtonSaveIntel = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_SAVE_INTEL, root);
		if (m_ButtonSaveIntel)
			m_ButtonSaveIntel.m_OnClicked.Insert(OnSaveIntelClicked);
	}

	//! Which half of the page is showing.
	//!
	//! A PLAYER READS AND A GAME MASTER WRITES, and the same box holds both --
	//! the read-only widget and an edit box in exactly the same place, one of
	//! them shown. An edit box always there would let a player rewrite the
	//! evidence they were sent to find.
	protected void ShowPaperAuthor()
	{
		bool authoring = m_bAuthor && m_bPageMode;
		bool typing = authoring && m_bPaperTyping;

		ShowIf(m_wNameEdit, typing);
		ShowIf(m_wHeadingEdit, typing);
		ShowIf(m_wStampEdit, typing);
		ShowIf(m_wBodyEdit, typing);

		if (m_wDeviceName)
			m_wDeviceName.SetVisible(!typing);

		if (m_wReadHeading)
			m_wReadHeading.SetVisible(!typing);

		if (m_wReadTimestamp)
			m_wReadTimestamp.SetVisible(!typing);

		if (m_wReadScroll)
			m_wReadScroll.SetVisible(!typing);

		ShowIf(m_wAuthorNote, authoring);
		ShowButtonIf(m_ButtonType, authoring && !m_bPaperTyping);
		ShowButtonIf(m_ButtonPageView, authoring && m_bPaperTyping);
		ShowButtonIf(m_ButtonAddPage, authoring);
		ShowButtonIf(m_ButtonDropPage, authoring && m_aVisible.Count() > 1);
		ShowButtonIf(m_ButtonSaveIntel, authoring);

		// LOG is the player's verb -- it files what they read onto the
		// planning board. A Game Master writing the page has nothing to file.
		if (authoring && m_ButtonLog)
			m_ButtonLog.GetRootWidget().SetVisible(false);
	}

	protected void ShowIf(Widget found, bool visible)
	{
		if (found)
			found.SetVisible(visible);
	}

	protected void ShowButtonIf(SCR_ButtonTextComponent button, bool visible)
	{
		if (button)
			button.GetRootWidget().SetVisible(visible);
	}

	//! Fills the Game Master's boxes with the page that is open.
	protected void FillPaperAuthor()
	{
		if (!m_bAuthor || !m_bPageMode)
			return;

		SetBoxText(m_wNameEdit, m_Content.DeviceName());

		if (m_iOpenEntry < 0 || m_iOpenEntry >= m_aVisible.Count())
		{
			SetBoxText(m_wHeadingEdit, "");
			SetBoxText(m_wStampEdit, "");
			SetBoxText(m_wBodyEdit, "");
			return;
		}

		MCF_Device_Item page = m_aVisible[m_iOpenEntry];
		SetBoxText(m_wHeadingEdit, page.m_sHeading);
		SetBoxText(m_wStampEdit, page.m_sTimestamp);
		SetBoxText(m_wBodyEdit, MCF_Device_Text.Body(page.m_sBody));
	}

	//! Takes what was typed and puts it on the draft.
	//!
	//! CALLED BEFORE ANYTHING MOVES -- turning a page, adding one, saving. The
	//! boxes are the only place the new text exists until this runs, so a page
	//! turn without it loses everything typed since the last one.
	protected void CommitPaperPage()
	{
		if (!m_bAuthor || !m_bPageMode || !m_bPaperTyping)
			return;

		if (m_Draft)
		{
			string named = MCF_Device_Script.Trim(BoxText(m_wNameEdit));
			if (!named.IsEmpty())
				m_Draft.m_sDeviceName = named;
		}

		if (m_iOpenEntry < 0 || m_iOpenEntry >= m_aVisible.Count())
			return;

		MCF_Device_Item page = m_aVisible[m_iOpenEntry];
		page.m_sHeading = BoxText(m_wHeadingEdit);
		page.m_sTimestamp = BoxText(m_wStampEdit);

		// A typed newline has to survive being stored: MCF_Device_Script.Clean
		// turns a real one into a space, so it goes onto the draft as the two
		// characters a config carries and MCF_Device_Text.Body unescapes it
		// again on the way back to the page.
		page.m_sBody = MCF_Device_Text.Encode(BoxText(m_wBodyEdit));
	}

	//! The app the pages of a paper or notepad live in.
	//!
	//! One app, because a letter has no apps -- the shell is in page mode
	//! precisely because there is nothing to index. A draft that arrived with
	//! none gets one rather than refusing to take a page.
	protected MCF_Device_App PaperApp()
	{
		if (!m_Draft)
			return null;

		if (!m_Draft.m_aApps)
			m_Draft.m_aApps = {};

		if (m_Draft.m_aApps.IsEmpty())
		{
			MCF_Device_App fresh = new MCF_Device_App();
			fresh.m_eKind = MCF_EIntelApp.GENERAL;
			fresh.m_aItems = {};
			m_Draft.m_aApps.Insert(fresh);
		}

		MCF_Device_App app = m_Draft.m_aApps[0];
		if (!app.m_aItems)
			app.m_aItems = {};

		return app;
	}

	protected void OnTypeClicked(SCR_ButtonTextComponent button)
	{
		m_bPaperTyping = true;
		FillPaperAuthor();
		ShowPaperAuthor();

		// An edit box takes keystrokes only in write mode, and the engine
		// raises no event when it starts -- vanilla polls IsInWriteMode() and
		// calls ActivateWriteMode() from its own pencil. This is that pencil.
		FocusBox(m_wBodyEdit);
		SetHint("Typing. PAGE shows it as the player will see it.");
	}

	protected void OnPageViewClicked(SCR_ButtonTextComponent button)
	{
		CommitPaperPage();
		m_bPaperTyping = false;
		ShowPaperAuthor();
		ShowEntry(m_iOpenEntry);
		SetHint("This is the page. Nothing is on the object until SAVE.");
	}

	protected void OnAddPageClicked(SCR_ButtonTextComponent button)
	{
		CommitPaperPage();

		MCF_Device_App app = PaperApp();
		if (!app)
			return;

		MCF_Device_Item page = new MCF_Device_Item();
		page.m_sHeading = "New page";
		app.m_aItems.Insert(page);

		m_Content.GetAllItems(m_aVisible);
		m_bPaperTyping = true;
		ShowEntry(m_aVisible.Count() - 1);
		ShowPaperAuthor();
		FillPaperAuthor();
		FocusBox(m_wHeadingEdit);
		SetHint("A page was added. SAVE writes it to the object.");
	}

	protected void OnDropPageClicked(SCR_ButtonTextComponent button)
	{
		if (m_aVisible.Count() < 2 || m_iOpenEntry < 0)
			return;

		MCF_Device_App app = PaperApp();
		if (!app)
			return;

		MCF_Device_Item page = m_aVisible[m_iOpenEntry];
		int at = app.m_aItems.Find(page);
		if (at < 0)
			return;

		app.m_aItems.Remove(at);
		m_Content.GetAllItems(m_aVisible);

		int land = m_iOpenEntry;
		if (land >= m_aVisible.Count())
			land = m_aVisible.Count() - 1;

		ShowEntry(land);
		ShowPaperAuthor();
		FillPaperAuthor();
		SetHint("A page was removed. SAVE writes it to the object.");
	}

	protected void OnSaveIntelClicked(SCR_ButtonTextComponent button)
	{
		CommitPaperPage();
		SendDraft();

		if (m_wDeviceName)
			m_wDeviceName.SetText(m_Content.DeviceName());

		ShowEntry(m_iOpenEntry);
		ShowPaperAuthor();
	}

	// ------------------------------------------- reading and writing a field

	//! MULTILINE IS A DIFFERENT CLASS, NOT A FLAG.
	//!
	//! MultilineEditBoxWidget and EditBoxWidget do not share a parent -- the
	//! engine's own SCR_EditBoxComponent carries one field for each and
	//! apologises for it in a comment -- so a cast to one returns null for the
	//! other and the box silently fills with nothing and saves nothing.
	protected string BoxText(Widget found)
	{
		if (!found)
			return "";

		EditBoxWidget one = EditBoxWidget.Cast(found);
		if (one)
			return one.GetText();

		MultilineEditBoxWidget many = MultilineEditBoxWidget.Cast(found);
		if (many)
			return many.GetText();

		return "";
	}

	protected void SetBoxText(Widget found, string value)
	{
		if (!found)
			return;

		EditBoxWidget one = EditBoxWidget.Cast(found);
		if (one)
		{
			one.SetText(value);
			return;
		}

		MultilineEditBoxWidget many = MultilineEditBoxWidget.Cast(found);
		if (many)
			many.SetText(value);
	}

	protected void FocusBox(Widget found)
	{
		if (!found)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(found);

		EditBoxWidget one = EditBoxWidget.Cast(found);
		if (one)
		{
			one.ActivateWriteMode();
			return;
		}

		MultilineEditBoxWidget many = MultilineEditBoxWidget.Cast(found);
		if (many)
			many.ActivateWriteMode();
	}

	protected void OnPrevClicked(SCR_ButtonTextComponent button)
	{
		// COMMITTED BEFORE THE PAGE MOVES. The boxes are the only place typed
		// text exists until CommitPaperPage runs, so turning a page without it
		// throws away everything written since the last turn.
		CommitPaperPage();

		if (m_iOpenEntry > 0)
			ShowEntry(m_iOpenEntry - 1);
	}

	protected void OnNextClicked(SCR_ButtonTextComponent button)
	{
		CommitPaperPage();

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

			// The lock screen's clock is the biggest thing on this device and
			// the one a player reads against the sky. Set once when the screen
			// opened, it drifted away from the mission's own time the longer
			// the phone stayed up; it is re-read here with everything else.
			if (m_bOnLock && m_wLockTitle)
				m_wLockTitle.SetText(ClockText());
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
