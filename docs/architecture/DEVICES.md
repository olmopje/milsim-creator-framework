# MCF Devices — design

Status: **run and working**, not polished. Break-in and reading both watched in
a live session on 2026-09-10.

Phones and laptops you can find, break into, and read. Written 2026-09-10.

> **`MCF_Devices` is no longer an addon.** It was one, and folding it back into
> `MCF_Ops` is the whole point of section 2 below. Everywhere this document says
> "MCF_Devices owns" a piece, read it as a **class namespace inside MCF_Ops**,
> and read the argument as what it is: the reason the split failed.
> `docs/architecture/STRUCTURE.md` is the structure of record.

---

## 1. What this module is, after the correction

`MCF_EIntelView` already had a `DEVICE` value, and the header of `MCF_Intel.c`
already said why:

> DELIBERATELY NOT ONE CLASS PER KIND. A letter, a phone, a photo and a map are
> the same data wearing different framing: a heading, some text, and optionally
> a place. Splitting them into separate systems would triple the work and
> produce three inconsistent viewers. The kind only decides how it is drawn.

So a phone is **already** intel data. This module adds no data model.

It was first written to add two things — a lock and a shell. **The shell has
since moved out.** A letter that looks like paper has nothing to do with
breaking into anything, and nobody should have to install a hacking module to
get a readable letter. Presentation is a property of intel, which is what the
paragraph above has said since the day it was written. So:

- `MCF_Intel_` owns presentation: `MCF_Intel_ShellMenu` and the four skins
  (`MCF_IntelPaper`, `MCF_IntelNotepad`, `MCF_IntelDevice`, `MCF_IntelLaptop`).
- `MCF_Devices_` owns the lock and the break-in games, and nothing else.

That is the whole feature. Everything else already existed.

## 2. Where each piece lives, and why the addon split failed

This started as two addons. `MCF_Devices` held the lock and the games while the
carrier and the shells stayed in `MCF_Ops` — and that produced the only illegal
cross-module edge the framework has ever had, plus two files whose entire
purpose was to reach across the boundary it created. It split **one feature**
down the middle: a locked laptop is intel you cannot read yet, not a separate
subject.

Folded back into `MCF_Ops` on 2026-09-10. Both namespaces now live in one addon:

| Piece | Namespace | Why |
|---|---|---|
| `MCF_Devices_LockComponent` | `MCF_Devices_` | Sits **beside** `MCF_Intel_CarrierComponent`, never inside it, so nothing about reading has to know that locks exist |
| The three break-in games | `MCF_Devices_` | |
| The break-in screen | `MCF_Devices_` | `MCF_Devices_HackScreen` — not a menu; it binds to a widget it is handed |
| The break-in layout | `MCF_Ops/UI/layouts/` | Created into the device's own `ScreenArea` at runtime, so one layout serves every skin |
| Game Master attribute classes | `MCF_Devices_` | Listed in Core's `Configs/Editor/MCF_EditorAttributes.conf` manifest, exactly as `MCF_RestraintPoseEditorAttribute` in `MCF_AI` is |
| **The paper / notepad / phone / laptop shells** | `MCF_Intel_` | Presentation is a property of intel — see §1 |
| Menu presets for the skins | **Core**'s `chimeraMenus.conf` manifest | Preset in Core, layout in the module |
| Intel data, store, viewer, read action | `MCF_Intel_`, unchanged | |

Dependencies: `MCF_Ops` -> Core. That is the whole graph now, and it is the
point.

The two namespaces one letter apart are a real defect — see section 4 of
`STRUCTURE.md` for what they should be called instead.

**Not in Core: the minigames.** It is tempting — "a skill check that gates an
interaction" is obviously generic, and a locked door or a safe would want the
same thing. But there is exactly one consumer today, and Core's rule is *what
more than one module needs*. It moves to Core the day a second module asks for
it. That move is a folder and a rename.

## 3. What happens with the lock absent

A smartphone prefab whose `MCF_Devices_LockComponent` cannot be resolved loses
that component and keeps everything else — proven behaviour, one `WORLD (E)`
line. So the phone still spawns, still carries its intel, and still reads —
and, since the shells moved to Ops, it still reads **as a phone**. Only the
lock is gone.

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
| Smartphone | 878 | 7.0 × 14.9 × 0.9 cm | 1 | imported: `MCF_Ops/Assets/Props/Devices/Smartphone/` |
| Laptop | 13 014 | 22.2 × 33.0 × 2.9 cm | 6 | imported: `MCF_Ops/Assets/Props/Intel/Laptop/`, split into body and lid |

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

The letter and the notepad are imported and wired. **The smartphone and the
laptop are not**, so `MCF_Devices_Phone.et` still wears a placeholder mesh.

`RigidBody { ModelGeometry 1 }` on those prefabs means the physics shape comes
from the model, so the `UTM_` collision mesh has to survive the import. If it
does not, that is the first thing to check when a letter falls through a table.

**Open, cosmetic:** both imported `.xob` files carry a reference to
`material/metal.gamemat` that does not resolve, logged once each at load.
Removing `SurfaceProperties` from the `.xob.meta` did not help because the
reference is baked into the `.xob`; both need re-importing.

## 5. The lock, and why the server has to hold it

The lock state lives on the device entity on the server and replicates with
`RplProp` — the pattern already proven on `MCF_Intel_CarrierComponent`.

The part worth getting right the first time is **who decides the minigame was
won**. A client that reports "I solved it" is a client that owns every phone in
the mission, and the read action is already server-checked, so a client-decided
lock would be the one soft spot in an otherwise tight system.

The shape that avoids it:

1. Client asks to break in. `MCF_RpcAsk_...` through `SCR_PlayerController`,
   the same route as every other MCF request.
2. **Server generates the challenge** — one integer seed — and sends it to that
   one client with `RplRcver.Owner`, with the difficulty.
3. Client plays it and returns what it produced. Never "I won".
4. **Server re-derives the puzzle from the seed it issued**, checks the answer,
   checks its own clock, and sets the lock.

That rules out any minigame whose result cannot be re-checked from a seed. A
pure reaction-time check does not qualify, and a "hold the button" bar is
decided entirely by the client.

**The honest limit.** Both machines derive the whole puzzle from the seed, so a
modified client can always read the answer — that is equally true of a keypad
whose sequence is flashed at the player anyway. What the server guarantees is
narrower, and is the thing that matters: nobody gets in without sending a
correct answer inside a window the server itself measured.

Two constraints from this project's own scars:

- **Escape stops the play session in the Workbench.** The break-in screen needs
  a visible, always-enabled close button, like every other MCF screen.
- **A Game Master attribute carries 12 bytes and cannot hold text.** Difficulty
  is therefore a number, and a slider-backed attribute writes a **float** —
  `CreateInt`/`GetInt()` round-trips as 0. Copy `MCF_RestraintPoseEditorAttribute`.

## 6. The Game Master's controls

On any intel object, through Core's editor attribute manifest:

| Attribute | Type | Meaning |
|---|---|---|
| Secured | bool | Whether this device needs breaking into at all |
| Difficulty | float slider, rounded | How hard the challenge is, for whichever game comes up |

Nothing here is text, which is the constraint, not a preference.

There is deliberately **no "which game" attribute**. A mission maker choosing
the puzzle would mean every phone in a mission plays the same one, which is the
outcome the second and third games exist to prevent.

## 7. The three break-in games

One seed, three games, and the seed decides which. That is the trick worth
keeping: the kind is derived from the seed on both machines
(`MCF_Devices_Challenge.KindFor`), so a second puzzle costs nothing on the wire
and the server holds no extra state — it does not have to remember what it
handed out, because the seed says.

| Game | What it asks | Clock, easy → hard |
|---|---|---|
| `SEQUENCE` — keypad | Repeat the order the 3×3 grid flashed | 12 s → 5 s |
| `FREQUENCY` — signal lock | Tune frequency and gain until your trace covers the reference | 30 s → 16 s |
| `PORTS` — port table | Open exactly the ports a stated rule covers, no more and no fewer | 40 s → 22 s |

Three clocks rather than one, because repeating six flashes and reading ten
rows of a table are not the same amount of work, and one clock for both would
make one a formality and the other unwinnable.

**Never the same game twice running on one device.** `IssueChallenge` rerolls
the seed while the kind matches the last one it issued, bounded at sixteen
tries — falling out with a repeat is a worse round, not a broken one.

**Adding a fourth touches three places**: the `MCF_EPuzzleKind` enum,
`GetPuzzle` beside it, and the panel switch in `MCF_Devices_HackMenu`. Not the
lock, not the player controller, not the wire format.

The server side is polymorphic (`MCF_Devices_Puzzle` — a time limit and a
verify) and the client side is not: a flashing grid, a pair of waveforms and a
port table have nothing in common visually, and forcing them through one
"give me your widgets" interface would produce a contract every implementation
lies about. So the menu switches on the kind and calls each puzzle's own
builders.

**One layout, three panels.** All three share the title, the clock, the stop
button and the whole submit path; three presets would have drifted the moment
one was fixed.

Notes on the two new ones:

- The **signal lock** draws both traces into *one* 16×9 grid, not two stacked
  graphs. A cell lit by the reference only, by yours only, or by both gets a
  different colour, so "solved" is the screen going one colour and needs no
  reading at all. Difficulty widens the frequency range rather than making the
  tuning finer — the search grows, not the dexterity.
- The **port table** is the only one where a wrong press is recoverable, so its
  pressure comes from the clock rather than from a single fumble. It refuses a
  rule that would cover every row or none, by walking one row's values until
  the table is mixed — deterministically, so both machines still agree.

## 8. Open

- Nothing here has been run. First launch after the rewrite is the real test.
- Whether the minigames move to Core. Revisit when a second module wants them.
- A **hacking training centre** — a place to practise the three games without a
  mission — is wanted and not designed. It needs a way to issue a challenge
  against no device, which today is `MCF_Devices_LockComponent`'s job alone.
- The smartphone and laptop models are still not imported.
- The laptop screen texture (`XYLDs.png`, 640 × 480) is a placeholder image
  baked into the model. A laptop that shows the shell content on its own screen
  would need a render target, which is a much bigger piece of work than the
  overlay and is not planned.
