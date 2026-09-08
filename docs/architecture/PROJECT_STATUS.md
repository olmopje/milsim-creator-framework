# Project Status

Supersedes `PHASE0_PROGRESS.md` (kept below for historical environment notes). Last updated after completing the full phased roadmap (ARCHITECTURE.md section 9, Phases 0-14) plus a follow-up session closing several "manual driver" gaps.

## Where things stand

Every phase in the roadmap has a working, confirmed-compiling implementation. See `CHANGELOG.md` [0.2.0] for the full file-by-file list. This is still first-version work throughout -- crude but functional, not polished -- consistent with how the project was built: get something real and testable first, refine later.

**What now runs automatically** (no longer needs a manual driver call):
- `MCF_Core_TickManagerComponent`, via `MCF_Core_GameLoopComponent`'s `EOnFrame`
- `MCF_AI_CommandWatchdogComponent`, via `MCF_Core_TickCritical`
- `MCF_Hostility_Manager` decay, via `MCF_Core_TickCosmetic` (once `StartAutoDecay()` is called)
- `MCF_React_SequencePlaybackComponent`, via its own `EOnFrame` -- and it now actually relocates its owner along the recorded path, not just exposes position data
- `MCF_AI_SimpleMoverComponent`, via its own `EOnFrame` while `MoveTo()` is active
- `MCF_AAR_DebriefManager` and `MCF_Hostility_Manager` decay can now both be started automatically at mission start by adding `MCF_Core_GameModeComponent` to the GameMode entity, instead of needing a manual kickoff call from somewhere

**What still needs a manual driver:**
- `MCF_React_SequenceRecorderComponent.RecordSample()`/`RecordCue()` (recording is an authoring-time action, not live gameplay -- lower priority than the pieces above)

## Confirmed-working engine APIs (no longer need re-verifying)

- `EOnInit`, `OnDelete`, `EOnFrame` + `SetEventMask(owner, EntityEvent.FRAME)`
- `GetGame().GetWorld().GetWorldTime()`
- `Math.RandomFloat01()`, `Math.RandomInt()`, `Math.Clamp()`, `Math.Acos()`, `Math.RAD2DEG`
- `vector.Distance()`, `vector.Dot()`, `.Normalize()`
- `ScriptInvoker` (via `GetInvoker().Insert()/.Remove()` -- **not** `ScriptInvokerBase` as a bare parameter type, that does not compile)
- `string.Split()`, `string.Format()`, `.ToInt()`
- `FindComponent()`, `Cast()`
- `SCR_EditableEntityComponent` requires a `RplComponent` with its own GUID to avoid a "missing RplComponent" error on placeable prefabs

## Confirmed native systems found via research (not yet integrated)

- **`SCR_AIGroup.AddWaypoint(waypoint)`** -- the real way vanilla AI groups move, confirmed via actual base-game source (`SCR_AmbientPatrolSpawnPointComponent.c`). Our `MCF_AI_SimpleMoverComponent` predates this discovery and is a straight-line `SetOrigin()` fallback for entities that are **not** in a real `SCR_AIGroup` (e.g. a standalone civilian) -- for anything with a proper AI group, use native waypoints instead of our mover.
- **`SCR_AIAnimationWaypoint`** -- a native `SCR_AIWaypoint` subclass specifically for triggering an animation on arrival, confirmed to exist alongside `SCR_DefendWaypoint`, `SCR_SuppressWaypoint`, etc. This is the correct tool for "AI plays an animation here", not something we need to build ourselves. `MCF_AI_WaypointAnimationComponent` remains the event-only fallback for entities outside an AI group.
- A speculative guess at `CharacterControllerComponent.PlayGesture()`/`CanPlayGesture()` for directly triggering a gesture from script did **not** compile ("Undefined function") -- reverted rather than guessed further. `CharacterControllerComponent`, `CharacterAnimationComponent`, and `AITaskPlayGesture` are all confirmed to exist and gestures are a real, working system in the base game (used in the training mission), but the exact current script-callable entry point was not found through available research tools. Next lead, if picked up again: check the Script Editor's live autocomplete on `GetAnimationComponent()`, or look at how `AITaskPlayGesture` (an `AITaskScripted`-style node) is wired into a behavior tree in the Behavior Editor -- that's a Workbench GUI task, not pure script.

## Deliberately unsolved, by design (not oversights)

- **No raycast/navmesh API was ever confirmed.** Rather than guess, self-built workarounds were built instead of the missing pieces we could reasonably approximate:
  - `MCF_AI_ComplianceComponent.IsBeingAimedAt()` -- distance + angle approximation, not true line-of-sight
  - `MCF_AI_FallbackPointRegistry` -- mission-maker-placed safe points instead of a geometry query
  - `MCF_AI_SimpleMoverComponent` -- straight-line movement via `SetOrigin()` each frame, not real pathfinding; used by Sequence Playback and Ambient Actor so things actually move now instead of just publishing an event. **Superseded by native `SCR_AIGroup` waypoints for anything with a real AI group** -- see above.
- **AND logic** (`MCF_Obj_LogicComponent`) is capped at 4 fixed input slots, not an arbitrary list -- Enforce Script has no closures to generate per-input callbacks dynamically.
- **Animation is still not wired up from our own components** (`MCF_AI_WaypointAnimationComponent` still only publishes a "this animation was requested" event) -- see the native `SCR_AIAnimationWaypoint` note above for the actual path forward with real AI groups.

## Reload/compile-check workflow reminder

`enfusion-mcp:wb_reload` is frequently flaky -- it often reports success without a fresh compile actually appearing in the log. When that happens: retry a few times with 15-20s waits, and if it stays stuck, ask whoever is at the PC to click into the Workbench window (giving it focus reliably un-sticks it). Always confirm via the log's `Compiling Game scripts took` / `SCRIPT (E)` lines, never trust the tool's "Reload Complete" message alone.

---

# Historical: original Phase 0 handoff notes

The section below is the original handoff document written after Phase 0 alone. Kept for the environment-setup context (Workbench launch quirks, MCP tool history); the "what compiles" and "not yet started" sections are outdated -- see above instead.

## Environment, confirmed working

- Repo: `G:\MCF` (git, `main` branch)
- Workbench project: `G:\MCF\addons\MCF\addon.gproj`
- **Launching Workbench reliably only works when the user starts it manually** (Steam UI or a desktop shortcut). Scripted launches (`wb_launch`, direct .exe, `steam.exe -applaunch`) were all unreliable for resolving the base-game addon path.
- Once running (World Editor mode, Net API enabled), `enfusion-mcp` tools work normally.
- `enfusion-mcp:mod_build` also spawns its own process and hits the same launch issue -- use `wb_reload` against an already-running, user-launched instance instead.
