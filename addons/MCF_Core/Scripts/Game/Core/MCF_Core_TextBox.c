//! Editing a whole block of text in a game window.
//!
//! WHAT THIS IS FOR. Enfusion gives you an edit box that takes one line, and a
//! different class that takes several -- and almost nothing else. No wrapping
//! until you ask for it, no Return key, no event when write mode starts, and no
//! shared parent between the two classes, so a cast to the wrong one silently
//! yields a box that fills with nothing and saves nothing. Everything below is
//! the set of workarounds that makes a multi-line box behave, measured rather
//! than guessed, and gathered here so nobody has to find them twice.
//!
//! WHY THIS IS IN CORE. "Let somebody type a paragraph" is not an intelligence
//! feature. It grew out of the letter and notebook editors, but a briefing
//! note, a sign, a form, a third party's mod -- all of them want the same box
//! and hit the same four traps.

//------------------------------------------------------------------------------------------------
//! The Return key, and the wrapping, for a MultilineEditBoxWidget.
//!
//! HOLD THE RETURNED HANDLER IN A `ref` FIELD. A widget keeps only a weak claim
//! on its handlers, so a dropped one is collected and Return stops working some
//! minutes later, which is a very hard bug to connect back to this line.
class MCF_Core_TextBoxInput : ScriptedWidgetEventHandler
{
	protected static const int KEY_RETURN = 13;
	protected static const int KEY_LINE_FEED = 10;

	//! What the box held at the last change. A programmatic SetText raises no
	//! OnChange, so Seed() exists to keep this honest.
	protected string m_sLast;

	//------------------------------------------------------------------------
	string Text()
	{
		return m_sLast;
	}

	//------------------------------------------------------------------------
	//! Tell the handler what the box now holds after you set it yourself.
	void Seed(string value)
	{
		m_sLast = value;
	}

	//------------------------------------------------------------------------
	override bool OnChange(Widget w, bool finished)
	{
		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(w);
		if (box)
			m_sLast = box.GetText();

		return false;
	}

	//------------------------------------------------------------------------
	//! RETURN DOES NOT INSERT A NEWLINE ON ITS OWN. The engine delivers the
	//! character and does nothing with it, so the newline is appended by hand
	//! and write mode is re-entered -- via a focus of nothing and back, because
	//! simply re-activating selects the whole text and the next keystroke
	//! replaces the paragraph.
	override bool OnChar(Widget w, int charCode)
	{
		if (charCode != KEY_RETURN && charCode != KEY_LINE_FEED)
			return false;

		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(w);
		if (!box)
			return false;

		m_sLast = box.GetText() + "\n";
		box.SetText(m_sLast);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			workspace.SetFocusedWidget(null);
			workspace.SetFocusedWidget(box);
		}
		else
		{
			box.ActivateWriteMode();
		}

		return true;
	}

	//------------------------------------------------------------------------
	//! Turn a MultilineEditBoxWidget into one that wraps and takes Return.
	//! Returns the handler, which the caller MUST keep in a `ref` field.
	static MCF_Core_TextBoxInput Attach(Widget found)
	{
		if (!found)
			return null;

		// Wrapping is off by default and there is no flag on the box itself.
		TextWidget text = TextWidget.Cast(found);
		if (text)
			text.SetTextWrapping(true);

		MultilineEditBoxWidget box = MultilineEditBoxWidget.Cast(found);
		if (!box)
			return null;

		MCF_Core_TextBoxInput handler = new MCF_Core_TextBoxInput();
		found.AddHandler(handler);

		return handler;
	}
}

//------------------------------------------------------------------------------------------------
//! Reading and writing an edit box without caring which of the two classes it
//! is.
//!
//! MULTILINE IS A DIFFERENT CLASS, NOT A FLAG. MultilineEditBoxWidget and
//! EditBoxWidget do not share a parent -- the engine's own SCR_EditBoxComponent
//! carries one field for each and apologises for it in a comment -- so a cast
//! to one returns null for the other, with no error and no clue.
class MCF_Core_TextBox
{
	//------------------------------------------------------------------------
	static string Get(Widget found)
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

	//------------------------------------------------------------------------
	static void Set(Widget found, string value)
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

	//------------------------------------------------------------------------
	//! Put the caret in the box and start taking keystrokes.
	//!
	//! AN EDIT BOX TAKES KEYSTROKES ONLY IN WRITE MODE, and the engine raises
	//! no event when it begins -- vanilla polls IsInWriteMode() and calls
	//! ActivateWriteMode() from its own pencil button. This is that pencil.
	static void Focus(Widget found)
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

	//------------------------------------------------------------------------
	//! A typed newline has to survive being stored. Anything that flattens a
	//! string for a config or a wire format turns a real newline into a space,
	//! so it travels as the two characters a config carries and comes back a
	//! real one on the way to a screen.
	static string Encode(string typed)
	{
		string result = "";
		int length = typed.Length();

		for (int i = 0; i < length; i++)
		{
			string character = typed.Substring(i, 1);

			if (character == "\n")
				result = result + "\\n";
			else
				result = result + character;
		}

		return result;
	}

	//------------------------------------------------------------------------
	static string Decode(string stored)
	{
		string result = "";
		int length = stored.Length();
		int i = 0;

		while (i < length)
		{
			string character = stored.Substring(i, 1);

			if (character == "\\" && i + 1 < length && stored.Substring(i + 1, 1) == "n")
			{
				result = result + "\n";
				i = i + 2;
				continue;
			}

			result = result + character;
			i = i + 1;
		}

		return result;
	}
}
