# MCF — Modularisation design

Status: **done.** MCF is eight addons. Every module depends on Core and on
nothing else. Sections 1-4 are the design as it ended up; section 5 records how
it got there. Nothing has been watched running since the final split.

Written 2026-09-10, against commit `4c17102`.

---

## 1. The question that decides everything, and its answer

The plan hinged on one unknown: **what does the engine do with a prefab that
names a script class from an addon that is not loaded?** Everything else
follows from that answer, so it was measured rather than reasoned about.

### The experiment

A throwaway prefab was written with one component that does not exist and
three that do:

```
GenericEntity {
 components {
  MCF_Probe_ThisClassDoesNotExist { }
  MCF_Obj_LogicComponent { }
  RplComponent { }
  Hierarchy { }
 }
}
```

It was placed in the test world through the World Editor.

### The result

```
WORLD     (E): Unknown class 'MCF_Probe_ThisClassDoesNotExist' at offset 293(0x125)
```

…and the entity loaded, with **Component Count: 3** — `MCF_Obj_LogicComponent`,
`RplComponent` and `Hierarchy` all present and correct.

**A component the engine cannot resolve is dropped. The entity survives it.**
The cost is one error line per missing class per prefab load.

Two more probes were run in the same launch:

| Probe | Result |
|---|---|
| `MenuPreset` naming a script `Class` that does not exist | **Completely silent.** No error at all. |
| `MenuPreset` naming a `Layout` GUID that does not exist | One `RESOURCES (E)` at `MenuManager config load`. The config still loaded; the other five presets were unaffected. |

All probes were reverted; the working tree is clean.

### What this rules in and out

- **Runtime component attachment is not needed.** There is no
  `AddComponent`/`CreateComponent` in the scripting API (searched the shipped
  script documentation; nothing), and after this result there is no reason to
  want one. That route is closed and does not need to be.
- **"The override lives in the most dependent module and drags everything"
  is not needed either.** Both of the options on the table were answers to a
  problem that does not exist.

### What still has to be proven

This was observed at prefab load in the World Editor, unpacked. Not yet
observed:

1. the same behaviour at runtime on a dedicated server;
2. the same behaviour after the addon is packed to `.pak`;
3. that a **user action** entry naming a missing class behaves like a
   component entry. It is the same container parser and almost certainly does,
   but `Character_Base` carries six user actions and they matter as much as the
   four components.

All three are cheap to fold into the two-peer session that is already queued.
Nothing below should be built on until (1) and (3) are watched.

---

## 2. The rule that follows

> **Exactly one MCF addon may override a vanilla GUID, and that addon is Core.
> Modules never override vanilla.**

There are four vanilla overrides, not one, and `Character_Base` is not even the
worst of them:

| Override | What it carries | Which modules need it |
|---|---|---|
| `Prefabs/Characters/Core/Character_Base.et` | 4 components, 6 user actions | Dialogue, AI, Subdue |
| `Prefabs/Editor/Modes/EditorModeEdit.et` | points at MCF's attribute list, context-action list and placeable registry | **every module with a placeable or an attribute** |
| `Configs/System/chimeraMenus.conf` | 5 menu presets | Ops, Dialogue |
| `Configs/System/chimeraInputCommon.conf` | `MCF_CharacterContext`, keys H and U | Subdue |

Core owns all four. Each becomes a **manifest**: it names every module's
contribution, whether or not that module is installed. A module that is absent
leaves an inert entry and, at worst, one line in the log.

Concretely, Core's `Character_Base` keeps exactly what it has today —
`MCF_Dialogue_Component`, `MCF_AI_DispositionComponent`,
`MCF_AI_ComplianceComponent`, `MCF_AI_SubjectControlComponent` and the six
actions — even though Core provides none of those classes. Install nothing but
Core and you get a vanilla character with four dropped components and four
error lines. Install Dialogue and the talk action wakes up. Install Subdue and
the restrain chain wakes up. Nothing breaks in between.

### The price, stated plainly

Core knows the *names* of everything every module contributes. That is a
manifest dependency, not a code dependency — no compile-time coupling, no
crash, no load order requirement. But it does mean **adding a module means
editing Core**, which is fine for a first-party set of modules and not fine for
third-party ones. If MCF ever wants third-party modules, the manifests are
where that fight happens, and the escape hatch is runtime registration for the
two manifests that might support it (`SCR_PlaceableEntitiesRegistry` and the
editor attribute list). Worth investigating later; not worth blocking on now.

---

## 3. Proposed module boundaries

### Layer 0 — MCF Core (required by everything)

Event bus, logging, tick manager, game loop, budget manager, persistent store,
validation registry, tag registry, object identity, faction helper, debug
overlay, game mode component, watcher registry — plus:

- the four vanilla overrides above and their three editor config files;
- `Prefabs/Systems/Milsim.et`, the game mode prefab;
- the `modded enum ChimeraMenuPreset` block;
- **the line/voice primitive**: `MCF_Voice_LineQueueManager`,
  `MCF_UI_LineDisplayComponent`, `MCF_Voice_TextLineComponent`,
  `MCF_Interact_HintComponent`;
- `MCF_AAR_DebriefManager`.

Folding the line primitive into Core is deliberate. Three modules call it
(Objectives, React, Interact), it is four small files, and splitting it out
creates three cross-module edges to buy a choice nobody wants — "MCF without
the ability to put a line of text on screen" is not a configuration anyone
asks for.

### Layer 1 — the modules

| Module | GUID | Contents | Depends on |
|---|---|---|---|
| **MCF Objectives** | `…4B02` | Logic, Objective, ProximityTrigger, ConeDetectionTrigger, TriggerZone, AlarmTrigger, SpottedByPlayer, ObservationNode, Infra_Node, 2 editor attributes | Core |
| **MCF Ops** | `…4B03` | Task, Task_Permissions, TaskStore, BoardComponent, BoardActions, PlanningBoardMenu, Intel, Intel_Record, IntelStore, CarrierComponent, SourceComponent, ReadAction, IntelEditorMenu, IntelViewerMenu, IntelEditContextAction, Squad_Cohesion | Core |
| **MCF Dialogue** | `…4B04` | Dialogue_Data, Library, Script, View, Component, AssignComponent, Menu, EditorMenu, EditContextAction, editor attributes, TalkAction | Core |
| **MCF Subdue** | `…4B05` | Shout, ShoutInput, Compliance, SubjectControl, SubdueActions, MarchBehavior, RestraintPoseEditorAttribute | Core |
| **MCF Ambient** | `…4B06` | SimpleMover, EventToWaypoint, AmbientActor, LifestylePOI, WaypointAnimation, CommandWatchdog, FallbackPointRegistry, CivilianBehaviorHook | Core |
| **MCF React** | `…4B01` | Recipe, SequencePlayback, SequenceRecorder, StepRunner | Core |
| **MCF Dev** | `…4B07` | the test world and the missions — depends on everything, never published | all |

### Three boundaries that moved after a second look

**Disposition and hostility are Core, not a module.** They only ever appear as
a dependency of Dialogue, Subdue and Ambient; nobody installs them for
themselves. Something that exists only as a dependency is a library, and
libraries belong in Core. This removed a whole addon and three dependency
edges, and makes the disposition component named in Core's `Character_Base`
manifest always resolvable.

**`MCF_Core_Roles` is Core; task permissions are Ops.** Resolving a player's
command tier out of vanilla has nothing to do with tasks, and it was the only
reason Dialogue depended on Ops. The role half moved to Core (`MCF_ERole`); the
table saying which tier may do what to a task stayed in Ops. That was the last
cross-module reference in the framework.

**Squad cohesion is Ops, not Ambient.** It is about player squads — position
sharing, muster, radio respawn — not about ambient AI. Ops is the
command-and-control module.

### Why tasks and intel are one module and not two

They look like two and are one. `MCF_PlanningBoardMenu` reads the intel store,
the intel store writes into the task store, `MCF_Intel_Record` references
`MCF_Task`, and "make a tasking out of this intel" is a headline feature. Split
them and the dependency is circular; keep them together and it is a single
coherent product — the operations board. If the board's intel panel is ever
made conditional they can be split later, but there is nothing to gain from
doing it first.

### Against the mental model in the brief

- "Core + Intel" → **Core + Ops**. Intel does not stand alone.
- "Core + Intel + Interaction" → **Core + Ops + AI + Dialogue**, and, if you
  want the surrender/restrain chain, + Subdue.

---

## 4. The event bus is a seam in intent, not yet in fact

`MCF_Core_EventManager`'s header says modules never reference each other
directly. A full symbol cross-reference across all 81 scripts says otherwise —
though the damage is much smaller than that sounds. Against the partition
above there are **eight** offending edges, and two files account for most of
them.

| # | Edge | Where | Fix |
|---|---|---|---|
| 1 | Core → Objectives | `MCF_Core_AutoWatcherRegistry` holds three typed arrays of Obj component types | One `MCF_Core_IControllableWatcher` base class, one array, register/unregister by interface |
| 2 | Core → Ops, Dialogue, AI | `MCF_Core_GameModeComponent.OnGameModeStart` boots the dialogue library, hostility manager, intel store, task store, AAR by name; `OnPlayerRegistered` pushes tasks and intel | A `MCF_Core_IModuleBootstrap` registry: each module registers a bootstrap object, Core calls it. The per-player task/intel push moves into Ops |
| 3 | Core → AAR | `MCF_Core_DebugOverlay` asks the debrief manager for a count (one line) | Moot — AAR stays in Core. Otherwise: registered reporters |
| 4 | Ops + Dialogue + Subdue tangled | `MCF_PlayerControllerTasks.c`, 37 KB, one `modded class SCR_PlayerController` carrying task, intel *and* dialogue RPCs | Split into three files, one per module. `modded class` merges across addons — that is what it is for |
| 5 | Objectives → Subdue | `MCF_Obj_ConeDetectionTrigger` and `MCF_Obj_SpottedByPlayer` call `MCF_AI_ComplianceComponent` | Publish an event |
| 6 | Dialogue → Subdue | `MCF_DialogueEditorAttributes` touches `MCF_AI_SubjectControlComponent` (one reference) | Move the attribute to Subdue, or route via Disposition |
| 7 | AI → Ops | `MCF_AI_DispositionComponent` reads `MCF_ETaskState` (one reference) | Cut it |
| 8 | naming | `MCF_Core_IntelStore`, `MCF_Core_TaskStore`, `MCF_Core_LineQueueEntry` are not Core; `MCF_AI_Shout.c` and `MCF_PlayerControllerTasks.c` sit in `Scripts/Game/Core/` | Rename and move |

Two things worth noting: `MCF_Core_ValidationRegistry` and
`MCF_Core_BudgetManager` looked like back-edges in a first pass but only
mention module classes **in comments**. They are clean.

Edge 4 rests on an assumption that has never been tested in this project:
that two addons may each declare `modded class SCR_PlayerController` and have
Enforce merge them. That is what `modded` is for, and vanilla does it across
its own modules, but this project has been bitten by "probably fine" before.
**Probe it before relying on it** — it is a two-file, one-launch test.

---

## 5. Staging

**Phase 0 — DONE 2026-09-10, no files left the addon.** All eight edges cut.
`Scripts/Game/` reorganised into one folder per future addon: `Core`,
`Objectives`, `Ops`, `Dialogue`, `AI`, `Subdue`, `Ambient`, `React`. Core's
game-mode component now publishes four lifecycle events instead of booting
modules by name, and three new module game-mode components
(`MCF_Ops_`, `MCF_Dialogue_`, `MCF_AI_GameModeComponent`) listen for them.
`MCF_PlayerControllerTasks.c` split four ways. Compiles clean:
`Module: Game; loaded 5746x files; 11271x classes`, no `(E)`, and the test
world opens with no `Unknown class` on the game mode.

**Watched running the same day, and it holds.** The listen-server race was
caught happening -- the host registered 100 ms before the game mode started,
the deferral fired, and the catch-up delivered the task. The faction re-push
works, event validation passes, all four split `modded class` blocks work over
the wire, and the three detection components register through the new base
class (`notifying 3 registered watcher(s)`, then two of them firing). Details
in `PROJECT_STATUS.md`.

Half of the phase-1 chain probe was answered early as a side effect: four
`modded class SCR_PlayerController` blocks in four files compile, with three of
them calling a method declared in the fourth. That is within one addon, where
the order comes from the file scan. Across addons it comes from the dependency
graph instead, and still has to be confirmed.

**Phase 1 — two probes, one launch.** (a) Two addons both declaring
`modded class SCR_PlayerController`. (b) A prefab from addon A carrying a
component class from addon B, with B not loaded, observed at runtime on a
dedicated server and packed to `.pak` — items (1) and (2) from §1.

**Phase 2 — DONE 2026-09-10. MCF React is a separate addon.**
`G:\MCF\addons\MCF_React`, GUID `6A50E40BA3B94B01`, depending on MCF. Both
halves measured: with the module loaded, `Module: Game; loaded 5746x files;
11271x classes` (the pre-split figure exactly), no `(E)`, and Core's placeables
manifest resolved `{77EA9923B4F69728}Prefabs/MCF_React_Recipe.et` across the
addon boundary; without it, `5742x files; 11257x classes`, no `(E)`, and the
dangling manifest entry cost nothing. Cross-addon GUID resolution and the
manifest pattern both hold. Details and the launch trap in `PROJECT_STATUS.md`.

**A new addon must be opened once through the Workbench UI before any
command-line launch of it will work.** A `.gproj` the Workbench has never seen
cannot find the game data addon and dies with
`Game addon '58D0FB3206B6F859' not found`. Neither the working directory nor
the recent-project list fixes it. Budget one manual open per module.

**Phase 3 — the rest, one module per step**, in dependency order: Objectives,
AI, Ambient, Subdue, Ops, Dialogue.

---

## 6. Two things to settle before Phase 2

**GUID space.** MCF hand-assigns GUIDs from one block, `6A1C4F0B39D2xxxx`. Once
there are seven addons that block has to be partitioned, one documented range
per module, or two modules will eventually mint the same GUID.

**`addons/MCF/EnfusionMCP/` and `Scripts/WorkbenchGame/EnfusionMCP/`** are the
MCP tool's own Workbench handlers, living inside the addon that gets packed and
handed to players. The same reasoning that moved research notes out of
`addons/MCF/` on 2026-09-10 applies here. They should be their own addon, and
the split is a natural moment to do it.
