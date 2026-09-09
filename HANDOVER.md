# MCF — Handover

**Read this first when starting a new chat for this project.**

This document exists in two places and they must be kept identical: as a doc in
the Claude project (which a new chat surfaces on its own) and as `HANDOVER.md`
in the repository root, which is tracked in git (it was gitignored until
`f80bff5`; the older text saying otherwise is stale). When you update one,
update the other.

Last updated: 2026-09-10, after the modularisation phase-0 session.

---

## What this is

Milsim Creator Framework — a mission framework for Arma Reforger, prefix `MCF_`.
Reusable building blocks for mission makers: triggers, objectives, AI behaviour,
hostility, conversations, intel and taskings, all authorable live in Game Master
without scripting.

| Thing | Where |
|---|---|
| Repository root | `G:\MCF` |
| The addon | `G:\MCF\addons\MCF` |
| Workbench project file | `G:\MCF\addons\MCF\addon.gproj` |
| Test world | `G:\MCF\addons\MCF\worlds\arland\MCFTestworld.ent` |
| GitHub | `olmopje/milsim-creator-framework`, branch `main` |
| Wiki | `.../wiki` — 11 pages, the public-facing documentation |

## Standing instructions from the user

- Work through the **`enfusion-workbench-mcp`** MCP tools.
- **All documentation and code comments in ENGLISH.** Conversation with the user
  is in Dutch.
- Keep responses short and focused. Do not over-research.
- When told to continue, keep going rather than checking in.

---

## Where the knowledge lives

Nothing important should live only in a chat. In order of authority:

| Document | What it is |
|---|---|
| `docs/architecture/PROJECT_STATUS.md` | The chronological record, ~1400 lines. Every session including the dead ends. **The primary source.** |
| `docs/architecture/MODULARISATION.md` | The plan for splitting MCF into separate addons, and the measurements it rests on. **The current main thread of work.** |
| `docs/ROADMAP.md` | What is verified, what is missing, what comes next |
| `docs/architecture/ARCHITECTURE.md` | The plan: layering, the core, the integration contract |
| `docs/guides/MISSION_MAKER_GUIDE.md` | Plain-language reference to every node and player-facing system |
| The GitHub wiki | The same material, organised for a reader who is not us. `Enfusion-Lessons` is the highest-value page |
| `CHANGELOG.md` | Release-by-release summary; current version 0.3.0 |

If you learn something that cost time to discover, it belongs in
`PROJECT_STATUS.md` and, if it is not MCF-specific, on the wiki's
`Enfusion-Lessons` page.

---

## State as of 2026-09-10

### Proven in a live session

- The event-driven core: event bus, tick manager, budget caps, persistence via
  `$profile:` (outside the engine's own saves, so a mod update cannot wipe it).
- All twelve placeable node types, each observed firing.
- The operations board: taskings and intel, read/amend split, create and edit.
- Conversations with any AI, gated on trust and fear, authored from a Game
  Master library. Reaches every character through an override of vanilla
  `Character_Base` rather than an MCF prefab.
- A custom keybind (H / U) appended to vanilla's input config. This was
  previously unproven for any Arma Reforger mod.
- Shout → surrender → restrain → interrogate → escort, end to end.
- `RplProp` on a ScriptComponent over a real wire, with `BumpMe()` sufficient.

**Everything in that list was proven BEFORE the phase-0 refactor below, and
nothing has been watched running since.** Treat it as "worked yesterday", not
as "works".

### Not proven

- **Phase 0 of the modularisation.** It compiles and the test world opens. No
  behaviour has been observed. See the next section.
- **Faction-scoped intel.** Implemented, needs two factions and two peers.
- **Late-join replication** of Game Master intel edits. Built on `RplProp` and
  believed correct, never watched with a client joining late.
- **The audience filter** on text lines. Plumbed end to end, never observed
  actually selecting a subset.

### Known gaps, deliberately open

- **The restrained pose is empty.** Restraining works and escape is disabled,
  but there is no hands-behind-back animation. The game ships none, and
  mounting the 23 KB narrative graph as a loiter attachment **crashes the
  Workbench natively** (vanilla's officer graph is 475 bytes). Needs a
  purpose-built one-clip graph.
- `m_bEveryoneMayDoEverything` in `MCF_Task_Permissions` is still `true`. Role
  resolution is written but switched off until roles have been watched in a
  session with real people in real slots.
- Conversation flags, trust and fear do not survive a server restart.
- Dropped intel objects do not respawn after a restart.
- `m_bVisibleOnMap` on objectives does nothing; there is no map integration.

---

## The modularisation, which is the current thread

The goal: MCF splittable into separate addons, so someone can install Core
alone, or Core plus a module, and adding or removing one breaks nothing. The
full design is `docs/architecture/MODULARISATION.md`. The three things worth
carrying in your head:

**The measured fact everything rests on.** A prefab that names a script class
from an addon that is not loaded still loads. The unresolvable component is
dropped, the rest of the entity is intact, and it costs one `WORLD (E)` line.
Measured 2026-09-10 with a throwaway prefab. This is why the design works at
all.

**The rule.** Exactly one MCF addon may override a vanilla GUID, and that addon
is Core. There are four such overrides — `Character_Base.et`,
`EditorModeEdit.et`, `chimeraMenus.conf`, `chimeraInputCommon.conf` — and each
becomes a manifest naming every module's contribution whether or not that
module is installed. Modules never override vanilla.

**The eight modules.** Core (which now also holds the line/voice primitive and
the AAR manager), Objectives, AI, Subdue, Ambient, Ops (tasks *and* intel — they
are one product, splitting them is circular), Dialogue, React.
`Scripts/Game/` already has one folder per module, so extracting one is a
folder move.

### Where phase 0 got to

Done: all eight cross-module back-edges cut, the folder reorganisation, Core's
game-mode component reduced to publishing four lifecycle events, three new
module game-mode components listening for them, and
`MCF_PlayerControllerTasks.c` split four ways. Compiles clean at
`Module: Game; loaded 5746x files; 11271x classes`.

**Not done: watching any of it run.** The lifecycle rewrite changed the order
in which the task store comes up relative to a player registering — the exact
race that cost a session before. Confirm the host still receives its sample
tasks before building anything on top.

### Next, in order

1. **A play session that proves phase 0 did not break anything.** Cheapest
   possible check: open the test world, confirm the sample tasks arrive.
2. **A two-peer, two-faction session.** Unblocks faction-scoped intel,
   late-join replication and the audience filter — and can fold in the three
   remaining modularisation unknowns: the dropped-component behaviour at
   runtime on a dedicated server, the same packed to `.pak`, and whether a
   **user action** naming a missing class behaves like a component
   (`Character_Base` carries six).
3. **Phase 1 probe:** two addons both declaring `modded class SCR_PlayerController`.
   Half-answered already — four such blocks in four files compile inside one
   addon — but across addons the chain order comes from the dependency graph,
   not the file scan.
4. **Phase 2: extract MCF React** as the first real addon. Four files, no
   vanilla overrides, nothing depends on it.
5. Turn off `m_bEveryoneMayDoEverything` and watch role resolution.
6. A one-clip animation graph for the restrained pose. Build it small — the
   crash is a size problem, not a concept problem. `arms_back` was the clip the
   user picked. Preview in `anims/workspaces/player/player_main.aw`.

---

## Environment quirks that will otherwise cost you an hour

**Launching the Workbench.** The project file is `addon.gproj`, not
`MCF.gproj`, and the path must be `G:\MCF\addons\MCF` — passing `G:\MCF` gives
`projectPath resolves outside every configured root`, after which the MCP tool
may silently auto-launch its own `EnfusionMCP.gproj` instead. `wb_reload` is
unreliable. The loop that works:

```
kill the Workbench
Start-Process -ArgumentList '-gproj','G:\MCF\addons\MCF\addon.gproj'
sleep ~55s
logs_filter for  \(E\)|Module: Game;
```

A clean compile reads `Module: Game; loaded 5746x files; 11271x classes` with no
`(E)` lines. That proves the scripts compiled and **nothing else** — see the
first rule below.

**PowerShell quoting breaks constantly** on nested quotes. Pass literal content
through the `var1`..`var4` parameters rather than inlining it. Use `.Contains()`
rather than `-like`. Normalise `\r\n` to `\n` before matching multi-line text.

**Files in `G:\MCF` are intermittently locked, and a failed write is SILENT.**
This bit again on 2026-09-10: `device_commit_files` reported success, the file
on disk was unchanged, and the same compile error reappeared from a file that
had "already been fixed". The create succeeds, the atomic replace does not.
**Always read the file back after writing it.** The reliable pattern is to
delete first:

```powershell
Remove-Item $p -Force
[IO.File]::WriteAllText($p, $new)
```

**`Show-TextFiles`** takes `-LineRange '10-70'` and `-Pattern`, not `-Head` or
`-StartLine`.

---

## Rules earned the hard way

The full list is the wiki's `Enfusion-Lessons` page. The ones most likely to
bite in the next session:

**Compiling proves nothing.** Three separate times in one day, code that
compiled cleanly and logged no errors could never fire. Verify by observing
behaviour, in the Peer Tool if it touches replication — a Workbench play session
is server and client at once and hides every replication bug.

**A GUID override REPLACES, it does not merge.** Generate overrides mechanically
with `game_duplicate`; never hand-write one. But `+{ }` inside a config override
**appends**, which is how the keybind and the menu presets work.

**An unresolvable component class in a prefab is DROPPED, not fatal.** One
`WORLD (E)` line, and the entity loads with its remaining components intact.
Measured 2026-09-10. A `MenuPreset` naming a missing script class is silent;
one naming a missing layout GUID logs a single `RESOURCES (E)`.

**`[BaseContainerProps()]` on every class that appears in a `.conf`**, or the
parser silently skips them and the config loads empty with no error.

**A `SCR_BaseGameModeComponent` subclass cannot name a method the base class
already has.** `OnPlayerRegistered` as a ScriptInvoker callback fails with
"Callbacks do not support overloaded methods". Prefix your own handlers.

**Game Master attributes carry 12 bytes and cannot hold text.** A pick is a
number; free text needs a custom menu and an RPC. A slider-backed attribute
writes a **float** — `CreateInt`/`GetInt()` round-trips as 0.

**`DeactivateAI()` is the only thing that actually stops an AI.** Loitering does
not suppress the behaviour tree. `SetYawPitchRoll` does nothing;
`AlignPosDirWS` only holds inside a loiter.

**No script API can raise a noise the AI hears.** `EarsSensor` and danger events
are engine-raised only, which is why the shout system asks each AI directly.

**Characters cannot be physically coupled.** `Character_Base` lists itself under
"Forbidden linking"; carry mods work only because their subject is unconscious.

**Enforce Script has no ternary and no closures**, `reference` is a reserved
word, `ToUpper()` mutates in place and returns an int, and a ScriptComponent
cannot declare a bare constructor.

**Escape stops the play session in the Workbench.** Every MCF screen needs a
visible, always-enabled close button.

---

## Working conventions

- Commits end with the Co-Authored-By and Claude-Session attribution lines.
- Research notes go in `docs/research/`, **never** under `addons/MCF/` —
  anything under the addon is packed into the shipped mod and handed to every
  player. This was fixed on 2026-09-10.
- `Scripts/Game/` has one folder per future addon: `Core`, `Objectives`, `Ops`,
  `Dialogue`, `AI`, `Subdue`, `Ambient`, `React`. Put a new file in the folder
  of the module it belongs to, and do not let a module name a class from a
  module it does not depend on.
- `server/` is untracked: it holds a launcher config with an admin password and
  a profile folder of pure runtime logs.
- `addons/MCF/EnfusionMCP/` and `Scripts/WorkbenchGame/EnfusionMCP/` are the MCP
  tool's own handlers, living inside the addon that gets packed for players.
  They should become their own addon during the split.
- The UI is deliberately placeholder and uniform, with one exception: the
  conversation screen is a running chat with the newest line at the top, and is
  meant to look different.
