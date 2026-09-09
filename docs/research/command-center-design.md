# The command centre: two layers, and the intel that flows between them

Recorded 2026-09-09, from the design decision that "there must be a clear
distinction: the Game Master has triggers as tasks, the commander in game
creates tasks out of intel they obtain from triggers."

This sharpens the two-layer split already stated in
`mcf-task-system-design.md` and names the thing that was missing from it: the
**intel** that carries information from one layer to the other.

## The two layers

**Layer 1 -- what the mission maker built.** Trigger zones, cone detection,
observation nodes, objectives, recipes. Authored before the session, placed in
the world or in Game Master. These are not tasks and must never appear in a
task list. They are the world reacting to what players do.

**Layer 2 -- the plan the players wrote.** A commander's five-paragraph order,
a squad leader's retasking mid-contact, a back brief from the subordinate who
received it. Authored at runtime, by players, out of what they know.

The distinction is not cosmetic. A mission maker's objective is *mission
content*: it exists whether or not anyone reads it, and it is the same for
everybody. A commander's task is *a decision about mission content*: it exists
because a person judged something, it belongs to whoever wrote it, and other
people may or may not be entitled to see it. Merging the two would make the
command hierarchy meaningless -- there would be nothing for a commander to
decide, only a checklist to work through.

## Intel is the bridge

Layer 1 currently ends at a text line on screen. That is a dead end: the
information reaches a player's eyes and then evaporates. For a commander to
"create a task out of intel obtained from triggers", the intel has to be a
thing that persists, that has a source, and that can be cited.

So layer 1 nodes stop only publishing events and start producing **intel
records**. A trigger that fires does two things: it publishes its event, as it
does today, and it files a report.

An intel record needs, at minimum:

- an id, so a task can cite it
- a headline and a body -- what is known, in the words the mission maker wrote
- where it came from: the event name and the node that produced it
- when it was observed
- where, if the producing node had a position
- who is entitled to see it, using the same audience model tasks already use

That last point matters more than it looks. Intel is exactly the thing that
should not be visible to everyone: a patrol that spotted movement knows
something the rest of the force does not, and the moment it becomes common
knowledge for free, the whole reason to have a command net disappears.

## What the planning board becomes

Two panes, not one list.

**INTEL** -- what this player is entitled to know, newest first, read only.
Produced by layer 1. Nobody authors it and nobody edits it.

**TASKS** -- the plan. Authored by players, editable by whoever is allowed,
acceptable by whoever is allowed.

The bridge between them is one action: **create a task from this intel**. The
new task starts with the intel's headline in its Situation paragraph and keeps
a reference back to the record it came from, so anyone reading the order can
see what it was based on. That single action is what makes the two layers
"flow into each other" rather than sit side by side.

A task can also close the loop in the other direction: `m_sCompleteEvent`
already lets a task be completed by a layer-1 event firing. So the full cycle
is trigger -> intel -> commander's task -> squad executes -> trigger ->
task complete. Layer 1 feeds the plan and then confirms it.

## Roles

Decided for now: **no restrictions**. Everyone may read, accept, edit and
publish. This is deliberate -- the UI has to be built and tested before
arguing about who may press what, and a permission bug that hides things is
much harder to notice than one that shows too much.

But the seam goes in now, because retrofitting one later means touching every
RPC and every button. A single resolver answers "what is this player allowed
to do", today returning yes to everything. Three sources can plug into it
later, in this order of preference:

1. Derived from vanilla: faction commander is the commander, group leader is
   the squad leader, everyone else is a soldier. Works with no setup.
2. MCF's own Game Master slotting -- fixed units, roles and loadouts a player
   picks at join, with the GM marking which squads carry command rights. This
   is the eventual intent recorded earlier in the session.
3. Unit-defined ranks, so a unit can add its own tiers rather than living with
   three.

Because (3) is wanted, the permission table must be data, not code: a role and
an action in, a yes or no out, from a config a unit can edit. What must not be
data-driven is the *enforcement point* -- that stays on the server, in the
request handlers, where a client cannot reach it.

## Build order

1. Prove the menu can exist at all. `modded enum ChimeraMenuPreset` is the one
   step in the whole registration chain with no vanilla precedent that could
   be found; everything downstream is wasted if it does not compile and link.
2. One action on the board opening that screen.
3. The board reading and showing tasks it already receives.
4. Creating and editing tasks, server-validated.
5. Intel records from layer 1, and the create-task-from-intel bridge.
6. Permissions, once there is something worth restricting.

## The map

Decided 2026-09-09: the board gets a map, and it reuses the **vanilla Arma
Reforger map system** rather than drawing one of MCF's own. Two reasons. It is
the map players already know how to read, with the same controls and the same
markers; and a hand-drawn one would have to re-solve terrain rendering,
zooming and coordinate conversion for nothing gained.

Two surfaces, one map:

- **Inside the planning board.** Tasks and intel plotted where they happened,
  so an order can be read against the ground it concerns rather than as text
  alone. This is where a commander plans.
- **On a large physical board in the world.** The same live map rendered big,
  in a briefing room, so a whole unit sees the plan at once without everyone
  opening their own screen. This is where a unit gets briefed.

The second is the interesting one, and it is the reason to keep the map a
*view* over task and intel data rather than something that owns its own
markers. Two surfaces reading the same server-held state stay in agreement for
free; two surfaces each keeping their own marker list would drift apart the
first time somebody edited one.

Not started. Vanilla entry points to investigate: `SCR_MapMenuUI` and the
`MapMenu` preset (`UI/layouts/Map/MapMenu.layout`), and the fact that
`SCR_CommandPostMapMenuUI` reuses that same layout with a different script
class -- which is direct evidence that the vanilla map can be hosted by
another menu class, which is exactly what MCF needs. For the in-world board,
the open question is whether a map can be rendered to a texture on a mesh at
all, or whether it has to stay a screen players walk up to and open.

## Intel as a carried object

Decided 2026-09-09. Intel is not a notification. It is a **physical thing that
has a location and must be carried**, and its journey up the chain of command
is the mechanic.

Three separable parts:

**The payload** -- what is known. An area (a point, a radius, and a
deliberate error in the point), written text, or both. Plus how reliable it
is and where it came from.

**The carrier** -- the object holding it. A map, a phone, a letter, a photo, a
notebook. Any prefab becomes one by adding `MCF_Intel_Carrier`. This is the
part that moves between people.

**The processing state** -- where it sits in the chain:
`UNREAD -> HELD -> SUBMITTED -> LOGGED`. The payload only enters the shared
store at LOGGED. Before that it exists on the object and reaches only whoever
is holding it.

That last rule is the whole design. A rifleman who finds a map knows something
the force does not, until a person carries it to a commander and the commander
enters it. Information has to physically travel.

### Deliberate imprecision

A marked area has a radius and a maximum offset of the drawn centre from the
true point. The offset is rolled **once, on the server, and stored on the
intel** -- never per viewer. Two players reading the same map must see the same
wrong circle, or it is not the enemy being unreliable, it is the mod. A third
switch decides whether the truth is guaranteed to lie inside the drawn circle.

### Two routes to the commander, both needed

Physical: hand the object over, the commander reads it and logs it.
Verbal: the finder reads it, reports it on the radio, and the commander types
it in themselves. The second is the better milsim -- the human is the
bandwidth. Both end in the same server call, so nothing downstream cares.

### What falls out for free

Intel can be **captured**. A scout who dies with the map on them leaves it on
the body. No extra work; it follows from the model.

And because reading is a recorded act, the server ends up holding who knew
what and when -- which is exactly what the AAR layer needs.

## Game Master configurability, and the wall it hits

Requirement: everything settable from Game Master.

**Already in place.** MCF's `Prefabs/Editor/Modes/EditorModeEdit.et` override
already registers both its placeable-entity registry and its own attribute
list (`Configs/Editor/MCF_EditorAttributes.conf`) on
`SCR_AttributesManagerEditorComponent`. So the pipeline works; adding a knob is
writing an attribute class plus a config entry.

Attributes are **global, not per entity**: every attribute is offered every
selected entity, and an attribute opts out by returning null from
`ReadVariable`. That is also why a placed MCF node with no matching attribute
shows "No properties" -- nothing to fix in the prefab.

**The wall: the vanilla attribute system cannot carry text.**
`SCR_BaseEditorAttributeVar` stores everything in a single `vector`, replicates
a fixed 12-byte snapshot, and offers exactly four factories -- `CreateInt`,
`CreateFloat`, `CreateBool`, `CreateVector`. There is no string variant, no
text-entry attribute layout, and no edit-box attribute UI component. A custom
layout does not help, because the value still has to fit in the vector.

So the split is forced, and it is a reasonable one:

- **Numbers, toggles and fixed choices** -> vanilla GM attributes. Radius,
  centre offset, reliability, viewer type, whether a commander is required,
  audience. All of these are int/float/bool/enum and fit.
- **Free text** (headline, body, phone messages, letter content) -> MCF's own
  editing screen, which already exists: the operations board proved the whole
  chain of a modded menu, inherited edit boxes, a server-validated RPC and
  persistence. The Game Master edits intel text there rather than in the
  vanilla attribute panel.

Writing a fake string transport into an int attribute (indices into a table of
canned phrases) was considered and rejected: it would look configurable and be
useless the first time a unit wants to write its own sentence.

## The viewer: one screen, several skins

A map, a phone, a letter and a photo are the same screen with different
framing. One `MCF_Intel_Viewer` menu with a mode, not four systems.

The phone in particular is nearly free: a list of items on the left and the
selected one on the right is exactly the shape the operations board already
uses, so a messages/contacts/notes phone is mostly reuse.

## Reading is a world action, not an inventory action

Decided: the object is picked up and put down like any item, and reading it is
a **user action on the object** -- the same `ActionsManagerComponent` path the
task board proved, including the `Position PointInfo { Offset ... }` that the
board needed before any prompt appeared at all.

This avoids inventory integration entirely for the first version, and it reads
better: you kneel next to the body and go through the papers.
