//! Return, and wrapping, in a multiline edit box.
//!
//! EVERYTHING HERE WAS MEASURED. The log lines are in the commit history:
//!
//!   [MCF] text input: control character 13         <- Return does arrive
//!   [MCF] return pressed: had 183 chars, wrote 184, box now reports 184
//!
//! So Return reaches script, and reading and writing a MultilineEditBoxWidget
//! works -- but the widget is editing an internal buffer, and text written
//! from outside is not what it goes on editing. Re-entering write mode is
//! what makes it pick the new text up, and that is also what selects all of
//! it, after which the next character typed replaces the page.
//!
//! There is no caret API -- nothing in scripts/Core/generated/UI/ reports or
//! moves it -- and no way to send a keystroke either: WidgetManager has
//! ReportMouse and nothing for keys. So the selection cannot be cleared by
//! pressing an arrow, which would otherwise be the obvious answer.
//!
//! WHAT IS LEFT IS THE FOCUS ROUTE. Clicking an edit box starts write mode
//! without selecting anything -- that is how every edit box in the game is
//! used -- so this takes the focus away and gives it back, which is the
//! nearest thing to a click that script can do. ActivateWriteMode is the
//! fallback for when that does not start write mode at all: selecting
//! everything is bad, not typing at all is worse.
class MCF_Device_TextInput : ScriptedWidgetEventHandler
{
	protected static const int KEY_RETURN = 13;
	protected static const int KEY_LINE_FEED = 10;

	//! The last text this box was seen holding, so a GetText that does not see
	//! the live buffer cannot make Return throw the page away.
	protected string m_sLast;

	override bool OnChange(Widget w, bool finished)
	{
		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(w);
		if (box)
			m_sLast = box.GetText();

		return false;
	}

	override bool OnChar(Widget w, int charCode)
	{
		if (charCode != KEY_RETURN && charCode != KEY_LINE_FEED)
			return false;

		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(w);
		if (!box)
			return false;

		string held = box.GetText();

		if (held.IsEmpty() && !m_sLast.IsEmpty())
			held = m_sLast;

		string grown = held + "\n";
		box.SetText(grown);
		m_sLast = grown;

		// The click, as near as script can get to one: drop the focus and
		// take it back. An edit box entered this way starts write mode with
		// nothing selected.
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			workspace.SetFocusedWidget(null);
			workspace.SetFocusedWidget(box);
		}

		bool writing = box.IsInWriteMode();

		// If that did not start write mode, the box takes no keys at all,
		// which is worse than a selection. ActivateWriteMode always works and
		// always selects everything.
		if (!writing)
		{
			box.ActivateWriteMode();
			box.SetText(grown);
		}

		MCF_Core_Log.Warn("return: had " + held.Length().ToString()
			+ ", wrote " + grown.Length().ToString()
			+ ", box reports " + box.GetText().Length().ToString()
			+ ", write mode after refocus " + writing.ToString()
			+ ", now " + box.IsInWriteMode().ToString());

		// Processed, so the widget cannot use Return to leave write mode --
		// which is what it does with it otherwise.
		return true;
	}

	//! Attaches one and switches wrapping on.
	//!
	//! THE CALLER HAS TO HOLD IT. A handler is attached to the widget but
	//! owned by script; dropped on the floor it is collected and Return goes
	//! back to doing nothing some minutes later, which is a bug that looks
	//! like a different bug.
	static MCF_Device_TextInput Attach(Widget found)
	{
		if (!found)
			return null;

		// A TextWidget wraps only when it carries the WRAP_TEXT flag -- its
		// own doc comment says so -- and a MultilineEditBoxWidget is one.
		TextWidget text = TextWidget.Cast(found);
		if (text)
			text.SetTextWrapping(true);

		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(found);
		if (!box)
			return null;

		MCF_Device_TextInput handler = new MCF_Device_TextInput();
		found.AddHandler(handler);
		return handler;
	}
}
