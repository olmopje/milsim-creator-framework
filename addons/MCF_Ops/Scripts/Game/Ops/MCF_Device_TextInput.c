//! Makes a multiline edit box behave like a text editor.
//!
//! TWO THINGS THE WIDGET DOES NOT DO ON ITS OWN, both traced to the same
//! fact: `MultilineEditBoxWidget` derives from `TextWidget`, not from
//! `EditBoxWidget`. The engine's own generated API in
//! scripts/Core/generated/UI/ says so, and its own SCR_EditBoxComponent
//! carries a separate field for each with the comment "Why aren't these
//! derived from a common parent :(".
//!
//! 1. A TextWidget wraps only when it carries the WRAP_TEXT flag. Without it
//!    a typed line runs off to the right forever.
//! 2. Return does not put a newline in -- the widget uses it to leave write
//!    mode instead. The character arrives here as OnChar(w, 13), which is
//!    measured and not assumed: the log says
//!    "[MCF] text input: control character 13" once per press.
//!
//! WRITING THE NEWLINE BACK IS THE HARD HALF. A box in write mode keeps its
//! own edit buffer, and a plain SetText during write mode is not reliably
//! what the box goes on editing. So this leaves write mode, writes the text,
//! and re-enters -- and keeps its own copy of the last text it saw through
//! OnChange, so that a GetText which comes back empty mid-edit cannot wipe
//! what the author typed.
//!
//! THE NEW LINE GOES ON THE END. There is no caret API -- nothing in the
//! scripted class reports or moves it -- so this cannot split a line in the
//! middle. For composing a page, which is the whole job of these boxes, that
//! is what a person expects; for editing the middle of one it is not, and the
//! answer then is a different widget rather than a cleverer handler.
class MCF_Device_TextInput : ScriptedWidgetEventHandler
{
	protected static const int KEY_RETURN = 13;
	protected static const int KEY_LINE_FEED = 10;

	//! The last text this box was seen holding. The safety net against a
	//! GetText that does not see the live edit buffer: appending a newline to
	//! an empty string would throw away the page.
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
		// Printable characters are the widget's own business.
		if (charCode >= 32)
			return false;

		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(w);
		if (!box)
			return false;

		if (charCode != KEY_RETURN && charCode != KEY_LINE_FEED)
			return false;

		string held = box.GetText();

		// If the widget will not tell us what is in it right now, use the last
		// thing it did tell us. Never append to nothing.
		if (held.IsEmpty() && !m_sLast.IsEmpty())
			held = m_sLast;

		string grown = held + "\n";

		box.SetText(grown);
		m_sLast = grown;

		// Re-entering write mode makes the box take the text it was just
		// given as the thing it is editing, rather than going on with the
		// buffer it had before.
		box.ActivateWriteMode();

		string back = box.GetText();
		MCF_Core_Log.Warn("return pressed: had " + held.Length().ToString()
			+ " chars, wrote " + grown.Length().ToString()
			+ ", box now reports " + back.Length().ToString()
			+ ", write mode " + box.IsInWriteMode().ToString());

		// Processed, so the widget never sees the Return and cannot use it to
		// leave write mode -- which is what it was doing with it.
		return true;
	}

	//! Switches wrapping on and, for a multiline box, hands back a handler the
	//! caller must keep alive.
	//!
	//! THE CALLER HAS TO HOLD IT. A handler is attached to the widget but
	//! owned by script; dropped on the floor it is collected and the box goes
	//! back to swallowing Return some minutes later, which is a bug that looks
	//! like a different bug.
	static MCF_Device_TextInput Attach(Widget found)
	{
		if (!found)
			return null;

		// A MultilineEditBoxWidget IS a TextWidget, so this reaches it -- and
		// reaches a plain read-only body just as well, which is why this is
		// the one place wrapping is switched on.
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
