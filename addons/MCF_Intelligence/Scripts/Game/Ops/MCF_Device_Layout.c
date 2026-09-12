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

	//! The phone's numbers, and the defaults, because it is the shell these were
	//! measured on.
	static const float PHONE_HEIGHT = 0.95;
	static const float PHONE_GLASS_X = 0.86;
	static const float PHONE_GLASS_Y = 0.90;

	//! PER SHELL, NOT PER CLASS. These were static consts while a phone was the
	//! only device, which quietly made "reusable for a laptop" untrue: a laptop
	//! drawn at 95% of screen HEIGHT is wider than the screen it is drawn on,
	//! because it is a landscape object and a phone is not. A device that is
	//! asked how big it is should also be asked what shape it wears.
	protected float m_fScreenHeight = PHONE_HEIGHT;
	protected float m_fGlassX = PHONE_GLASS_X;
	protected float m_fGlassY = PHONE_GLASS_Y;
	protected float m_fGlassUp;
	protected float m_fModelDown;
	protected float m_fBoxAspect;

	//! The shift actually applied to the preview widget, in reference units.
	//! Kept because the glass is placed against that widget and has to move
	//! with it.
	protected float m_fModelShift;

	//! How much of the glass the text columns use, leaving the margin you would
	//! expect down each side of a screen.
	static const float TEXT_WIDTH = 0.90;

	//! \param screenHeight How much of the screen's height the device fills.
	//! \param glassX       How much of the device's face is glass, across.
	//! \param glassY       The same, down. A visible bezel is what makes it
	//!                     look held rather than overlaid: at 0.92 x 0.94 the
	//!                     phone's screen covered the body almost edge to edge
	//!                     and what was left read as a glow around a panel.
	//! \param glassUp     How far ABOVE the middle of the device the glass sits,
	//!                    as a fraction of the device's height. Zero for
	//!                    anything whose whole face is screen. A laptop is not
	//!                    that: its silhouette is a screen standing on a
	//!                    keyboard, so a centred glass covers the keyboard and
	//!                    leaves a strip of dead screen along the top.
	//! \param modelDown  How far DOWN the drawn device sits in its box, as a
	//!                   fraction of that box's height. The preview always
	//!                   centres the item in the widget, so this is the only
	//!                   way to choose which end of an overflowing device runs
	//!                   off the screen: a laptop drawn big enough to be worth
	//!                   reading should lose the bottom of its keyboard, not
	//!                   the top of its lid.
	void Configure(float screenHeight, float glassX, float glassY, float glassUp = 0, float modelDown = 0)
	{
		m_fScreenHeight = screenHeight;
		m_fGlassX = glassX;
		m_fGlassY = glassY;
		m_fGlassUp = glassUp;
		m_fModelDown = modelDown;
	}

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
	protected float m_fGlassHeight;

	//! The screen's four corners in the model's OWN space, engine axes.
	//!
	//! WHY A QUAD AND NOT TWO FRACTIONS. Fractions of a bounding box work for a
	//! device that is all screen -- a phone, a sheet of paper. A laptop is a
	//! screen standing on a keyboard at an angle, so no fraction of its
	//! silhouette is its screen, and every attempt to guess one was wrong in a
	//! different way. Four points read off the mesh are not a guess, and they
	//! survive any camera: they are projected through the same preview the
	//! player is looking at, so wherever the laptop ends up on screen, the
	//! glass ends up on its screen.
	protected ref array<vector> m_aScreenQuad;

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

	//! Overrides the shape of the box the preview is drawn into.
	//!
	//! WHAT THIS IS FOR. The preview world draws its own sky -- a treeline,
	//! from InventoryPreviewWorld.et's HDRi -- and SetClearColor does not
	//! govern it. So the sky is removed by leaving it nowhere to be: the box is
	//! cropped to the width of the device itself, and the device covers what is
	//! left. Measured from the log rather than guessed: the screen quad came
	//! back 1022.01 wide in a box 1026 tall, and the lid is 1.046 times the
	//! LCD's width, so the laptop is 1.04 box-heights across.
	//!
	//! Zero means "use the model's own proportions", which is right for
	//! anything that fills its box.
	void SetBoxAspect(float aspect)
	{
		m_fBoxAspect = aspect;
	}

	//! Tells the layout exactly where this device's glass is, in the model's own
	//! coordinates. Leave it unset for a device whose whole face is screen.
	void SetScreenQuad(notnull array<vector> corners)
	{
		m_aScreenQuad = corners;
	}

	//! Asks the render target not to draw the preview world's own sky.
	//!
	//! The inventory preview world is InventoryPreviewWorld.et and it carries
	//! SkyPreset HDRi_inventory.emat -- a treeline under a dark sky. At the
	//! size of an inventory tile that reads as a soft backdrop; at the size of
	//! a laptop it reads as a photograph somebody left behind the device.
	//!
	//! Called AFTER the item is set, and again on every frame of the fit,
	//! because the manager configures the render target when it takes it over
	//! and a clear colour set before that is lost.
	//!
	//! OPAQUE BLACK, NOT TRANSPARENT, on purpose. Transparent was tried first
	//! and the treeline stayed, which leaves two possibilities: either this
	//! call does not govern the background at all, or it does and the sky is
	//! drawn over it. Black tells those apart in one look -- and if it works it
	//! is also the better backdrop for a lit screen.
	void ClearPreviewBackground()
	{
		if (m_wModel)
			m_wModel.SetClearColor(true, 0xFF000000);
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

	//! The same, down. Anything that has to FILL the glass rather than fit
	//! inside its width -- the break-in panel -- needs both numbers.
	float GetGlassHeight()
	{
		return m_fGlassHeight;
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

		float boxH = screenHeight * m_fScreenHeight;
		float boxW = boxH * Aspect();

		// A LANDSCAPE DEVICE RUNS OUT OF WIDTH FIRST. Sizing from height alone
		// is right for a phone and wrong for a laptop: at the same fraction of
		// the screen's height, something wider than it is tall ends up wider
		// than the screen. Whichever side runs out first decides.
		float screenWidth = workspace.DPIUnscale(workspace.GetWidth());
		float maxW = screenWidth * m_fScreenHeight;

		if (boxW > maxW)
		{
			boxW = maxW;
			boxH = boxW / Aspect();
		}

		m_fModelShift = boxH * m_fModelDown;

		PlaceCentred(m_wModel, boxW, boxH, m_fModelShift);
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

		if (m_aScreenQuad && m_aScreenQuad.Count() >= 3)
			return FitToScreenQuad();

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

		// UNITS, AND THIS ONE HID FOR WEEKS BECAUSE IT ONLY BITES IN A SMALL
		// WINDOW. GetScreenSize answers in PHYSICAL pixels; NodePoint answers
		// in the REFERENCE resolution. They agree only when the DPI scale is 1,
		// which is why a full-size window fitted the glass to the phone and a
		// 1335x615 window did not: the guard below saw 850 against a box of
		// 584, called it a different coordinate space, and left the glass at
		// 86% of the whole preview BOX -- a screen with no phone around it.
		// FitToScreenQuad learned this at line 364 and converts; this path
		// never did.
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			boxW = workspace.DPIUnscale(boxW);
			boxH = workspace.DPIUnscale(boxH);
		}

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

	//! Puts the glass exactly where the screen is, by projecting the screen's
	//! own corners through the preview the player is looking at.
	protected bool FitToScreenQuad()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		// UNITS. GetScreenSize answers in PHYSICAL pixels;
		// TryGetItemNodePositionInWidgetSpace answers in the REFERENCE
		// resolution, and so does FrameSlot. Mixing them put the glass a fifth
		// of the screen up and to the left of the laptop it belonged on -- and
		// it was the log that said so, because the screen's middle came back at
		// 667.9 against a half-box of 890.6, which is 668.1 once the box is
		// converted. Those two agreeing to a quarter of a pixel is the whole
		// proof.
		float boxW, boxH;
		m_wModel.GetScreenSize(boxW, boxH);
		if (boxW <= 0 || boxH <= 0)
			return false;

		boxW = workspace.DPIUnscale(boxW);
		boxH = workspace.DPIUnscale(boxH);

		float minX, minY, maxX, maxY;
		bool first = true;

		foreach (vector corner : m_aScreenQuad)
		{
			float px, py;
			if (!NodeAt(corner, px, py))
				return false;

			if (first)
			{
				minX = px;
				maxX = px;
				minY = py;
				maxY = py;
				first = false;
				continue;
			}

			if (px < minX)
				minX = px;
			if (px > maxX)
				maxX = px;
			if (py < minY)
				minY = py;
			if (py > maxY)
				maxY = py;
		}

		float w = maxX - minX;
		float h = maxY - minY;

		float midX = (minX + maxX) * 0.5;
		float midY = (minY + maxY) * 0.5;

		MCF_Core_Log.Debug("screen quad in widget space: " + w.ToString() + "x" + h.ToString()
			+ " centred at " + midX.ToString() + "," + midY.ToString()
			+ " in a box of " + boxW.ToString() + "x" + boxH.ToString() + " (reference units)");

		if (w < MIN_SENSIBLE_PIXELS || h < MIN_SENSIBLE_PIXELS)
			return false;

		// The same units guard as below: an answer bigger than the box that
		// contains it is not a measurement in this widget.
		if (w > boxW * 1.05 || h > boxH * 1.05)
		{
			MCF_Core_Log.Debug("preview answered in a different coordinate space than the widget -- keeping the box-relative fit");
			return false;
		}

		m_fGlassWidth = w * m_fGlassX;
		m_fGlassHeight = h * m_fGlassY;

		// Widget space runs from the widget's top-left corner, and the preview
		// widget is placed centred, so the glass moves by how far the screen's
		// middle is from the widget's middle.
		// Plus the preview widget's own shift: the quad is measured inside that
		// widget, and the glass is a sibling of it.
		PlaceCentred(m_wScreenArea, m_fGlassWidth, m_fGlassHeight,
			midY - boxH * 0.5 + m_fModelShift, midX - boxW * 0.5);

		CapWidth(W_LIST_WIDTH, m_fGlassWidth);
		CapWidth(W_READ_WIDTH, m_fGlassWidth);
		return true;
	}

	// ------------------------------------------------------------- internals

	protected float Aspect()
	{
		if (m_fBoxAspect > 0)
			return m_fBoxAspect;

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
		float glassW = deviceW * m_fGlassX;
		float glassH = deviceH * m_fGlassY;
		m_fGlassWidth = glassW;
		m_fGlassHeight = glassH;

		if (m_wScreenArea)
			PlaceCentred(m_wScreenArea, glassW, glassH, -deviceH * m_fGlassUp);

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
	protected void PlaceCentred(notnull Widget widget, float width, float height, float shiftY = 0, float shiftX = 0)
	{
		FrameSlot.SetAnchor(widget, 0.5, 0.5);
		FrameSlot.SetSize(widget, width, height);
		FrameSlot.SetPos(widget, -width * 0.5 + shiftX, -height * 0.5 + shiftY);
	}

	//! One point on the model, in the model's own space, as a position in the
	//! widget. Enfusion passes transforms as four vectors and the fourth is the
	//! translation, so the point goes in the last row of an identity.
	protected bool NodePoint(float x, float z, out float outX, out float outY)
	{
		return NodeAt(Vector(x, 0, z), outX, outY);
	}

	//! Any point on the model, in the model's own space, as a position in the
	//! widget. Enfusion passes transforms as four vectors and the fourth is the
	//! translation, so the point goes in the last row of an identity.
	protected bool NodeAt(vector point, out float outX, out float outY)
	{
		vector offset[4];
		Math3D.MatrixIdentity4(offset);
		offset[3] = point;

		vector inWidget;
		if (!m_wModel.TryGetItemNodePositionInWidgetSpace(-1, offset, inWidget))
			return false;

		outX = inWidget[0];
		outY = inWidget[1];
		return true;
	}
}
