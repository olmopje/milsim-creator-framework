# MCF — Handover

Last updated: 2026-09-10, after the documentation sweep.

**Read this first, then stop reading and go look at the thing you are changing.**
This file is the map, not the territory. Everything it claims is claimed
somewhere else in more detail, and that somewhere else is named.

---

## What this is

Milsim Creator Framework — a mission framework for Arma Reforger, prefix `MCF_`.
Reusable building blocks for mission makers: triggers, objectives, AI behaviour,
hostility, conversations, intel and taskings, all authorable live in Game Master
without scripting.

| Thing | Where |
|---|---|
| Repository root | `G:\MCF` |
| The addons | `G:\MCF\addons\` — `MCF` (Core), `MCF_Ops`, `MCF_Dialogue`, `MCF_AI`, `MCF_Objectives`, `MCF_Dev`. Six |
| Workbench project | **`addons/MCF_Dev/addon.gproj`** — the only one that depends on all five others, and it holds the test world |
| Test world | `G:\MCF\addons\MCF_Dev\worlds\arland\MCFTestworld.ent` |
| GitHub | `olmopje/milsim-creator-framework`, branch `main` |
| Wiki | a separate repository, cloned at `G:\MCF\.wiki` (gitignored) so it can be edited in the same session as the code |

**Core is the mod.** Not one addon among six: it is the framework a server
installs and the thing everything else plugs into. The four modules are what
extends it. `MCF_Dev` is the test environment and is never distributed.

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
| `docs/architecture/STRUCTURE.md` | **The structure of record.** The six addons, the measured dependency graph, which addon owns which namespace, the Core rule, which `.gproj` to open, where a new module goes. If anything else disagrees with it, it is stale |
| `docs/architecture/PROJECT_STATUS.md` | The chronological record, every session including the dead ends. **Not** the current state — it says so at the top |
| `docs/architecture/ARCHITECTURE.md` | The plan and the reasoning: layers, the Core, the integration contract |
| `docs/guides/GM_PROPERTIES.md` | **Generated.** All 186 settable properties across 47 components. Never edit by hand — re-run `tools/generate_gm_reference.ps1` |
| `docs/guides/MISSION_MAKER_GUIDE.md` | Plain-language reference to every node and player-facing system |
| `docs/architecture/DEVICES.md` | Intel devices: locks, break-in games, the model pipeline |
| `docs/ROADMAP.md` | What is verified, what is missing, what comes next |
| `CHANGELOG.md` | Release-by-release summary; current version 0.3.0 |
| The GitHub wiki | The same material organised for a reader who is not us. `Enfusion-Lessons` is the highest-value page; `Game-Master` explains the three places MCF is configured |

### The measurement scripts, which outrank all of the above

Documents drift. These do not, because they read the code:

| Script | Answers |
|---|---|
| `tools/measure_addon_graph.ps1` | Which addon depends on which, counted from real code references. **Comment lines are stripped** — counting them once reported five illegal edges that were five comments |
| `tools/generate_gm_reference.ps1` | Every `[Attribute(...)]` in the framework, and regenerates `GM_PROPERTIES.md` |

**Run the first one before believing any structural claim anywhere.** This
project's structure document has been wrong twice: once because nobody
re-measured for a week, once because it was updated in a copy and not in the
repository.

### Two traps in how the documentation itself is kept

1. **The wiki is a separate git repository.** It cannot be updated in the same
   commit as the code, which is exactly why its pages spent a day describing an
   addon layout that no longer existed. Structure facts belong in the repo;
   the wiki links to them. `G:\MCF\.wiki` is a working clone — commit and push
   it separately.
2. **The Claude project's knowledge base holds copies** of `STRUCTURE.md`,
   `HANDOVER.md` and `DEVICES.md`. That is how the eight-versus-six divergence
   happened. When you change one of those three, update both.

---

## State as of 2026-09-10

### Proven by the automated self test, on two peers on two factions

```
SELFTEST start -- 2 player(s): 2 (USSR), 3 (US)
faction-intel:  PASS -- player 2 (USSR) holds its own record and not the other's
line-audience:  PASS -- player 3 (US) filtered out a line for USSR
device-profile: PASS -- player 2 sees the profile the server wrote
picture-fetch:  PASS -- player 3 has the picture on disk
SELFTEST done -- 8 passed, 0 failed
```

Faction-scoped intel in both directions, the audience filter on text lines,
device profiles over `RplProp` reaching a non-host client, and two clients
fetching the same picture independently.

**How to run it:** put `MCF_Dev_SelfTestComponent` on the game mode, tick
`m_bEnabled`, start two peers **on different factions**, read the log. Nothing to
click. It waits for **factions, not players** — the first version started as soon
as two players existed, before either had picked a side, and every filter check
then compared nothing against nothing.

### Proven in a live session

Twelve placeable nodes, every one watched firing. The operations board with its
read/amend split. Conversations with any character, vanilla or modded, through
the `Character_Base` override. Shout, surrender, restrain and escort on a real
custom keybind. Four readable objects — letter, notepad, phone, laptop — each
drawn as the thing it is, with break-in on the device's own screen.

Persistence through `$profile:` and `FileIO`, deliberately outside the engine's
world and session saves, so a mod update cannot wipe a campaign.

### Still open, and honest about why

- **Late join.** A client arriving into a mission that already has authored
  content. The self test cannot help: it asks the players who were already
  there. **Parked** — the user cannot realistically test this right now.
- **Role enforcement.** `MCF_Task_Permissions` resolves roles correctly and
  logs them, but `m_bEveryoneMayDoEverything` is `true`, so every check except
  DESTROY is bypassed. Nothing has ever been refused. Deliberate — watch it be
  right before letting it say no.
- **The restrained pose is empty.** Restraining works and escape is disabled;
  the pose needs a one-clip animation graph. Build it small — the crash was a
  size problem, not a concept problem. `arms_back` is the clip. Preview in
  `anims/workspaces/player/player_main.aw`.
- **Phone read state does not survive a server restart.** It is keyed on the
  object's RplId, which is session-scoped. Deliberate: keying it on the profile
  id made every handset carrying the same profile share one read log, and being
  right within a session beats being wrong across them.
- **Editing a shared device profile on purpose** is not possible. Every edit
  from inside the phone is per object, because the draft clears its id. Doing
  it deliberately needs its own row and its own warning.
- **MAP intel view** exists in the data model and does nothing.
- **GROUP-assigned tasks** go only to their author until squad membership lands.
- **Dropped intel objects do not respawn** after a restart. The board record
  survives; the physical document does not.

---

## The next steps, in the order I would take them

### 1. Close the Edit intel / Edit device overlap — DONE 2026-09-10

Was: `MCF_Intel_EditContextAction.CanBeShown` had no view filter, so "Edit intel"
was offered on every carrier including phones and laptops, and its VIEW toggle
only knew DOCUMENT and DEVICE — one APPLY rewrote a phone into a flat document.

What changed, four files in MCF_Ops:

- `MCF_Intel_CarrierComponent.IsDeviceView(view)` — the PHONE/LAPTOP/DEVICE test,
  now stated once. Both context actions read it, so the two lists cannot drift.
- `MCF_Device_EditContextAction` uses it instead of its own inline list.
- `MCF_Intel_EditContextAction` refuses device views, which is that filter
  mirrored. Edit intel now appears only on DOCUMENT, PAPER, NOTEPAD and MAP.
- `MCF_Intel_EditorMenu`: the VIEW button labels the real view instead of
  calling a NOTEPAD "DOCUMENT", flips only DOCUMENT ↔ DEVICE and refuses the
  rest in the status line, and APPLY now carries the object's own action verb
  instead of resetting it to Read/Search on every write.

Compiles clean: `Module: Game; loaded 5774x files; 11330x classes`, 0 script
errors. **Not yet watched in a live Game Master session** — right-click a phone
and confirm only "Edit device" is offered, and that a notepad's VIEW button
refuses rather than flattens.

### 2. Kill the `metal.gamemat` noise — DONE 2026-09-10

Was: `{536BF67B2052B869}material/metal.gamemat` resolves to nothing and cost two
`RESOURCES (E)` lines per model load.

**The received wisdom about this was wrong, and that is why it kept coming
back.** Stripping `SurfaceProperties` out of the `.xob.meta` does nothing on its
own: all six metas had been clean since 17:48 and every `.xob` still carried the
GUID in its bytes, because the meta was cleaned and the resource was never
rebuilt. `DEVICES.md` had this right and this file did not.

What worked, and it is a two-step:

1. Rewrite the `.fbx` (same bytes is enough — the watcher goes on mtime).
2. Give the Workbench window focus. `wb_resources rebuild` does **not** do it;
   the file watcher is what rebuilds, and it only runs on focus.

Then read the `.xob` back as ASCII and check the GUID is gone — the meta being
clean proves nothing. Five meshes rebuilt (Letter, Notepad, Smartphone,
Laptop_Body, LaptopOpen; LaptopLid has no collider and never carried it).

The importer did **not** re-inject it, which contradicts the old note: with
`SurfaceProperties` empty in the meta, the rebuilt `.xob` came out clean.
Verified on a fresh Workbench session: the phone streams in at 22:22:07 with no
error line behind it, and the session's `error.log` holds 0 errors.

### 3. Turn `m_bEveryoneMayDoEverything` off — SWITCHED 2026-09-10, still needs a session

`MCF_Task_Permissions.m_bEveryoneMayDoEverything` is now `false`, with a note at
the field saying how to put it back. The role checks bite from the next
Workbench start onward; DESTROY was always asked properly regardless.

What is still owed is the *session*: the system has still never been watched
refusing anything, and a permission table that has only ever said yes is not
evidence of anything. Play, and note what gets refused that should not — roles
come from vanilla's command hierarchy, so solo you resolve high.

The Game Master half of that evening was watched and works: "Edit intel" and
"Edit device" no longer overlap, and a NOTEPAD keeps its shape through APPLY.

### 4. Dedicated server — MEASURED 2026-09-10. Packed to `.pak` — still open, and blocked

**The server half is answered, in both shapes.** A probe component naming a
class that does not exist was put on the test world's game mode entity, and a
probe *user action* naming a missing class on the phone prefab. One dedicated
server run, all six addons, `MCFTestworld.conf`:

```
WORLD (E): Unknown class 'MCF_Probe_MissingComponent' at offset 1832(0x728)
WORLD (E): Unknown class 'MCF_Probe_MissingAction'    at offset 6872(0x1ad8)
...
DEFAULT : Entered online game state.
SCRIPT  : [MCF] GameMode start -- resetting per-mission state
SCRIPT  : [MCF] intel action registered on 'Mobile phone'
```

One error line each, nothing else. The entity keeps its other components, the
phone keeps its real action, the game mode starts, the server goes online. **A
user action entry behaves exactly like a component entry** — that was listed as
unobserved in STRUCTURE §9 and is now observed. The manifest pattern holds on a
dedicated server. Both probes were reverted.

A second, unplanned measurement fell out of a run with `-addons MCF` alone: the
eleven `MenuPreset` entries whose layouts live in modules each cost one
`RESOURCES (E)` and the rest of the presets loaded normally. Core alone compiles
`5686x files; 11058x classes`; all six give `5774x; 11252x` on the server.

**Packing is blocked by the launcher, not by MCF.** `-buildData` never runs:
Steam's wrapper re-emits the command line and drops the output-directory
argument, so the Workbench opens its GUI and writes nothing.

```
CLI Params: -wbModule ResourceManager -buildData PC -wbProjectPath G:\MCF\addons\MCF_Dev\addon.gproj
                                                 ^ the out dir is simply gone
```

Tried and all equivalent: `wb_build_data` (run and start/poll), `mod build`,
and a hand-rolled `Start-Process` with both back- and forward-slash paths. The
same wrapper is what makes `wb_validate_scripts` fall back to a stub session.
What has not been tried: building through the Workbench GUI (Resource Manager /
publish), which is where the remaining answer probably is, and which needs a
human at the machine.

**Running the dedicated server, written down because it cost time twice:**

```powershell
Start-Process -FilePath 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Server\ArmaReforgerServer.exe' `
  -WorkingDirectory 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Server' `
  -ArgumentList '-server','{B8BD092E327C2224}Missions/MCFTestworld.conf',
                '-addonsDir','G:\MCF\addons',
                '-addons','MCF,MCF_Objectives,MCF_Ops,MCF_Dialogue,MCF_AI,MCF_Dev',
                '-profile','G:\MCF\server\profile','-maxFPS','60'
```

- **The working directory matters.** One of the server's addon dirs is the
  relative `./addons`, which is where the base game data addon (`58D0FB3206B6F859`,
  `ArmaReforger.gproj` — not a mod, and not the MCP tool) is found. Launch from
  anywhere else and it reports `Game addon '58D0FB3206B6F859' not found` /
  `Unable to initialize Enfusion`, which reads like a broken dependency in MCF
  and is nothing of the kind.
- **`-addons` must list every addon the scenario needs.** `MCFTestworld.conf`
  lives in MCF_Dev, so `-addons MCF` alone compiles fine and then dies with
  `Unable to initialize the game`.
- The game mode components are **not** loaded from `Prefabs/Systems/Milsim.et`
  in this scenario. The test world places them directly on its
  `GameMode_Editor_Full` entity in `MCFTestworld_Layers/default.layer`. A probe
  put in Milsim.et is therefore never loaded — check where the components
  actually live before concluding anything from a quiet log.

### 5. The phone finishing pass — DONE 2026-09-11

The handset is finished. What that took, and what is worth knowing:

**It is drawn, not previewed.** The 3D model behind the UI is gone. Rendering
it meant measuring where it landed, in a preview whose answer arrives in a
different unit than the widget it lands in, a frame or three later, sometimes
never — every fault this screen ever had came from that measurement. A chassis
PNG and two rectangles from one aspect ratio, recomputed every frame, are
correct on the first one. The laptop still uses the model; its screen is a quad
on a mesh and there is a real reason to project it.

**Its own art.** Eight app icons, a signal and a battery glyph, a rounded tile,
a circle, a wallpaper and the chassis, in
`addons/MCF_Ops/UI/images/MCF_Phone/`. PNG plus a hand-written `.edds.meta`
(`PNGResourceClass : TextureColorMap.conf`), then focus the Workbench and the
watcher builds the `.edds` against our GUID. No import dialog.

**One control.** The home bar. One tap steps back, two taps close, and on a
locked phone one tap opens the passcode pad. The three grey buttons under the
screen are gone — they were the last thing on it that said "game menu" out
loud. The bar must be a `ButtonWidget` above the chassis: an `ImageWidget`
never receives a click, and a widget underneath the drawn body is covered by
it. `style blank`, or the button paints a white block.

**Break-in is behind a keypad.** Typing `1337` starts the puzzle. A phone that
offers a BREAK IN button is a mod; a phone that has a passcode screen is a
phone.

**Unread, per client.** See DEVICE_CONTENT §10. The one thing to carry in your
head: read state is keyed on the object's **RplId**, not the profile id, or
three handsets sharing `smuggler_phone` share one read log.

**Authoring on the device.** Edit device opens the phone itself in author mode.
The draft has `m_sId` cleared, or the server writes it back into the shared
library and every handset in the mission changes at once.

**One app, one screen.** Messages is a bubble thread, mail has a header card,
contacts have a list and a card, calls is a dialler, photos is a grid. Notes,
files and settings keep the plain reader and should. DEVICE_CONTENT §11 has the
authoring conventions — in particular that `" - "` in a heading splits it into
who and when.

### 6. The laptop desktop — DONE 2026-09-11

The laptop is no longer a phone on a bigger screen. It is a KDE-shaped desktop
with a panel, a launcher, real draggable stacking windows, a lock screen, a file
manager with a folder tree, and three editors a file opens into and can be typed
into and saved from. DEVICE_CONTENT §12 is the design; this is what it cost.

**It reuses everything and reinvents nothing.** Same `MCF_Device_Profile`, same
`MCF_Device_Content` presenter, same rows where a row makes sense. The phone's
fixes — the linear colours, the `Background` child on a button, the always-visible
`Sizer` that declares a row's height, `"Horizontal Alignment"` — were all already
paid for and are simply used.

**The layout is generated.** `tools/generate_desktop_layout.py` writes
`MCF_IntelDesktop.layout`: ~19,000 lines, 2,630 GUIDs off the stem
`6A1C4F0B39E0xxxx`. `tools/generate_desktop_art.py` writes all 36 art pieces at
4× and downsamples. **Hand-editing either output and then regenerating loses the
edit silently.**

**Folders are paths in `m_sHeading`.** Nothing was added to the data. A trailing
`/` is an empty folder. `MCF_Device_Text` owns every split; nothing else may cut
a heading by hand.

**A file's extension is the whole rule** for which editor it opens in. Those
window slots are not persisted anywhere, unlike `MCF_EIntelApp`, so they are safe
to move.

**Thirteen windows, ten of them apps.** The launcher and the taskbar iterate
`APP_WINDOWS`, not `WINDOWS`, or they offer "Spreadsheet" with nothing in it.

**A player reads, a Game Master types.** Every editor carries a read-only pane
and an edit box in the same box and shows one, off `m_bAuthor`.

**`Draft()` now runs at `OnMenuOpen`, not at the first edit.** It swaps the
presenter's profile for a copy, so anything already holding an item from the old
one goes on editing a copy nobody will ever send. Making the draft before a
single window is filled means every item the shell hands around is a draft item.

**Dragging works the way §"Dragging a widget" describes** — there is no
`OnMouseMove` and no mouse capture, so a per-frame tick polls the mouse and a
transparent full-screen catcher takes the release.

Verified: `Module: Game`, 5778 files, 11341 classes, no errors. Not yet verified
in a live session with a second client.

### 7. The visual IS the editor — the pattern, started 2026-09-11

**This is how every new intel view gets authored from now on.** A Game Master
who rewrites a letter should be looking at the letter, with the writing
switched on. Not at a form with three fields that claims to be one.

The phone and the laptop already worked this way. The handwritten letter
(PAPER) and the field notebook (NOTEPAD) do now too, and the shape is meant to
be copied:

1. **The player's layout carries both halves.** Each read-only widget has an
   edit box in exactly the same box, shipped `"Is Visible" 0`. One of the pair
   is shown. An edit box that is always there lets a player rewrite the
   evidence they were sent to find.
2. **A tool column, not a toolbar.** It sits on the dim area beside the sheet,
   which is empty in both layouts, so it never fights the player's own row of
   buttons at the bottom.
3. **`tools/add_intel_author_chrome.py` writes that chrome**, one entry per
   visual with that visual's own anchors, and refuses to write a layout
   containing a duplicate widget name. Re-running it replaces the block rather
   than stacking a second one.
4. **The right-click action routes by view.** `MCF_Intel_EditContextAction`
   opens the visual where there is one and falls back to
   `MCF_Intel_EditorMenu` where there is not. DOCUMENT and MAP still use the
   form and should keep using it until each has a visual of its own — losing
   the only editor a view has is worse than an ugly one.
5. **Saving is the route that already exists.** The shell's `SendDraft()`
   sends the profile over `MCF_RequestWriteDeviceProfile`; the server writes
   it onto the object with `SetProfileFromServer` and the override wins over
   the prefab's own `m_aEntries`. No new server code was needed and none
   should be for the next view either.

Two things that are easy to get wrong and were:

- **Commit before anything moves.** The edit boxes are the only place typed
  text exists until `CommitPaperPage()` runs, so turning a page, adding one or
  saving without it throws away everything typed since the last turn.
- **A typed newline does not survive on its own.** `MCF_Device_Script.Clean()`
  turns a real newline into a space, so a body goes onto the draft through
  `MCF_Device_Text.Encode()` and comes back through `Body()`.

**Typing on the visual is a solved problem now** -- see "Typing on a device:
the pattern to copy" under the rules below. It is the same recipe for every
device that gets an author half, and it is the part that took the longest to
get right, so read it before building the next one.

Still on the form: DOCUMENT and MAP. The obvious next one is DOCUMENT, which
already has a visual in the viewer and needs only the author half.

### Parked: the laptop desktop

Basically functional at the user's call on 2026-09-11 — it opens, drags,
stacks, browses folders, edits and saves files — and wants an overhaul pass
before it is finished. Not blocking anything.

### 8. The namespace rename, with a fresh head

`MCF_Devices_` → `MCF_Lock_`, so it stops differing from `MCF_Device_` by one
letter while meaning something else. And decide what to do about `MCF_AI_` and
`MCF_Interact_` spanning two addons each. Mechanical across roughly sixty files —
which is exactly why it should not be done at the end of a long session.

### Deliberately not next

**The phone.** Parked at the user's request on 2026-09-11: "dat zijn de functies
zoals ik ze wou hebben". Its layouts are still in sRGB rather than linear, which
is cosmetic and deliberately deferred.

**The laptop's 3D side.** The desktop UI is done; the preview framing and the
flat-colour materials on the model are still parked mid-tuning. They block
nothing.

**MAP intel, GROUP tasks, the restrained pose.** These are new features, not
finishing what exists.

---

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

With every addon loaded a clean compile reads
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
collision surface, which does not resolve. Deleting the `SurfaceProperties`
block from the `.xob.meta` is only half of it — the GUID is also baked into the
built `.xob`, which has to be rebuilt afterwards (rewrite the FBX, focus the
Workbench). Measured 2026-09-10: with the meta clean, the rebuild does not
re-inject it.

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
never called back in any test here; and there is no API to build a texture from
bytes (`ScreenshotTextureData` is an engine pointer with no constructor). Treat
FILE as untrustworthy rather than proven dead -- the probe that condemned it had
the same callback fault described below, so it may never have been given a fair
hearing.

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

**RestApi: four ways to be silent, and only one of them is an error.** Getting
the chain above to actually fire took most of an afternoon on 2026-09-10, and
every fault presented identically -- the request goes out and nothing ever comes
back, no success, no error, not even the timeout set on the context. Written out
because none of it is guessable and all of it is cheap once known:

1. **Hold the object that owns the handlers, not just the callback.** The docs
   say "If callback is not stored as ref then it will be deleted after its
   execution finishes", which is true and is half of it. `SetOnSuccess(OnSuccess)`
   binds a METHOD, so the object that method lives on must survive too. Hold the
   `RestCallback` alone and it lives on with nothing left to call: the request
   completes, the server answers 200, and script hears nothing.
2. **The handler prototype is `RestCallbackFunc` -- one argument, the callback
   itself.** The body is asked for afterwards with `GetData()`. The compiler
   names this if you get it wrong: a handler shaped like the obsolete virtual
   (`string data, int dataSize`) is "too many arguments". `SetOnTimeout` does not
   exist, so a request that dies quietly dies quietly -- keep a deadline of your
   own in the UI.
3. **The wiki's REST API Usage page is older than the engine.** It shows a
   `RestCallback` subclass overriding `OnSuccess`/`OnError`/`OnTimeout`; those
   virtuals compile with "'OnSuccess' is obsolete: Use
   RestCallback.SetOnSuccess() instead". Its `GetContext("")` returns nothing at
   all. Trust the compiler over the page.
4. **A `RestContext` cannot be held as a ref** -- "Method '~RestContext' is
   private". The engine owns it, so a local is correct and there is no lifetime
   to manage.

`GetContext("https://host")` with the path passed to `GET` works, and so do the
two other splittings; the address shape was never the problem in any of this.

**Trim every authored string, especially urls.** A url pasted with a leading
space is still a url to a person and is not one to RestApi: the request answers
`http 0`, which reads as a network failure rather than as a typing mistake.
`MCF_Device_Script.Clean` trims on the way in and `MCF_Device_Item.ImageUrl`
trims on the way out, because data written before the fix is already in the
store.

**`Substring` truncated a 35 611 character string to about 8 190.** Silently.
The decoder was cutting the payload out of the response with `Substring` and
decoding the copy; the result was a truncated jpeg that still began with the
right signature, passed validation, reached disk, and was refused by the texture
loader with no message. Prefer passing the original string with a pair of
indices over copying a large one out of it.

**A `LayoutSlot` with `HorizontalAlign 3` fills its parent's width and ignores
`SetSize`.** `SizeToContent 1` does not stop it. Use `HorizontalAlign 0` for
anything sized in script. This hid on the phone for a while because the column
there is capped to roughly the width the image was being set to, so the stretch
was invisible -- on a wide column it is immediate.

## An opening laptop needs no rig, no bones and no animation clip

This was researched after I claimed twice, wrongly, that the laptop model was a
single closed mesh. It is not, and the way Enfusion animates a lid is far
cheaper than either of us assumed.

### What is actually in the model

There are two laptop FBX files and they are not the same object.

  art/Devices/Laptop/Laptop.fbx          2 nodes, 122 meshes merged into one,
                                         no hinge, no animation
  art/Devices/Laptop/source/             159 nodes, 122 geometries,
    laptop_leather.fbx                   AnimationStack "Take 001"

The source is the Sketchfab download. Its animation drives exactly one node,
pCylinder3, and only one channel of it moves: Lcl Rotation Y, from -97 degrees
to +20 degrees over five seconds. Translation, scaling and visibility are
constant across all three keys. pCylinder3 is the hinge; pCube4 (lid panel),
pCube134 (screen) and pCylinder2 are parented to it, so that single rotation
swings the whole lid. Hinge rest transform: T = (1.654, 0.192, -1.15),
R = (0, 20, 90), S = (0.075, 0.15, 0.075).

The file in art/ is a flattened export of that. Whoever exported it merged the
hierarchy away. Read the source, not the export.

### How BI opens a lid

Not with an animation graph. Vanilla doors are ordinary entities that the
engine rotates bodily, and a lid is just a door lying on its back.

Prefabs/Structures/BuildingParts/Doors/Door_Base.et carries DoorComponent with
`DoorAnimationType WholeEntity`, and every concrete door under it supplies only
a mesh and some numbers:

    DoorComponent {
      OpenTime     2
      AngleRange   -90         // degrees of travel
      ClosedAngle  0
      InitialAngle 0
      TestContacts 1
      TestCollider "UBX_..."   // refuses to open into something
      DoorAction SCR_DoorUserAction { ... }
    }

The leaf mesh is a plain .xob with its origin on the hinge. There is no
skeleton in it and no clip anywhere.

The pattern for a lid on a body -- exactly our case -- is
HatchDoor_Roof_01/HatchSet_Roof_01_EXT_COV_B.et: a parent entity holding the
frame mesh, with the hatch nested inside it as a child entity that carries
DoorComponent and

    Hierarchy { PivotID "socket_HatchDoor" }

The socket is a named point in the PARENT mesh. It supplies the child's
position and orientation, which is how the same component that swings a house
door around a vertical axis tips a roof hatch onto a horizontal one: the
rotation is about the child entity's own local up axis, and the socket is what
decides where that axis points. (Read from the prefabs, not measured -- confirm
with a 90 degree test swing before trusting the sign.)

### So the export has to produce

  Laptop_Body.xob   everything except pCylinder3 and its children.
                    Origin at the laptop's own base. Carries a socket point
                    on the hinge line, oriented so its up axis runs along the
                    hinge -- name it socket_LaptopLid.

  Laptop_Lid.xob    pCube4, pCube134, pCylinder2. Origin ON THE HINGE, at
                    pCylinder3's position, in the closed pose.

  UBX_Laptop_Lid    a collider on the lid if we want TestContacts.

Then the prefab is body + nested child with DoorComponent, AngleRange 117
(the model's own -97 to +20), OpenTime around 1, and PivotID socket_LaptopLid.

Also still to do in Blender, unrelated to the hinge: drop directionalLight1
and the second Maya light, and rename the six default materials.

### The caveat worth knowing before building it

Doors are static world entities. Our laptop is an inventory item -- Item_Base
with InventoryItemComponent -- and a child entity with DoorComponent on
something that can be picked up and stuffed in a backpack is not a
configuration vanilla ever ships. As an intel prop placed on a table by the
Game Master it is fine. If it also has to be carryable, expect the lid child to
need attention on pick-up, and test that before assuming.

## Enfusion facts learned building the laptop, all of them the hard way

Every one of these cost at least one wrong turn. They are written down because
none of them is discoverable from the published documentation, which is
generated from 1.1.0.42 while this machine runs 1.8.0.13.

### Blender's FBX exporter does not bake the axis change into the vertices

It leaves vertex data in Blender's own axes and writes `Lcl Rotation -90 0 0`
on every node, so the engine applies `(x,y,z) -> (x, z, -y)` at import. The
consequence that costs time: **a local axis is not the same axis on both
sides.** Blender local Z is what arrives as engine Y. Reasoning "local axes
survive a global conversion" is wrong and produced four consecutive wrong
answers here.

The thing that finally worked is checked in as
`art/Devices/Laptop/engine_preview.py`: it reads the exported .fbx back,
converts the vertices the way Enfusion does, applies the prefab's child
transform and a given door angle, and renders it. Use it for any orientation
question. Rendering what the engine would build was right first time; every
attempt to reason about it was not.

### An opening lid needs no rig, no bones and no animation clip

`DoorComponent` with `DoorAnimationType WholeEntity` rotates the entity bodily
about its own **local Y**. The proof that it is local Y: a plain door's hinge
is vertical and its entity is unrotated. So the leaf mesh is an ordinary .xob
with its origin on the hinge, and there is no skeleton in it.

A lid on a body is `HatchSet_Roof_01`: parent holds the frame mesh, the lid is
a nested child entity carrying `DoorComponent`. Vanilla positions that child
with `Hierarchy { PivotID "socket_..." }`, but an FBX null exported from
Blender did **not** become a point the engine could look up by name -- the lid
pivoted about the parent's origin instead. Explicit `coords` and `angles` on
the child work and are easier to reason about.

`angles` roll is the opposite sign from right-handed maths. Reforger is
left-handed; expect to flip it.

### A component GUID that does not match the inherited one ADDS a component

It does not override it. Five GUIDs each one character off `Item_Base` gave the
laptop two `InventoryItemComponent`s, two `ActionsManagerComponent`s and two
`Hierarchy`s, and the editor died on a null inside
`SCR_PlacingEditorComponent.CreateEntityServer`. Copy the GUIDs from a working
sibling prefab; never type them from memory.

### The layout parser: bad property survivable, bad slot class fatal

An unknown **property** name is reported and skipped -- the device layouts
carried eleven of them for weeks with no ill effect. An unknown or wrong
**slot class** derails the parser: it misreads the next few lines as
properties, and at widget-creation time everything downstream is attached to
the wrong parent. In this case the rest of the tree landed in a
`SizeLayoutWidget` that accepts one child, and every button in the layout
vanished at once -- including five that had worked for weeks.

`SizeToContent` inside an `AlignableSlot` was the culprit. If widgets go
missing wholesale, map the `GUI (E) ... at offset N` values back to line
numbers before assuming the newest change is at fault.

### GetButtonText does not traverse hidden subtrees; FindAnyWidget does

`SCR_ButtonTextComponent.GetButtonText` will not find a button inside a
subtree the layout marks `"Is Visible" 0`. Ship such a panel visible and hide
it from script after binding. `FindAnyWidget` has no such problem, which is
why the photo overlay never showed this and the lock screen did.

### Importing an FBX

- The workbench reads `<name>.xob.meta`, **not** `<name>.fbx.meta`. A
  hand-written `.fbx.meta` is silently ignored.
- It assigns its own GUID on import and does not honour a pre-written one, so
  wire prefabs **after** the import, not before.
- A metafile without a corresponding `.xob` is treated as garbage and the FBX
  is skipped entirely.
- The sequence that works: a `.xob.meta` must exist (an empty one from
  `wb_resources register` is enough), then the FBX has to be written again,
  then the workbench window needs focus -- its watcher only rebuilds on focus.
- **A material name shared with an already-imported mesh makes the second
  import parse as empty**: no MeshParams, no GeometryParams, one line of log
  and no reason. The lid shared `Laptop_Leather` with the body and would not
  build until its materials were renamed.
- `.emat` syntax, read out of the shipped materials rather than guessed:
  `Color` is an RGBA tint that works with no map at all, alongside
  `RoughnessScale`, `MetalnessScale`, `BCRMap` and `NMOMap`.
- The importer injects `{536BF67B2052B869}material/metal.gamemat` into every
  collider it builds. That GUID resolves to nothing in this install and costs
  an error line per mesh per load. Strip the `SurfaceProperties` block from the
  `.xob.meta` **and then rebuild the mesh** — rewrite the `.fbx` and focus the
  Workbench — because the GUID is baked into the `.xob` too and a clean meta
  beside a stale `.xob` looks exactly like a fix that did not take. With the
  meta clean the rebuild comes out clean; it does not come back.

### A SizeLayoutWidget's width override cannot be set from script

There is no `SetWidthOverride`. A scroll pane wrapped in a `SizeLayoutWidget`
with `AllowWidthOverride 1` / `WidthOverride 200` is 200 units wide forever, in
a glass more than twice that — which is what made every list on the phone use
less than half the screen for weeks.

The fix is in the layout, not in script: `AllowWidthOverride 0` plus
`HorizontalAlign 3` on the size layout's own slot, so it stretches to the
scroll's viewport instead of imposing a number.

And the other half of the same lesson, already paid for once: a
`SizeLayoutWidget` **scales its content**, it does not reserve height. A row
built inside one came out with letters an inch tall. Row height comes from the
tallest child plus the overlay's padding, the way every other row here gets one.

### A PowerShell cmdlet that writes by replacing the file can report the new text and change nothing

`Update-MatchInFile` prints a green diff of the result and then fails with
`Unable to move the replacement file to the file to be replaced` — the preview
is of what it intended, not of what landed. The file is untouched and a second
`Select-String` is the only thing that says so.

This is the same trap as `device_commit_files` reporting success on an unchanged
file, in a different tool. **Read the file back and assert on the new text, in
the same call that wrote it.** Every patch script in this session ends with a
readback that raises if the new string is absent; do that.

When the replace-and-move route fails, `[IO.File]::ReadAllText` /
`WriteAllText` with a `UTF8Encoding($false)` writes in place and works.

### Centring text: the property is `"Horizontal Alignment"` and its value is a word

```
"Horizontal Alignment" Center      // Left | Center | Right
"Vertical Alignment" Center        // Top  | Center | Bottom
```

Two wrong guesses came first and both cost a build:

- **`Alignment 0.5 0`** is a *FrameWidgetSlot* property -- the pivot -- and the
  engine ignores it outright whenever a slot's min and max anchors differ,
  which is every stretched box in this project. The handset's app-tile labels
  carry it and have never been centred; the boxes are narrow enough that nobody
  could tell.
- **`"Text Horizontal Align"`** does not exist. The string appears nowhere in
  the engine binary. An unknown key in a layout is dropped in silence, so it
  validates, loads, and does nothing. `MCF_PlanningBoard` had carried it for
  months.

At runtime there is no `SetTextAlign`. Alignment is a widget flag:
`w.SetFlags(WidgetFlags.CENTER)` / `ClearFlags`, with `RALIGN` for right and
`VCENTER` for vertical. There is no `HCENTER`.

And the related rule, straight out of the engine's own warning string
(*"Position/Size works only when min and max anchor is the same in given
direction"*): a FrameWidgetSlot whose anchors differ **does** stretch the widget
across the span, and `PositionX/SizeX/Alignment` are ignored in that direction.
So a text that looks left-aligned in a wide box is not a narrow box -- it is a
full-width box with no alignment flag.

### A layout's `Color` is linear, not sRGB

Write the values a designer works in and every flat fill comes out pale. The
desktop's window bodies were authored as #282D32 and rendered mid-grey; the
accent as #C26314 and rendered a washed tangerine. Convert before writing:

```
c <= 0.04045 ?  c / 12.92  :  ((c + 0.055) / 1.055) ^ 2.4
```

Alpha is not a colour and passes through. **Textures are not affected** -- the
importer handles their colour space -- which is why the panel gradient and the
wallpaper always looked right while everything drawn as a tinted white tile did
not. `tools/generate_desktop_layout.py` converts in `col()`; the handset's
layouts are still written in sRGB and are mildly pale where they use a flat
fill, which is cosmetic and not worth disturbing a screen that is signed off.
### A button's fill must be a child called `Background`

`SCR_ButtonTextComponent` tints the widget named `Background` inside its own
subtree with `m_BackgroundDefault` / `Hovered` / `Selected`. That is the whole
mechanism -- there is no styling anywhere else. An image under any other name is
never tinted and paints at full white, which is how every filled button on the
desktop's first build came out as a white block.

Two consequences worth knowing before drawing one:

- A transparent button (`m_BackgroundDefault 1 1 1 0`) must NOT contain the
  texture it wants always visible, because the component will tint it away. A
  window's title-bar gradient therefore sits behind the bar as a sibling, not
  inside it.
- Duplicated `Background` names across one layout file are fine. The lookup is
  relative to the button, and the row layout has done it for months.

### A rounded tile stretched wide becomes an oval

`tile_rounded` is 64 pixels square with a radius to match. Stretched across a
270-unit password field its corners become half-circles, which is why that
field came out as a grey pill. `tile_window` is 256 with a proportionally
smaller radius and survives the same stretch. Rule of thumb: the small tile for
anything roughly square, the big one for anything wider than about 150 units.

### Dragging a widget: there is no `OnMouseMove`

`ScriptedWidgetEventHandler` offers `OnMouseButtonDown`, `OnMouseButtonUp`,
`OnClick`, `OnDoubleClick`, the enter/leave pair, `OnMouseWheel` and a per-frame
`OnUpdate`. There is no per-move callback and no mouse capture --
`WidgetManager` has `GetWidgetUnderCursor` but nothing that sets it.

The shipped game has the same problem and its map ruler solves it this way,
which is what MCF_Desktop_ShellMenu copies:

1. The press sets a flag and records where the pointer and the widget were.
2. A per-frame tick polls `WidgetManager.GetMousePos(x, y)` and moves the widget
   with `FrameSlot.SetPos`.
3. The release clears the flag.

Two traps go with it:

- **`GetMousePos` answers in physical pixels; `FrameSlot` wants reference
  units.** `workspace.DPIUnscale()` sits between them. Same mismatch as the
  handset's 3D preview, in a place where it instead makes a window fly away
  from the cursor at anything but 100% DPI.
- **The release does not come back to the widget that was pressed.** By the
  time the button comes up the cursor is over whatever the window was dragged
  across. A transparent full-screen button, shown only for the length of the
  drag, catches it.

Stacking needs none of this: `Widget.SetZOrder(int)` exists, higher is in front,
and no re-parenting trick is required.

### `SetZOrder` does not make a screen cover another screen

The desktop's lock screen is declared after the panel and the desktop icons AND
given a higher Z order than either, and on the first build both still drew
straight through it. Whatever the ordering rules are between a FrameWidget's
children, they are not "highest Z wins" in the way a CSS stacking context is.

So a screen that must cover another one hides it rather than covering it. That
is also the more honest model: a locked machine has no taskbar and no shortcuts,
it has the way in and nothing else.

### `Size` on an OverlayWidget is ignored; on an ImageWidget it works

Two lines of text in a phone row overlapped, and the obvious fix — set the row's
height — did nothing, because the row is an OverlayWidget and `Size` on one is
not read. The second attempt made the row's avatar transparent so it would still
declare the height; that failed too, and the failure is the more useful fact:

**A hidden widget contributes nothing to a layout.** Hiding the avatar collapsed
the row it was supposed to be holding open. Transparent is not hidden — but the
avatar was being hidden, not tinted, and the row went with it.

What works is a **`Sizer`**: an ImageWidget that is always visible, `Color 1 1 1 0`
so it draws nothing, with an explicit `Size`. It declares the row's height and
costs one widget. `MCF_PhoneRow.layout` and `MCF_DesktopRow.layout` both carry
one, and any new row layout should.

### The config parser does not unescape `\n`

`m_sBody "line one\nline two"` in a `.conf` arrives in script as the six
characters `l i n e ... \ n ...` — a literal backslash and a literal n, printed
on screen exactly like that. The parser stores the bytes and does nothing else
to them.

So the unescape happens at display time, in `MCF_Device_Text.Body()`, and every
place that puts a body on screen goes through it. Doing it at load time instead
would mean touching the presenter, the draft path and the replication payload;
doing it at display time is one function and one call site per pane.

### An EditBoxWidget in a generated layout

It needs its own `EditBoxWidgetClass` with an `EditBoxFilterComponent` and
`style blank` — **not** an override of `SCR_EditBoxComponent`, which drags in
chrome the layout does not have and paints over what is behind it. `SetText`
does not reach one through `TextWidget.Cast` or `RichTextWidget.Cast`, so any
shared text helper has to try `EditBoxWidget.Cast` as well; ours does, which is
why the same `SetText(root, name, value)` fills a read-only pane and an edit box
without the caller knowing which it hit.

### Two widgets with one name: the most expensive silent fault a layout has

`FindAnyWidget` and `SCR_ButtonTextComponent.GetButtonText` return **whichever
match they reach first**. A second widget with the same name does not warn, does
not fail and does not log — it quietly steals every lookup for that name, and
the symptom shows up somewhere else entirely.

It cost the desktop's editors two things at once:

- **SAVE did nothing.** The pane's heading strip carried the Game Master's
  toolbar, whose fourth button is `<W>Save`; the editors called their own SAVE
  the same. The handler went onto the toolbar's tick, which `ShowToolbar` hides
  for a window with no app kind, so the visible button had no handler at all.
- **The body was blank.** A window's pane container is `<W>Body`, and the text
  and document editors called their read-only rich text the same, so every
  `SetText(root, p + "Body", …)` wrote into a FrameWidget and vanished.

Both looked like logic bugs and neither was.

**`generate_desktop_layout.py` now refuses to write a layout containing a
duplicate name**, `Background` excepted — that one is mandatory (it is the only
child `SCR_ButtonTextComponent` tints) and is only ever reached through its own
button. Any generator that writes a layout should carry the same assertion; a
hand-written layout should be swept for it whenever a lookup returns something
surprising.

### A widget's text colour defaults to white, everywhere

`TextWidgetClass`, `RichTextWidgetClass` and `EditBoxWidgetClass` all draw white
when no `Color` is given, and a generator that treats `colour` as optional will
happily put white on a white page. The spreadsheet's seventy-eight cells and the
document editor's two edit boxes all did exactly that and read as empty.

So: **every widget drawn on a light surface must be given an explicit `Color`**,
and a helper that can be placed on one needs the parameter — `editbox(colour=)`
and `flatbtn(ink=)` exist for that reason. On a dark surface the default happens
to be right, which is what makes the omission so easy to miss.

### Typing on a device: the pattern to copy

**This is settled. Use it for every device that gets an author half — the
laptop's editors, and anything new.** It took most of a day and six wrong
turns; none of it needs repeating.

#### Read the engine, do not guess

`scripts/Core/generated/UI/` in the game data has one file per widget class,
and `game_read` reaches them straight out of the .pak. That folder answers
more in a minute than an afternoon of reading the binary.

```
sealed class EditBoxWidget: UIWidget           // GetText, SetText, ActivateWriteMode, IsInWriteMode
sealed class MultilineEditBoxWidget: TextWidget    // ActivateWriteMode, IsInWriteMode
```

#### The four facts everything else follows from

1. **They share no parent.** `EditBoxWidget.Cast` on a multiline box returns
   null. The engine's own SCR_EditBoxComponent carries a field for each and
   says so: *"Why aren't these derived from a common parent :("*. Every read
   and write tries both — `BoxText` / `SetBoxText`.
2. **A multiline box does not wrap unless told.** TextWidget's own doc
   comment: *"Automatic wrapping is turned on by the WRAP_TEXT flag."* So
   `SetTextWrapping(true)`, or a typed line runs off to the right forever.
3. **Return does not insert a newline.** It arrives as `OnChar(w, 13)` on a
   `ScriptedWidgetEventHandler` attached to the widget. Measured:

   ```
   [MCF] text input: control character 13
   [MCF] return pressed: had 183 chars, wrote 184, box now reports 184
   ```

   So reading and writing work, even mid-edit. Returning **true** takes
   Return away from the widget, which otherwise uses it to leave write mode.
4. **There is no caret API and no way to send a keystroke.** Nothing in the
   generated classes reports or moves the caret; `WidgetManager` has
   `ReportMouse` and nothing for keys, so triggering a Right-arrow to collapse
   a selection — the obvious fix — cannot be done from script.

#### The recipe

- **Layout**: `MultilineEditBoxWidgetClass`, `style blank`, its own
  `SCR_EventHandlerComponent`, an explicit `Color` (every widget draws white
  by default, and half these sit on white paper), and **no**
  `EditBoxFilterComponent` — the engine says it does not work on multiline.
  Never an override of `SCR_EditBoxComponent`.
- **Wrapping and Return**: `MCF_Device_TextInput.Attach(box)`, and **hold the
  handler in a field**. `Widget.AddHandler` attaches it but script owns it;
  one nobody keeps is collected and Return stops working minutes later, as
  what looks like a different bug.
- **Getting in**: a TYPE button that does `SetFocusedWidget` then
  `ActivateWriteMode()`. A box takes keystrokes only in write mode and there
  is no event for entering it — vanilla polls `IsInWriteMode()` and activates
  from its own pencil button.
- **Return itself**: write `text + "\n"`, then drop the focus and take it back
  (`SetFocusedWidget(null)`, then the box). That is the nearest thing to a
  click script can do, and a clicked box enters write mode *without* selecting
  everything. `ActivateWriteMode()` is the fallback for when that does not
  start write mode at all: a selection is bad, not typing is worse.
- **Reading it back**: **not with `GetText` at save time.** By then a click has
  moved the focus and the box's answer is unreliable. The handler watches
  `OnChange`, which fires per character typed and per character deleted, so it
  knows the text while it is being written. Seed it whenever the box is filled
  programmatically — `SetText` raises no `OnChange`.
- **Newlines on the wire**: `MCF_Device_Text.Encode()` on the way in and
  `Body()` on the way out. `MCF_Device_Script.Clean()` turns a real newline
  into a space, so a typed line break would otherwise arrive on the next
  client as one paragraph.
- **Saving**: at every moment the author stops typing — page turn, page view,
  add, remove, menu close — and never per keystroke, because each one is an
  RPC carrying the whole profile. Keep the explicit button too: it is the only
  thing that reports back.

#### The glitch that is accepted

Return leaves the text selected in some cases. Accepted on 2026-09-11: you can
write a whole letter with it. It is one place to fix if Bohemia ever finishes
the widget — nothing else in MCF depends on the workaround.

#### What a line-at-a-time input taught, and why it is gone

An input that took one line at a time sidestepped the caret completely and was
rejected on sight: *"dat wil ik niet, wil gewoon heel de inhoud universeel
kunnen aanpassen en editen"*. An author edits text, they do not feed it in.
Do not bring it back.

#### Still to port

The laptop's text and document editors have the wrapping and the casts but
not the Return handling or the save-as-you-go. The laptop is parked; this
goes in with its overhaul pass.

**A handler must be kept alive by the caller.** Said twice on purpose.
`Widget.AddHandler` attaches it, script owns it, and one nobody holds is
collected — after which the widget quietly goes back to its old behaviour
some minutes later.

## A live map on a board, and the four wrong answers on the way there

`MCF_Map_BoardComponent` puts the world's real map on a panel in the world.
It works. The detail is in `docs/research/map-board-and-drawing.md`; this is
what a future session needs to not repeat.

**A blank board that is the RIGHT blue is not a broken render target.** The
map widget's `ClearColor` is `0.173 0.344 0.62` linear, which arrives on
screen as a pale steel blue. Seeing that colour on the panel proves the whole
chain -- widget tree, `SetRenderTarget`, the material's `$rendertarget` --
already works and only the content is missing. Check the colour before
blaming the plumbing.

**An `RTTextureWidget` on the screen is a hole.** The obvious diagnostic --
hang the board's widget tree on the workspace and see whether the map draws
-- cannot answer anything, because the thing in that tree sends its subtree
to the entity's mesh instead of to the screen. Any such test needs the same
`MapWidget` with NO render target around it.

**The character camera is innocent.** `OpenMap` switches its render off and
that really is an optimisation: in Game Master the map and the world are
visibly drawn at the same time.

**"Open" and "being drawn" are two different things**, and this is the whole
feature. `m_bIsOpen`, the widget it panned last, the modules, the character
camera -- bookkeeping, and it belongs to the player. Drawing is three native
calls on the entity: `EnableVisualisation(true)`, `SetFrame(worldRect)` and
the layer. They are the last line of `OnMapOpen` and the second-to-last of
`OnMapClose`, none of them looks at `m_bIsOpen`, and a `MapWidget` draws
whenever they are set.

**A board that holds the map open breaks M.** There is one `SCR_MapEntity`,
and while a board holds it open whatever decides what M should do has already
been told a map is open. Handing it back does not work either: `CloseMap`
calls `EnableVisualisation(false)` and the board goes blank.

**`SCR_MapEntity` is the singleton; `MapEntity` is not.** The class it
inherits is a plain `GenericEntity` carrying the entire map. Each board now
spawns one of its own (`MCF_Map_BoardEntity`), borrows the real map's
configuration and numbers once during a setup open, and drives its own from
then on -- so the board's zoom and pan are the board's, replicated between
everyone looking at it, and nobody's personal map is touched in either
direction.

**Zoom, pan, layer and grid live on the entity, not on the widget.** A board
that does not re-state them inherits whatever a player left behind. Re-state
them; do NOT set the board up again to fix it -- the setup open is itself an
open map, and a board that cannot tell its own open from a player's will
prime itself forever, once a second, which also breaks M. The test is
`GetMapWidget() == m_wMapWidget`.

**Markers are widgets, not map items.** `SCR_MapMarkersUI` is a map UI
component, and components only update while `IsOpen()`. So markers on a board
are not a setting -- they are a piece of work, and they conflict with the
board not holding the map.

### Where the map board was parked, 2026-09-11

It works, and it is not finished. "Basis maar niet goed" is the user's own
verdict and it is the right one.

**Settled, do not re-litigate:**

- The render target, the material and the mesh are all fine. A board showing
  the map is proof of the whole chain.
- A board must NOT hold the map open. One map exists; holding it breaks M.
- A board must NOT set itself up again to recover -- it cannot tell its own
  setup open from a player's without `GetMapWidget() == m_wMapWidget`, and
  without that test it primes forever, once a second.
- A second `MapEntity` does not give an independent view. Measured: it
  writes into the same native state and `InitializeLayers` on it destroys the
  real map's layers, which shows up as the PLAYER'S map losing its terrain.

**The arrangement that stands:** the board drives the shared entity, re-states
its own four numbers (`EnableVisualisation`, layer, `ZoomChange`,
`PosChange`, `SetFrame`) four times a second, and touches nothing at all
while `IsOpen()`. Its view is two replicated numbers on its own component --
zoom step and centre -- so everyone at that board sees the same thing and no
personal map is affected.

**The pan formula, now measured rather than guessed.** `PosChange` takes the
map's TOP-LEFT CORNER in the widget, not the screen position of the point
being centred. The setup open logs both:

```
primed zoom 0.683594 pan <162, 0, 0> | computed zoom 0.683594 pan <350, 350, 0>
```

Widget 1024 x 700, island 4096 m, fitted zoom 0.170898 px/m, so the island
draws 700 x 700 and centred sits (1024 - 700) / 2 = 162 from the left and 0
from the top. So `pan = half the widget - the centred point in map pixels`.
The zoom ratio was right from the start; only the pan was wrong. Fixed, but
NOT yet seen in game -- that is the first thing to check tomorrow, and the
log line is still there to check it with.

**Still open, roughly in the order they matter:**

1. Confirm the corrected pan: zoom in and centre should now land where they
   say. Then decide whether the log line stays.
2. The board follows a player's map for the seconds it is open. Unavoidable
   with one entity; possibly worth hiding behind the white fade instead of
   showing their view.
3. Markers are `SCR_MapMarkersUI`, a map UI component, and components only
   update while `IsOpen()`. Markers on a board are a piece of work, not a
   setting, and they pull against the board not holding the map.
4. More frames: paper map on a table, whiteboard, beamer board.
5. Freeform drawing on the in-game map, then channel permissions.
6. Unique maps with their own stored information -- a found enemy map.

### The map board, finished enough to build on -- 2026-09-12

Supersedes the parked note above. The board shows the world's real map on a
panel, with the grid and with the markers players place on their own maps,
at a zoom and position of its own that everybody at the board shares.

**Four things were learned the hard way and must not be re-litigated:**

1. A board must not hold the map open. One map exists; holding it breaks M.
2. A second `MapEntity` does not help. It writes into the same native state
   and `InitializeLayers` on it destroys the real map's layers -- which shows
   up as the PLAYER'S map losing its terrain.
3. `CanvasWidgetBase.SetZoom` / `SetOffsetPx` are the canvas transform, NOT
   what the engine renders the map at. The picture follows the entity through
   `ZoomChange` and `PosChange`. One entity, one rendered view.
4. **But the map entity is per client**, and that is what makes the feature
   work. A player opening their map takes it over on their machine only. The
   only screen a board can be wrong on is the screen of somebody reading a
   map instead of looking at the board -- so on that one screen the board
   goes white until they close it.

**The shape that works:**

- The board opens the map ONCE at birth to have `SCR_MapEntity` do the setup
  nobody should retype (layers, props, descriptors, `SizeInUnits`, zoom
  bounds), reads the island's frame off it, and closes it again.
- Its view is two replicated numbers on its own component -- zoom step and
  centre -- turned into zoom, pan, layer and frame by `ComputeView` and
  re-stated four times a second.
- Nothing is written to the entity while a foreign map is open.
- **Pan is the map's TOP-LEFT CORNER in the widget**, not the screen position
  of the point being centred: `pan = half the widget - the centred point in
  map pixels`. Measured, not guessed.

**The overlay is the important part for everything still to come.** A
`MapWidget` is a `CanvasWidget`, so the board's tree carries its own
`"Overlay"` canvas between the map and the whiteout, and everything the
engine does not draw for us is an array of draw commands on it: markers
today, freeform strokes next.

- The command array must be a FIELD -- "the callee takes just a pointer".
- `CreateCommandFromImageSet` returns a command flagged `STRETCH` only, so
  **add `WidgetFlags.BLEND` or the icon's transparency is drawn black**.
- `CANVAS_COMMAND_VERTICES_LIMIT` is 400, so a long stroke is several
  commands.
- Marker data is readable at any time from `SCR_MapMarkerManagerComponent`'s
  static instance; read BOTH the static and the disabled arrays. What a
  client holds is already filtered to what that player may see, so the board
  needs no permission code of its own.
