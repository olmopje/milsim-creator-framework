//! Every screen MCF adds to the game, declared in one place.
//!
//! WHY ONE FILE. Each MCF menu could declare its own `modded enum` block, and
//! two of them briefly did. Enforce merges modded enums, so it may well work --
//! but nothing in vanilla or in any local mod does it twice for one enum, and
//! this project has been bitten enough times by "probably fine" to not spend
//! that risk on file placement. One block, one place to look.
//!
//! WARNING: a value appended by a modded enum takes the next free ordinal,
//! which depends on what else is loaded. Never persist these numbers and never
//! send one over the wire -- refer to them only by name.
//!
//! Each name here must match a `MenuPreset <name>` entry in MCF's override of
//! Configs/System/chimeraMenus.conf exactly. The engine links them by name;
//! menuManagerDoc.c states that verbatim, and a mismatch fails silently with
//! a menu that simply never opens.
modded enum ChimeraMenuPreset
{
	//! The operations board: taskings, orders, and what a commander does
	//! about them. Opened from the physical board in the world.
	MCF_PlanningBoard,

	//! Reading an intel object as a plain list -- the fallback skin, and what
	//! every object used before the shells below existed. Opened by the read
	//! action on the object itself.
	MCF_IntelViewer,

	//! Where a Game Master rewrites what an intel object says.
	MCF_IntelEditor,

	//! The three presentation skins. Same data, same menu class
	//! (MCF_Intel_ShellMenu), different layout: a single sheet of paper, a
	//! ring-bound notepad you page through, a handheld device screen with app
	//! icons. Which one opens is decided by the object's MCF_EIntelView, not
	//! by the caller.
	MCF_IntelPaper,
	MCF_IntelNotepad,
	MCF_IntelDevice,

	//! The fourth skin, and the reason the geometry stopped being a set of
	//! constants: a laptop is landscape. Same menu class as the three above,
	//! same data, a wider screen and an app grid five across instead of three.
	//!
	//! SUPERSEDED BY MCF_IntelDesktop below and kept only so that a saved
	//! mission referring to it still resolves. Nothing opens it any more.
	MCF_IntelLaptop,

	//! The laptop as a desktop: a panel at the bottom, an application
	//! launcher, and windows that drag and stack. Its own menu class, because
	//! a screen where ten things are open at once has nothing in common with
	//! one that shows a single app at a time -- what the two share is the
	//! device profile, which is the part worth sharing.
	MCF_IntelDesktop,

	//! A probe, not a feature. Answers whether the game's own map can be put
	//! in a widget we choose and whether a render target can hold one --
	//! everything about the map board rests on it, and it cannot be answered
	//! by reading. DELETE THIS, its menu class and its layout once the answer
	//! is written down.

	//! Standing at a map board and taking the map. Vanilla's own map window
	//! -- its MapFrame inherits UI/layouts/Map/Map.layout, so the markers,
	//! the right-click menu and the tools are the game's, not ours -- but
	//! opened on the BOARD'S view, and driving it while it is open.
	MCF_MapBoardControl,

	//! Talking to somebody: what they say, and what you may say back.
	//! Opened by the talk action on the person themselves.
	MCF_Dialogue,

	//! Where a Game Master writes a conversation. Opened from the right-click
	//! menu on the person it is being written for.
	MCF_DialogueEditor,

	//! Where a Game Master writes what is on a device: its apps and what is in
	//! them. Opened from the right-click menu on the device itself.
	MCF_DeviceEditor,

	//! What the server is remembering between restarts: how much of it there
	//! is, throwing parts of it away, and the named starting positions a
	//! mission maker keeps. Opened from the right-click menu on the operations
	//! board -- the only screen in MCF that deletes anything.
	MCF_DataManager
}
