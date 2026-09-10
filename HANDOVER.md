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
- **MAP intel view** exists in the data model and does nothing.
- **GROUP-assigned tasks** go only to their author until squad membership lands.
- **Dropped intel objects do not respawn** after a restart. The board record
  survives; the physical document does not.

---

## The next steps, in the order I would take them

### 1. Close the Edit intel / Edit device overlap — one line

`MCF_Intel_EditContextAction.CanBeShown` has no view filter, so "Edit intel" is
offered on **every** carrier including phones and laptops. Its VIEW toggle only
knows DOCUMENT and DEVICE, so pressing APPLY there silently rewrites a phone into
a flat document. `MCF_Device_EditContextAction` already has the filter that
should be mirrored here, inverted.

Silent data loss for a mission maker who picks the wrong one of two screens with
similar names. Cheapest real fix on the list.

### 2. Kill the `metal.gamemat` noise

`{536BF67B2052B869}` resolves to nothing and is re-injected into every
`.xob.meta` on reimport, so every model load costs two `RESOURCES (E)` lines.
This is not cosmetic: those lines are read past dozens of times per debugging
session, and error noise is how a real error gets missed.

### 3. Turn `m_bEveryoneMayDoEverything` off and play a session

The permission system is fully written and completely unexercised — it has never
once said no. Roles come from vanilla's command hierarchy, so even solo you can
watch resolution and confirm DESTROY is gated. Do this before anything is built
on top of the tasking model.

### 4. Dedicated server, then packed to `.pak`

The two remaining modularisation unknowns — whether an unresolvable component is
dropped gracefully at runtime on a dedicated server, and whether it still is once
packed — were only ever measured in the World Editor, unpacked. Both are testable
without a second person, and both must be answered before any publish. The
manifest pattern that lets Core name every module's contributions rests on this
behaviour.

### 5. The namespace rename, with a fresh head

`MCF_Devices_` → `MCF_Lock_`, so it stops differing from `MCF_Device_` by one
letter while meaning something else. And decide what to do about `MCF_AI_` and
`MCF_Interact_` spanning two addons each. Mechanical across roughly sixty files —
which is exactly why it should not be done at the end of a long session.

### Deliberately not next

**The laptop.** It works; the preview framing and the flat-colour materials are
parked mid-tuning at the user's request. It blocks nothing, and it is the kind of
work that eats an evening without closing an open question.

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
