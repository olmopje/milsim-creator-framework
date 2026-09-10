# MCF — Handover

**Read this first when starting a new chat for this project.**

This document exists in two places and they must be kept identical: as a doc in
the Claude project (which a new chat surfaces on its own) and as `HANDOVER.md`
in the repository root, which is tracked in git (it was gitignored until
`f80bff5`; the older text saying otherwise is stale). When you update one,
update the other.

Last updated: 2026-09-10, after the modularisation was completed.

---

## What this is

Milsim Creator Framework — a mission framework for Arma Reforger, prefix `MCF_`.
Reusable building blocks for mission makers: triggers, objectives, AI behaviour,
hostility, conversations, intel and taskings, all authorable live in Game Master
without scripting.

| Thing | Where |
|---|---|
| Repository root | `G:\MCF` |
| The addons | `G:\MCF\addons\` — `MCF` (Core), `MCF_Objectives`, `MCF_Ops`, `MCF_Dialogue`, `MCF_Subdue`, `MCF_Ambient`, `MCF_React`, `MCF_Dev` |
| Workbench project | open **`MCF_Dev`** with every `MCF*` addon ticked — it depends on all of them and holds the test world |
| Test world | `G:\MCF\addons\MCF_Dev\worlds\arland\MCFTestworld.ent` |
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

All of it was re-confirmed in a live session after the phase-0 refactor on
2026-09-10.

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

## The modularisation — done

MCF is eight addons. Every module depends on Core and on nothing else. The full
design and every measurement is `docs/architecture/MODULARISATION.md`; the
chronology is in `PROJECT_STATUS.md`. What to carry in your head:

**The measured fact it all rests on.** A prefab that names a script class from
an addon that is not loaded still loads: the unresolvable component is dropped,
the rest of the entity is intact, and it costs one `WORLD (E)` line. The same
holds for a config entry naming a missing class (silent) and a menu preset
naming a missing layout (one `RESOURCES (E)`, config still loads).

**The rule.** Exactly one MCF addon may override a vanilla GUID, and that addon
is Core. There are four — `Character_Base.et`, `EditorModeEdit.et`,
`chimeraMenus.conf`, `chimeraInputCommon.conf` — and each is a **manifest**
naming every module's contribution whether or not that module is installed.
Core's editor configs (attributes, context actions, placeables) work the same
way. Modules never override vanilla.

**What is Core, and why.** Core holds what more than one module needs, plus
what can only live in a vanilla manifest: the event bus, logging, tick and game
loop, budget, persistent store, validation registry, tags and identity,
factions, the line/HUD channel, the player-controller message channel,
`MCF_Core_Roles` (who is in the chain of command), hostility and disposition,
and the AAR manager. A thing that only ever exists as somebody's dependency is
a library, and libraries go in Core.

**Verified end to end.** All eight addons loaded:
`Module: Game; loaded 5746x files; 11270x classes`, no `(E)`, and the test
world in `MCF_Dev` initialised prefabs from four different addons.

### Watched running, and it holds

A live session with all eight addons exercised every module across an addon
boundary: the listen-server race and the deferral, both stores loading from
module addons on Core's event, the join push and faction re-push, all three
detection triggers registering through Core's watcher registry and firing, the
planning board opening from a preset in Core's manifest with a layout in
`MCF_Ops`, a conversation assigned and opened (`trust=50 fear=0` — the
disposition component from Core's `Character_Base` manifest, read by the
dialogue module, with the Game-Master check going through `MCF_Core_Roles`),
and `shout keys bound` from `MCF_Subdue` against Core's input manifest.

**The `modded class SCR_PlayerController` merges across four addons at
runtime**, not merely at compile time. No `Wrong GUID/name`, no
`Unknown class`; every `(E)` in the session is pre-existing or vanilla.

Not exercised: an actual shout (only the key binding), and the restrain/escort
chain.

### The devices module — functional 2026-09-10

Break-in and intel presentation both run in a live session. See
`docs/architecture/DEVICES.md` for the design and the open list.

- **Presentation moved out of MCF_Devices into MCF_Ops.** A letter that looks
  like paper has nothing to do with hacking. `MCF_Intel_ShellMenu` draws three
  skins — paper, notepad, phone — chosen by the object's own `MCF_EIntelView`.
  MCF_Devices keeps the lock and the games, nothing else.
- **Three break-in games**, and the seed decides which: keypad, signal lock,
  port table. Nothing about the puzzle travels except the seed, so a fourth
  game touches three places and none of them is the wire format.
- Confirmed working, not polished. Nobody has tuned the difficulty curve, and
  the smartphone and laptop models are still not imported.

### Next, in order

1. **A two-peer, two-faction session.** Unblocks faction-scoped intel,
   late-join replication and the audience filter — and can fold in the two
   remaining modularisation unknowns: the dropped-component behaviour at
   runtime on a dedicated server, and the same packed to `.pak`.
2. **Decide what gets published and how.** Eight addons is a lot for a user to
   install. Worth checking whether a Workshop dependency chain does the work,
   or whether a bundle is needed.
3. Turn off `m_bEveryoneMayDoEverything` and watch role resolution.
4. A one-clip animation graph for the restrained pose. Build it small — the
   crash is a size problem, not a concept problem. `arms_back` was the clip the
   user picked. Preview in `anims/workspaces/player/player_main.aw`.

## Environment quirks that will otherwise cost you an hour

**A brand-new addon cannot be launched from the command line, and stage 1
always looks like a failure.** `-gproj` on a project the Workbench has never
opened ends in `Game addon '58D0FB3206B6F859' not found` /
`Cannot initialize game project settings!`. That message also appears on
launches that then succeed — it is the launcher stub, and what matters is
whether a second session follows with the full `Addon dirs:` block including
`G:/SteamLibrary/steamapps/common/Arma Reforger/addons`.

To make a new addon usable: add it to
`Documents\My Games\ArmaReforgerWorkbench\profile\.projectList_app1874910_user<id>.conf`
(that file populates the launcher's addon list), then open the project through
the launcher with the addon ticked. The log confirms it with
`using additional addon: <GUID> (<path>)` per addon. Edit that file with the
Workbench closed — rewriting it while the launcher is waiting just makes it
wait longer.

**Launching the Workbench.** For an already-registered project the path must
be the `addon.gproj` itself — passing `G:\MCF` gives
`projectPath resolves outside every configured root`, after which the MCP tool
may silently auto-launch its own `EnfusionMCP.gproj` instead. `wb_launch` on an
unregistered project did exactly that: it opened something else entirely
(`loaded 5660x files` and endless `Failed to call not existing Net API function
'EMCP_WB_Ping'`). **Always check the file/class count before trusting a
session.** `wb_reload` is unreliable. The loop that works:

```
kill the Workbench
Start-Process -ArgumentList '-gproj','<a registered addon.gproj>'
sleep ~55s
logs_filter for  \(E\)|Module: Game;
```

With all eight addons loaded a clean compile reads
`Module: Game; loaded 5746x files; 11270x classes` with no `(E)` lines. That
proves the scripts compiled and **nothing else** — see the first rule below.

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

**Enfusion materials take two PACKED textures, not separate maps.**
`MatPBRBasic` wants `BCRMap` (base colour RGB, roughness in alpha) and `NMOMap`
(normal X/Y in R/G, metalness in B, occlusion in A). An NMO has no blue Z
channel, so it previews olive-green — that is correct. Name files `_BCR` and
`_NMO` and the importer picks the right preset by itself, but it leaves the
`.emat` empty for you to fill.

**An import can write the `.meta` and not the resource.** One texture in eight
came out as `metafile without corresponding resource`. Re-importing produced it
with the same GUID. **Check the file exists, not just its meta.** The FBX
importer also assigns `{536BF67B2052B869}material/metal.gamemat` as the
collision surface, which does not resolve — delete the `SurfaceProperties`
block from the `.xob.meta`.

**A model needs one mesh named `LOD0` and collision named `UTM_<x>`**, at
real-world scale with the origin at the base. Both are confirmed to survive the
import. Store models arrive at absurd scales — check before trusting one.

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

**Enforce Script has no ternary and no closures**, `reference` and `out` are
reserved words — `out` used as a local name is a "Broken expression (missing
';'?)" with no hint why — `ToUpper()` mutates in place and returns an int, and a
ScriptComponent cannot declare a bare constructor.

**A script method cannot take a `func` parameter.** "func arguments are not
supported in script methods", so a helper cannot be handed a callback to bind.
Bind several buttons to one handler and tell them apart by the component the
click carries.

**`Math.RandomInt` breaks down on a large range.** `Math.RandomInt(1, 0x7FFFFFFE)`
returned, on four consecutive calls, `-1`, `65535`, `1` and `65536` — negative,
clustered on powers of two, and outside the lower bound it was given. Keep each
draw well under ~30 000 and mix several together if you need a wide value.
Measured 2026-09-10, and worth remembering because the symptom was nowhere near
the cause: it looked like a puzzle picker that always picked the same puzzle.

**Escape stops the play session in the Workbench.** Every MCF screen needs a
visible, always-enabled close button.

**A widget wraps text only with `Wrap 1`.** `Clipping` decides whether overflow
is drawn or cut, never whether there is any. And a RichText wraps at the width
it is GIVEN -- a scroll gives as much as is asked for, so a `SizeLayoutWidget`
has to cap it. Three things, and getting two right looks exactly like getting
none right.

**`GetScreenSize` reports the previous layout pass.** Reading it in the same
frame as a `FrameSlot` change returns the old value, which reads exactly like
the call did nothing. Wait a frame.

**A `FrameWidgetSlot`'s fields pair up**: PositionX/OffsetLeft,
PositionY/OffsetTop, SizeX/OffsetRight, SizeY/OffsetBottom. Position and Size
are the box, the Offsets are padding. Writing the box into the Offsets leaves
the size at zero and every widget vanishes with no error.

**External images ARE possible, by a route nobody signposts. Measured
2026-09-10, every link.** The obvious ones are all shut:
`ImageWidget.LoadImageTexture` refuses an http address outright;
`RestContext.FILE`, the only file download, is `[Obsolete("Not supported")]` and
INERT -- it never calls back, never errors and never writes the file; and there
is no API to build a texture from bytes (`ScreenshotTextureData` is an engine
pointer with no constructor).

What works is a chain of four supported calls:

    RestContext.GET               text from a URL
    base64 -> array<int>          in script; 5216 chars in 7 ms
    FileHandle.WriteArray(a,1,n)  raw bytes to $profile: -- one byte per element
    LoadImageTexture(.., true)    png from disk, no import, no conversion

So the image travels as TEXT and is rebuilt as a file on the machine that draws
it. `MCF_Device_ImageCache` is that, written out. Two conditions: whoever
publishes the image must publish a base64 copy beside it (a raw .jpg URL cannot
be used -- `GetData` returns a string and binary dies at the first zero byte),
and EACH CLIENT fetches its own, so a player behind a firewall has no picture
and that has to read as normal rather than as an error.

`fromLocalStorage` on `LoadImageTexture` is the flag that makes any of it work:
it skips the resource database, so a path is handed to the file system instead
of looked up as an imported asset. `.png` loads directly -- no `.edds`
conversion needed.
