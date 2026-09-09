# Project Status

Supersedes `PHASE0_PROGRESS.md` (kept below for historical environment notes). Last updated after completing the full phased roadmap (ARCHITECTURE.md section 9, Phases 0-14) plus a follow-up session closing several "manual driver" gaps.

## MAJOR UPDATE (2026-09-09, later session): EOnInit never fired -- framework-wide

**This supersedes the "What now runs automatically" list below. Until this fix,
almost none of it actually ran.**

### The bug

Every MCF component used `EOnInit` without ever setting the INIT event mask.
In Enfusion, `EOnInit` only fires if `EntityEvent.INIT` is in the entity's
event mask, and that mask has to be set from `OnPostInit`:

```c
override void OnPostInit(IEntity owner)
{
    SetEventMask(owner, EntityEvent.INIT);
}
```

Confirmed against vanilla `scripts/Game/Components/AreaMesh/SCR_BaseAreaMeshComponent.c`,
which does exactly this. 19 MCF components were missing it.

### What that actually broke

`MCF_Core_GameLoopComponent` sets its FRAME mask *inside* `EOnInit`. With
`EOnInit` never firing, `EOnFrame` never ran, so `MCF_Core_TickManagerComponent`
never updated, so `MCF_Core_TickCritical` / `MCF_Core_TickCosmetic` never fired.
Everything described below as "self-driving" was inert. Placed trigger nodes
initialised nothing, registered no tick, and silently did nothing forever.

It also explains the `m_Owner=NULL` seen earlier when writing a radius through
the Editor Attributes panel: `m_Owner` is assigned in `EOnInit`, which had
never run.

Nothing logged an error. The components compiled, placed and looked fine.

### Fix

`OnPostInit` + `SetEventMask(owner, EntityEvent.INIT)` added to all 19
affected components. `MCF_ProximityAreaMeshComponent` was skipped -- it
inherits `SCR_BaseAreaMeshComponent`, whose own `OnPostInit` already sets the
mask. That file is orphaned (referenced by no prefab) and is a deletion
candidate.

**Rule going forward: any new ScriptComponent that overrides `EOnInit` must
also override `OnPostInit` and set the INIT mask, or it will silently never
initialise.**

## The missing output layer

`MCF_Voice_LineQueueManager` published `MCF_Voice_LineDisplayed` and nothing
consumed it, so no MCF module could ever put anything on a player's screen.
The framework had no player-visible output at all.

New: `MCF_UI_LineDisplayComponent` (Scripts/Game/Modules/). Subscribes to
`MCF_Voice_LineDisplayed`, renders through vanilla
`SCR_PopUpNotification.GetInstance().PopupMsg(text, duration, subtitle)`, and
calls `MarkLineFinished()` after the duration so the queue drains. Place it on
the GameMode entity. `SCR_PopUpNotification` spawns itself on first use and
skips headless, so no scene setup is needed.

## First working vertical slice (confirmed in play)

Placed in `worlds/arland/MCFTestworld.ent`:

- `MCF_Slice_ProximityTrigger` at 1891 11.4 1821, radius 15, trigger-once off
- `MCF_Slice_Recipe` at 1896 11.4 1821, trigger event `MCF_Obj_ProximityDetected`,
  one step `PLAY_TEXT_LINE:Contact! Sentries are alerted.`
- `MCF_UI_LineDisplayComponent` on `GameMode_Editor_Full`

Walking a possessed character into the radius produced, in one millisecond:

```
[MCF] ProximityTrigger FIRED, publishing MCF_Obj_ProximityDetected
[MCF] Recipe TRIGGERED by MCF_Obj_ProximityDetected
[MCF] Recipe running step: PLAY_TEXT_LINE:Contact! Sentries are alerted.
[MCF] LineDisplay showing: Contact! Sentries are alerted.
```

...and the popup on screen. Detection -> event bus -> recipe -> line queue ->
screen, four components cooperating, visible to the player. This is the first
time any part of MCF has worked end to end.

`[MCF]` diagnostic Prints were left in on init and fire paths deliberately --
they are what made this bisectable, and this framework is young enough to want
them. Strip them when things stabilise.

## Second slice: multi-node chaining and state aggregation (confirmed in play)

Extends the first slice to prove that event chaining holds across several
nodes and that state can accumulate — the mechanism follow-up actions will be
built on. Placed in `worlds/arland/MCFTestworld.ent`, all configuration, no
new script:

- `MCF_Slice_TriggerA` at 1860 14.5 1840, radius 15 -> `MCF_Slice_PointA`
- `MCF_Slice_TriggerB` at 1905 14.5 1840, radius 15 -> `MCF_Slice_PointB`
- `MCF_Slice_Logic`, AND mode, inputs PointA + PointB -> `MCF_Slice_BothVisited`
- `MCF_Slice_Alarm`, relays `MCF_Slice_BothVisited` -> `MCF_Slice_ReportReady`
- `MCF_Slice_RecipeA` / `RecipeB` / `RecipeReport`, one line each

Confirmed trace:

```
14:11:53  ProximityTrigger FIRED, publishing MCF_Slice_PointA
14:11:53  LineDisplay showing: Position ALPHA checked.
14:11:53  Logic AND state: 1=true 2=false 3=false 4=false     <- correctly does not fire
14:12:05  ProximityTrigger FIRED, publishing MCF_Slice_PointB
14:12:05  LineDisplay showing: Position BRAVO checked.
14:12:05  Logic AND state: 1=true 2=true 3=false 4=false
14:12:05  Logic AND satisfied, publishing MCF_Slice_BothVisited
14:12:05  AlarmTrigger relaying MCF_Slice_BothVisited -> MCF_Slice_ReportReady
14:12:05  Recipe TRIGGERED by MCF_Slice_ReportReady
14:12:09  LineDisplay showing: Both positions checked. Sending report.
```

Proven working: several independent triggers with distinct event names,
several Recipes coexisting each on its own event, `MCF_Obj_LogicComponent` AND
mode aggregating state across nodes and correctly staying silent on a partial
condition, `MCF_Obj_AlarmTriggerComponent` as a relay, and a five-link chain
end to end.

### Known behaviour: the line queue serialises, and that costs latency

Note the last line: the report was published at 14:12:05 but only reached the
screen at 14:12:09 — a four second gap. That is not a bug. It is
`MCF_Voice_LineQueueManager` doing its job: "Position BRAVO checked." was still
on screen, so the report waited for `MarkLineFinished()`, which
`MCF_UI_LineDisplayComponent` calls after `m_fDisplayDuration`.

It confirms the queue and the release logic work. It also means that in a
busier mission, related messages will visibly lag behind the events that
caused them. Worth tuning before the framework is used for anything paced:
options are a shorter default duration, using the priority argument the queue
already supports, or letting some categories of message bypass the queue and
render immediately.

## Objective node made functional, and a static-singleton hazard

`MCF_Obj_ObjectiveComponent` was inert in both directions: `Complete()` and
`Fail()` were only reachable from script and nothing called them, and its
title/description/map flag were stored but never rendered. It now takes
`m_sCompleteEvent` / `m_sFailEvent` so it is wired by event name like every
other node, and announces unlock/completion/failure through the line queue.
`Complete()` respects the intel gate. `m_bVisibleOnMap` still does nothing —
see `docs/research/objective-task-system.md` for the vanilla task system
research and the three integration options.

### Static singletons survive the editor -> play transition

Found the hard way. The objective announced itself when it was placed in the
World Editor. There is no `SCR_PopUpNotification` widget in that context, so
the render failed — but `MCF_Voice_LineQueueManager` had already set
`m_bBusy = true`, and nothing ever called `MarkLineFinished()`. Because the
queue manager is a `private static ref` singleton, that stuck state carried
straight into the play session: every line queued correctly and none were ever
shown. The whole logic chain worked perfectly and was completely invisible.

Three fixes:

- `MCF_UI_LineDisplayComponent.Show()` releases the queue immediately when
  `SCR_PopUpNotification` is unavailable, instead of relying on a `CallLater`
  that may never run.
- `MCF_Voice_LineQueueManager.Reset()` added.
- `MCF_Core_GameModeComponent.OnGameModeStart()` calls it, so per-mission state
  is cleared at mission start.

**Rule going forward: any static manager holding per-mission state must be
reset from `OnGameModeStart()`.** `MCF_Core_EventManager`,
`MCF_Core_AutoWatcherRegistry`, `MCF_Core_ValidationRegistry`,
`MCF_AI_FallbackPointRegistry`, `MCF_Hostility_Manager` and
`MCF_AAR_DebriefManager` are all statics and have not been audited for this.

### Confirmed working, with measured queue latency

```
14:23:07  LineDisplay showing: Position ALPHA checked.
14:23:11  LineDisplay showing: New objective: Recon both approach routes
14:23:25  LineDisplay showing: Position BRAVO checked.
14:23:29  LineDisplay showing: Both positions checked. Sending report.
14:23:33  LineDisplay showing: Objective complete: Recon both approach routes
```

Note the cost of serialising: the objective-complete line reached the screen at
14:23:33 for an event that fired at 14:23:25 — **eight seconds late**, because
two other lines were ahead of it at four seconds each. This is now a measured
problem rather than a theoretical one, and it is the most obvious thing to
tune next: a shorter default duration, use of the priority argument the queue
already supports, or letting some categories bypass the queue entirely.

## MCF runs on a real dedicated server, and custom data persists

Two firsts on 2026-09-09.

### Running the dedicated server with an unpublished local mod

`ArmaReforgerServer.exe` is a separate Steam app (1874900) from the game and
the Tools. Once installed, the working invocation is:

```
ArmaReforgerServer.exe
  -server "{B8BD092E327C2224}Missions/MCFTestworld.conf"
  -addonsDir "G:\MCF\addons"
  -addons MCF
  -profile "G:\MCF\server\profile"
  -maxFPS 60
```

**`-config` and `-addons` are mutually exclusive** — using both fails hard with
`-config cannot be used together with addons!`. The documented route for
testing an unpublished mod is `-server` (which overrides `-config`) combined
with `-addonsDir` and `-addons`. A `server.json` exists at
`G:\MCF\server\MCF_Server.json` and validates clean, but is unused while
testing this way; it is there for when MCF is published to the Workshop.

Confirmed from the server log: the addon is found and loaded
(`gproj: 'G:/MCF/addons/MCF/addon.gproj' guid: '6A50E40BA3B94A4F'`), scripts
compile, the world loads, every MCF component initialises, `OnGameModeStart`
fires, and `Starting RPL server, listening on address 0.0.0.0:2001` /
`Entered online game state.` follow.

Iterating no longer requires Workbench: the server compiles the addon's scripts
itself, so a code change plus a server restart is the whole loop.

**It also confirmed the networking problem is real.** `[MCF] LineDisplay init`
runs on the server, as does every trigger and recipe. With no
`Replication.IsServer()` guard anywhere, the same chain will run independently
on every client too. See `docs/research/multiplayer-and-audience.md`.

### Custom data survives restarts — via file IO, not the engine save system

`FileIO` (`scripts/Core/generated/System/FileIO.c`) lets a mod read and write
files under `$profile:`, `$logs:` and `$saves:`. `$profile:` resolves inside the
directory given to `-profile`.

New: `MCF_Core_PersistentStore` (`Scripts/Game/Core/`), a key/value store
written as `key=value` lines to `$profile:MCF_store.txt`, loaded from
`MCF_Core_GameModeComponent.OnGameModeStart()`. Verified across two server
runs — run 2 read what run 1 wrote and reported
`this server has started 2 times -- previous runs survived restart`.

This deliberately does not use the engine's world/session persistence, because
mod changes can invalidate those saves ("a large mod change" is effectively a
world reset). A unit running a server all week while the mod is still being
developed would otherwise lose a week of planning to a mod update. Plans and
similar cross-session data belong in this store, keyed by name, not in world
state.

Note `GameSessionStorage` is a different thing: it survives script/addon
reloads within one executable run, not restarts.

This answers the blocking question for the persistent-server design in
`docs/research/persistent-server-and-phases.md`.

## Server authority and the presentation split

MCF had no network awareness at all. Every trigger, logic node, recipe and
objective ran unguarded, which on a real server means every client runs its own
copy: its own detection against its own view of the world, its own events, its
own trigger-once state. "Fires once" would have meant once per machine.

Two changes, both following the vanilla pattern in `SCR_EditorTask` and
`SCR_TaskSystemNetworkComponent`.

### Detection is server-authoritative

`MCF_Obj_ProximityTriggerComponent`, `MCF_Obj_ConeDetectionTriggerComponent`
and `MCF_Obj_SpottedByPlayerComponent` now return early from `EOnInit` when
`!Replication.IsServer()`, so a client never subscribes to the tick and never
evaluates detection. Everything downstream (Logic, Alarm, Recipe, Objective)
follows automatically, because the events that drive them are only published
server-side.

### Presentation is broadcast, and filtered locally

`MCF_UI_LineDisplayComponent` is now the seam between server logic and player
screens:

- **server** hears `MCF_Voice_LineDisplayed`, calls
  `Rpc(RpcDo_ShowLine, text, audience)` (broadcast) plus the same method
  locally, then releases the line queue
- **client** receives the RPC, checks the audience, and renders through
  `SCR_PopUpNotification`

A plain `ScriptComponent` can host RPCs — confirmed against
`SCR_TaskSystemNetworkComponent`, which is exactly that. The component sits on
the GameMode entity, which carries an `RplComponent` and exists on every
machine.

The RPC already carries an `MCF_EAudience` so the wire format will not change
when real filtering lands. Only `EVERYONE` is implemented; anything else logs
and shows to everyone rather than silently swallowing the message. The command
hierarchy is the intended audience model — see
`docs/research/mcf-task-system-design.md`.

### The line queue no longer serialises

Previously each line was held on screen for `m_fDisplayDuration` before the
next was allowed through, which cost a measured eight seconds between an event
and its line appearing, and could deadlock if a line was never marked finished.

`SCR_PopUpNotification` maintains its own priority queue and orders overlapping
popups itself, so MCF's serialisation was redundant on top of it. The queue is
now released immediately after broadcast and presentation order is left to
vanilla. `m_fDisplayDuration` is purely how long the popup stays visible.

### VERIFIED on a Peer Tool session

Proven 2026-09-09 with Workbench Local Host plus two Peer Tool clients — three
separate processes, three separate log files.

Host:

```
15:17:51.784  [MCF] ProximityTrigger FIRED, publishing MCF_Slice_PointB
15:17:51.784  [MCF] Logic AND satisfied, publishing MCF_Slice_BothVisited
15:17:51.784  [MCF] AlarmTrigger relaying ... -> MCF_Slice_ReportReady
15:17:51.784  [MCF] LineDisplay broadcasting: Both positions checked. Sending report.
```

Peer 1 and Peer 2, ~20 ms later, in their own processes:

```
15:17:51.802  [MCF] LineDisplay showing: Both positions checked. Sending report.
15:17:51.808  [MCF] LineDisplay showing: Both positions checked. Sending report.
```

All six lines of the run reached both peers. Neither peer logged a single
`broadcasting`, `FIRED` or `Logic AND` line — they ran none of the logic, only
the rendering. The split works in both directions.

Client-side confirmation of the detection guard:

```
[MCF] LineDisplay init on client -- will render lines received by RPC
[MCF] Controllable spawned -- notifying 0 proximity and 0 spotted triggers
```

Zero triggers registered on a client, because the guard returns before
registration.

### The Peer Tool is the way to test this

`ArmaReforgerServer.exe` cannot load an unpublished mod together with a
`server.json`: `-config` and `-addons` are mutually exclusive, and putting the
addon in the config's `mods` array fails with `Addon <guid> - Addon was not
found on workshop` because that list is validated against the Workshop API
regardless of the addon being present locally. Running with `-server` works but
leaves the server unregistered and with no A2S port, so the client's manual
connect reports `SERVER_NOT_FOUND`.

The Peer Tool sidesteps all of it. It sits in the **dropdown next to the Play
button** in the World Editor (Local Host + Peer Tool), and it spawns peers as
separate processes logging to `Documents\My Games\PeerPlugin1` and
`PeerPlugin2`. Its peers launch with `-client -addonsDir <mod> -addons <name>`,
so the unpublished mod loads without any Workshop involvement.

Watch the port: Local Host binds UDP 2001, so a dedicated server started
alongside it dies with `Unable to start replication`. Only one of them at a
time.

### Follow-up found by this test

`OnGameModeStart` fires on **every** machine, clients included — its own
documentation says so. The peers were therefore loading and rewriting
`MCF_Core_PersistentStore` in their own profiles and incrementing a server run
counter that means nothing there. `LoadPersistentState()` and
`OnControllableSpawned` are now guarded with `Replication.IsServer()`. Compiles
clean; the guarded behaviour itself is not yet re-verified on a peer session.

## Two stale sections below, now resolved

- The "Still open" section on the in-game Game Master properties panel is done:
  Editor Attributes work (see `docs/research/editor-attributes-research.md`).
- The `m_EditableEntityFlags` / `m_sDisplayName` section is superseded. The real
  problem was `m_Flags` being authored as a braced name list instead of an
  integer bitmask, which broke the whole prefab parse. Removing the block fixed
  all 12 prefabs. See `docs/research/prefab-mflags-parse-bug.md`.

## Where things stand

Every phase in the roadmap has a working, confirmed-compiling implementation. See `CHANGELOG.md` [0.2.0] for the full file-by-file list. This is still first-version work throughout -- crude but functional, not polished -- consistent with how the project was built: get something real and testable first, refine later.

**What now runs automatically** (no longer needs a manual driver call):
- `MCF_Core_TickManagerComponent`, via `MCF_Core_GameLoopComponent`'s `EOnFrame`
- `MCF_AI_CommandWatchdogComponent`, via `MCF_Core_TickCritical`
- `MCF_Hostility_Manager` decay, via `MCF_Core_TickCosmetic` (once `StartAutoDecay()` is called)
- `MCF_React_SequencePlaybackComponent`, via its own `EOnFrame` -- and it now actually relocates its owner along the recorded path, not just exposes position data
- `MCF_AI_SimpleMoverComponent`, via its own `EOnFrame` while `MoveTo()` is active
- `MCF_AAR_DebriefManager` and `MCF_Hostility_Manager` decay can now both be started automatically at mission start by adding `MCF_Core_GameModeComponent` to the GameMode entity, instead of needing a manual kickoff call from somewhere
- `MCF_Obj_ProximityTriggerComponent` and `MCF_Obj_SpottedByPlayerComponent` auto-register every newly spawned controllable entity as a watcher, via `MCF_Core_AutoWatcherRegistry` + `MCF_Core_GameModeComponent.OnControllableSpawned()` -- no more manual `RegisterWatchedEntity()`/`RegisterWatcher()` calls needed for the common case. Note: this fires for AI-controlled entities too, not confirmed player-only.
- `MCF_Interact_HintComponent` now has a real player-facing trigger: `MCF_Interact_TalkAction`, a `ScriptedUserAction` that shows a "Talk" prompt and calls `Interact()` -- this was the missing link between a placed NPC and an actual player pressing a button.

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
- **`Prefabs/Systems/Milsim.et`** -- our own GameMode prefab, inheriting from the confirmed vanilla base `{1B76F75A3175E85C}Prefabs/MP/Modes/Plain/GameMode_Plain.et`, with `MCF_Core_GameModeComponent`/`MCF_Core_TickManagerComponent`/`MCF_Core_GameLoopComponent` added. Building it this way (a `prefab_create` with the correct `parentPrefab` GUID) was necessary -- using `prefabType: "gamemode"` with no parent produces a `GenericEntity`-rooted prefab that fails at runtime ("required type=SCR_BaseGameMode! This is not allowed!").
  - **Two Workbench crashes (access violations) happened testing this live in-world**, both while creating/loading an entity from this prefab. Root cause: `prefab_create` had also emitted an empty `SCR_RespawnSystemComponent {}` override, which reset the parent's already-correct spawn configuration instead of leaving it alone, producing "SCR_RespawnSystemComponent is missing SCR_SpawnLogic!" followed by a crash. Fixed by removing that empty override (and an invalid `m_sDisplayName` block) from the file.
  - **A world only ever has one GameMode entity, and Workbench's Resource Browser correctly refuses to let you drag a second one in** -- this is not a bug, it's what stopped us from creating the duplicate-gamemode conflict a second time. Placing a whole separate GameMode prefab (ours or vanilla) into a world is the wrong mental model entirely.
  - **The correct, confirmed-working approach: use Workbench's own "Game Mode Setup" plugin** (Plugins menu), which runs a wizard that scans the world, auto-generates every required entity (`SCR_FactionManager`, `SCR_LoadoutManager`, `SCR_AIWorld`, `PerceptionManager`, etc. -- the same list BI's own General Game Mode Setup wiki page describes) in one step, and creates a mission header (`.conf`) pointing at a complete, working GameMode entity (e.g. `SCR_GameModeEditor` for a Game Master scenario). **Then add our three MCF components directly to that wizard-generated GameMode entity** (`wb_component add`, same as any other component) instead of trying to swap in a separate prefab. This is what actually worked, confirmed zero errors, world saved successfully. `Prefabs/Systems/Milsim.et` is consequently no longer the recommended path for a real scenario -- kept for reference/reuse in a from-scratch prefab-based setup if ever needed, but the wizard+add-components route is simpler and proven.
  - A leftover "Multiple game mode entities present!" warning citing a phantom `oldEntity: ENTITY:0 ('SCR_BaseGameMode') at 0,0,0` appeared consistently across every world tested, including brand new ones, **without causing a crash** when the rest of the setup was correct -- this looks like a benign Workbench-internal editor artifact, not a real duplicate, based on it appearing even when only one real GameMode entity existed.
  - The wizard's auto-generated `MapEntity` ships with empty "Map Geometry Data" and "Satellite background" fields, which the wizard itself flags ("MapEntity: Set up map textures") but doesn't block on. Leaving them empty causes a **NULL pointer Virtual Machine Exception in native `SCR_MapEntity::UpdateViewPort`** the moment a player opens the in-game map -- not an MCF bug, confirmed by the stack trace being entirely inside `Scripts/Game/Map/`. Fix: select the MapEntity, and fill in both fields via their `..` browse button with the map's own data (e.g. Arland-specific map geometry/satellite resources for an Arland-based world).
- **"Spotted" detection (an AI noticing the player via its own perception, not a distance/cone approximation) is not built.** The native `PerceptionManager`/`EAIDangerEventType` system (seen in the Game Mode Setup wizard's auto-generated entities and the AI script API's enum group) is presumably the real hook for this, but the exact script-callable entry point hasn't been researched/confirmed yet -- same category of gap as the gesture-trigger API. `MCF_Obj_ProximityTriggerComponent`/`MCF_Obj_ConeDetectionTriggerComponent` cover the geometric approximations (distance, facing direction); genuine AI-perception-based detection is a distinct, still-open follow-up.
- **All hand-written `SCR_EditableEntityComponent { m_EditableEntityFlags "PLACEABLE" m_sDisplayName "..." }` blocks in our prefabs are wrong and silently do nothing** -- confirmed via `WORLD (E): Unknown keyword/data 'm_EditableEntityFlags'`/`'m_sDisplayName'` on placement. This affects every prefab we've hand-written this way (all 8+ `.et` files under `Prefabs/`), not just new ones -- earlier ones just happened not to surface this specific error message when we checked them.
  - **This does NOT block using our nodes at all** -- every prefab still places and works fine via Workbench (`wb_entity_create`, or dragging in the World Editor) despite the flag being silently ignored. The flag only matters for a player using the **in-game, live** Game Master placement menu during actual gameplay to spawn a new instance from scratch -- Workbench-time placement while building a scenario is unaffected.
  - Attempted the BI-documented fix (Workbench's "Create/Update Selected Editable Prefabs" plugin, which needs an `EditablePrefabsConfig.conf` folder-rule mapping our `Prefabs/` source to a `PrefabsEditable/` target) but could not get it working: the base game's own config (at `ArmaReforger:Configs/Workbench/EditablePrefabs/EditablePrefabsConfig.conf`) has no MCF entry, and creating a local override in `MCF:Configs/Workbench/EditablePrefabs/EditablePrefabsConfig.conf` with a guessed `m_aFolderRules { SCR_EditablePrefabsFolderRule { m_SourceDirectory ... m_TargetDirectory ... } }` structure failed with "Unknown keyword/data 'm_aFolderRules'" -- the guessed class/field names are wrong, and no source confirms the correct ones. **Reverted the override to empty** rather than leave a broken guess in the repo.
  - **Open follow-up, not currently blocking:** find the correct `EditablePrefabsConfig`/`SCR_EditablePrefabsFolderRule`-equivalent schema (Script Editor's live autocomplete on a fresh `EditablePrefabsConfig {}` block would likely reveal the real field names immediately -- faster than more external research) so our prefabs can eventually support live in-game Game Master placement too.

## Deliberately unsolved, by design (not oversights)

- **No raycast/navmesh API was ever confirmed.** Rather than guess, self-built workarounds were built instead of the missing pieces we could reasonably approximate:
  - `MCF_AI_ComplianceComponent.IsBeingAimedAt()` -- distance + angle approximation, not true line-of-sight
  - `MCF_AI_FallbackPointRegistry` -- mission-maker-placed safe points instead of a geometry query
  - `MCF_AI_SimpleMoverComponent` -- straight-line movement via `SetOrigin()` each frame, not real pathfinding; used by Sequence Playback and Ambient Actor so things actually move now instead of just publishing an event. **Superseded by native `SCR_AIGroup` waypoints for anything with a real AI group** -- see above.
- **AND logic** (`MCF_Obj_LogicComponent`) is capped at 4 fixed input slots, not an arbitrary list -- Enforce Script has no closures to generate per-input callbacks dynamically.
- **Animation is still not wired up from our own components** (`MCF_AI_WaypointAnimationComponent` still only publishes a "this animation was requested" event) -- see the native `SCR_AIAnimationWaypoint` note above for the actual path forward with real AI groups.

## Reload/compile-check workflow reminder

`enfusion-mcp:wb_reload` is frequently flaky -- it often reports success without a fresh compile actually appearing in the log. When that happens: retry a few times with 15-20s waits, and if it stays stuck, ask whoever is at the PC to click into the Workbench window (giving it focus reliably un-sticks it). Always confirm via the log's `Compiling Game scripts took` / `SCRIPT (E)` lines, never trust the tool's "Reload Complete" message alone.

After many reload cycles in one long session, Workbench can pop an Enfusion engine "Assertion failed: Resources are leaking!" dialog (in `GameApp.cpp`, unrelated to any of our own script files). Clicking **Retry** has been observed to resolve it and let Workbench continue working normally -- this is an engine-internal resource-tracking assertion, not something caused by MCF code.

---

# Historical: original Phase 0 handoff notes

The section below is the original handoff document written after Phase 0 alone. Kept for the environment-setup context (Workbench launch quirks, MCP tool history); the "what compiles" and "not yet started" sections are outdated -- see above instead.

## Environment, confirmed working

- Repo: `G:\MCF` (git, `main` branch)
- Workbench project: `G:\MCF\addons\MCF\addon.gproj`
- **Launching Workbench reliably only works when the user starts it manually** (Steam UI or a desktop shortcut). Scripted launches (`wb_launch`, direct .exe, `steam.exe -applaunch`) were all unreliable for resolving the base-game addon path.
- Once running (World Editor mode, Net API enabled), `enfusion-mcp` tools work normally.
- `enfusion-mcp:mod_build` also spawns its own process and hits the same launch issue -- use `wb_reload` against an already-running, user-launched instance instead.

---

# MAJOR UPDATE (2026-09-09 session): Game Master live placement -- SOLVED

The `m_EditableEntityFlags`/`m_sDisplayName` issue described below under "Deliberately unsolved" was investigated to full resolution in a marathon debugging session. **In-game Game Master placement of MCF prefabs now works.** The old notes below are kept for historical context but are superseded by this section wherever they conflict.

## The complete, confirmed-working recipe for a placeable "System"-type MCF prefab

Every one of the required pieces below was independently confirmed necessary by controlled A/B testing (adding one piece at a time, or duplicating a known-working vanilla prefab and comparing byte-for-byte). Missing any one of them results in the prefab loading with **zero errors in the log** but never appearing in the Game Master Entity Browser -- this silent-failure mode is what made it so hard to find.

```
GenericEntity {
 ID "<GUID>"
 components {
  <YourGameplayComponent> "<GUID>" {
   ...
  }
  SCR_EditableEntityComponent "<GUID>" : "{996046FE206C699A}Prefabs/Editor/Components/Default_SCR_EditableEntityComponent.ct" {
   m_EntityType SYSTEM
   m_Flags {
    PLACEABLE
    VIRTUAL
    HAS_AREA
   }
   m_UIInfo SCR_EditableEntityUIInfo "<GUID>" {
    Name "Your Display Name"
    m_aAuthoredLabels {
     ENTITYTYPE_SYSTEM
    }
   }
  }
  RplComponent "<GUID>" {
  }
  Hierarchy "<GUID>" {
  }
 }
}
```

**Every piece and why it's required:**

1. **`SCR_EditableEntityComponent` MUST inherit from `Default_SCR_EditableEntityComponent.ct`** (`{996046FE206C699A}Prefabs/Editor/Components/Default_SCR_EditableEntityComponent.ct`), not be declared bare. Confirmed via the official BI wiki + `Configs/Workbench/EditablePrefabs/EditablePrefabsComponent_EditableEntity.conf`, which states entities using this template are "already flagged as PLACEABLE".
2. **`m_EntityType SYSTEM`** must be set explicitly. Default is `GENERIC` (index 0) -- confirmed via `Scripts/Game/Editor/Enums/EEditableEntityType.c`. `GENERIC` items do not show under any Entity Browser filter tab we could find.
3. **`m_Flags` needs more than just `PLACEABLE`.** Decoded from three different working vanilla examples (`E_EditorRestrictionZoneSmall.et`, `E_SpawnPoint_US.et`, `EffectModule_MineField_Small_US.et`) via `game_duplicate` + `wb_resources getInfo`: all three use `PLACEABLE | VIRTUAL` at minimum, with `HAS_AREA` added for anything that represents a zone/trigger radius. `VIRTUAL` ("Entity is represented by virtual objects that have to be updated" -- `Scripts/Game/Editor/Enums/EEditableEntityFlag.c`) is very likely the single most important addition for mesh-less logic entities like ours.
4. **`m_UIInfo` must be `SCR_EditableEntityUIInfo`, not generic `SCR_UIInfo`.** Confirmed by the crash stack trace when it's missing/wrong (`SCR_EditableEntityUIInfo.c:245 Function ExtractEditableUIInfoFromPrefab`).
5. **`m_aAuthoredLabels { ENTITYTYPE_SYSTEM }` inside `m_UIInfo` -- this was the actual missing piece, found last, after everything else above was already correct and the entity still didn't appear.** All three vanilla comparison examples consistently include this label array. It appears the Entity Browser's category filtering reads this label array, not (only) the `m_EntityType` enum value. This is separate from `m_aAutoLabels`, which the wiki explicitly warns not to hand-edit (auto-generated by tooling).
6. **A `Hierarchy` component must be present** on the prefab, even though nothing in our own gameplay logic uses it. All three vanilla comparison examples include one. Omitting it was not individually isolated as a hard blocker, but it was present in every known-working example so keep it.
7. **`.et.meta` file's `Name` field must have the correct, current GUID matching the prefab's actual `ID`.** If you ever change a prefab's GUID (e.g. regenerating it), the `.meta` file does NOT auto-update -- Workbench resolves the resource's true GUID from `.meta`, not from the `ID` field inside the `.et` text. A stale `.meta` produces persistent "Wrong GUID for resource" errors that survive full Workbench restarts and resource-database rebuilds until the `.meta` itself is fixed.
8. **The registry `.conf` (`SCR_PlaceableEntitiesRegistry`) also needs its own correct `.meta` file** (`CONFResourceClass` structure, matching the pattern in any wizard-generated `.conf.meta`, e.g. `Missions/*.conf.meta`). A registry `.conf` with no `.meta` at all produces "Wrong GUID for resource ... in property inherited-name" when `EditorModeEdit.et` tries to reference it.
9. **The full original content of `EditorModeEdit.et` must be preserved when overriding it in your addon.** Using Workbench's "Override in addon" + editing only the one component you care about (e.g. just adding your registry to `SCR_PlacingEditorComponent.m_Registries`) can result in Workbench saving back an incomplete file that's missing dozens of other unrelated vanilla components (`SCR_CameraEditorComponent`, `SCR_ContentBrowserEditorComponent`, etc.) -- this alone causes catastrophic, hard-to-diagnose editor UI crashes ("Cannot find editor component 'SCR_PlacingEditorComponent', local instance of editor manager not found!" plus dozens of `NULL pointer to instance. Variable 'menu'` exceptions across every editor toolbar). Fix: read the real vanilla file via `game_read`, and write back its **full** content plus your one addition, never a partial diff.

## Debugging techniques that got us there

- **`game_duplicate`** (from the `enfusion-workbench-mcp`/Goldwep fork -- see below) to pull a known-working vanilla editable prefab into the mod folder unmodified, then `wb_resources getInfo` on it to see its fully-resolved JSON representation (flags, entity type, UI info structure) side by side with our own. This "find a working example, diff against it" approach succeeded where guessing from documentation alone had failed for hours.
- **A/B control test:** add the *unmodified* vanilla duplicate to our own registry and confirm it also doesn't show up before concluding our components were the problem -- this correctly ruled out registry-wiring issues at one point and redirected effort to the component definitions themselves.
- **`resolve_guid`** to check whether a suspicious/stale GUID seen in an error message corresponds to any real, currently-indexed resource (it didn't -- confirming it really was stale data, not a legitimate conflict).
- **Reading the actual `.c` source of `SCR_EditableEntityComponent`/`SCR_EditableEntityComponentClass`/`EEditableEntityType`/`EEditableEntityFlag`** directly via `game_read` was far more reliable than any wiki page -- e.g. the wiki never mentions `m_aAuthoredLabels` needing a value for the browser filter to work; the vanilla `.et` examples were what actually revealed it.

## MCP tooling change

Switched from the original `enfusion-mcp` (Articulated7, npx-based) to **`enfusion-workbench-mcp`** (Goldwep fork, cloned from `https://github.com/Goldwep/enfusion-workbench-mcp.git`, built locally at `G:\enfusion-workbench-mcp`). 112 tools vs. the original's much smaller set. Key additional tools that were decisive this session: `prefab` (action=inspect, shows fully-merged inheritance chain with per-value provenance), `game_duplicate`, `resolve_guid`, `wb_diagnose`, `script_class_hierarchy`, richer `api_search`/`component_search`. The other two previously-configured servers (`arma-reforger-mcp`, `arma-reforger-api`) were never actually connected/used and have been removed from `claude_desktop_config.json`.

**Config gotcha:** `ENFUSION_PROJECT_PATH` needs to point directly at the addon root (`G:\MCF\addons\MCF`) for the `project` (read/write/browse) tool family, but `game_duplicate`/`wb_entity_duplicate` want the *parent* of addon folders plus an explicit `modName`. If both are needed in the same session, be aware a write to a `project`-relative path while the env var is set to the wrong level will silently create files in the wrong location (this happened once this session -- files landed in `addons/Prefabs/` instead of `addons/MCF/Prefabs/` -- caught via `git status` showing an unexpected new top-level directory).

## Still open (separate topic, not started)

**In-game Game Master "Scenario properties" panel (per-instance configurable attributes after placement)** -- e.g. Arsenal's placed-instance editor shows "Set faction", "Enable arsenal", weapon toggles etc.; our placed MCF prefabs currently show "No properties, this entity has no properties to edit." This is a distinct system from everything solved above (likely `SCR_AttributesManagerEditorComponent` + a per-component attribute-list config), not yet investigated. Next session starting point: read `Scripts/Game/Editor/Containers/Attributes/SCR_BaseEditorAttribute.c` and find a simple (non-Arsenal) vanilla System-type prefab that has editable instance properties, and diff its component setup the same way the placement fix was found above.
## Task push ordering: OnPlayerRegistered can beat OnGameModeStart

Observed in the Peer Tool session of 2026-09-09:

```
16:12:31.275  [MCF] sent 0 task(s) to player 1
16:12:31.397  [MCF] TaskStore loaded 1 task(s) from previous sessions
16:12:43.268  [MCF] sent 1 task(s) to player 2
16:12:45.305  [MCF] sent 1 task(s) to player 3
```

Players 2 and 3 (the two peers) joined after the store was up and received
their task. Player 1 -- the host's own local player on a listen server --
registered 122 ms *before* the store finished loading and was therefore sent
an empty list. Nothing errored. The host simply had no tasks, and the only
way to notice was to read the timestamps.

There is no documented ordering guarantee between `OnPlayerRegistered` and
`OnGameModeStart`, and on a listen server the host's player is already present
when the game mode starts, so this is the normal case rather than a race that
happens only sometimes.

Fix in `MCF_Core_GameModeComponent`:

- `m_bTaskStoreReady` starts false and is reset to false on every
  `OnGameModeStart`.
- `OnPlayerRegistered` returns early while it is false, logging that the
  player's tasks were deferred rather than silently sending nothing.
- `LoadPersistentState()` sets it true after the store has loaded, then calls
  `SendTasksToConnectedPlayers()`, which walks
  `GetGame().GetPlayerManager().GetPlayers(...)` and pushes to everyone who
  registered early.

No player can register between the flag being set and the catch-up running --
both happen synchronously inside `OnGameModeStart` -- so nobody receives a
task twice.

`PlayerManager.GetPlayers(array<int>)` verified against vanilla
`SCR_BaseGameMode.c:1699`.

### Enum drift, confirmed on disk

The stored task from before the `PUBLISHED` value was inserted:

```
task.t1=...  assigneeType=2  assigneeId=2nd Squad  state=1  ...
```

`state=1` was written as `ASSIGNED` and read back as `PUBLISHED`. Both
`MCF_ETaskState` and `MCF_ETaskAssignee` now carry append-only warnings. The
three stale stores (Workbench profile, PeerPlugin1, PeerPlugin2) were deleted
so the next run starts from clean data. The two PeerPlugin stores were
themselves evidence of the earlier client-writes-server-state bug; if they
reappear after a peer test, the `Replication.IsServer()` guard has regressed.


## Late-join intel edits, and roles read out of vanilla (2026-09-09)

### Game Master edits to an intel object now survive joining and streaming

The first implementation of runtime intel authoring broadcast the new text
with an RPC. That is correct for everyone listening at that moment and wrong
for everyone else: a player who joins later, or who streams the object in for
the first time afterwards, never heard the broadcast and reads the prefab's
original text. Pushing the override on join does not fix it either, because at
join time the object usually has not been streamed to that client yet, so the
push lands on nothing.

`MCF_Intel_CarrierComponent` now holds the override as replicated state
instead:

```c
[RplProp(onRplName: "OnContentReplicated")]
protected string m_sContentOverride;
```

`SetContentFromServer` writes the field, applies it locally and calls
`Replication.BumpMe()`. Joining, streaming and reconnecting all resolve
themselves, because the value travels with the entity's own state rather than
as an event that can be missed.

RULE: if a value must be true for anyone who ever sees the entity, it is
state, not an event. Broadcasts are for things that happen; RplProp is for
things that are.

### Roles are resolved from vanilla, shown, and not yet enforced

`MCF_Task_Permissions.ResolveRole` is real. In order of precedence:

1. Game Master rights -- `SCR_EditorManagerCore.GetEditorManager(playerId)`
   non-null and `!IsLimited()`. Someone running the mission outranks the
   in-fiction chain of command, which has usually been shot.
2. Faction commander -- `SCR_Faction.IsPlayerCommander(playerId)`. Vanilla
   really does carry this: `SCR_FactionCommanderHandlerComponent` replicates
   the commander ids and pushes them into `SCR_Faction.SetCommanderId`.
3. Group leader -- `SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId)`
   then `SCR_AIGroup.IsPlayerLeader(playerId)`.

Anyone else is a soldier. No MCF-specific setup is needed for a mission to
have a chain of command.

TRAP, worth knowing before the enforcement switch is flipped: the per-player
editor map in `SCR_EditorManagerCore` exists on the server only -- it early-
returns out of creating managers on a client. A client can therefore only
answer the Game-Master question about itself, which is all it needs it for
(greying its own buttons). The server, which is the side that enforces,
has the full map.

`m_bEveryoneMayDoEverything` is still `true`. The board now prints the
resolved role in its status line ("YOU: COMMAND" / "SECTION COMD" /
"SOLDIER") so a wrong answer is visible before it can lock anybody out.
Resolve first, watch it be right, enforce second -- the other order means
debugging a locked-out player instead of reading a line of log.



### Intel is faction-scoped

`MCF_Intel_Record` gained `m_sFactionKey`, captured at the moment of logging
rather than looked up from the logger afterwards -- a player who changes sides
must not drag the report across with them. `MCF_Core_IntelStore.IsVisibleTo`
now compares it.

Two escape hatches, both deliberate:

- A record with **no faction** is legacy data and stays visible to everyone. A
  mission appearing to lose intel it already had looks like a bug even when it
  is a policy.
- A player with **no faction** (unassigned, spectating, still in the lobby)
  sees nothing rather than everything. "Not on a side yet" is not "on every
  side".

This cost exactly one field because the per-player transport was already
built: `SendIntelToPlayer` filters through `IsVisibleTo`, and the existing
`GetOnPlayerFactionChanged_S` hook re-sends after a clear, so switching sides
swaps what a player holds instead of leaving a stale copy on their board.

The persistence format absorbed the new field for free -- records serialise as
`key=value` pairs and the reader ignores keys it does not know, so old saves
load and new saves are readable by old code. Worth remembering the next time a
field is added: positional formats would have forced a migration here.

Both intel and task pushes now log the faction alongside the count. "2
records" reads identically whether the filter ran or silently passed
everything through.



## The trigger layer and the command centre are joined (2026-09-09)

Until now MCF had two halves that never touched. The trigger layer noticed
things and published events; the command centre handled what a force knows.
But intel had to be placed by hand before the mission started, so it could
never be a *consequence* of what players did -- which is the whole point of
"the Game Master's triggers are the mission's tasks, and the in-game commander
writes orders from the intel those triggers produce".

`MCF_Intel_SourceComponent` is that sentence made real. It listens on one MCF
event and produces intel in one of two modes.

**DROP** spawns a physical object at this entity's transform plus an offset.
Somebody has to find it, pick it up and carry it to the board before the force
knows anything. This is the default on purpose: the carrying is the
interesting part, and a mode that skips it quietly deletes the mechanic.

**SIGNAL** writes straight to one faction's board -- a radio intercept, a
report passed down. It **refuses to fire without a faction key** rather than
defaulting to one, because a record with no faction is visible to everyone and
that would read as a broken filter rather than a missing attribute.

One-shot by default (`m_bOnce`). A proximity trigger fires on every crossing;
intel that respawns each time leaves a pile of identical letters on the ground.

### One prefab, many documents

`MCF_Intel_CarrierComponent.SetSinglePageFromServer(device, heading, stamp, body)`
lets a source write text onto the object it just spawned. So a mission maker
builds one "handwritten letter" prefab and every trigger fills in what that
particular letter says, instead of needing a prefab per piece of intel. The
wire format is assembled inside the carrier, not by the caller -- the
separators are that class's private business, and a caller that built the
string itself would break the moment they changed.

Leave the text fields empty and the spawned object keeps whatever the prefab
already says, which is what you want when a specific letter was authored and
just needs to appear.

### Known gap, stated rather than discovered later

Dropped objects are not persisted. A server restart loses them, and because
`m_iProduceCount` lives in memory the source will drop again on the next
trigger. Signalled records *are* persisted, because they go through
`MCF_Core_IntelStore`. Closing this means the store owning a list of dropped
objects and their positions, the same way it owns records -- worth doing,
not done.

### New in the Game Master browser

`Prefabs/MCF_Intel_Source.et`, registered in `MCF_PlaceableEntities.conf` as
`{6A1C4F0B39D2D030}`. Registering it there is not optional: an unregistered
prefab is invisible in the browser, which already cost one full test round.



## Conversations, and feelings to hang them on (2026-09-09)

An audit of the AI layer before building found it thinner than it looked. No
dialogue system of any kind -- `MCF_Interact_TalkAction` fired one canned line
into the voice queue and did not even publish an event. No disposition per
NPC: the only attitude number anywhere was `MCF_Hostility_Manager`'s per-AREA
hostility. Nothing plays a gesture on a specific character
(`MCF_AI_WaypointAnimationComponent` publishes an event and its header records
that `PlayGesture()` failed to compile and was removed). `MCF_React_StepRunner`
has three step types, no delays, no conditions, and cannot say *which* AI acts
-- `REQUEST_ANIMATION` parses its value and throws it away.

So the infrastructure was never the problem. The last metre was.

### MCF_AI_DispositionComponent -- two numbers, not one

TRUST is whether they think talking to you will end well. FEAR is how
frightened they are right now, and high fear does not make someone hostile, it
makes them **useless**: they agree with everything and give you nothing. A
player who gets what they want by frightening a civilian should learn the
wrong lesson has been learned.

One "friendly" number cannot tell apart a man who trusts you and is terrified
of the people outside from a man who is calm and thinks you are the problem.
The scenes worth playing live in that gap.

Reported fear is personal fear **plus a share of the area's live hostility**,
read at the moment it is asked for rather than cached at spawn. That means the
compliance system already wired up -- where ordering civilians about at
gunpoint without cause raises area hostility -- now has a consequence a player
can hear: the whole village gets harder to talk to. Nothing needed connecting.

Server state, never replicated. Replicating the numbers would hand every
client a readout of how to play each civilian.

### MCF_Dialogue_Component -- the graph, decided server-side

Nodes and choices, authored as attributes. A choice may require a minimum
trust, a maximum fear, or a flag; it may move trust and fear, set a flag, and
**publish one MCF event**. That is the only thing a reply can do to the world.

That constraint is the design. A conversation cannot spawn intel, fail an
objective or raise an alarm by itself -- it publishes, and the trigger layer
does the rest. So the day something new can be caused, conversations can cause
it without being touched. Hang an Intel Source on a reply's event and talking
to the right man produces a document; that is the loop the mission maker
actually wanted, assembled from parts that already existed.

Per-player position in the graph (two players hold separate places in one
conversation), per-NPC flags. Flags are not persisted across a restart -- the
same gap dropped intel objects have, written down rather than discovered.

### The screen: a conversation that runs, not a panel

A band across the lower half with the world still visible above, so you are
talking to a person you can see. What has been said stays and scrolls; your
own replies are written into it. Only the last six lines are kept -- not for
memory, but so it always fits without anybody scrolling mid-conversation.

LOCKED REPLIES ARE SHOWN, GREYED, WITH THE REASON AFTER THEM. That line is the
only feedback a player ever gets that trust and fear are real. Hide them and
the system becomes invisible: the civilian simply says less, and nobody learns
that waving a rifle around caused it. An author who wants a reply genuinely
secret ticks `m_bHideWhenLocked` and the server never sends it.

The client is sent one screenful and replies with an INDEX into it -- never a
node id, never an effect, never the graph, never the numbers. Picking a reply
it was not offered is therefore a bounds check. The server re-derives the same
view and re-checks availability on apply, because state can move while the
player reads.

### Three compile errors worth keeping

1. **A ScriptComponent cannot declare a bare constructor.** `void
   MCF_Dialogue_Component()` fails with "Overloaded function
   'MCF_Dialogue_Component' not compatible" -- a message that names the
   constructor but not the reason. Initialise `ref array` fields inline
   instead.
2. **`string.ToUpper()` mutates in place and returns an int**, the same trap as
   `Replace`. Passing `name.ToUpper()` to a string parameter is a type error;
   copy, mutate, then pass.
3. **`RplComponent` exposes no `GetOwner()` to script.** It is an engine class,
   so there is no way back from it to the entity. Entity addressing over the
   wire must go through `SCR_EditableEntityComponent`, which is the route the
   intel editor already proved. Consequence: a person must be an editable
   entity to be talkable -- true for anything a Game Master places.

### Still missing for the scene as described

The wave. Nothing plays a gesture on a named character, and the one previous
attempt at `PlayGesture()` did not compile. That is research with real risk and
it is untouched. The conversation works without it; the scene does not feel
directed without it.



## Talking to anybody, on any faction (2026-09-09)

### Reaching every AI: Character_Base, and why nothing smaller works

Conversations had to work on the people already in a mission -- placed by a
mission maker, spawned ambiently, part of a composition, or belonging to a
modded faction MCF has never heard of. A dedicated MCF civilian prefab would
only ever have made MCF's own civilians talkable.

Per-faction bases (CIV/US/USSR/FIA) are small and tempting, and they do not
reach modded factions, which inherit `Character_Base` directly. So the
override is `Character_Base.et` itself.

MEASURED, NOT ASSUMED: a GUID override REPLACES a prefab, it does not merge.
Vanilla `EditorModeEdit.et` is 16.4 KB and MCF's override of it is 17.0 KB --
a whole copy plus additions. So the Character_Base override carries all 37 KB
of vanilla's character definition.

It was generated mechanically and must be regenerated the same way:
`game_duplicate` the vanilla file out of the paks into a scratch addon folder,
insert the MCF block, write it with vanilla's GUID in the .meta. Doing it by
hand is a typo away from breaking every character in the game.

THIS COPY GOES STALE SILENTLY. After a game update the characters keep the old
values and nothing in any log says why. Known conflict: any other mod that
overrides Character_Base fights with this one, last loaded wins. That price
was accepted deliberately over per-faction overrides that cannot reach modded
factions at all.

An untouched mission is unaffected: the dialogue component has no nodes,
`HasConversation()` is false, and the talk action hides itself.

### The interaction context

The talk action does not sit on vanilla's `default` context (a 0.4 m sphere at
spine3, which players found far too precise). It has its own `MCF_Talk`
context at Spine5 with a 1.1 m radius. Adding a context is free; widening
vanilla's would have changed every action on it.

### Making an AI face the player: the full mechanism, learned in three passes

**Pass 1 -- `SetYawPitchRoll`. Did nothing.** A character's transform belongs
to the animation system and the command handler; they put it back the next
frame. Tell: no vanilla script anywhere sets a character's rotation that way.
That absence was the evidence, and it was there before the first attempt.

**Pass 2 -- `CharacterHeadingAnimComponent.AlignPosDirWS` alone. Turned, then
snapped back.** Two separate reasons, and both had to be fixed:

- `StartLoitering` does NOT suppress the behaviour tree. It disables movement
  *controls*, not behaviour selection, and vanilla's loiter behaviour has
  priority 1.5 -- it only outranks Idle. The AI simply selected something else
  and took its heading back. The one vanilla call that stops the tree outright
  is `AIControlComponent.DeactivateAI()` (vanilla uses it in
  `SCR_ChimeraAIAgent.OnLifeStateChanged`).
- The align only holds INSIDE a loiter. `SCR_GetDisableMovementControls()` is
  what keeps the controls off while a character turns, and it reads
  `m_iLoiteringType`, which is zero unless `StartLoitering` set it. Alignment
  and loitering are one mechanism; taking half of it gets a turn that is
  immediately undone.

**Pass 3 -- what works.** `DeactivateAI()`, then
`StartLoitering(null, ELoiteringType.LOITERING, true, true, true, target)`
with the target matrix built by `Math3D.DirectionAndUpMatrix(dir, up, mat)`
and `mat[3] = ownerPos`. On leaving: `StopLoitering(false)` and
`ActivateAI()`. `ELoiteringType` lives in
`scripts/Game/AI/UserActions/SCR_LoiterUserAction.c`, and `NONE` is rejected --
there is no align-only mode.

RULE: when a subsystem owns a value, do not write the value. Find the call the
engine already routes through that subsystem, and use the whole of it.

### The head look came from the tutorial

The user's hint -- "the tutorial world uses animations like this" -- was the
fastest lead of the session. Reforger's tutorial NPCs do not use the AI system
at all: `ChimeraAIControlComponent { Enabled 0 }`, and
`SCR_NarrativeComponent` re-issues a head IK target every fixed frame:

```c
m_CharacterAnimation.SetIKTarget("HeadLook", "HeadLook", owner.CoordToLocal(headPos), {0, 0, 0});
```

`SetIKTarget` is graph-independent; the ENABLE variable is not. Tutorial NPCs
run `narrative_npc_main.agr` with a float `NarrativeLookAtIntensity`; ordinary
characters run `player_main.agr`, where the equivalent is the bool `Look`
(used by `SCR_AICinematicLookAt`). MCF binds `Look` behind a guard, so a graph
without it means a head that does not turn rather than a failure.

The event mask is set only while somebody is being looked at. Every character
in the game carries this component now, and a fixed frame on all of them for a
head that is not turning would be a real cost for nothing.

KNOWN LIMIT: the head look is client-side, so only the player in the
conversation sees it. The body turn is server-side and everyone sees it.

### Two compile-and-config traps

- **`[BaseContainerProps()]` is required on every class that appears in a
  .conf.** Without it the parser silently skips the entries: the library
  loaded, cast fine, and reported "0 conversation(s)". `MCF_Intel_Entry` had
  the attribute, which is why intel worked and dialogue did not.
- **A slider-backed editor attribute writes a FLOAT.** `CreateInt` +
  `GetInt()` round-trips as zero, so every pick read back as "no
  conversation" and the slider appeared to snap back on its own. The radius
  attribute next to it had always used `CreateFloat`.

### Game Master authoring, within the twelve-byte limit

`SCR_BaseEditorAttributeVar` packs into one vector and carries twelve bytes:
`CreateInt`, `CreateFloat`, `CreateBool`, `CreateVector`, and nothing else. A
custom attribute class does not escape that -- it is the transport, not the
class. So a Game Master cannot type a conversation in.

But a PICK is a number. `Conversation`, `Trust` and `Fear` are sliders on the
person themselves; the conversation slider is a one-based index into
`Configs/Dialogue/MCF_Conversations.conf`, with 0 meaning silent. The words
live in the library where text belongs; the Game Master decides who says them.

`MCF_Dialogue_AssignComponent` remains for mission makers working in the World
Editor, where prefab attributes are editable and a radius covers a village in
one placement.



## Game Masters can write conversations (2026-09-09)

Right-click a person -> "Edit conversation". Four columns: the library, the
steps of the conversation, the selected step's text, and the replies to it with
their fields.

### It edits the library, not the person

The first instinct was to write on the person -- quicker to build, and quicker
to regret: the same words then have to be typed again for the next ten
civilians, and they die with the first one. A library entry has a name, is
given to a hundred people with the Conversation slider, outlives everybody, and
is saved with the mission. The person the editor was opened on is simply who it
is assigned to on save.

Conversations authored at runtime are persisted through
`MCF_Core_PersistentStore`, beside tasks and intel, and reloaded at mission
start. Same kind of thing: text authored during play, held by the server,
expected to still be there tomorrow.

### Rules that are load-bearing rather than cosmetic

- **Shipped conversations are marked `[mod]` and cannot be deleted.** They are
  content, not session state; deleting one would break every mission that
  refers to it with no way back short of reinstalling.
- **A runtime conversation SHADOWS a shipped one of the same id.** So a Game
  Master can rewrite `farmer_mill` for tonight without the mod's copy changing.
- **Ids are sanitised of commas and spaces on save.** The persistence index and
  the list sent to the editor are comma-separated; an id with a comma would
  split into two ids resolving to nothing, and the conversation would vanish on
  the next restart with nothing in any log.
- **List order is config first, then runtime, and never reordered.** That order
  IS the meaning of the Conversation slider, so a new conversation appends
  rather than inserting, and nobody's slider silently changes what it points
  at.
- **Only a Game Master may save or delete.** This writes shared, persisted
  state that every player in the mission reads.

### Everything is fetched from the server, nothing read locally

Both the conversation text and the library index are requested over RPC even
though the client has the shipped config. On a dedicated server a Game
Master's machine does not have conversations written this session -- reading
the local library would open an empty editor over somebody's existing words
and then save over them.

### The client is told the name, never the tree

`m_sConversationId` replicates so the client can draw the prompt at all, and
`m_sAssignedSpeaker` / `m_sAssignedVerb` ride along so it reads "Talk: Farmer"
rather than the prefab default -- a Game Master can write a conversation
mid-mission, so a client's own library is not the same as the server's and the
name has to be told rather than looked up.

`HasConversation()` on a client therefore checks only that an id is assigned.
The node tree stays on the server, which is the whole reason a client is sent
one screen at a time.

### Not done

- Per-choice flags (`m_sRequiresFlag` / `m_sSetFlag`) exist in the format and
  in the config, but have no field in the editor yet.
- Conversation flags and dropped intel objects still do not survive a restart.
- The whole thing has still only ever run on a listen server.



### VERIFIED on a Peer Tool session (2026-09-09)

Conversations were tested with a real client, not a listen server. Game Master
edits reach the peer immediately and correctly.

What that actually proves, stated narrowly:

- **`RplProp` on a ScriptComponent works.** `m_sConversationId`,
  `m_sAssignedSpeaker` and `m_sAssignedVerb` reach a client that is not the
  server. This was the pattern taken on faith when the intel carrier's
  broadcast was replaced; it is no longer on faith.
- **`Replication.BumpMe()` after writing the fields is enough** -- no manual
  push per player was needed.
- **The talk prompt appears on a client**, so `HasConversation()` answering
  from the replicated id alone is sufficient. Withholding the node tree from
  clients costs nothing visible.
- **The server-authoritative conversation loop survives a real wire.** One
  screen out, an index back, the next screen out.
- **Fetching the library and the conversation text over RPC works** rather than
  reading a local copy.

What it does NOT prove, and is still open:

- Faction-scoped intel. That needs two players on DIFFERENT factions; a single
  peer cannot show a filter working from one that silently passes everything.
- Late-join: joining after an edit, and streaming an already-edited entity in
  for the first time.
- The operations board, tasks, and intel carrying with two peers.
- Whether `DeactivateAI` / loiter facing looks right to a THIRD player watching
  the conversation from outside it. The head look is client-side by design and
  is known not to reach them; the body turn should.



## A mod CAN add its own keybind (2026-09-09) -- VERIFIED in play

Every input action in Arma Reforger lives in one file,
`Configs/System/chimeraInputCommon.conf`, **241 KB, 9,584 lines, 504 actions
and 100 contexts**. Nothing in the game ships a second one, and none of the
thirty mods installed on this machine adds a keybind -- a grep across every
`.conf`/`.et`/`.c` in the workbench addons directory for `ActionManager`,
`chimeraInputCommon` and the GUID returned zero hits. So there was no example
to copy anywhere.

It works anyway, for one reason: **vanilla uses the config append operator
inside that file itself**, exactly once, at lines 9125-9129:

```
ActionContext MenuContext : "{FA0C9BC320018CF8}Configs/System/ActionContext/MenuContext.conf" {
   ActionRefs +{
    "MenuCalibrateMotionControl"
   }
}
```

That is the same `+{ }` that lets MCF's `chimeraMenus.conf` override add three
menus without losing the other fifty-eight. So a GUID override carrying only
`Contexts +{ ... }` ADDS to the input set rather than replacing it.

**GUID: `Configs/System/chimeraInputCommon.conf` = `{795184CF9AD764DB}`.**
Found in `addons/data/ArmaReforger.gproj`, which is unpacked and readable:

```
InputManagerSettings InputManagerSettings "{50A48858902D5F64}" {
 Default "{795184CF9AD764DB}Configs/System/chimeraInputCommon.conf"
 UiMappings "{0AD37213EFEB0B61}Configs/System/chimeraMapping.conf"
```

That `.gproj` is the way to find any base-game config GUID: packed vanilla
assets have no sidecar `.meta`, `resolve_guid` indexes only user projects, and
`asset_search` never returns GUIDs for configs.

### The shape that works

MCF adds its own CONTEXT rather than appending to a vanilla one. Vanilla's
example appends to a context that has an external base; `CharacterGeneralContext`
is defined inline, and appending into an inline context from an override was
not worth the risk when a new context costs one line of script.

```
ActionManager {
 Contexts +{
  ActionContext MCF_CharacterContext {
   Priority 10
   Flags 0x2 0
   Actions {
    Action MCF_ShoutSurrender { InputSource ... Input "keyboard:KC_H" ... }
   }
  }
 }
}
```

**`Flags 0x2` means the context must be re-activated every frame**, which is
what vanilla's own conditional contexts do. `modded class
SCR_CharacterControllerComponent.OnPrepareControls` calls
`am.ActivateContext("MCF_CharacterContext")`. A context that is never
activated is a key that never fires, with nothing anywhere to say why.

Listeners go in `OnControlledByPlayer`, added when the local player takes
control and removed when they lose it -- the pattern vanilla uses for every
gameplay keybind on that class. Handler shape:
`void Handler(float value = 0.0, EActionTrigger trigger = 0)`.

Verified in play: both keys fire, and `IsWeaponRaised()` reads correctly from
the handler. All vanilla keybinds still work.

STILL TO DO: the actions are not in `Configs/System/keyBindingMenu.conf`, so
they cannot be rebound by a player from the Controls menu yet. Key choice is
therefore load-bearing -- H and U were picked after J turned out to be tasks
and K the compass.

### What a shout cannot do

There is **no script API to raise a noise the AI can hear.** Characters carry
an `EarsSensor` with `MaxRange 120` and `SoundIntensityMin_db 10`, and the
danger-reaction system handles `Danger_WeaponFire`, `Danger_Explosion`,
`Danger_VehicleHorn` and nine others -- but every one of those events is
raised engine-side. No script in the entire game creates one; there is no
`AddDangerEvent` and no noise-emission call.

So "the AI in the area hears you" has to be a server-side
`QueryEntitiesBySphere` around the shouter. Functionally the same, with one
honest difference: walls do not block it unless MCF checks for them itself.



## Subject control: shouting, surrender, escort (2026-09-09/10)

### Shouting replaced a menu on a person

The first version put "Order: hands up" and "Stay back" on the person as user
actions. That was wrong and the user said so: a list of orders on one civilian
reads as a menu, and shouting is not something you do to whoever you happen to
be hovering over. It goes to a room, and everyone in earshot answers for
themselves -- a crowd scattering while one man stands his ground is the point.

`H` shouts surrender, `U` shouts stay back, both with the weapon-raised state
read on the SERVER rather than trusted from the client. The two per-person
actions were deleted.

What is left on the person is only what is genuinely done to one person up
close: Restrain, Let them go, Come with me, Walk in front of me, Wait here,
and Talk.

### The compliance component had been unreachable since it was written

`MCF_AI_ComplianceComponent` shipped early. It knew whether a person was
armed, rolled against a compliance chance, and penalised the area's hostility
when a player pointed a rifle at a civilian for no reason. **Nothing ever
called any of it** -- no action, no key, no trigger. It sat there looking
finished.

Connecting it was most of what "build a surrender system" turned out to mean.
Its old roll (`AttemptCompliance`) was REPLACED by `WillSurrender`, not kept
beside it: two rolls that disagree are how you end up debugging the wrong one.
`MCF_AI_SurrenderAction` and `MCF_AI_StandOffAction` were deleted with it.

Every term in the new roll is a dial on the person -- base chance, fear
weight, armed resistance, weapon-raised weight, plus a closeness bonus.
"Civilians give up sooner than soldiers" is a mission maker's decision
expressed as numbers, not a branch in the framework.

FEAR CUTS BOTH WAYS, deliberately. A frightened man surrenders sooner, and is
then nearly useless under interrogation because a frightened person says what
he thinks you want to hear. The fast way to make somebody comply is the slow
way to learn anything from him.

### Escorting: two mechanisms, not one with a different destination

**Following** pathfinds -- `SCR_AIMoveIndividuallyBehavior` at player
priority, re-issued only once the escort has moved far enough.

**Marching somebody in front of you** does not. Pathfinding to a point four
metres away means arrive, stop, get told again, start -- which is exactly what
the first attempt looked like. It now uses **combat-move requests**, the
mechanism behind vanilla's half-second sidestep when a player bumps an AI:
built for short, repeated, immediate movement rather than for going somewhere.

Two fixes turned "glitchy" into "relatively good", and the first was
self-inflicted:

- **The behaviour is kept alive and re-pointed, not rebuilt.** Failing it and
  adding a fresh one every tick is a stop and a start twice a second however
  well the individual steps overlap.
- **Pace is read from the escort's velocity**, not fixed at a walk. A prisoner
  walking while you jog falls behind and then sprints to catch up, which is
  the most obvious way it stops looking like an escort.

Steering falls out of the target being a point in front of the escort: turning
swings it, standing still lets him arrive and stop.

ESCAPE only applies to somebody NOT restrained -- otherwise tying hands is
pointless. The chance is nerve times defiance: terrified people do not run and
trusting people have no reason to, so the likely runner is the calm man who
thinks you are the problem. Same gap between the two numbers that makes
conversations worth having.

### There is no way to physically couple two characters

Asked directly, and the answer is no. No carry state, no drag state, no
character-to-character compartment. The one rigid-coupling system that exists
lists `Character_Base.et` under `"Forbidden linking"` and has no script entry
point at all.

The carry mods that exist work because their subject is UNCONSCIOUS -- the
Dragger mod's own description says the animation is IK on the DRAGGER's arm.
An inert body can be parented to a bone with its physics off. A prisoner who
is walking has to produce his own walk, so none of it applies.

### The restrained pose: a crash, and what it taught

Arma Reforger ships no restrained, bound or surrendering animation. The full
set is seven gestures (point, stop, follow, move, get in twice, salute) and
six loiter poses (sit, lean twice, smoke, loiter, pushups). The closest clip
in the game is `anims/anm/Tutorial/arms_back.anm` -- an instructor's parade
rest, hands clasped behind the back, which from behind reads as bound wrists.

Pointing the pose hook at `anims/workspaces/narrative/narrative_npc_main.agr`
-- the graph that holds that clip -- **crashed the Workbench natively**, with
no script frames in the log.

The reason is visible in what vanilla itself attaches: the officer mission's
graph is **475 bytes**. The narrative graph is **23 KB** and is a complete
character graph in its own right. `PreAnim_SetAttachment` mounts a SUB-graph,
not a replacement, and a whole graph is not one.

So the hook stays, defaulted to empty -- no pose, no error, restrain works --
and a real pose needs a small purpose-built graph holding one clip.
`SCR_LoiterCustomAnimData` is the right route and is already replicated; the
missing piece is asset work in the Animation Editor, not script.

Two details already handled for when that graph exists:

- The loiter is started with input disabled, or a prisoner stands up by
  tapping sprint (loitering self-cancels on fire/sprint/raise/ADS/reload).
- Giving any movement order leaves the pose first, because loitering disables
  movement controls and a posed prisoner cannot walk at all.

### Previewing animations

The Animation Editor opens `.anm` files with no body because a clip is only
motion data. The workspace that binds a character is
`anims/workspaces/player/player_main.aw`. Open that, then load clips into it.
`.anm`, `.agf`, `.agr` and `.aw` are all binary -- nothing about an animation
can be read from outside the editor, only looked at inside it.


## Repository hygiene (2026-09-10)

Three things were tidied up while wrapping up this session's work.

The research notes that had been living in `addons/MCF/docs/research/` moved to
`docs/research/` at the repository root. Anything under `addons/MCF/` is packed
into the shipped addon, so design notes stored there were being distributed to
every player for no reason. The repository now has exactly one documentation
tree, at `docs/`.

`.gitignore` was rewritten in English (it was still partly Dutch) and gained
two rules. `*.bak` now covers stray editor and tool backups, not just
`*.c.bak`. And the whole `server/` folder is ignored: it holds a local
dedicated-server launcher config with machine-specific addresses and an admin
password, plus a `profile/` folder that is nothing but runtime logs and saves.
A contributor who wants a local server writes their own config; it is not
shared state.

Dead code removed: `MCF_AI_SurrenderAction` and `MCF_AI_StandOffAction` (both
superseded by the shout system, which decides surrender from the shouter's
position and the AI's own fear rather than from a per-person menu entry), and
`MCF_AI_ComplianceComponent.AttemptCompliance` (a second, unreachable
surrender roll that predated `WillSurrender`).


## Modularisation, phase 0 (2026-09-10)

MCF is to become splittable into separate addons -- Core alone, Core plus a
module, and so on -- so that a mission maker installs only what they use. The
design is in `docs/architecture/MODULARISATION.md`. This entry records the
first step, which moved no files out of the addon and changed no behaviour.

### The measurement the whole design rests on

The open question was what the engine does with a prefab that names a script
class from an addon that is not loaded. It was measured rather than reasoned
about: a throwaway prefab with one non-existent component and three real ones
was placed in the test world.

```
WORLD (E): Unknown class 'MCF_Probe_ThisClassDoesNotExist' at offset 293(0x125)
```

The entity loaded with **Component Count: 3** -- the three real components all
present and correct. **An unresolvable component is dropped and the entity
survives it**, at a cost of one error line per missing class per prefab load.

Two further probes in the same launch: a `MenuPreset` naming a script `Class`
that does not exist is **completely silent**; a `MenuPreset` naming a `Layout`
GUID that does not exist logs one `RESOURCES (E)` at `MenuManager config load`
and the other five presets load normally.

This kills both options that had been on the table. Runtime component
attachment is not needed (and there is no `AddComponent` in the scripting API
anyway), and the `Character_Base` override does not have to live in the most
dependent module and drag everything with it. Instead:

> **Exactly one MCF addon may override a vanilla GUID, and that addon is Core.
> Modules never override vanilla.**

Each of the four vanilla overrides -- `Character_Base.et`, `EditorModeEdit.et`,
`chimeraMenus.conf`, `chimeraInputCommon.conf` -- becomes a manifest that names
every module's contribution whether or not that module is installed.

Still unproven, and folded into the two-peer session: the same behaviour at
runtime on a dedicated server, the same behaviour packed to `.pak`, and whether
a **user action** entry naming a missing class behaves like a component entry.
`Character_Base` carries six of them.

### The event bus was a seam in intent, not in fact

`MCF_Core_EventManager`'s header claims modules never reference each other
directly. A full symbol cross-reference across all 81 scripts found eight
places where they did, most of them pointing the wrong way -- from Core into a
module that is meant to be optional. All eight are now cut:

- `MCF_Core_AutoWatcherRegistry` held three typed arrays, one per detection
  component, so Core named three Objectives classes. It now holds one array of
  `MCF_Core_ControllableWatcherComponent`, a new base class Core owns; the
  three detection components extend it and override `OnControllableSpawned`.
  The fan-out is otherwise unchanged -- same registry, same order, same call
  per spawn.
- `MCF_Core_GameModeComponent` booted the dialogue library, the hostility
  manager, the task store and the intel store by name, and pushed tasks and
  intel to each player itself. It now publishes four lifecycle events --
  `MCF_Core_PersistentStoreReady`, `MCF_Core_MissionStart`,
  `MCF_Core_PlayerRegistered`, `MCF_Core_PlayerFactionChanged` -- and three new
  module game-mode components listen for the ones they care about:
  `MCF_Ops_GameModeComponent`, `MCF_Dialogue_GameModeComponent`,
  `MCF_AI_GameModeComponent`. The module settings moved with them, so
  `m_bCreateSampleTask` is now an Ops attribute and the hostility decay pair
  an AI attribute.
- `MCF_PlayerControllerTasks.c` -- 37 KB, one `modded class SCR_PlayerController`
  carrying task, intel *and* dialogue RPCs -- split into four files by module:
  `MCF_PlayerController_Core.c` (the message pair everything uses),
  `_Ops.c`, `_Dialogue.c`, `_Subdue.c`.
- `MCF_RestraintPoseEditorAttribute` moved out of the dialogue attributes file
  into its own, under Subdue. It was the only thing making Dialogue name a
  Subdue class.
- Renames: `MCF_Core_IntelStore` -> `MCF_Intel_Store`, `MCF_Core_TaskStore` ->
  `MCF_Task_Store`, `MCF_Core_LineQueueEntry` -> `MCF_Voice_LineQueueEntry`.

Three apparent back-edges turned out to be comments only and needed nothing:
`MCF_Core_ValidationRegistry` -> `MCF_Obj_LogicComponent`,
`MCF_Core_BudgetManager` -> `MCF_React_RecipeComponent`, and
`MCF_AI_DispositionComponent` -> `MCF_ETaskState`.

### The split modded class compiles

The old file was deliberately one block. The comment said why: Enforce chains
modded classes, a block only sees members declared earlier in the chain, and
the order across files was not something the project controlled. Splitting it
was therefore the one genuinely risky change here.

It compiles. `Module: Game; loaded 5746x files; 11271x classes` with no `(E)`,
against a baseline of 5738/11261. Four `modded class SCR_PlayerController`
blocks in four files, three of them calling `MCF_SendMessage` declared in the
fourth. That is half of the phase-1 probe answered early, within one addon; it
still has to be confirmed *across* addons, where the chain order comes from the
dependency graph rather than the file scan.

The order is made safe in one direction only: `MCF_SendMessage` lives in Core's
block, every other block calls it, and nothing in Core's block calls anything a
module declares. If that is ever wrong the compiler says so -- this is not a
failure mode that can go quiet, which is what made the split acceptable.

### Folder layout

`Scripts/Game/` was reorganised from `Core` / `Modules` / `Editor` / `UI` into
one folder per future addon: `Core`, `Objectives`, `Ops`, `Dialogue`, `AI`,
`Subdue`, `Ambient`, `React`. Extracting a module is now a folder move rather
than an untangling.

`Prefabs/Systems/Milsim.et` and the test world's `default.layer` gained the
three new module game-mode components. Milsim.et is now explicitly documented
as a manifest.

### What was verified

Everything, in a live session on 2026-09-10 immediately after the refactor.

**The race the rewrite was most likely to break, caught happening.** The host
registered 100 ms *before* the game mode started, which is the normal ordering
on a listen server and the thing that cost a session before:

```
01:19:30.224  player 1 registered before the task store was ready -- deferring their tasks
01:19:30.324  GameMode start -- resetting per-mission state
01:19:30.326  restored 1 authored conversation(s)          <- MCF_Dialogue_GameModeComponent
01:19:30.326  TaskStore loaded 2 task(s)                   <- MCF_Ops_GameModeComponent
01:19:30.326  IntelStore loaded 3 record(s)
01:19:30.326  TaskStore ready -- catching up 1 already-connected player(s)
01:19:30.327  sent 1 task(s) to player 1: t5
01:19:30.329  Event validation passed
```

The deferral and the catch-up both work through the new event route, and
`Event validation passed` confirms all four new event names have a registered
publisher.

**The faction re-push**, which is the other new event:

```
01:20:15.067  player 1 changed faction to US
01:20:15.067  resending tasks to player 1 after a faction change
01:20:15.068  sent 1 task(s) to player 1 (US): t5
```

**All four split `modded class SCR_PlayerController` blocks over the wire**, not
merely compiling: the operations board opened and listed a persisted task
(Ops), a conversation opened on a civilian (Dialogue), a Game Master wrote and
assigned a conversation (Dialogue), and `shout keys bound` (Subdue).

**The watcher registry.** This needed a second pass: the test world contained no
detection triggers at all, so the first session only ever logged
`notifying 0 registered watcher(s)`, which proves nothing. One of each of the
three trigger types was placed and the session re-run:

```
01:24:21.500  Controllable spawned -- notifying 3 registered watcher(s)
01:24:21.500  ProximityTrigger now watching 1 entities
01:24:27.852  Controllable spawned -- notifying 3 registered watcher(s)
01:24:27.852  ProximityTrigger now watching 3 entities
01:24:27.985  ProximityTrigger FIRED, publishing MCF_Obj_ProximityDetected
01:24:51.627  ConeDetection FIRED, publishing MCF_Obj_ConeDetected
```

All three registered through the new `MCF_Core_ControllableWatcherComponent`
base class, the fan-out reached each overridden `OnControllableSpawned`, and
two of them fired end to end.

That the sample tasks did not appear is correct, not a failure:
`CreateSampleTasks` skips when the store is not empty, and the store came back
with two tasks from earlier sessions. The store loading is the stronger result.

**Worth fixing separately:** the test world had no detection trigger in it, in a
project whose detection components are among its oldest. That is why this gap
could sit unnoticed. The three probes should stay in the world as fixtures.


## Modularisation, phase 2: MCF React is a separate addon (2026-09-10)

The first module actually left the addon. React was chosen because it is the
cheapest possible proof: four scripts, one prefab, no vanilla override, and
nothing else in MCF depends on it.

```
G:\MCF\addons\MCF_React\
  addon.gproj                          GUID 6A50E40BA3B94B01
  Prefabs\MCF_React_Recipe.et(.meta)   GUID 77EA9923B4F69728, unchanged
  Scripts\Game\React\*.c               4 files
```

```
GameProject {
 ID "MCF_React"
 GUID "6A50E40BA3B94B01"
 TITLE "MCF React"
 Dependencies {
  "58D0FB3206B6F859"   // ArmaReforger
  "6A50E40BA3B94A4F"   // MCF
 }
}
```

The prefab kept its `.meta`, so its GUID did not change and nothing that
referenced it had to be edited.

### Both halves measured

**With the module.** Opening MCF_React (MCF comes in as a dependency):

```
FileSystem: Adding relative directory 'G:\MCF\addons\MCF_React' under name MCF_React
FileSystem: Adding relative directory 'G:\MCF\addons\MCF'       under name MCF
Module: Game; loaded 5746x files; 11271x classes
```

No `(E)`. 5746/11271 is exactly the pre-split figure, so nothing was lost in
the move. The test world then opened and logged:

```
Init entity @"{77EA9923B4F69728}Prefabs/MCF_React_Recipe.et"
```

That entry lives in **Core's** `Configs/Editor/MCF_PlaceableEntities.conf` and
resolved to a prefab in **another addon**. Cross-addon GUID resolution works,
and the manifest pattern from section 2 of MODULARISATION.md holds in practice.

**Without the module.** Opening MCF alone, with React physically gone from the
addon but still named in Core's placeables manifest:

```
Module: Game; loaded 5742x files; 11257x classes
```

No `(E)`, and no `Wrong GUID/name` for the missing React entry. Four files and
fourteen classes fewer -- exactly React's four scripts. **Core is installable
without the module, and the dangling manifest entry costs nothing.**

### The environment trap this cost an hour on

**A `.gproj` the Workbench has never opened cannot be launched from the command
line.** Every `-gproj G:\MCF\addons\MCF_React\addon.gproj` attempt died with:

```
ENGINE (E): Addon 'MCF_React' dependency '58D0FB3206B6F859' can't be added
ENGINE (E): Game addon '58D0FB3206B6F859' not found
ENGINE (E): Cannot initialize game project settings!
```

The reason is visible in the `Addon dirs:` block. A working launch lists the
game's own addon directory:

```
dir: 'G:/MCF/addons/MCF/'
dir: 'G:/SteamLibrary/steamapps/common/Arma Reforger/addons'      <- the game
dir: 'C:/Program Files (x86)/.../Arma Reforger Tools/Workbench/addons'
... every workshop addon ...
dir: 'G:/SteamLibrary/steamapps/common/Arma Reforger/addons/data/'
```

A failing one has that slot filled with the literal string `./addons`, resolved
against the process working directory:

```
dir: 'G:/MCF/addons/MCF_React/'
dir: './addons'                                                    <- fallback
dir: 'C:/Users/.../ArmaReforgerWorkbench/addons'
```

So the game data path is not discovered per launch; it comes from state the
Workbench writes when a project is opened through its own UI. Things that do
**not** fix it: setting the process working directory, and hand-editing
`profile\.projectList_app1874910_user<id>.conf` (that file is the recent-project
list for the picker, nothing more).

**What does work: open the new project once through the Workbench itself.** The
log then shows

```
DEFAULT : using additional addon: 6A50E40BA3B94B01 (G:/MCF/addons/MCF_React/addon.gproj)
```

and everything resolves. After that first open the project is registered and
behaves like any other.

Two consequences for the remaining six modules: **each new addon needs one
manual open in the Workbench before any automated launch of it will work**, and
the MCP's `wb_launch` on an unregistered project silently opened a *different*
project instead (`Module: Game; loaded 5660x files` and a stream of
`Failed to call not existing Net API function 'EMCP_WB_Ping'` -- the MCF
handlers were not loaded at all). Always confirm the file/class count matches
what you expect before trusting a session.

### Still open

- The test world lives in Core and places module prefabs, so Core would end up
  depending on Objectives and Ops. It has to move to its own addon that depends
  on everything -- and that addon becomes the development entry point.
- GUID space: modules are being minted from `6A50E40BA3B94Bxx`. One documented
  range per module, before two of them collide.
- The split `modded class SCR_PlayerController` has still not been tested
  *across* addons. React contains no such block. Ops, Dialogue and Subdue do,
  and they are the ones that will answer it.


## Modularisation, phase 3: the whole framework is modular (2026-09-10)

MCF is now eight addons. Everything loads, compiles clean, and resolves across
addon boundaries.

```
MCF             6A50E40BA3B94A4F   24 scripts,  4 prefabs             Core
MCF_Objectives  6A50E40BA3B94B02   11 scripts,  8 prefabs
MCF_Ops         6A50E40BA3B94B03   18 scripts,  4 prefabs, 4 layouts
MCF_Dialogue    6A50E40BA3B94B04   13 scripts,  1 prefab,  2 layouts
MCF_Subdue      6A50E40BA3B94B05    8 scripts
MCF_Ambient     6A50E40BA3B94B06    8 scripts,  2 prefabs
MCF_React       6A50E40BA3B94B01    4 scripts,  1 prefab
MCF_Dev         6A50E40BA3B94B07   the test world and the missions
```

86 scripts, the same count as before the split. Every module depends on Core
and on nothing else. `MCF_Dev` depends on all of them and is the development
entry point; it is never published.

### Three boundaries that moved after a second look

**Disposition and hostility went into Core, not into a module of their own.**
The earlier plan had an `MCF_AI` module holding `MCF_Hostility_Manager` and
`MCF_AI_DispositionComponent`. Nobody would ever install that for itself -- it
only ever appears as a dependency of Dialogue, Subdue and Ambient. Something
that exists only as a dependency is a library, and libraries belong in Core.
Moving it removed an entire addon and three dependency edges, and it makes the
disposition component named in Core's `Character_Base` manifest always
resolvable. The hostility decay attributes went back onto
`MCF_Core_GameModeComponent` and `MCF_AI_GameModeComponent` was deleted.

**`MCF_Task_Permissions` was two things wearing one name.** Resolving a
player's command tier -- Game Master rights, faction commander, group leader,
all read out of vanilla -- has nothing to do with tasks. It was the only reason
the dialogue module depended on the operations board: "only a Game Master may
write a conversation" was asking the task board for an answer. That half is now
`MCF_Core_Roles` in Core, with the enum renamed `MCF_ETaskRole` -> `MCF_ERole`.
What stayed in Ops is the part that really is about tasks: the table saying
which tier may READ/ACCEPT/EDIT/CREATE/PUBLISH. **That was the last remaining
cross-module reference in the whole framework.**

**Squad cohesion moved from Ambient to Ops.** It is about player squads --
position sharing, a muster gate, a radio respawn hint -- not about ambient AI.
Ops is the command-and-control module; that is where it belongs.

### Measured

Opening `MCF_Dev` with all seven other addons:

```
using additional addon: 6A50E40BA3B94B06 (G:/MCF/addons/MCF_Ambient/addon.gproj)
using additional addon: 6A50E40BA3B94B07 (G:/MCF/addons/MCF_Dev/addon.gproj)
... all eight ...
Module: Game; loaded 5746x files; 11270x classes
```

No `(E)`. 5746 files is the pre-split figure exactly; 11270 classes is one
fewer than before, which is right to the class: `MCF_AI_GameModeComponentClass`
and `MCF_AI_GameModeComponent` gone (-2), `MCF_Core_Roles` added (+1),
`MCF_ERole` replacing `MCF_ETaskRole` (0).

The test world -- which lives in `MCF_Dev` -- then opened and initialised
prefabs from four different addons:

```
[MCF] ConeDetection init      {2D283A489D1D8BCC}Prefabs/MCF_Obj_ConeDetectionTrigger.et   MCF_Objectives
[MCF] SpottedByPlayer init    {6C5FE2CCB6B45596}Prefabs/MCF_Obj_SpottedByPlayer.et        MCF_Objectives
[MCF] ProximityTrigger init   {8A24F93EA862750C}Prefabs/MCF_Obj_ProximityTrigger.et       MCF_Objectives
[MCF] operations board registered  {6A1C4F0B39D27E10}Prefabs/MCF_Task_Board.et            MCF_Ops
[MCF] Recipe init                                                                        MCF_React
[MCF] TextLine init, LineDisplay init                                                    MCF
```

The only errors in the whole session are the pre-existing
`Multiple map entities present!` pair, which predates the split.

### The manifest pattern, seen failing safely one more time

In an intermediate session where `MCF_Ops` and `MCF_Dialogue` were not yet
loaded, Core's `chimeraMenus.conf` produced exactly five lines:

```
RESOURCES (E): Wrong GUID/name for resource @"{...}UI/layouts/MCF/MCF_PlanningBoard.layout" in property "Layout"
RESOURCES (E): ... MCF_IntelViewer.layout ... MCF_IntelEditor.layout
RESOURCES (E): ... MCF_Dialogue.layout ... MCF_DialogueEditor.layout
```

The config still loaded and the rest of the presets were unaffected. Five menu
presets in Core naming layouts that live in absent addons cost five log lines
and nothing else -- the same shape as the component and placeable cases.

### How a new addon actually gets loaded, corrected

The earlier note said "budget one manual open per module". That was not quite
right, and the real mechanism matters:

- **`-gproj` on a project the Workbench has never seen never works.** Stage 1 of
  the launcher always ends in `Game addon '58D0FB3206B6F859' not found` and
  `Cannot initialize game project settings!` -- *including on launches that
  then succeed*. That message is not the failure; it is normal. What matters is
  whether stage 2 follows, which is the real Workbench with the full
  `Addon dirs:` block including
  `G:/SteamLibrary/steamapps/common/Arma Reforger/addons`.
- **The launcher's addon list is populated from**
  `Documents\My Games\ArmaReforgerWorkbench\profile\.projectList_app1874910_user<id>.conf`.
  A new addon has to be in that file to be tickable. Writing it there by hand
  works for making it *selectable*; it does not by itself make `-gproj` work.
- **The addons are then selected in the launcher UI and the project opened
  there**, which logs `using additional addon: <GUID> (<path>)` per addon.

So the workflow for a new module addon is: create the folder and `addon.gproj`,
add it to the project list file, then open the project once through the
launcher with the addon ticked. Rewriting the project list while the launcher
is waiting makes it wait longer -- do it with the Workbench closed.

Also worth remembering: `wb_launch` on an unregistered project silently opened
a completely different project (`loaded 5660x files`, and a stream of
`Failed to call not existing Net API function 'EMCP_WB_Ping'` because the MCF
handlers were not loaded at all). **Check the file and class count before
trusting any session.**

### Not verified

Nothing has been watched running since the split. The refactor moved
`MCF_Task_Permissions.ResolveRole` to `MCF_Core_Roles` and every dialogue
Game-Master check with it, and folded the hostility decay setting back into
Core's game mode component. A play session still has to confirm the board, a
conversation, and a shout all behave. And the split `modded class
SCR_PlayerController` now genuinely spans four addons -- Core, Ops, Dialogue
and Subdue -- which compiles, but has not been exercised over a wire since.
