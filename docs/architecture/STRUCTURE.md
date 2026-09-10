# MCF — the addon structure

**This is the structure of record.** If another document disagrees with this one,
this one is right and the other is stale — including the copy of it that lives in
the Claude project, which has been wrong before precisely because it was a copy.

Everything below was measured on **2026-09-10** by running
`tools/measure_addon_graph.ps1`. Re-run it before believing any of it. The
document this replaces was wrong for a week because nobody did, and it was wrong
again on the day it was rewritten: it was updated in the project's knowledge base
and never in the repository, so the repository kept telling readers there were
eight addons for a day after there were six.

---

## 0. Core is the mod

MCF Core is not one addon among six. It is **the framework** — the thing a
server installs, the thing everything else plugs into, and the thing the project
is named after. The other five are modules that extend it. Core depends on
nothing; every module depends on Core; a server that runs Core alone runs a
working, if quiet, framework.

That settles what the Workshop listing looks like when it happens: **Core is the
headline entry and the modules are dependent entries**, not six equal siblings a
mission maker has to assemble.

**Six addons is not six Workshop entries.** It is Core plus four modules — five
things that could ever be published — and `MCF_Dev`, which is not a distribution
addon at all. MCF_Dev is the test environment: the world, the missions and the
self-test harness we develop against. It depends on every other addon precisely
because a test harness must reach everywhere, and that is only safe because it
never ships. Nothing a player installs may ever depend on it.

**And it is exactly why the Core rule needs guarding.** "Core is the main mod"
makes it easy to argue that anything important belongs there. It does not. The
rule is *shared*, not *important*:

> Anything used by two modules goes in Core. Anything used by one module stays
> in that module, however central it feels.

Core is already the second-largest addon at 207 KB. If that boundary slips, in a
year Core is the mod and the modules are empty shells — which is the single-addon
project this structure was built to get away from.

---

## 1. The six addons

Every one is a folder under `addons/` with its own `addon.gproj`.

| Addon | GUID | Declares | Script | What it is |
|---|---|---:|---:|---|
| **MCF** | `…4A4F` | 43 | 207 KB | **Core.** Required by everything. |
| **MCF_Ops** | `…4B03` | 62 | 523 KB | Tasks, intel, devices, the operations board, squad cohesion |
| **MCF_Dialogue** | `…4B04` | 23 | 126 KB | Conversations: data, library, view, components, menus |
| **MCF_AI** | `…4B06` | 32 | 71 KB | Ambient life and the subdue/compliance chain |
| **MCF_Objectives** | `…4B02` | 30 | 53 KB | Logic nodes, objectives, triggers, recipes, sequences |
| **MCF_Dev** | `…4B07` | 3 | 140 KB | Test environment: the world, the missions, the self-test harness. **Not a distribution addon.** |

"Declares" is classes and enums; "Script" is the size of the `.c` under that
addon. MCF_Dev declares almost nothing and weighs a lot because a test world is
data, not code.

### What is in Core, and why

Event bus, logging, tick manager, game loop, budget manager, persistent store,
data sets and snapshots, validation registry, tag registry, object identity,
faction helper, roles, disposition and hostility, debug overlay, game mode
component, watcher registry. Plus:

- the four vanilla overrides and their editor config files (see section 3);
- `Prefabs/Systems/Milsim.et`, the game mode prefab;
- the `modded enum ChimeraMenuPreset` block;
- the line/voice primitive: `MCF_Voice_LineQueueManager`,
  `MCF_UI_LineDisplayComponent`, `MCF_Voice_TextLineComponent`,
  `MCF_Interact_HintComponent`;
- `MCF_AAR_DebriefManager`.

**The rule, stated by the project owner and kept in his words: anything shared
between modules goes in Core.** Not in whichever module happened to need it
first. Disposition, hostility and roles are in Core for exactly this reason —
they exist only as dependencies of other things, and something that exists only
as a dependency is a library.

The rule has a cost worth naming. Core is the second-largest addon and it is
going to keep growing, because every time two modules want the same thing it
lands there. That is the intended trade: one growing library beats a web of
modules reaching into each other.

---

## 2. The measured graph

Not asserted — counted, by extracting every class and enum each addon declares
and searching every other addon's scripts for those names:

```
MCF              -> (none)
MCF_AI           -> MCF 8
MCF_Dialogue     -> MCF 12
MCF_Objectives   -> MCF 7
MCF_Ops          -> MCF 12
MCF_Dev          -> MCF 4, MCF_Ops 8
```

Every module depends on Core and on nothing else. MCF_Dev is the single
exception and is allowed to be: it is the test harness, it reaches wherever it
must, and it is never published.

**Comment lines are stripped before counting, and that is not a nicety.**
Counting them reported five illegal edges that do not exist — Core "depending
on" Ops because `MCF_Core_Roles.c` has the words `MCF_ETaskState` in a sentence
explaining why an enum is append-only, and Ops "depending on" Dialogue because
`MCF_Device_Library.c` opens with "this is `MCF_Dialogue_Library` with the nouns
changed". A cross-reference is code or it is nothing. The first version of the
measuring script did not know that and would have had this document announce a
architecture violation that was five comments.

---

## 3. Why exactly one addon may override a vanilla GUID, and why it is Core

> **Modules never override vanilla. Core owns every override, as a manifest.**

There are four vanilla overrides:

| Override | What it carries | Which modules need it |
|---|---|---|
| `Prefabs/Characters/Core/Character_Base.et` | 4 components, 6 user actions | Dialogue, AI |
| `Prefabs/Editor/Modes/EditorModeEdit.et` | MCF's attribute list, context-action list and placeable registry | every module with a placeable or attribute |
| `Configs/System/chimeraMenus.conf` | the menu presets | Ops, Dialogue |
| `Configs/System/chimeraInputCommon.conf` | `MCF_CharacterContext`, keys H and U | AI |

Each is a manifest naming every module's contribution whether or not that module
is installed. This works because of a measured fact:

**A component the engine cannot resolve is dropped, and the entity survives it.**
A probe prefab with one non-existent component and three real ones loaded with
Component Count 3 and one error line. A `MenuPreset` naming a missing script
class is *completely silent*; one naming a missing layout GUID costs one error
and leaves the other presets alone.

The price: Core knows the *names* of what every module contributes. That is a
manifest dependency, not a code dependency — no compile-time coupling, no load
order requirement. It does mean adding a module means editing Core, which is
fine for a first-party set and would not be for third-party ones.

---

## 4. Namespaces, and where they actually live

A class name is `MCF_<Namespace>_<Thing>`. The namespace used to tell you which
addon owned a class. **It no longer does**, and pretending otherwise is how
someone ends up looking for `MCF_AI_DispositionComponent` in the MCF_AI addon,
where it is not.

| Namespace | Addon | Covers |
|---|---|---|
| `MCF_Core_` | MCF | Event bus, identity, tick manager, stores, validation |
| `MCF_Hostility_` | MCF | Hostility and reputation |
| `MCF_Voice_` | MCF | Voice lines and comms |
| `MCF_UI_` | MCF | Player-facing display components |
| `MCF_AAR_` | MCF | After-action review |
| `MCF_AI_` | **MCF and MCF_AI** | Disposition and roles in Core; ambient life, compliance, subdue in MCF_AI |
| `MCF_Interact_` | **MCF and MCF_Dialogue** | The hint primitive in Core; the talk action in Dialogue |
| `MCF_Dialogue_` | MCF_Dialogue | Conversation data, library, component, menus |
| `MCF_Obj_` | MCF_Objectives | Objective, POI and logic nodes |
| `MCF_Infra_` | MCF_Objectives | Infrastructure network |
| `MCF_React_` | MCF_Objectives | Behaviour recipes and sequence playback |
| `MCF_Ops_` | MCF_Ops | The game mode component |
| `MCF_Task_` | MCF_Ops | Taskings, board, permissions |
| `MCF_Intel_` | MCF_Ops | Intel records, carriers, sources, the reading shells |
| `MCF_Device_` | MCF_Ops | Device *content*: profiles, library, presenter, layout |
| `MCF_Devices_` | MCF_Ops | Device *locks*: the lock component, the break-in games |
| `MCF_Data_` | MCF_Ops | The data manager screen |
| `MCF_Squad_` | MCF_Ops | Squad cohesion |
| `MCF_Dev_` | MCF_Dev | The self-test harness |

### Two known defects in this table

**`MCF_Device_` and `MCF_Devices_` differ by one letter** and mean genuinely
different things — what is on a device versus whether you may look at it. That
is a name nobody can hold in their head, and it is a trap for exactly the kind
of typo a compiler will not catch. It wants renaming; the sensible split is
`MCF_Device_` for content and `MCF_Lock_` for the lock and its games.

**`MCF_AI_` and `MCF_Interact_` each span two addons**, which is a direct
consequence of the Core rule: the shared half moved to Core and kept its name.
That is defensible, but the table above has to exist for anyone to know it, and
a name that requires a table is a name that is doing less work than it should.

Neither is urgent. Both are recorded here rather than quietly tolerated, because
the point of a namespace is that you do not have to look anything up.

---

## 5. Which project file to open

Six addons means six `addon.gproj` files, and the Workbench opens one project at
a time. The dependency list decides what comes with it.

**Open `addons/MCF_Dev/addon.gproj`.** It depends on Core and on all four
modules, so it is the only one that loads the whole framework — and it carries
the test world the nodes are exercised in. This is the project file for any
normal session.

Open a module's own `.gproj` only when you deliberately want to check that the
module still stands up without its siblings.

```
MCF            -> ArmaReforger
MCF_AI         -> ArmaReforger, MCF
MCF_Dialogue   -> ArmaReforger, MCF
MCF_Objectives -> ArmaReforger, MCF
MCF_Ops        -> ArmaReforger, MCF
MCF_Dev        -> ArmaReforger, MCF, MCF_Objectives, MCF_Ops, MCF_Dialogue, MCF_AI
```

**A new or renamed addon must be opened once through the Workbench UI** before
any command-line launch of it works. A `.gproj` the Workbench has never seen
cannot find the game data addon and dies with
`Game addon '58D0FB3206B6F859' not found`. Neither the working directory nor the
recent-project list fixes it. Renaming an addon folder counts as new.

---

## 6. Where does a new module go?

In order, stop at the first that applies:

1. **Two modules would both use it** → Core. This is the rule, and it is not a
   judgement call.
2. **It only makes sense together with an existing module's feature** → that
   module. Devices went into Ops because a locked laptop is intel you cannot
   read yet, not a separate subject.
3. **It is four scripts and 9 KB** → the nearest existing module. A new addon
   costs a `.gproj`, a dependency GUID, a Workshop listing and the
   open-it-once-in-the-Workbench tax. Recipes did not earn that; they went to
   Objectives.
4. **Otherwise** → a new addon, and Core's four manifests need editing to name
   its contributions.

---

## 7. Why these boundaries and not others

**Tasks and intel are one module.** They look like two. The planning board reads
the intel store, the intel store writes into the task store, `MCF_Intel_Record`
references `MCF_Task`, and "make a tasking out of this intel" is a headline
feature. Split them and the dependency is circular.

**Devices belong with intel, for the same reason.** MCF_Devices existed for a
while as its own addon holding the lock and the break-in puzzles while the
carrier and the shell stayed in Ops. That split one feature down the middle and
produced the only illegal edge the framework has ever had. Folded back in on
2026-09-10, along with two files whose whole purpose was to reach across the
boundary it created.

**Ambient and Subdue are one module.** The argument was sitting in the tree:
every class in both addons is named `MCF_AI_something`. Neither referenced the
other; the merge cost nothing.

**Recipes are Objectives.** Sequence playback and step running are mission logic.

**Against the mental model in the original brief:** "Core + Intel" is really
**Core + Ops** — intel does not stand alone. "Core + Intel + Interaction" is
**Core + Ops + AI + Dialogue**.

---

## 8. Traps, all of them paid for

**Moving files between addons is free if the relative paths stay the same.**
Resource references are `{GUID}relative/path`, and the path is relative to the
addon root — so `Prefabs/X.et` moved from addon A to addon B at the same
relative path leaves every reference byte-identical. Both merges in the
consolidation changed nothing outside the moved files.

**GUID space.** MCF hand-assigns from one block, `6A1C4F0B39D2xxxx`, and it is
not partitioned per module. Two collisions have already happened — a prefab and
a mesh each claiming a GUID that belonged to something else, one of them caught
only by `ResourceDB: duplicate GUID found` in the log. Partition the block, or
check with `git grep` before minting.

**`modded class` merges across addons.** Four `modded class SCR_PlayerController`
blocks in four files compile and work over the wire, including one calling a
method declared in another. Within an addon the order comes from the file scan;
across addons it comes from the dependency graph.

**A component that will not resolve is dropped silently-ish.** See section 3.
This is what makes the manifest pattern safe, and it is also what will hide a
typo in a component name for weeks.

---

## 9. Still open

**`addons/MCF/EnfusionMCP/` and `Scripts/WorkbenchGame/EnfusionMCP/`** are the
MCP tool's own Workbench handlers, living inside the addon that gets packed and
handed to players. They should be their own addon, or outside `addons/`
entirely.

**Publishing.** The shape is decided — Core as the headline entry, the four
modules as dependent entries, MCF_Dev not published at all. What is not decided
is the mechanics: whether Reforger's Workshop dependency handling makes four
separate module entries pleasant enough to be worth it, or whether the first
release is Core plus everything in one entry with the split kept only in the
repository.

**The two namespace defects in section 4.**

**Never observed:** the missing-component behaviour at runtime on a dedicated
server, and after packing to `.pak`. Both were measured only in the World
Editor, unpacked. Also unobserved: whether a **user action** entry naming a
missing class behaves like a component entry. It is the same container parser
and almost certainly does, but `Character_Base` carries six user actions.
