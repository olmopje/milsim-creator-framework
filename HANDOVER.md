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
| The addons | `G:\MCF\addons\` — `MCF` (Core), `MCF_Ops`, `MCF_Dialogue`, `MCF_AI`, `MCF_Objectives`, `MCF_Dev`. Six. |
| Workbench project | open **`addons/MCF_Dev/addon.gproj`** — the only one that depends on all five others, and it holds the test world |
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
| `docs/architecture/STRUCTURE.md` | **The structure of record.** What the six addons are, the measured dependency graph, which namespace lives where, and the Core rule. If anything else disagrees with it, it is stale. |
| `docs/architecture/PROJECT_STATUS.md` | The chronological record, ~1400 lines. Every session including the dead ends. **The primary source for how we got here.** |
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

### Proven by the self test, 2026-09-10

All four of these had been "believed correct, never watched" for weeks. They are
now watched, automatically, on two peers on two factions:

    SELFTEST start -- 2 player(s): 2 (USSR), 3 (US)
    faction-intel:  PASS -- player 2 (USSR) holds its own record and not the other's
    line-audience:  PASS -- player 3 (US) filtered out a line for USSR
    device-profile: PASS -- player 2 sees the profile the server wrote
    picture-fetch:  PASS -- player 3 has the picture on disk
    SELFTEST done -- 8 passed, 0 failed

- **Faction-scoped intel**, in both directions.
- **The audience filter** on text lines, shown to the faction addressed and
  filtered out by the other.
- **Device profiles over RplProp**, seen by clients that are not the host.
- **Per-client picture fetching**, two clients fetching the same url
  independently -- the shared pending map does not get in their way.

### How to run it

Put `MCF_Dev_SelfTestComponent` on the game mode, tick `m_bEnabled`, start two
peers **on different factions**, and read the log. Nothing to click. It is in
MCF_Dev and never ships.

WAIT FOR FACTIONS, NOT FOR PLAYERS -- that is the trap it was built around. The
first run started as soon as two players existed, which was before either had
picked a side, and every filter check then compared nothing against nothing.
The component now waits for people who are actually on a team, and says so if
they never arrive.

The run writes one intel record per faction, and deletes them when it finishes.

### Still not proven

- **Late join.** The self test asks everyone who was present when it began; a
  client connecting to a mission that already has authored content is a
  different question and still an open one.

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

**MCF is six addons, and Core is the mod.** Core is the framework a server
installs; the other five plug into it. Every module depends on Core and on
nothing else. `MCF_Dev` is the test environment, not a distribution addon: it
depends on everything precisely because a harness must reach everywhere, and
that is only safe because it never ships.

The full design, the measured graph and the namespace map are in
`docs/architecture/STRUCTURE.md`; the chronology is in `PROJECT_STATUS.md`.
`tools/measure_addon_graph.ps1` re-measures it — run that before believing any
of it. What to carry in your head:

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

**Verified end to end.** All addons loaded:
`Module: Game; loaded 5746x files; 11270x classes`, no `(E)`, and the test
world in `MCF_Dev` initialised prefabs from four different addons.

### Watched running, and it holds

A live session with every addon loaded exercised every module across an addon
boundary: the listen-server race and the deferral, both stores loading from
module addons on Core's event, the join push and faction re-push, all three
detection triggers registering through Core's watcher registry and firing, the
planning board opening from a preset in Core's manifest with a layout in
`MCF_Ops`, a conversation assigned and opened (`trust=50 fear=0` — the
disposition component from Core's `Character_Base` manifest, read by the
dialogue module, with the Game-Master check going through `MCF_Core_Roles`),
and `shout keys bound` from `MCF_AI` against Core's input manifest.

**The `modded class SCR_PlayerController` merges across four addons at
runtime**, not merely at compile time. No `Wrong GUID/name`, no
`Unknown class`; every `(E)` in the session is pre-existing or vanilla.

Not exercised: an actual shout (only the key binding), and the restrain/escort
chain.

### The devices module — functional 2026-09-10

Break-in and intel presentation both run in a live session. See
`docs/architecture/DEVICES.md` for the design and the open list.

- **It is all one addon now.** `MCF_Devices` was its own addon holding the lock
  and the games while the carrier and the shell stayed in Ops — one feature
  split down the middle, and the only illegal edge the framework ever had. Both
  halves live in `MCF_Ops`; `MCF_Devices_` survives only as a class namespace,
  and section 4 of `STRUCTURE.md` records why that name should change.
- **`MCF_Intel_ShellMenu` draws four skins** — paper, notepad, phone, laptop —
  chosen by the object's own `MCF_EIntelView`. A letter that looks like paper
  has nothing to do with hacking, which is why presentation is intel's job.
- **Three break-in games**, and the seed decides which: keypad, signal lock,
  port table. Nothing about the puzzle travels except the seed, so a fourth
  game touches three places and none of them is the wire format.
- **The break-in is drawn on the device's own screen**, not in a window over it:
  `MCF_Devices_HackScreen` binds to a widget it is handed and the shell creates
  that panel inside the device's `ScreenArea`. The old `MCF_Devices_HackMenu`
  and its menu preset are gone.
- Confirmed working, not polished. The smartphone and laptop models are both
  imported; the laptop's preview framing and its flat-colour materials are
  parked mid-tuning, and nobody has tuned the difficulty curve.

### Next, in order

1. **A two-peer, two-faction session.** Unblocks faction-scoped intel,
   late-join replication and the audience filter — and can fold in the two
   remaining modularisation unknowns: the dropped-component behaviour at
   runtime on a dedicated server, and the same packed to `.pak`.
2. **Decide how it gets published.** The shape is settled — Core is the headline
   entry, the four modules are dependent entries, MCF_Dev is never published.
   What is not settled is whether Reforger's Workshop dependency handling makes
   four separate module entries pleasant enough to be worth it, or whether the
   first release is one entry with the split kept only in the repository.
3. Turn off `m_bEveryoneMayDoEverything` and watch role resolution.
4. A one-clip animation graph for the restrained pose. Build it small — the
   crash is a size problem, not a concept problem. `arms_back` was the clip the
   user picked. Preview in `anims/workspaces/player/player_main.aw`.

### The mission data screen — working 2026-09-10

Game Master, right-click the operations board, "Mission data". Shows every
registered persistent set with a live count, clears them one at a time or all
at once behind a two-click confirmation, and keeps named whole-store snapshots.
Watched working: 16 keys cleared to 1 across four sets with the clients going to
zero taskings without a restart, and a snapshot restored both before and after a
server restart.

To add a fifth kind of persisted data, call `MCF_Core_DataSets.Register` in the
module that owns it and insert that module's reload into
`MCF_Core_DataSets.GetOnReloaded()`. Nothing in Core needs to change.

**`m_bEveryoneMayDoEverything` is still `true`, but this screen is not behind
it.** Clearing and restoring ask for `MCF_ETaskAction.DESTROY`, which is the one
action that ignores the master switch and always resolves the role properly.
COMMANDER only.

Reading the counts, and saving a snapshot, are gated as ordinary authoring
instead: looking is how somebody decides whether anything needs clearing, and
saving a copy takes nothing away. A person who may not clear can still keep a
snapshot and fetch someone who may.

WHY THIS ONE SCREEN IS STRICTER THAN THE REST, since that inconsistency will
otherwise read as an accident. The screen hangs off a Game Master context
action, and Game Master access is already granted per player by the server -- so
on any normal server a player cannot reach it. What the permission adds is that
the RPCs are not behind the button: MCF_RequestClearAllData is callable by any
client that talks to the server directly, without the UI. For every other
authoring screen the worst case is text somebody has to type back. Here it is a
week of planning with nothing to put it back, and MCF is going to other units'
servers. Weighed and chosen 2026-09-10; the alternative -- keeping it consistent
with the other authoring RPCs until roles are enforced everywhere -- is one line
in MCF_PlayerController_Data.

### ANSWERED: MCF snapshots cannot be paired with the engine's own saves

Asked on 2026-09-10, because "see all the save data in one place" is a fair
thing to want. The entry point does not exist in 1.8.0.13:

    ArmaReforgerScripted.GetSaveManager    Undefined function
    SCR_SaveManagerCore                    Unknown type
    SCR_SaveLoadComponent                  Unknown type
    SCR_SaveWorkshopManager                Unknown type

and fourteen further plausible renames -- SCR_SaveManager, SaveManager,
SCR_GameSaveManager, SCR_SessionSaveManager, SCR_MissionSaveManager,
SCR_SaveGameManager, SCR_SavesManager, SCR_SaveFileManager,
SCR_ScenarioSaveManager, SCR_SaveLoadManager, SCR_PersistenceManager,
SCR_SessionStorage, SCR_SaveManagerComponent, SCR_GameModeSaveManagerComponent
-- are all absent too. `SCR_CreateNewSaveDialog` and `SCR_MissionHeader` DO
exist, so saving has been renamed rather than removed, but the name is not
guessable and the engine's own scripts are inside data.pak.

Do not repeat this search from the published API documentation. That is
generated from **1.1.0.42** and this machine runs **1.8.0.13**; every signature
it gives for the save system is wrong. The fallback, if the overview is still
wanted, is to list `$saves:` with `FileIO` -- read-only, no engine API, cannot
break on a rename.

**Technique worth keeping: the compiler is a type lookup.** A file declaring one
variable per candidate type, in a method that is never called, costs one
Workbench start and answers for all of them at once -- "Unknown type 'X'" names
everything absent, and silence names everything present. Include one type you
know exists as a control, so a round that reports everything missing can be told
apart from a round where the probe itself was broken. This is far cheaper than
one guess per restart, which is how the RestApi afternoon went.

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
  an error line per mesh per load. It comes back on every reimport; strip it
  with a regex over the `.xob.meta` files.
