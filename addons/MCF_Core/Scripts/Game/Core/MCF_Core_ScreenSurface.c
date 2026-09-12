//! A live widget UI rendered onto a surface in the world.
//!
//! WHAT THIS IS FOR. Put it on any entity whose material samples
//! `$rendertarget` and it will draw a layout onto that surface while somebody
//! is near enough to read it, and nothing at all while nobody is. The entity
//! decides WHAT is on the screen; this component only decides WHEN there is a
//! screen at all, and owns the widget tree's whole life.
//!
//! HOW A UI GETS ONTO A SURFACE, which is vanilla's recipe and not a trick --
//! SCR_DataDisplayGadget does exactly this to put a ballistic page on the range
//! table you hold in your hands:
//!
//!   1. CreateWidgets(layout) with NO PARENT. The tree lives outside the screen
//!      hierarchy, so it is never drawn to the screen.
//!   2. RTTextureWidget.SetRenderTarget(owner) -- the entity's material can now
//!      sample it as $rendertarget.
//!   3. RemoveRenderTarget(owner) on the way out. Not optional; the engine's
//!      own comment says it must be called when the widget goes away.
//!
//! WHY THIS IS IN CORE AND NOT IN A MODULE. A screen on a thing is not an
//! intelligence feature. A radio set, a clipboard, a command tent, a departure
//! board, somebody else's mod entirely -- all of them want the same three steps
//! and the same distance rule, and none of them should have to install a module
//! about spycraft to get it. It is a service: you hand it a layout and it hands
//! you back a root widget and two moments.
//!
//! WHAT MAKES IT A HARD SWITCH-OFF, which the map board it grew out of does not
//! do. That board hides its screen behind a white sheet when nobody is near:
//! the render target stops, which is the expensive part, but the widget tree
//! stays built and its watchdog keeps ticking for the rest of the mission. For
//! one board that is nothing. For a phone with message lists it is a tree per
//! device per client, forever, for something nobody is looking at.
//!
//! So out of range this component DESTROYS the tree and takes the render target
//! off the mesh. There is then no widget, no texture, no per-frame work -- only
//! one distance check twice a second, which is the price of knowing when to
//! come back. Coming back is cheap because the content is rebuilt from data:
//! that is what m_OnRaised is for.
[ComponentEditorProps(category: "MCF/Core", description: "Draws a widget layout onto this entity's surface while a viewer is near, and destroys it entirely when nobody is.")]
class MCF_Core_ScreenSurfaceClass : ScriptComponentClass
{
}

class MCF_Core_ScreenSurface : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Layout drawn onto this entity's surface. Must contain an RTTextureWidget named as below.", params: "layout")]
	protected ResourceName m_sLayout;

	[Attribute("RTTexture0", desc: "Name of the RTTextureWidget inside the layout.")]
	protected string m_sRenderTargetWidget;

	[Attribute("Fade", desc: "Optional widget faded in as the viewer walks away. Leave empty for no fade.")]
	protected string m_sFadeWidget;

	[Attribute("10", UIWidgets.Slider, desc: "Metres at which the screen goes dark and is torn down. 0 keeps it on always.", params: "0 200 1")]
	protected float m_fActivationDistance;

	[Attribute("2", UIWidgets.Slider, desc: "Metres before that distance over which the screen fades out.", params: "0 50 0.5")]
	protected float m_fFadeBand;

	[Attribute("10", UIWidgets.Slider, desc: "Frames per second the surface is redrawn at. A still screen needs very few.", params: "1 60 1")]
	protected int m_iFramesPerSecond;

	[Attribute("0.5", UIWidgets.Slider, desc: "Render resolution scale. Below 1 also turns FSR on.", params: "0.1 1 0.05")]
	protected float m_fResolutionScale;

	//! How often the distance is measured. The only cost this component has
	//! while nobody is near, and deliberately the only one.
	protected static const int TICK_MS = 500;

	protected Widget m_wRoot;
	protected RTTextureWidget m_wRenderTarget;
	protected Widget m_wFade;

	protected bool m_bRaised;
	protected float m_fFade = -1;

	//! Fired with the root widget when the screen comes alive, so the owner can
	//! fill it; and again with nothing when it goes away, so the owner can drop
	//! every reference it took. An owner that keeps a widget past OnLowered is
	//! holding a pointer into a destroyed tree.
	protected ref ScriptInvoker m_OnRaised = new ScriptInvoker();
	protected ref ScriptInvoker m_OnLowered = new ScriptInvoker();

	//------------------------------------------------------------------------
	// WHAT AN OWNER USES
	//------------------------------------------------------------------------

	//! Fires as (Widget root). Fill the screen here, every time -- it is a new
	//! tree each time, not the old one revealed.
	ScriptInvoker GetOnRaised()
	{
		return m_OnRaised;
	}

	//! Fires with no arguments, just before the tree is destroyed.
	ScriptInvoker GetOnLowered()
	{
		return m_OnLowered;
	}

	//! The live root, or null when nobody is near. Never cache this across an
	//! OnLowered.
	Widget GetRoot()
	{
		return m_wRoot;
	}

	//------------------------------------------------------------------------
	bool IsRaised()
	{
		return m_bRaised;
	}

	//------------------------------------------------------------------------
	//! Point this at a different layout. Takes effect the next time the screen
	//! is raised; raises it again now if somebody is already looking.
	void SetLayout(ResourceName layout)
	{
		if (layout == m_sLayout)
			return;

		m_sLayout = layout;

		if (!m_bRaised)
			return;

		IEntity owner = GetOwner();
		Lower(owner);
		Watch();
	}

	//------------------------------------------------------------------------
	//! How far the nearest viewpoint is, in metres. Owners sometimes want this
	//! to decide how much detail to put on the screen.
	float ViewerDistance()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return float.MAX;

		return DistanceTo(owner);
	}

	//------------------------------------------------------------------------
	// LIFE
	//------------------------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// A surface is something a person looks at, so there is nothing to do
		// on a machine with nobody at the screen.
		if (System.IsConsoleApp())
			return;

		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (System.IsConsoleApp())
			return;

		GetGame().GetCallqueue().CallLater(Watch, TICK_MS, true);
	}

	//------------------------------------------------------------------------
	//! Twice a second: is anybody near enough, and how faded should it be.
	protected void Watch()
	{
		IEntity owner = GetOwner();
		if (!owner || owner.IsDeleted())
			return;

		float distance = DistanceTo(owner);
		bool wanted = m_fActivationDistance <= 0 || distance < m_fActivationDistance;

		if (wanted && !m_bRaised)
			Raise(owner);
		else if (!wanted && m_bRaised)
			Lower(owner);

		if (m_bRaised)
			Fade(FadeFor(distance));
	}

	//------------------------------------------------------------------------
	protected void Raise(notnull IEntity owner)
	{
		if (m_bRaised)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		if (m_sLayout.IsEmpty())
		{
			MCF_Core_Log.Warn("screen surface: no layout named, so there is nothing to draw");
			return;
		}

		// NO PARENT. That is the whole reason this never appears on screen.
		m_wRoot = workspace.CreateWidgets(m_sLayout);
		if (!m_wRoot)
		{
			MCF_Core_Log.Warn("screen surface: the layout would not load: " + m_sLayout);
			return;
		}

		m_wRenderTarget = RTTextureWidget.Cast(m_wRoot.FindAnyWidget(m_sRenderTargetWidget));
		if (!m_wRenderTarget)
		{
			MCF_Core_Log.Warn("screen surface: the layout carries no RTTextureWidget named " + m_sRenderTargetWidget);
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
			return;
		}

		if (!m_sFadeWidget.IsEmpty())
			m_wFade = m_wRoot.FindAnyWidget(m_sFadeWidget);

		m_wRenderTarget.SetRenderTarget(owner);

		// The two dials the engine hands over for exactly this, and the reason
		// a surface is affordable at all. Most screens are still; a handful of
		// frames a second at half resolution is more than they need.
		if (m_iFramesPerSecond > 0)
			m_wRenderTarget.SetMaxFPS(m_iFramesPerSecond);

		if (m_fResolutionScale > 0 && m_fResolutionScale < 1)
		{
			m_wRenderTarget.SetResolutionScale(m_fResolutionScale);
			m_wRenderTarget.ToggleFSR(true);
		}

		m_bRaised = true;

		// Dark until proven near, so a screen never flashes its contents for
		// one frame before deciding nobody is looking.
		m_fFade = -1;
		Fade(1);

		m_OnRaised.Invoke(m_wRoot);
	}

	//------------------------------------------------------------------------
	//! The hard switch-off. Everything goes: the owner's references first, then
	//! the render target off the mesh, then the tree itself.
	protected void Lower(IEntity owner)
	{
		if (!m_bRaised)
			return;

		m_bRaised = false;

		// Told BEFORE anything is destroyed, so an owner can still read the
		// widgets it is about to lose.
		m_OnLowered.Invoke();

		// MANDATORY, and the engine says so: the render target has to be taken
		// off the entity's mesh before the widget goes. Leaving it is a
		// dangling pointer into a deleted widget.
		if (m_wRenderTarget && owner && !owner.IsDeleted())
			m_wRenderTarget.RemoveRenderTarget(owner);

		m_wRenderTarget = null;
		m_wFade = null;

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_fFade = -1;
	}

	//------------------------------------------------------------------------
	//! 0 near, 1 gone. The band is walked through before the screen is torn
	//! down, so it dims rather than blinking out.
	protected float FadeFor(float distance)
	{
		if (m_fActivationDistance <= 0)
			return 0;

		if (distance >= m_fActivationDistance)
			return 1;

		float begins = m_fActivationDistance - m_fFadeBand;

		if (m_fFadeBand <= 0 || distance <= begins)
			return 0;

		return (distance - begins) / m_fFadeBand;
	}

	//------------------------------------------------------------------------
	protected void Fade(float amount)
	{
		if (Math.AbsFloat(amount - m_fFade) < 0.01)
			return;

		m_fFade = amount;

		if (m_wFade)
			m_wFade.SetOpacity(amount);
	}

	//------------------------------------------------------------------------
	protected float DistanceTo(notnull IEntity owner)
	{
		vector eye;
		bool found = false;

		CameraManager cameras = GetGame().GetCameraManager();
		if (cameras)
		{
			CameraBase camera = cameras.CurrentCamera();
			if (camera)
			{
				eye = camera.GetOrigin();
				found = true;
			}
		}

		if (!found)
		{
			PlayerController controller = GetGame().GetPlayerController();
			if (controller)
			{
				IEntity player = controller.GetControlledEntity();
				if (player)
				{
					eye = player.GetOrigin();
					found = true;
				}
			}
		}

		if (!found)
			return float.MAX;

		return vector.Distance(eye, owner.GetOrigin());
	}

	//------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Watch);

		Lower(owner);

		super.OnDelete(owner);
	}
}
