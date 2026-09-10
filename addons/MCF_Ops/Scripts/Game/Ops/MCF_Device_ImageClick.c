//! Makes a picture clickable.
//!
//! WHY A HANDLER AND NOT A BUTTON. A button would have to be positioned over
//! the image, and the image is sized in script every time one is shown -- so
//! the button would have to be moved in step with it, and any frame where the
//! two disagreed would be a click that landed nowhere or on nothing. A handler
//! attached to the image itself cannot get out of alignment with it, because
//! it IS it.
//!
//! WHY AN INVOKER AND NOT A REFERENCE BACK. The handler knows nothing about
//! phones. A laptop shell, and the planning board, want the same behaviour
//! from the same class, and none of them should have to be a type this file
//! has heard of.

class MCF_Device_ImageClick : ScriptedWidgetEventHandler
{
	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	override bool OnClick(Widget w, int x, int y, int button)
	{
		m_OnClicked.Invoke();

		// Handled, so it stops here rather than reaching whatever is behind the
		// picture -- which, on a device screen, is the item list.
		return true;
	}
}
