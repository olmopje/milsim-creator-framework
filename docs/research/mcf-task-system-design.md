# Design note: MCF's own task system

Status: **design captured, not started.** Vision from the project owner,
sharpened here and grounded in research into how milsim units actually work.

## The goal

Not the vanilla task system. Take inspiration from it, but build MCF's own,
because the shape of what is wanted is fundamentally different.

Views wanted:

- **Game Master** — overview of everything in the mission: tasks, triggers,
  nodes, their state.
- **Commander** — creates a task out of available intel, in his own words.
- **Squad leader** — sees tasks, assigns them to specific squad members.
- **Soldier** — sees the tasks assigned to him.

Milsim principle: players decide their own approach. Access should be through a
**physical item** — a whiteboard in a command post, a clipboard in inventory —
not an abstract menu bolted to the HUD.

## The central insight: two layers, not one

Vanilla collapses these into one thing, which is exactly why it does not fit.

**Layer 1 — mission-authored intel.** What the mission maker places:
`MCF_Obj_Objective`, triggers, observation nodes. Ground truth about what the
mission contains, producing *intel*: things known, or knowable once a gate
opens.

**Layer 2 — player-authored plan.** What a commander creates from that intel
and delegates down the chain. Not mission content — the players' plan *about*
the mission content.

Milsim is that separation. The mission says "the enemy holds this village."
The players decide "2nd squad flanks left, 1st supports by fire." A task system
that cannot express both layers cannot express milsim.

Consequence: `MCF_Obj_Objective` feeds layer 1. It is a source of intel, not a
task. Tasks are created at runtime by players.

# Research findings

## How milsim units actually plan (UNITAF Force Manual)

UNITAF, a large established unit, publishes its SOP. Key points:

- **SMEAC is the standard format** for every briefing: Situation, Mission,
  Execution, Administration/Logistics, Command/Signal. This is real military
  doctrine (the five-paragraph operations order), not a game convention, and
  unit leaders are already trained to write and read it.
- **Roles are more differentiated than our four views**: Campaign Manager sets
  strategic context, Field Leader plans and executes, Game Master runs OPFOR,
  squad leaders and subordinates execute. Note the Field Leader (in-fiction
  commander) is a different person from the Game Master.
- **Information flows both ways.** Orders go down via the OPORD; subordinates
  give **back briefs** — briefing their own plan back up to confirm
  understanding. Our design so far only modelled downward delegation.
- **Planning happens days in advance and mostly out of game**, in an
  "Operations Centre editor", with standardised map markers (WP for waypoint,
  C for compound, OP for observation point).
- **"The planning process should take no more than 30 minutes."** A hard speed
  constraint on any authoring UI.

## What already exists in the ecosystem

- **TacOps** (tacops.gg) — the notable Reforger planning tool. Web-based,
  entirely out of game. Full APP-6 military symbol library, maneuver arrows,
  grid overlays, a read-only Briefing Mode, and Mission Notes for commander's
  intent, phases, comms plan and ROE. Shared by one link, no account needed.
  **Explicitly does not do squad-level assignment or role-based command
  hierarchy.**
- **2-7 TSLF Framework** (Reforger Workshop) — in-game, but scoped to group
  roles, group size caps, per-group radio frequencies and kick functionality.
  No tasks, briefings or command interface. Its own tagline: "TSLF is a
  toolset. You build the doctrine — TSLF just makes the engine stop fighting
  you."
- **Vanilla Reforger tasks** — built for mission-author-created tasks with
  faction scoping and assignment, not player authoring.

## The gap, stated plainly

Out-of-game planning and symbology is well served. In-game group structure is
served. **Nobody covers in-game, player-authored, hierarchy-aware task
delegation and tracking.** That is a real gap and a defensible reason to build
rather than reuse.

## The strategic consequence: build for execution, not planning

This is the most important conclusion from the research. Units already plan
days ahead, out of game, in browser tools and on Discord. An in-game system
that tries to replace that phase is competing with TacOps on its own ground and
will lose, while solving a problem units do not have.

What units have no support for is what happens **after** contact: the plan
meets reality, and orders change. Fragmentary orders, reassignment, a squad
leader retasking his men, the commander re-prioritising off new intel, and
everyone knowing what the current task actually is.

So MCF should aim to be where the plan is **carried into the mission and
tracked as it changes**, not where the plan is written. That is a narrower,
more defensible scope, it plays to what only an in-game system can do, and it
matches the two-layer split above.

Practical implication: an import path matters more than an authoring suite.
Getting an existing plan in — a briefing text, a link, a set of markers —
beats rebuilding map drawing.

# Design decisions this points to

1. **Structure a task as SMEAC, not title plus description.** Even a reduced
   set (Mission: who/what/when/where/why; Execution: intent and scheme;
   Command/Signal: comms) is what leaders already write. Free text throws away
   structure the users already have in their heads.
2. **Support back briefs.** A task should carry an acknowledgement and a
   subordinate's stated plan back up, not just flow down.
3. **Authoring must be fast.** Thirty minutes covers a whole operation, so a
   task must be creatable in seconds: templates, presets, derive-from-intel,
   never a blank paragraph box.
4. **Use APP-6 conventions and the short codes units already use** (WP, OP, C)
   if map markers are built.
5. **Model the roles the units actually run**, which distinguishes the
   in-fiction Field Leader from the out-of-fiction Game Master. Our "commander
   view" is the Field Leader; the GM view is a different thing with different
   permissions.

## What already exists and should not be rebuilt

- `SCR_GroupsManagerComponent` (56 KB) and `SCR_PlayerControllerGroupComponent`
  (62 KB) in `scripts/Game/Groups/`: groups, membership, roles, leadership,
  join requests, group UI.
- `SCR_GroupsManagerComponent` and `SCR_CommandingManagerComponent` are already
  on the `GameMode_Editor_Full` entity in MCFTestworld.

The command hierarchy is already live. Read it; do not build squad structure.
What MCF adds is task data hung off that hierarchy.

## This is the same problem as the audience question

`docs/research/multiplayer-and-audience.md` asks who a message is shown to.
This design answers it: **the command hierarchy is the audience model.** A task
assigned to a squad is visible to that squad; a Field Leader's tasks are
visible within his faction; the GM sees everything. Build them together.

## Networking is no longer optional

Everything MCF has today is a local script singleton with no replication. That
was survivable while nodes only reacted to local triggers. It is not survivable
here: task state is player-authored, shared, and must persist for the mission.
A commander creates a task on his machine and a soldier must see it on his.

The multiplayer work in `multiplayer-and-audience.md` is therefore a
prerequisite, not a follow-up.

## The physical item is a real design decision, not decoration

Putting the task overview in a carried or placed object makes information a
thing that can be carried, shared, left behind, captured or lost. That is
genuine tactical texture and worth protecting as a requirement.

MCF already has the interaction pattern: `MCF_Interact_TalkAction` is a
`ScriptedUserAction` that puts a prompt on an entity. A whiteboard is the same
pattern with a different action and UI. An inventory clipboard is a separate,
larger piece — an actual inventory item.

Open question to decide early: is the physical item the only way in, or is
there a fallback key? Strict is more milsim, lenient is more playable. Make it
a mission setting rather than a hard-coded choice.

## The real cost is UI

Four views is the largest part of this build, and it is Workbench layout work
rather than script. The dialogue system in `docs/research/dialogue-system.md`
needs the same skill and groundwork. Whichever is built first should establish
the layout conventions for the other.

## Suggested sequence

1. **Server-authoritative task store.** A replicated list of tasks, each with:
   id, SMEAC fields, author, assignee (player or group), state, optional world
   position, acknowledgement/back brief, and the intel it derives from. No UI
   yet — prove creation and replication through log output, the way the trigger
   chain was proved.
2. **One read-only view**, opened from a placed object via a ScriptedUserAction.
   Soldier view first: simplest, and it validates the layout approach.
3. **Authoring** (Field Leader): create a task from intel, fast, template-driven.
4. **Delegation** (squad leader): assign to members using group membership from
   `SCR_GroupsManagerComponent`. Add back-brief acknowledgement here.
5. **GM overview** — widest read, no authoring permissions beyond "sees all".
6. **Inventory clipboard** last: an item problem, not a UI problem.

Prove each step on a dedicated server. A Workbench play session is server and
client at once and will hide every replication bug in this design.

## Sources

- UNITAF Force Manual, mission planning SMEAC:
  https://unitedtaskforce.net/training/sop/field-leadership/mission-planning-smeac
- Five paragraph order (background):
  https://en.wikipedia.org/wiki/Five_paragraph_order
- TacOps, tactical planning for Arma Reforger: https://tacops.gg/
- 2-7 TSL Framework, Reforger Workshop:
  https://reforger.armaplatform.com/workshop/66CCE7AD019F3E4D
