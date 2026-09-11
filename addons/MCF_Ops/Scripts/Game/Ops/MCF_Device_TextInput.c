//! Typing into an intel page, and why it is done a line at a time.
//!
//! THE CARET CANNOT BE MOVED. Nothing in the engine's scripted widget API
//! reports or sets it -- check scripts/Core/generated/UI/, which has one file
//! per widget class and is the only honest source for this. Everything below
//! follows from that one missing function, and it was measured rather than
//! guessed:
//!
//!   [MCF] text input: control character 13         <- Return does arrive
//!   [MCF] return pressed: had 183 chars, wrote 184, box now reports 184
//!
//! So reading and writing a MultilineEditBoxWidget works perfectly, including
//! while it is being typed in. What does not work is putting the caret after
//! the newline that was just written. It stays where it was, so the next
//! character is typed BEFORE the newline and the line never breaks -- and
//! ActivateWriteMode(), which does move it, moves it to the start and selects
//! everything, so the next character wipes the page.
//!
//! A CARET IS ONLY HARMLESS IN AN EMPTY BOX. That is the whole design: the
//! author types one line at a time in a box that is emptied on every Return,
//! and the finished lines go onto the page above, where they are drawn as the
//! player will see them. It reads like writing a letter, which is what it is,
//! and no part of it depends on a caret.
class MCF_Device_LineInput : ScriptedWidgetEventHandler
{
	protected static const int KEY_RETURN = 13;
	protected static const int KEY_LINE_FEED = 10;

	//! Fired when Return is pressed on the line being typed.
	ref ScriptInvoker m_OnLine = new ScriptInvoker();

	override bool OnChar(Widget w, int charCode)
	{
		if (charCode != KEY_RETURN && charCode != KEY_LINE_FEED)
			return false;

		m_OnLine.Invoke();

		// Processed, so the widget never sees the Return and cannot use it to
		// leave write mode -- which is what it does with it otherwise.
		return true;
	}

	//! Attaches one to a single-line box and hands it back.
	//!
	//! THE CALLER HAS TO HOLD IT. A handler is attached to the widget but
	//! owned by script; dropped on the floor it is collected and Return goes
	//! back to doing nothing some minutes later, which is a bug that looks
	//! like a different bug.
	static MCF_Device_LineInput Attach(Widget found)
	{
		if (!found)
			return null;

		MCF_Device_LineInput handler = new MCF_Device_LineInput();
		found.AddHandler(handler);
		return handler;
	}
}

//! Wrapping, which is the other half of what these widgets do not do alone.
class MCF_Device_TextInput
{
	//! A TextWidget wraps only when it carries the WRAP_TEXT flag -- its own
	//! doc comment says so -- and without it a long line runs off to the right
	//! forever. MultilineEditBoxWidget derives from TextWidget, so this
	//! reaches one of those just as well as a plain body.
	static void Wrap(Widget found)
	{
		if (!found)
			return;

		TextWidget text = TextWidget.Cast(found);
		if (text)
			text.SetTextWrapping(true);
	}
}
