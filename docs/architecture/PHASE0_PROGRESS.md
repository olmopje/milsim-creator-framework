# Session Progress Notes -- Phase 0 (Core)

Written at the end of the first hands-on coding session, when chat context
was running low. Purpose: let a fresh session (this chat continued, Claude
Code, or anyone else on the team) pick up exactly where this left off without
re-deriving anything below.

## Environment, confirmed working

- Repo: `G:\MCF` (git, `master` branch, clean tree as of this note)
- Workbench project: `G:\MCF\addons\MCF\addon.gproj` (ID `MCF`, GUID `6A50E40BA3B94A4F`, depends on `58D0FB3206B6F859` = base ArmaReforger data addon)
- **Launching Workbench reliably only works when the USER starts it manually** (Steam UI or their own shortcut). Every scripted launch attempt (direct .exe via PowerShell, `steam.exe -applaunch`, `enfusion-mcp`'s own `wb_launch`) intermittently failed to resolve the `G:/SteamLibrary/.../Arma Reforger/addons` path, causing `Game addon '58D0FB3206B6F859' not found`. Root cause not fully diagnosed -- worked around, not fixed. **Do not spend more time trying to script the launch; ask the user to start it.**
- Once Workbench is running (World Editor mode, not game/play mode) with Net API enabled (File > Options > General > Net API), `enfusion-mcp:wb_connect` / `wb_state` work normally.
- `enfusion-mcp:mod_build` also spawns its own fresh process and hits the same launch issue -- **do not rely on it either**. Use `wb_reload` (target: scripts) against the already-connected, user-launched instance instead, then read the newest file in `C:\Users\Administrator\Documents\My Games\ArmaReforgerWorkbench\logs\` (console.log) for `SCRIPT (E)` lines to check for compile errors. Reload can take several seconds; poll rather than assume instant.
- `enfusion-mcp`'s NET API handler scripts get auto-copied into `addons/MCF/Scripts/WorkbenchGame/EnfusionMCP/` -- these are gitignored on purpose (see `.gitignore`), not part of the mod. Call `wb_cleanup` before ever publishing.
- MCP filesystem/git connectors are scoped to specific allowed directories only (see `claude_desktop_config.json`, `filesystem` server args) -- `G:\MCF` was added there manually.

## What exists and compiles (all confirmed via wb_reload + log check, not just written)

- `addons/MCF/Scripts/Game/Core/MCF_Core_EventManager.c` -- Event Bus. Pattern: `GetInstance().GetInvoker(eventName).Insert(YourMethod)`, `.Publish(eventName, payload)`. Deliberately does NOT centralise Subscribe/Unsubscribe with a generic handler parameter -- `ScriptInvokerBase` used bare as a parameter type does not compile ("Not enough parameters for template class"). Use the concrete `ScriptInvoker` returned by `GetInvoker()` directly.
- `addons/MCF/Scripts/Game/Core/MCF_Core_TagRegistry.c` -- tag string -> `IEntity` lookup.
- `addons/MCF/Scripts/Game/Core/MCF_Core_ObjectIdentityComponent.c` -- placeable component giving an entity a tag, registers/unregisters itself with `MCF_Core_TagRegistry` via `EOnInit`/`OnDelete`. Includes the paired `MCF_Core_ObjectIdentityComponentClass : ScriptComponentClass`.

## Key lesson carried forward

Event Bus **lifecycle cleanup is NOT centralised**. Every module's own component removes its own subscriptions in its own `EOnDeactivate`, calling `MCF_Core_EventManager.GetInstance().GetInvoker(eventName).Remove(itsOwnNamedMethod)` directly -- this works because every `ScriptComponent` already gets `EOnDeactivate` for free, and because the method reference is concretely typed at that call site (no generic parameter needed). Do not try to route this through `ObjectIdentityComponent` or any other central place again; that was tried and reverted for a language-level reason, not a design preference.

## Not yet started

- `MCF_Core_ModuleRegistry` (module registration, load-order, version/dependency checks -- ARCHITECTURE.md 3, Core table)
- `MCF_Core_TickManager` (priority-tiered central update loop -- ARCHITECTURE.md 3 + 7.8)
- Phase 0's actual deliverable: one GM-placeable Trigger Zone entity using `SCR_EditableEntityComponent` + `PLACEABLE`, with live editor attributes and a full save/load round-trip test (ARCHITECTURE.md section 9, Phase 0 row)
- Autotest suite / Stress Profile registration for any of the above (ARCHITECTURE.md 3.2 -- was declared mandatory from Phase 0 onward, not yet honoured for the three files that exist)

## Immediate next step recommendation

Before writing more Core classes: register a Stress Profile / Autotest suite for what already exists (EventManager, TagRegistry, ObjectIdentityComponent), since section 3.2 made that mandatory "from Phase 0" and it has been skipped twice now under time pressure. Then continue with ModuleRegistry/TickManager, or jump straight to the Trigger Zone entity if validating the placement/save-load pipeline is the higher priority.
