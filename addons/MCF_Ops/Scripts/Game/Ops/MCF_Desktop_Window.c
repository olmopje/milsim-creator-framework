//! One window on the laptop's desktop, and the handler that lets it be dragged.

//! What the shell remembers about a window.
//!
//! WHY ONE RECORD PER WINDOW AND NOT ONE OPEN WINDOW AT A TIME. A phone shows
//! one screen; a desktop shows several at once, and the whole point of the
//! laptop is that a player can have the mail open beside the file list. Each
//! window therefore carries its own selection, its own item list and its own
//! box -- there is no single "current app" on this device.
class MCF_Desktop_Window
{
	//! Which of the layout's ten window frames this is. The prefix every widget
	//! inside it is named with ("W3Title", "W3EntryList") is "W" + this.
	int m_iSlot;

	//! The MCF_EIntelApp this window shows, or -1 for a window that is not an
	//! app at all -- the terminal, which has no items and never will.
	int m_eKind;

	//! Which pane shape got generated into it: list, mail, chat, contacts,
	//! photos or term. The fill routine switches on this, not on the kind, so
	//! that two app kinds can share a pane without sharing a window.
	string m_sPane;

	Widget m_wRoot;
	Widget m_wBody;

	SCR_ButtonTextComponent m_Bar;
	SCR_ButtonTextComponent m_Min;
	SCR_ButtonTextComponent m_Max;
	SCR_ButtonTextComponent m_Close;

	//! The app this window is bound to for the device currently open. Null
	//! means the device has no such app, and the window cannot be opened.
	MCF_Device_App m_App;

	bool m_bOpen;
	bool m_bMin;
	bool m_bMax;

	//! The box, in reference units, relative to the desktop's top left. NOT an
	//! anchor: a window that can be dragged has no fixed anchor by definition,
	//! so these are absolute and FrameSlot is told them every time they change.
	float m_fX;
	float m_fY;
	float m_fW;
	float m_fH;

	//! Where it was before it was maximised, so restoring puts it back rather
	//! than somewhere reasonable.
	float m_fPrevX;
	float m_fPrevY;
	float m_fPrevW;
	float m_fPrevH;

	//! How big it wants to be, as a share of the desktop. Set from the window
	//! table when the menu opens; a laptop screen is not a fixed size, so a
	//! window measured in pixels would be a postage stamp on one machine and
	//! full screen on another.
	float m_fWantW;
	float m_fWantH;

	int m_iOpenEntry = -1;

	//! Which folder the file manager is showing. "" is the root, which is also
	//! the desktop -- on a real machine those are the same folder, and making
	//! them the same here is what lets a right-click on the wallpaper create a
	//! file you can then find in the window.
	string m_sPath;

	//! Every folder on the device, in order, for the tree on the left.
	ref array<string> m_aFolders = {};

	//! The tree's own buttons. Separate from m_aRows because the tree and the
	//! file list are two scroll panes, not one list with folders on top.
	ref array<SCR_ButtonTextComponent> m_aTree = {};

	//! An editor window shows ONE file rather than an app's list, so this is
	//! what it is showing. Null for every window that is an app.
	MCF_Device_Item m_Doc;

	//! The spreadsheet's cells, row-major, and which one is selected. The grid
	//! is fixed in the layout, so this is built once when the window is bound
	//! and never rebuilt.
	ref array<SCR_ButtonTextComponent> m_aCells = {};
	int m_iCellRow = -1;
	int m_iCellCol = -1;

	ref array<ref MCF_Device_Item> m_aVisible = {};
	ref array<SCR_ButtonTextComponent> m_aRows = {};

	string Prefix()
	{
		return "W" + m_iSlot.ToString();
	}
}

//! The title-bar press, which is all the engine will tell us about a drag.
//!
//! ENFUSION HAS NO OnMouseMove AND NO MOUSE CAPTURE. ScriptedWidgetEventHandler
//! offers OnMouseButtonDown, OnMouseButtonUp, OnClick, the enter/leave pair and
//! a per-frame OnUpdate -- and nothing that reports the pointer moving. The
//! shipped game has the same problem and solves it the same way its map ruler
//! does: the press sets a flag, and a per-frame tick polls
//! WidgetManager.GetMousePos and moves the widget itself.
//!
//! So this class is deliberately tiny. It says "pressed" and "released" and the
//! menu does the rest on a tick it already pays for.
class MCF_Desktop_Drag : ScriptedWidgetEventHandler
{
	//! NOT a ref. The menu owns the handler, the handler only points back at
	//! it, and two refs pointing at each other is a pair of objects neither of
	//! which is ever collected.
	protected MCF_Desktop_ShellMenu m_Menu;
	protected int m_iSlot;

	void MCF_Desktop_Drag(MCF_Desktop_ShellMenu menu, int slot)
	{
		m_Menu = menu;
		m_iSlot = slot;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		// 0 is the left button. A drag on the right button is not a thing any
		// desktop does, and swallowing it here would kill a context menu later.
		if (button != 0 || !m_Menu)
			return false;

		m_Menu.BeginDrag(m_iSlot, x, y);

		// FALSE, so the button still does its own hover and click work. This
		// handler is an observer; it is not trying to own the input.
		return false;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_Menu)
			return false;

		m_Menu.EndDrag();
		return false;
	}
}

//! A right-click, which is the only input on this device that is not a click.
//!
//! Enfusion reports the button as an int on the same OnMouseButtonDown every
//! other press comes through, so this is the whole mechanism: button 1 is the
//! right one, and the menu is opened wherever the cursor was.
class MCF_Desktop_Menu : ScriptedWidgetEventHandler
{
	protected MCF_Desktop_ShellMenu m_Menu;

	void MCF_Desktop_Menu(MCF_Desktop_ShellMenu menu)
	{
		m_Menu = menu;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!m_Menu)
			return false;

		if (button == 1)
		{
			m_Menu.OpenContextMenu();
			return true;
		}

		m_Menu.CloseContextMenu();
		return false;
	}
}

//! The transparent sheet that covers the desktop while a window is being
//! dragged, so the release has somewhere to land.
//!
//! A title bar hears the press and then nothing: by the time the button comes
//! up the cursor is over whatever the window was dragged across, and the event
//! goes there instead. This catches it wherever it lands. It is invisible, and
//! it is only up for the length of the drag.
class MCF_Desktop_Catch : ScriptedWidgetEventHandler
{
	protected MCF_Desktop_ShellMenu m_Menu;

	void MCF_Desktop_Catch(MCF_Desktop_ShellMenu menu)
	{
		m_Menu = menu;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_Menu)
			return false;

		m_Menu.EndDrag();
		return false;
	}
}
