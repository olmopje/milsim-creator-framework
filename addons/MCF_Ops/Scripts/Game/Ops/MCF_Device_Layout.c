//! Where a rendered device sits on screen, and where its glass is.
//!
//! WHY THIS IS ITS OWN CLASS. It is the part of the device shell that has
//! nothing to do with intel, nothing to do with phones in particular, and
//! everything to do with fitting a 3D preview and laying a screen over it. A
//! laptop shell wants it verbatim with different numbers. It is also the part
//! that cost a full day and five silent failures to get right, so it is worth
//! keeping in one place where those lessons stay attached to the code.
//!
//! THE FIVE THINGS THAT WENT WRONG, because every one of them will look like
//! something else when it happens again:
//!
//!   1. A FrameWidgetSlot's fields pair up as PositionX/OffsetLeft,
//!      PositionY/OffsetTop, SizeX/OffsetRight, SizeY/OffsetBottom. Position
//!      and Size are the box; the Offsets are padding. Writing the box into
//!      the Offsets leaves the size at zero and every widget vanishes, with no
//!      error -- just a black screen with the backdrop on it.
//!
//!   2. GetScreenSize reports what the LAST layout pass produced. Reading it in
//!      the same frame as a FrameSlot change reports the old value, which reads
//!      exactly like the call did nothing.
//!
//!   3. Anchors have to collapse to a point before SetSize means anything. A
//!      slot whose anchors are stretched takes its size from them and ignores
//!      SetSize entirely -- silently.
//!
//!   4. The preview does not fill the box it is given, and how much of it it
//!      fills depends on the box's SHAPE, not only its height. Widening a box
//!      from 280 to 600 pixels at the same height made the model SMALLER. So
//!      the box is given the model's own proportions: one shape, one variable.
//!
//!   5. TryGetItemNodePositionInWidgetSpace answers in the reference
//!      resolution, not in the widget's. Fullscreen the two coincide and the
//!      placement is exact; in a Workbench viewport they do not and the device
//!      comes back taller than the box containing it. The answer is used only
//!      when it fits.
//!
//! Units, once, because they are the trap underneath three of those:
//! GetScreenSize and WorkspaceWidget.GetHeight report PHYSICAL pixels, while
//! FrameSlot and SizeLayoutWidget work in the REFERENCE resolution. DPIUnscale
//! converts physical to reference. They are not interchangeable.

class MCF_Device_Layout
{
	//! The widgets a device layout is expected to contain. A layout missing any
	//! of them still works -- that piece is simply not placed.
	static const string W_MODEL = "Model";
	static const string W_SCREEN_AREA = "ScreenArea";
	static const string W_BODY = "Body";
	static const string W_LIST_WIDTH = "ListWidth";
	static const string W_READ_WIDTH = "ReadWidth";

	//! How much of the screen's height the device is drawn at.
	static const float SCREEN_HEIGHT = 0.95;

	//! How much of the device's face is glass. A visible bezel is what makes it
	//! look held rather than overlaid: at 0.92 x 0.94 the screen covered the
	//! body almost edge to edge and what was left read as a glow around a panel.
	static const float GLASS_X = 0.86;
	static const float GLASS_Y = 0.90;

	//! How much of the glass the text columns use, leaving the margin you would
	//! expect down each side of a screen.
	static const float TEXT_WIDTH = 0.90;

	//! Below this the preview has not drawn yet and is answering with a
	//! degenerate box.
	protected static const float MIN_SENSIBLE_PIXELS = 8.0;

	protected Widget m_wRoot;
	protected ItemPreviewWidget m_wModel;
	protected Widget m_wScreenArea;
	protected Widget m_wBody;

	//! The device's real size in metres, as the model is built. Everything else
	//! is derived from it: the box's shape, and the corners asked about.
	protected float m_fSizeX;
	protected float m_fSizeZ;

	//! Remembered from the last placement, because it is the one number
	//! everything else on the screen is measured against.
	protected float m_fGlassWidth;

	//! Binds to a layout and to the object being drawn.
	//! \param size The model's real size in metres. X across the face, Z along
	//!             its length. Y -- its thickness -- is not used.
	//! \return True if there is a preview widget to work with.
	bool Attach(notnull Widget root, vector size)
	{
		m_wRoot = root;
		m_wModel = ItemPreviewWidget.Cast(root.FindAnyWidget(W_MODEL));
		m_wScreenArea = root.FindAnyWidget(W_SCREEN_AREA);
		m_wBody = root.FindAnyWidget(W_BODY);

		m_fSizeX = size[0];
		m_fSizeZ = size[2];

		// A size of nothing would divide by zero below and give the box an
		// aspect of infinity. Fall back to something phone-shaped rather than
		// refusing to draw.
		if (m_fSizeX <= 0 || m_fSizeZ <= 0)
		{
			MCF_Core_Log.Warn("device has no preview size -- falling back to phone proportions");
			m_fSizeX = 0.07;
			m_fSizeZ = 0.149;
		}

		return m_wModel != null;
	}

	ItemPreviewWidget GetModelWidget()
	{
		return m_wModel;
	}

	//! How wide the glass is, in reference units. Anything that has to be sized
	//! to the screen -- a photograph, for one -- asks here rather than measuring
	//! again and getting a different answer.
	float GetGlassWidth()
	{
		return m_fGlassWidth;
	}

	//! Whether the drawn fallback panel is shown instead of the model.
	void ShowFallbackBody(bool visible)
	{
		if (m_wBody)
			m_wBody.SetVisible(visible);

		if (m_wModel)
			m_wModel.SetVisible(!visible);
	}

	//! Step one, on the first frame: give the preview box the model's own
	//! proportions and lay the glass on it by ratio.
	//!
	//! This is the placement that always works. It assumes the model fills the
	//! box, which it does not quite, so the glass ends up a little generous --
	//! but it is on the screen, at any resolution, without asking anything of
	//! the preview.
	void FitToBox()
	{
		if (!m_wModel)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float screenHeight = workspace.DPIUnscale(workspace.GetHeight());
		if (screenHeight <= 0)
			return;

		float boxH = screenHeight * SCREEN_HEIGHT;
		float boxW = boxH * Aspect();

		PlaceCentred(m_wModel, boxW, boxH);
		PlaceGlass(boxW, boxH);
	}

	//! Step two, a couple of frames later: ask the preview where the device
	//! actually ended up, and put the glass exactly there.
	//!
	//! Two frames, not one. See the note at the top about GetScreenSize
	//! reporting the previous layout pass -- a measurement taken in the same
	//! frame as the placement measures the placement before it.
	//!
	//! \return True if the preview answered usefully and the glass was moved.
	bool FitToDevice()
	{
		if (!m_wModel || !m_wScreenArea)
			return false;

		float halfX = m_fSizeX * 0.5;
		float halfZ = m_fSizeZ * 0.5;

		float left, right, top, bottom, ignoredX, ignoredY;
		if (!NodePoint(-halfX, 0, left, ignoredY))
			return false;

		if (!NodePoint(halfX, 0, right, ignoredY))
			return false;

		if (!NodePoint(0, halfZ, ignoredX, top))
			return false;

		if (!NodePoint(0, -halfZ, ignoredX, bottom))
			return false;

		// Signs depend on which way the camera looks at it; only the extent
		// matters.
		float deviceW = Extent(right - left);
		float deviceH = Extent(bottom - top);

		float boxW, boxH;
		m_wModel.GetScreenSize(boxW, boxH);

		MCF_Core_Log.Debug("device in widget space: " + deviceW.ToString() + "x" + deviceH.ToString()
			+ " against a box of " + boxW.ToString() + "x" + boxH.ToString());

		if (deviceW < MIN_SENSIBLE_PIXELS || deviceH < MIN_SENSIBLE_PIXELS)
			return false;

		// The units guard. An answer larger than the box that contains it is
		// not a measurement of anything in this widget, so it is refused and
		// the box-relative placement stands.
		if (boxH > 0 && deviceH > boxH * 1.05)
		{
			MCF_Core_Log.Debug("preview answered in a different coordinate space than the widget -- keeping the box-relative fit");
			return false;
		}

		PlaceGlass(deviceW, deviceH);
		return true;
	}

	// ------------------------------------------------------------- internals

	protected float Aspect()
	{
		return m_fSizeX / m_fSizeZ;
	}

	protected float Extent(float value)
	{
		if (value < 0)
			return -value;

		return value;
	}

	//! The glass, and the width the text inside it is allowed to reach.
	protected void PlaceGlass(float deviceW, float deviceH)
	{
		float glassW = deviceW * GLASS_X;
		m_fGlassWidth = glassW;

		if (m_wScreenArea)
			PlaceCentred(m_wScreenArea, glassW, deviceH * GLASS_Y);

		CapWidth(W_LIST_WIDTH, glassW);
		CapWidth(W_READ_WIDTH, glassW);
	}

	//! Tells a scrolling column how wide it may be.
	//!
	//! WITHOUT THIS NOTHING WRAPS, however many wrap flags are set on the text.
	//! A RichText folds at the width it is GIVEN, and a scroll gives as much as
	//! is asked for -- that is what scrolling is. The SizeLayoutWidget is the
	//! cap, and the cap is only knowable once the glass has been measured.
	protected void CapWidth(string name, float glassWidth)
	{
		if (!m_wRoot)
			return;

		SizeLayoutWidget sizer = SizeLayoutWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!sizer)
			return;

		sizer.EnableWidthOverride(true);
		sizer.SetWidthOverride(glassWidth * TEXT_WIDTH);
	}

	//! Pins a widget to the middle of the screen at an exact size.
	//!
	//! The anchors collapse to a point first. A slot whose anchors are stretched
	//! takes its size from them and ignores SetSize, silently.
	protected void PlaceCentred(notnull Widget widget, float width, float height)
	{
		FrameSlot.SetAnchor(widget, 0.5, 0.5);
		FrameSlot.SetSize(widget, width, height);
		FrameSlot.SetPos(widget, -width * 0.5, -height * 0.5);
	}

	//! One point on the model, in the model's own space, as a position in the
	//! widget. Enfusion passes transforms as four vectors and the fourth is the
	//! translation, so the point goes in the last row of an identity.
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
}
