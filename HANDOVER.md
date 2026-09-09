# MCF — Handover

**Read this first when starting a new chat for this project.**

This document exists in two places and they must be kept identical: as a doc in
the Claude project (which a new chat surfaces on its own) and as `HANDOVER.md`
in the repository root (which is gitignored, so it never leaves this machine).
When you update one, update the other.

Last updated: 2026-09-10.

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
| `docs/architecture/PROJECT_STATUS.md` | The chronological record, ~1290 lines. Every session including the dead ends. **The primary source.** |
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

Working tree clean, everything pushed. Last commit `4c17102`.

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
- Shout to surrender to restrain to interrogate to escort, end to end.
- `RplProp` on a ScriptComponent over a real wire, with `BumpMe()` sufficient.

### Not proven

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

## Suggested next steps, ordered by dependency

1. **A two-peer, two-faction session.** This unblocks three unproven things at
   once: faction-scoped intel, late-join replication and the audience filter.
   Cheapest verification available and it is overdue.
2. **Turn off `m_bEveryoneMayDoEverything`** and watch role resolution in that
   same session. One switch, revertible without a rebuild.
3. **A one-clip animation graph for the restrained pose.** Build it small —
   the crash is a size problem, not a concept problem. `arms_back` was the
   clip the user picked. Preview animations in
   `anims/workspaces/player/player_main.aw` (it loads without a body; add one).
4. Persist conversation flags and dropped intel objects across a restart.
5. Map integration for objectives.

None of this is committed to. Ask the user what they want rather than assuming
this order.

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
sleep ~48s
find the newest log directory
logs_filter for  \(E\)|Module: Game;
```

A clean compile reads `Module: Game; loaded 5738x files; 11261x classes` with no
`(E)` lines. That proves the scripts compiled and **nothing else** — see the
first rule below.

**PowerShell quoting breaks constantly** on nested quotes. Pass literal content
through the `var1`..`var4` parameters rather than inlining it. Use `.Contains()`
rather than `-like`. Normalise CRLF to LF before matching multi-line text.

**Files in `G:\MCF` are intermittently locked.** `Add-LinesToFile` and friends
fail with *"Unable to move the replacement file to the file to be replaced"* —
the create succeeds, the atomic replace does not, and the file is silently
unchanged. Always verify the write landed. The reliable pattern for editing an
existing file:

```powershell
$raw = [IO.File]::ReadAllText($p)   # or build the new content
Remove-Item $p -Force
[IO.File]::WriteAllText($p, $new)
```

Take a backup copy outside the repo first if the file matters.

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

**`[BaseContainerProps()]` on every class that appears in a `.conf`**, or the
parser silently skips them and the config loads empty with no error.

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
- `server/` is untracked: it holds a launcher config with an admin password and
  a profile folder of pure runtime logs.
- The UI is deliberately placeholder and uniform, with one exception: the
  conversation screen is a running chat with the newest line at the top, and is
  meant to look different.
