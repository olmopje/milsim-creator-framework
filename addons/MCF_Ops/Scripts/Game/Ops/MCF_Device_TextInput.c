//! Makes a multiline edit box behave like a text editor.
//!
//! TWO THINGS THE WIDGET DOES NOT DO ON ITS OWN, both traced to the same
//! fact: `MultilineEditBoxWidget` derives from `TextWidget`, not from
//! `EditBoxWidget`. The engine's own generated API says so, and its own
//! SCR_EditBoxComponent carries a separate field for each with the comment
//! "Why aren't these derived from a common parent :(".
//!
//! 1. A TextWidget wraps only when it carries the WRAP_TEXT flag. Without it
//!    a typed line runs off to the right forever, which is exactly what the
//!    first build of these editors did.
//! 2. Return does not put a newline in. The character arrives here as an
//!    OnChar event -- the documented route for "the user types on a focused
//!    widget that accepts text input (EditBoxWidget, MultilineEditBox...)" --
//!    and a newline is put in instead of letting the widget swallow it.
//!
//! THE NEW LINE STARTS AT THE END OF THE TEXT. Nothing in the scripted class
//! reports or moves the caret, so this cannot split a line in the middle. For
//! composing a page, which is the whole job of these boxes, typing and
//! pressing Return does what a person expects.
class MCF_Device_TextInput : ScriptedWidgetEventHandler
{
	protected static const int KEY_RETURN = 13;
	protected static const int KEY_LINE_FEED = 10;

	override bool OnChar(Widget w, int charCode)
	{
		// Printable characters are the widget's own business. Anything below
		// space is logged once so that a key which turns out not to arrive
		// here can be told apart from one that arrives and is ignored --
		// which is the difference between this fix and the next one.
		if (charCode >= 32)
			return false;

		MCF_Core_Log.Debug("text input: control character " + charCode.ToString());

		if (charCode != KEY_RETURN && charCode != KEY_LINE_FEED)
			return false;

		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(w);
		if (!box)
			return false;

		box.SetText(box.GetText() + "\n");

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
