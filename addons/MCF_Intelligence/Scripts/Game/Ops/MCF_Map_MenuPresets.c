//! This module's own screens, declared by this module.
//!
//! THE SECOND HALF OF THE MODULE-OWNED MENU EXPERIMENT. MCF_Core's
//! MCF_MenuPresets.c held all fourteen preset names, and its header said it did
//! so out of caution rather than necessity: "Enforce merges modded enums, so it
//! may well work" -- never tested. This tests it, and the test is free, because
//! a `modded enum` that does NOT merge is a compile error rather than a silent
//! failure: every `ChimeraMenuPreset.MCF_MapProbe` in MCF_Map_ProbeContextAction
//! stops resolving.
//!
//! If this compiles, a module can name its own screens and MCF_Core never has
//! to -- which, together with the module-owned chimeraMenus.conf beside this
//! file, is what lets somebody ship a module MCF_Core has never heard of.
modded enum ChimeraMenuPreset
{
	//! A Game Master's probe of what the map entity is doing. Opened from the
	//! editor context menu; the class and layout both live in this addon.
	MCF_MapProbe,
}
