# MCF Devices — design

Status: **proposal**. No code written. The models are in place (see §4).

Phones and laptops you can find, break into, and read. Written 2026-09-10.

---

## 1. The thing that makes this cheap

`MCF_EIntelView` already has a `DEVICE` value, and the header of `MCF_Intel.c`
already says why:

> DELIBERATELY NOT ONE CLASS PER KIND. A letter, a phone, a photo and a map are
> the same data wearing different framing: a heading, some text, and optionally
> a place. Splitting them into separate systems would triple the work and
> produce three inconsistent viewers. The kind only decides how it is drawn.

So a phone is **already** intel data. This module does not add a data model. It
adds two things:

1. **A lock** — a device refuses to be read until somebody gets past it.
2. **A shell** — the same `MCF_Intel_Entry` list drawn as a phone or a laptop
   screen instead of a plain list.

That is the whole feature. Everything else already exists.

## 2. Where each piece lives

| Piece | Addon | Why |
|---|---|---|
| `MCF_Devices_LockComponent` | MCF_Devices | Sits **beside** `MCF_Intel_CarrierComponent`, never inside it, so Ops never learns that hacking exists |
| The minigame (screen + logic) | MCF_Devices | |
| The phone / laptop shell UI | MCF_Devices | An alternative renderer for the `DEVICE` view |
| Smartphone and laptop prefabs | MCF_Devices | |
| Game Master attribute classes | MCF_Devices | Listed in Core's `Configs/Editor/MCF_EditorAttributes.conf` manifest, exactly as `MCF_RestraintPoseEditorAttribute` in Subdue is |
| Menu presets for the two screens | **Core**'s `chimeraMenus.conf` manifest | Same as the planning board: preset in Core, layout in the module |
| Layouts | MCF_Devices | |
| Intel data, store, viewer, read action | MCF_Ops, unchanged | |

Dependencies: `MCF_Devices` -> Core, Ops.

**Not in Core: the minigame.** It is tempting — "a skill check that gates an
interaction" is obviously generic, and a locked door or a safe would want the
same thing. But there is exactly one consumer today, and Core's rule is *what
more than one module needs*. It goes in MCF_Devices and moves to Core the day
a second module asks for it. That move is a file and a rename.

## 3. What happens with the module absent

A smartphone prefab whose `MCF_Devices_LockComponent` cannot be resolved loses
that component and keeps everything else — proven behaviour, one `WORLD (E)`
line. So the phone still spawns, still carries its intel, and still reads
through Ops' plain `DEVICE` viewer.

**It reads *unlocked*.** That is the deliberate choice: the alternative is a
device that is permanently unopenable, which is a mission silently broken
rather than a mission slightly easier. Losing a lock is a degradation the
mission maker can see; losing the content is not.

## 4. Model pipeline

Four models, cleaned headlessly in Blender on 2026-09-10 and measured:

| Model | tris | size | materials | where |
|---|---|---|---|---|
| Letter (envelope, paper, wax seal) | 7 642 | 6.2 × 15.5 × 2.6 cm | 3 | `MCF_Ops/Assets/Props/Intel/Letter/` |
| Notepad | 4 188 | 13.7 × 21.0 × 1.7 cm | 1 | `MCF_Ops/Assets/Props/Intel/Notepad/` |
| Smartphone | 878 | 7.0 × 14.9 × 0.9 cm | 1 | `art/Devices/Smartphone/` — moves into MCF_Devices when it exists |
| Laptop | 13 014 | 22.2 × 33.0 × 2.9 cm | 6 | `art/Devices/Laptop/` |

What the cleanup did: joined every mesh into one `LOD0`, deleted the
`directionalLight1` and `aiSkyDomeLight1` that shipped inside the laptop scene,
oriented each model flat (smallest dimension on Z, longest on Y), scaled to
real-world size, moved the origin to the base centre, and added a `UTM_` box for
collision. Textures went from 4096² to 1024², which took the set from 45.6 MB
to 5.7 MB.

Two of the four arrived badly scaled: the notepad was 75 × 115 cm and the
laptop 3.6 cm. Worth remembering that store models cannot be trusted on scale.

**One trap, recorded because it cost a pass.** The letter and the laptop keep
their meshes parented to empties. Deleting the empties first collapses the
children onto each other — the envelope came out 3 cm wide instead of 6.2 and
the laptop 17.8 instead of 22.2, with no error anywhere. Unparent with
`CLEAR_KEEP_TRANSFORM` *before* removing the empties.

**The remaining step is manual.** FBX -> XOB is a Workbench Resource Manager
import, and the material hookup happens there. Until that runs there is no
`.xob` to point a prefab at, so `MCF_Intel_Letter.et` and
`MCF_Intel_Notebook.et` still both use the same vanilla notebook model. Once
the import has produced the `.xob` files and their GUIDs, wiring the two
prefabs is a two-line change each.

`RigidBody { ModelGeometry 1 }` on those prefabs means the physics shape comes
from the model, so the `UTM_` collision mesh has to survive the import. If it
does not, that is the first thing to check when a letter falls through a table.

## 5. The lock, and why the server has to hold it

The lock state lives on the device entity on the server and replicates with
`RplProp` — the pattern already proven on `MCF_Intel_CarrierComponent`.

The part worth getting right the first time is **who decides the minigame was
won**. A client that reports "I solved it" is a client that owns every phone in
the mission, and the read action is already server-checked, so a client-decided
lock would be the one soft spot in an otherwise tight system.

The shape that avoids it, and the reason to pick the minigame with this in
mind:

1. Client asks to break in. `MCF_RpcAsk_...` through `SCR_PlayerController`,
   the same route as every other MCF request.
2. **Server generates the challenge** — a seed, a sequence, a target — and
   sends it to that one client with `RplRcver.Owner`.
3. Client plays it and returns what it produced.
4. **Server checks the answer against the seed it issued** and sets the lock.

That rules out any minigame whose result cannot be re-checked from a seed. A
"press these six keys in this order" or "stop the marker in this window, three
times" both work: the server knows the order and the windows. A pure
reaction-time check does not, and a "hold the button" bar is decided entirely
by the client. Pick from the first kind.

Two constraints from this project's own scars:

- **Escape stops the play session in the Workbench.** The minigame screen needs
  a visible, always-enabled close button, like every other MCF screen.
- **A Game Master attribute carries 12 bytes and cannot hold text.** Difficulty
  is therefore a number, and a slider-backed attribute writes a **float** —
  `CreateInt`/`GetInt()` round-trips as 0. Copy `MCF_RestraintPoseEditorAttribute`.

## 6. The Game Master's controls

On any intel object, through Core's editor attribute manifest:

| Attribute | Type | Meaning |
|---|---|---|
| Secured | bool | Whether this device needs breaking into at all |
| Difficulty | float slider, rounded | How hard the challenge is |
| Device kind | float slider, rounded | Which shell opens — phone or laptop |

Nothing here is text, which is the constraint, not a preference.

## 7. The shell

A full-screen menu that draws the existing `MCF_Intel_Entry` list as a phone or
laptop screen: a list of messages or files, one open at a time, the same data
Ops' plain viewer shows.

One thing the data model will need before this looks right: an entry currently
has a heading, a timestamp and a body. A phone wants to group entries — this
message thread, that email folder, these photos. That is one field on
`MCF_Intel_Entry` and it belongs in **Ops**, not here, because it is a property
of intel rather than of devices. Ops' own viewer can ignore it.

## 8. Open

- Whether the minigame moves to Core. Revisit when a second module wants it.
- The laptop screen texture (`XYLDs.png`, 640 × 480) is a placeholder image
  baked into the model. A laptop that shows the shell content on its own screen
  would need a render target, which is a much bigger piece of work than the
  overlay and is not planned.
- Nothing here is built. This document is the plan, not a record.
