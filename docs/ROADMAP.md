# MCF Roadmap

Single entry point. `PROJECT_STATUS.md` is the chronological record of what was
found and when; this file says what is true now and what happens next.

Last updated 2026-09-09.

---

## What MCF verifiably is today

Everything in this section has been observed working, not merely compiled.
That distinction matters here: two framework-wide defects found on 2026-09-09
(`m_Flags` and the missing `EntityEvent.INIT` mask) had both been silently
broken for a long time while everything compiled cleanly.

**A working event-driven core.** `MCF_Core_EventManager` (event bus),
`MCF_Core_TickManagerComponent` driven by `MCF_Core_GameLoopComponent`, and
`MCF_Core_GameModeComponent` tying them to the game mode lifecycle.

**Server-authoritative detection.** Proximity triggers detect registered
entities and publish events. Clients evaluate nothing — confirmed by
`notifying 0 proximity and 0 spotted triggers` on peer clients.

**Multi-node chaining with state.** Proven five links deep: two proximity
triggers -> a Logic node in AND mode -> an Alarm relay -> a Recipe -> text on
screen. The AND node correctly stays silent on a partial condition.

**Objectives.** Event-driven complete/fail, with a working intel gate.

**Player-visible output over the network.** `MCF_UI_LineDisplayComponent`
broadcasts by RPC from the server; each client renders locally through
`SCR_PopUpNotification`. Verified across three processes with the Peer Tool.

**Cross-restart persistence.** `MCF_Core_PersistentStore` writes to
`$profile:` via `FileIO`, deliberately outside the engine's world/session
saves so a mod update cannot wipe it.

**Runs on a real dedicated server** with an unpublished local addon.

## Every node type is now verified

All twelve placeable types were exercised on 2026-09-09 and observed firing,
not merely compiling. Verifying them turned up four silent defects, which is
the point of having done it.

**Working event nodes** — each observed publishing its event in a live session:
Proximity Trigger, Cone Detection Trigger, Spotted By Player, Logic Node (AND),
Alarm Trigger, Recipe, Objective, Trigger Zone, Observation Node, Text Line.

**Markers, not nodes** — Lifestyle POI and Safe Fallback Point. Neither has an
event surface and neither should. Lifestyle POI is a slot-tracking data holder
for an ambient-actor system that does not exist yet; Safe Fallback Point is a
self-registering marker the AI Command Watchdog queries. They work as intended;
they are simply not triggers. Whether they belong in the placement browser at
all is the question in `placeable-vs-attached-nodes.md`.

### The four defects verification exposed

1. **Trigger Zone, Observation Node and Text Line had no input at all.** Each
   had a method (`Activate()`, `Report()`, `Play()`) that nothing in the
   framework ever called. A mission maker could place and configure them but
   never fire them. All three now take a trigger-event attribute, wired by
   event name like every other node.
2. **Cone Detection could never detect anything.** It ticked every frame over a
   watch list that nothing ever filled, because it was missing from
   `MCF_Core_AutoWatcherRegistry` — only Proximity and Spotted registered
   there. It compiled, initialised, and logged nothing. Now registered; the
   before/after is visible in the log as `notifying 3 proximity and 0 spotted`
   becoming `notifying 3 proximity, 1 cone and 1 spotted`.

All four were invisible: no errors, no warnings, clean compiles. That is now
three separate occasions in one day where "it compiles" meant nothing.

## The task store exists and survives restarts

First concrete piece of the task system (stage 3), built on everything proven
earlier the same day.

`MCF_Task` carries **SMEAC** fields — Situation, Mission, Execution,
Admin/Logistics, Command/Signal — because that is the five-paragraph order
milsim unit leaders are already trained to write and read. Structuring a task
that way asks them for something they already produce; a free-text box would
throw that structure away. Every field except id and title is optional, so a
squad leader retasking mid-contact is not forced through five paragraphs.

It also carries an author, an assignee (player, group or faction), a state, and
a **back brief** — the subordinate's own plan briefed back up, which real units
do and which a purely top-down model would have missed.

`MCF_Core_TaskStore` is server-authoritative (`CreateTask` refuses to run on a
client, so ids are allocated in one place) and persists through
`MCF_Core_PersistentStore`, deliberately outside the engine's world saves so a
mod update during a week-long server cannot wipe a week of planning.

**A task serialises itself**, and both the store and the network layer use that
one implementation — two formats for the same object would drift apart, and
that bug is miserable to find. One task is one line:

```
task.t1=id=t1<TAB>title=Recon the north approach<TAB>mission=2nd squad confirms...
```

Verified across two server runs: created and assigned in run 1, restored intact
in run 2 with state and assignee.

### Tasks are sent per player, not broadcast

Text lines are broadcast and filtered on the client. Tasks are not, and the
difference is deliberate.

In milsim a rifleman holding the commander's entire plan in client memory is
wrong *in the fiction*, not merely wasteful. So the server decides what each
player may see (`IsVisibleTo`) and sends only that. `RplRcver.Owner` routes an
RPC to one client, which is why the transport will hang off a modded
`SCR_PlayerController` rather than the GameMode entity.

Current visibility rules: a DRAFT is the author's alone; an author always keeps
sight of what they wrote; PLAYER and FACTION assignments resolve; **GROUP
withholds**, because squad membership is not resolvable yet and erring toward
telling people too little is the safer default.

**Not networked yet** — the transport is the next increment. Nothing authors a
task in game either; `m_bCreateSampleTask` on the game mode component is a
development aid, off by default, that seeds one task so the system can be
exercised at all.

## Audience filtering: plumbed, partly implemented

A line now carries its audience from the node that created it, through the
queue, into the RPC, to a check on each client. `MCF_EAudience` lives with the
line queue; Text Line and Objective expose it as a dropdown.

- **EVERYONE** — implemented, and the default.
- **FACTION** — implemented, comparing against
  `SCR_FactionManager.GetLocalPlayerFaction().GetFactionKey()`.
- **PLAYER** — implemented, comparing against
  `GetGame().GetPlayerController().GetPlayerId()`. No editor path sets it yet;
  it exists for script and for "tell whoever triggered this".
- **GROUP** — deliberately NOT implemented. It needs the squad hierarchy that
  arrives with the task system, and building the mechanism before anything can
  express "send this to squad 2" would repeat the mistake this consolidation
  pass just finished fixing: a mechanism with no caller. Selecting it shows to
  everyone and logs a warning.

Compiles clean and initialises correctly. **The filtering itself has not been
observed working** — that needs two clients on different factions, which the
Peer Tool can provide but has not yet been used for.

## Still not verified

- **Audience filtering behaviour**, as above: plumbed, not proven.
- **The client-side guards** on `LoadPersistentState` and
  `OnControllableSpawned` compile but were not re-checked on a peer session.
- **The AI layer** is largely unexercised — ambient actors, compliance, command
  watchdog, waypoint animation, squad cohesion, sequence record/playback. These
  are not placeable nodes, so they were out of scope for the node verification
  pass, but given its hit rate, assume they hold similar defects.

## Known gaps, deliberately open

- `m_bVisibleOnMap` on objectives does nothing; there is no map integration.
- Detection is distance-and-angle geometry, not line of sight. No raycast API
  was ever confirmed.
- AI movement uses `SetOrigin()` rather than native `SCR_AIGroup` waypoints,
  which were later found and are the correct route.
- Animations publish an event but play nothing.
- Logic AND is capped at four fixed input slots (Enforce Script has no
  closures).

---

## How the research documents relate

Written across 2026-09-09. They are design notes, not commitments, and some
supersede others.

### Foundation — settled, applies now

| Document | What it settles |
|---|---|
| `addons/MCF/docs/research/prefab-mflags-parse-bug.md` | Why prefabs were invisible in Game Master. Resolved. |
| `addons/MCF/docs/research/editor-attributes-research.md` | How placed nodes expose editable properties. |
| `addons/MCF/docs/research/multiplayer-and-audience.md` | Server authority and the broadcast/filter pattern. Partly implemented. |

### Design questions — open, ordered by when they must be answered

1. **`placeable-vs-attached-nodes.md`** — which nodes belong in the placement
   browser at all. Answer this before the node count grows; the cost rises
   with every prefab added.
2. **`objective-task-system.md`** — what vanilla's task system is and the three
   options for using or replacing it. Feeds directly into the next item.
3. **`mcf-task-system-design.md`** — MCF's own task system: the two-layer split
   between mission-authored intel and player-authored plan, grounded in how
   milsim units actually run (SMEAC, back briefs, the 30-minute planning
   budget).
4. **`persistent-server-and-phases.md`** — the persistent server, in-game HQ,
   briefing room, slotting, and the phase model.
5. **`dialogue-system.md`** — parked. Needs the same UI groundwork as the task
   views; whichever is built first should set the layout conventions.

### One reversal, recorded so it is not re-litigated

`mcf-task-system-design.md` concluded **"build for execution, not planning"** —
because units plan out of game in tools like TacOps, and an in-game planner
would compete there and lose.

`persistent-server-and-phases.md` **supersedes that**. The conclusion assumed
planning must happen out of game. A persistent server with an in-game HQ whose
plans survive into a briefing room is not the same product as a browser tool;
it is a category a browser cannot enter. The original reasoning still holds for
the narrow case of rebuilding map-drawing in game, and nothing else.

### What this makes MCF

Stated plainly because it changes the project's scope: the direction that
emerged is no longer a mission-scripting framework but **unit infrastructure** —
a persistent operations environment spanning planning, slotting, briefing,
execution and debrief. That is a much larger and more valuable thing, and it
should be entered deliberately rather than by drift.

---

## Sequence forward

Ordered by dependency, not by appeal.

**1. Consolidation (in progress).** Verify the node types that have never run;
rewrite `guides/MISSION_MAKER_GUIDE.md` against reality (it currently documents
a framework that provably did nothing before 2026-09-09); remove test
scaffolding; put diagnostics behind a switch.

**2. Finish the networking layer.** Real audience filtering, driven by the
command hierarchy — `SCR_GroupsManagerComponent` already provides groups,
membership and leadership and is live on the GameMode entity. This is shared
foundation for everything in stage 3.

**3. Pick one of the big pieces, not several.** The task system and the
persistent-server model are the same problem seen from two angles and should be
designed together. Dialogue can wait; it competes for the same UI effort.

**4. Publish.** Publishing to the Workshop also fixes dedicated-server mod
loading, since the `mods` config list is validated against the Workshop API and
cannot reference a local addon.

## Rules earned the hard way

Each of these cost real time to discover.

- **Compiling proves nothing.** Both framework-wide defects compiled cleanly
  and logged no errors. Verify by observing behaviour.
- Any ScriptComponent overriding `EOnInit` must also override `OnPostInit` and
  set `EntityEvent.INIT`, or it silently never initialises.
- **Every MCF node runs on the server only.** Detection, logic, reaction and
  objectives all return early from `EOnInit` when `!Replication.IsServer()`.
  Clients render what the server tells them and nothing else. A peer test found
  reaction nodes still subscribing to events on clients -- inert, because those
  events never fire there, but it left clients holding framework state and
  would have diverged the moment anything published locally.
- Any static manager holding per-mission state must be reset from
  `OnGameModeStart()` — statics survive the editor-to-play transition.
- `OnGameModeStart` fires on every machine. Guard server state with
  `Replication.IsServer()`.
- Read the vanilla source before writing anything. Both major bugs, the RPC
  pattern, the popup widget and the file IO API were all answered there.
- A Workbench play session is server and client at once and hides every
  replication bug. Use the Peer Tool (dropdown beside the Play button).
- Local Host binds UDP 2001; a dedicated server started alongside it dies with
  `Unable to start replication`.

## Where documents live

Developer-facing docs (`ARCHITECTURE.md`, `PROJECT_STATUS.md`, this file,
guides, module notes) live in `G:\MCF\docs`. Research notes written alongside
the code live in `G:\MCF\addons\MCF\docs\research` and ship with the addon.
Keep new research with the code; keep planning and status here.


---

# Rules earned the hard way, part two (2026-09-09, UI and intel)

Every one of these cost at least one restart-and-test round. None of them
would have been caught by a compiler or a linter.

**`reference` is a reserved word in Enforce Script.** `string reference = ...`
fails with `Broken expression (missing ';'?)` on that line and nothing else.
The method `GetReference()` is fine; only the variable name is poisoned.

**`Replication.FindItemId` takes the replicated COMPONENT, not the entity.**
Passing an `IEntity` returns an invalid id and the RPC silently addresses
nothing. Vanilla's attribute manager addresses edited objects by their
`SCR_EditableEntityComponent`, and so must we.

**Vanilla prefab `ID` is not a reliable resource GUID.**
`Papers_Personal_Civilian.et` carries the same `ID` as `Props_Base.et` --
copied and never regenerated. Do not derive a GUID from a prefab's own ID;
read it from a place that *references* the prefab, or build from a mesh GUID.

**The Game Master attribute system cannot carry text.**
`SCR_BaseEditorAttributeVar` packs every value into one `vector`, replicates a
fixed 12-byte snapshot, and offers only `CreateInt`, `CreateFloat`,
`CreateBool`, `CreateVector`. There is no string variant, no text-entry
attribute layout, and no edit-box attribute UI component. A *custom* attribute
class does not escape this -- the value still has to fit. Free text has to
leave the attribute system entirely; MCF does it with a context action opening
its own menu and its own RPC.

**A user action with no `Position` on its context never appears.** An empty
`Position {}` leaves the interaction with no anchor in the world, and no
radius fixes it. `PointInfo` has exactly three fields -- `PivotID`, `Offset`,
`Angles` -- and a static prop with no bones uses `Offset`, as vanilla's arsenal
box does with `Offset 0 0.464 0`.

**`Prefabs/Items/Core/Item_Base.et` is what makes a thing pickable**, and it
brings `InventoryItemComponent`, `SCR_PickUpItemAction`, `RplComponent` and
the pickup sounds with it. But it is a *base*: it declares only a weight, so
`CanInsertItem` refuses and Pick up shows greyed out. Every real item adds
`SizeSetupStrategy Manual`, `ItemDimensions` and `ItemVolume`.

**`additionalActions +{ ... }` appends to an inherited actions list** rather
than replacing it -- confirmed in game: MCF's read action appears next to
vanilla's Pick up on the same object.

**`modded enum ChimeraMenuPreset` works**, and MCF's GUID-override of
`Configs/System/chimeraMenus.conf` with `MenuPresets +{ ... }` appends: all 58
vanilla presets survive. Vanilla's own documented route -- overriding
`Game.GetMenuPreset()` -- is NOT open to a mod, because `ChimeraGame` is
generated and Reforger already spent that hook.

**Enforce Script has no ternary operator.** `x ? a : b` does not compile.

**Declare a modded enum once.** Two `modded enum` blocks for the same enum in
different files may well work, but nothing in vanilla or in any local mod does
it, so MCF keeps all its presets in `MCF_MenuPresets.c`.

**Escape is not a way out in the Workbench** -- it stops the play session. Every
MCF screen needs a visible close button, always enabled.

## What is verified working end to end

- Operations board: taskings and intel, read/amend split, create, edit,
  accept, issue, complete, cancel, remove -- all server-validated, persisted,
  and surviving restarts.
- Intel as a carried object: pick up, read, carry, and enter on the board.
  Entering is gated on standing at a board, checked on the server against the
  entity the player controls, so information has to physically travel.
- Game Master authoring: right-click an intel object, rewrite its name, its
  pages and how it reads, applied server-side and broadcast.

## Known gaps, named rather than hidden

- A client joining *after* an intel object is edited misses the broadcast and
  reads the prefab's original text. Needs the override kept server-side and
  pushed on join, the way tasks and intel already are.
- Logged intel is visible to everyone. `MCF_Core_IntelStore.IsVisibleTo` is
  the single place that changes when faction-scoped intel is wanted.
- Permissions are open to everyone. The seam is in
  `MCF_Task_Permissions.Can`, and one flag closes it.
- The map view (`MCF_EIntelView.MAP`) exists in the data model and is not
  built. `SCR_CommandPostMapMenuUI` reusing `MapMenu.layout` with its own
  class is direct evidence a mod's menu can host the vanilla map.
- The AI layer -- ambient actors, compliance, watchdog, waypoint animation,
  squad cohesion, sequence record/playback -- has still never been run.
