# MCF Mission Maker Guide

Every placeable MCF node in plain language: what it does, what its settings
mean, and how to wire them together. No scripting needed.

For the design behind it, see `docs/architecture/ARCHITECTURE.md`. For where
the project is going, `docs/ROADMAP.md`.

## Honesty about status

Everything described as working here has been **observed firing in a live
session**, not just assumed to work. That distinction is deliberate: on
2026-09-09 several nodes were found that compiled cleanly, initialised without
complaint, and could never fire. Anything still missing or faked is called out
in its own entry rather than glossed over.

---

# The one concept: nodes talk by event name

MCF nodes never reference each other directly. Each node **publishes** an event
name, or **listens** for one, or both. You connect two nodes by typing the same
event name into both.

- A node's *output* field is usually called something like "Triggered Event",
  "Event Name", "Output Event" or "Report Event".
- A node's *input* field is usually called "Trigger Event", and some nodes have
  more specific ones ("Complete Event", "Intel Gate Event", "Source Event").

**The spelling must match exactly.** `VillageAlerted` and `villageAlerted` are
two different events, and nothing will warn you loudly — the downstream node
simply never fires.

Event names are yours to choose. Pick names that describe what happened, not
which node did it: `BridgeDestroyed` reads better than `Trigger7Fired` and
survives you moving things around.

## A worked example

This chain is placed in the test world and confirmed working. It reads: the
player must check two separate positions before command accepts the report.

| Node | Setting | Value |
|---|---|---|
| Proximity Trigger "ALPHA" | Radius | `15` |
| | Triggered Event | `PointA` |
| Proximity Trigger "BRAVO" | Radius | `15` |
| | Triggered Event | `PointB` |
| Logic Node | Mode | `AND` |
| | And Input 1 | `PointA` |
| | And Input 2 | `PointB` |
| | Output Event | `BothVisited` |
| Alarm Trigger | Source Event | `BothVisited` |
| | Triggered Event | `ReportReady` |
| Recipe "report" | Trigger Event | `ReportReady` |
| | Steps | `PLAY_TEXT_LINE:Both positions checked. Sending report.` |
| Objective | Title | `Recon both approach routes` |
| | Intel Gate Event | `PointA` |
| | Complete Event | `ReportReady` |

Walking into ALPHA fires `PointA`, which unlocks the objective and tells the
Logic node one input is satisfied. Nothing else happens yet — that is the AND
node doing its job. Reaching BRAVO satisfies the second input, the Logic node
fires `BothVisited`, the Alarm relays it as `ReportReady`, the Recipe puts a
line on screen and the Objective completes.

The Alarm in the middle is optional. It exists so that downstream nodes listen
for a meaningful name (`ReportReady`) rather than the Logic node's raw output.

---

# Detection nodes

These watch the world and publish an event. Every newly spawned player and AI
is registered with them automatically — you do not wire that up.

**Detection runs on the server only.** Clients evaluate nothing.

## Proximity Trigger (`MCF_Obj_ProximityTrigger`)

Fires when a watched entity comes within the radius. Radially symmetric, so it
does not care which way anything is facing — a fixed position fully describes
it. This is the workhorse; use it for "players get close".

*Trigger Once* fires the first time and never again. Turned off, it fires each
time something enters after having left, not every frame.

## Cone Detection Trigger (`MCF_Obj_ConeDetectionTrigger`)

Like Proximity, but also requires the target to be within a forward-facing cone
of **this node's own facing**, so rotation matters when you place it.

**It is not line of sight.** It checks distance and angle only and cannot tell
whether a wall is in the way. Good for "the sentry is facing roughly your way";
not reliable for "the sentry can actually see you".

Because it reads its own rotation, dropping it loose in the world gives you a
cone frozen in whatever direction you placed it — fine for a fixed sentry post
or camera, less so for anything that should turn.

## Spotted By Player (`MCF_Obj_SpottedByPlayer`)

The inverse: put this on the thing that must be *seen*, and it fires once any
player has it within range and within their view cone. Use it to gate a
scripted moment on someone actually having looked — don't spring the ambush
reveal, don't count the intel as found, until it has been seen.

Set the view angle wider than a weapon-aim cone: this approximates "did you
glance that way", not "are you aiming at it". Same wall-blindness caveat.

---

# Logic and flow

## Logic Node (`MCF_Obj_Logic`)

Combines several events into one outcome. Set Mode to:

- **OR** — fires as soon as any listed input event happens.
- **COUNTER** — fires once the listed inputs have happened a set number of
  times in total.
- **AND** — fires once each of the filled input slots has happened at least
  once.

OR and COUNTER read their inputs from the Input Events list. **AND uses four
separate fixed slots** (`And Input 1-4`) instead, and ignores the list. Leave
unused slots empty. Four is a hard cap: Enforce Script has no closures, so the
slots cannot be generated from a list.

Once a Logic node fires it stays fired; it does not reset.

## Alarm Trigger (`MCF_Obj_AlarmTrigger`)

A relay: listens for one event, republishes it under another name. Use it to
give a chain a meaningful name in the middle, so five downstream nodes can
listen for `VillageAlerted` instead of all needing to know which trigger
originally fired.

## Recipe (`MCF_React_Recipe`)

The easiest way to chain several actions without scripting. Set a Trigger
Event, then list steps in order, each written `TYPE:value`:

- `PUBLISH_EVENT:SomeName` — fires another event, so recipes chain.
- `PLAY_TEXT_LINE:Some text` — puts a line on players' screens.
- `REQUEST_ANIMATION:` — publishes an animation-request event. **Nothing plays
  an animation from this yet**; it only announces the request.

Steps run immediately, one after another, with no delay between them.

## Trigger Zone (`MCF_Obj_TriggerZone`)

Publishes an event when activated, and is activated by an event — so it is a
pass-through you can name.

**It has no shape and detects nothing by itself.** For "player walks into an
area", use a Proximity Trigger, which actually detects. Collision-volume
detection is not built.

## Observation Node (`MCF_Obj_Observation`)

A point of interest that reports when triggered, and counts how often. Feed
several into one Logic node in OR mode for "watch these locations, react to
whichever gets used first".

---

# Telling the player something

## Text Line (`MCF_Voice_TextLine`)

Shows a line of text on screen when its trigger event fires. A stand-in for
voice lines, which are parked.

Priority orders lines that arrive together. Use this when several different
things should be able to say the same line, or when you want a line as its own
placeable object; for a line that belongs to one chain, a Recipe step is
simpler.

## How text reaches players

Lines are shown through the game's own popup notification, top of screen. The
server decides and broadcasts to every machine; each client then decides
whether the line is for it.

## Choosing who sees a line

Text Line and Objective both have an **Audience** setting:

- **EVERYONE** (default) — everyone in the mission.
- **FACTION** — only players of the faction whose key you type in the
  *Audience Faction Key* field. Leave the key empty and it falls back to
  everyone.
- **PLAYER** — a single player. Nothing sets this from the editor yet; it is
  there for script, and for the "tell the player who triggered this" case.
- **GROUP** — **not implemented.** It needs the squad hierarchy that arrives
  with the task system. Selecting it shows the line to everyone and logs a
  warning, rather than silently hiding messages.

Recipe steps (`PLAY_TEXT_LINE:`) always go to everyone; the step syntax has no
room for an audience. Use a Text Line node when a line needs one.

The filtering is wired end to end and compiles, but **has not yet been observed
working with two differing clients** — proving it needs two players on
different factions. Until then, treat FACTION and PLAYER as plumbed rather than
confirmed.

## Objective (`MCF_Obj_Objective`)

A narrative task with a title and description.

- **Intel Gate Event** — leave empty and the objective starts available; set it
  and the objective stays locked until that event fires. A locked objective
  cannot be completed, which is the point of a gate.
- **Complete Event** / **Fail Event** — the events that resolve it.
- **Announce** — on by default; puts unlock, completion and failure on screen.

Completing or failing also publishes `Objective_Complete` / `Objective_Fail`,
so other nodes can react.

**"Visible on Map" does nothing yet.** There is no map integration. An
objective is announced on screen, not tracked in the player's task list.

---

# Markers, not triggers

These two appear in the placement browser but have no events. They are support
objects other systems query.

## Lifestyle POI (`MCF_AI_LifestylePOI`)

A location with a role ("Shop", "Well") and a number of occupancy slots. It is
the parking spot, not the person parked there — the ambient-actor system that
would use it is not wired up.

## Safe Fallback Point (`MCF_AI_SafeFallbackPoint`)

Place near a spot where AI reliably gets stuck, such as beside a vehicle door.
The AI Command Watchdog picks the nearest one when it force-corrects a stuck
entity. Registers itself; nothing to configure. If none exist, the Watchdog
simply has nowhere to send anything.

---

# Setting up a scenario

Place **one** Tick Manager and **one** Game Loop, on the same entity, plus the
Game Mode Starter. Together they are MCF's heartbeat; without them, nothing
that ticks will run.

**Do not drag a separate GameMode prefab into your world.** A world has exactly
one GameMode entity and Workbench will refuse a second. Instead use Workbench's
own **Plugins → Game Mode Setup** wizard, which generates everything a working
GameMode needs (Faction Manager, Loadout Manager, AI World and the rest). Then
find the GameMode entity it created and add these components to it directly:

- `MCF_Core_GameModeComponent` — starts MCF at mission start
- `MCF_Core_TickManagerComponent` — the heartbeat
- `MCF_Core_GameLoopComponent` — drives the heartbeat every frame
- `MCF_UI_LineDisplayComponent` — puts MCF's text on players' screens

The wizard's generated MapEntity ships with empty map texture fields. Fill both
in, or the game crashes the moment a player opens the map. That is a vanilla
issue, not an MCF one.

---

# Background systems

Not placeable nodes, and largely unexercised — assume rough edges.

**Hostility** tracks a 0-100 tension value per area that Civilian Behavior and
Compliance read. No placeable node; raising and lowering it happens from other
systems. Decay toward zero can be enabled on the Game Mode Starter.

**Civilian Behavior Hook** reports "neutral", "fearful" or "hostile" for its
area, but does not change the civilian's behaviour on its own.

**Compliance** decides whether one person gives in when shouted at. It is no
longer something a menu entry calls; the shout system drives it. See "Shouting,
surrender and escort" below.

**AI Command Watchdog** detects AI that has not moved for a while and
force-corrects it to the nearest Safe Fallback Point.

**Simple Mover** walks an entity toward a position in a straight line — a
stand-in for pathfinding, so it walks through obstacles. For anything in a real
AI group, use the native waypoint system instead.

**Waypoint Animation** publishes an animation request and plays nothing. For a
real AI group, use Arma Reforger's own `SCR_AIAnimationWaypoint`, which is built
for exactly this.

**Native AI Waypoint Bridge** adds a tagged native waypoint to a tagged AI
group when an event fires — the proper route for moving real AI groups.

**Interaction Hint** gives an NPC a few lines players can ask for. Add an
`MCF_Interact_TalkAction` to the same entity for the "Talk" prompt.

**Sequence Recorder / Playback** records a path plus timed cues and replays
them. Recording still has to be driven manually; playback runs itself.

**Squad Cohesion** offers position sharing within a squad, a muster gate
("everyone within X metres of a point"), and a radio-respawn hint. All off by
default.

**Budget Config** sets caps like `AIGroup:20`. It stores the limit; it does not
enforce it.

---

# When something does not fire

## MCF checks your event names for you

At mission start MCF compares every event nodes *listen for* against every
event nodes *publish*, and warns about any that nothing sends:

```
(W): [MCF] event "VilageAlerted" is never published, but is expected by
     MCF_Obj_LogicComponent (output 'Alarm', AND input 1)
(W): [MCF] Event validation found 1 event name(s) nothing publishes
```

That is a typo caught before you go looking for it — the warning names both the
misspelled event and which node expects it. When everything lines up you get:

```
[MCF] Event validation passed -- every consumed event has a publisher
```

Note it only catches listeners with no publisher. A node publishing an event
that nothing listens to is not reported, because that is often deliberate.

## Reading the trace

Every node writes a line when it initialises, saying what it listens for and
what it publishes:

```
[MCF] Recipe init, listening for ReportReady with 1 steps
[MCF] ProximityTrigger init, radius=15 event=PointA
[MCF] TriggerZone init, activated by 'PointA' publishes 'ZoneFired'
```

and another when it fires:

```
[MCF] ProximityTrigger FIRED, publishing PointA
[MCF] Logic AND state: 1=true 2=false 3=false 4=false
```

Reading these top to bottom shows exactly where a chain stops. That
`Logic AND state` line is particularly useful: it tells you which inputs have
arrived and which you are still waiting on.

If a node does not appear in the log at all, it is not in the world, or you are
reading a client's log instead of the server's — detection and logic run on the
server only.

## Turning the trace off

It is on by default, because this framework is young enough that you want it.
For a production server that would rather not have the chatter:

```
MCF_Core_Log.SetEnabled(false);
```

Warnings and errors ignore that switch, so validation problems still surface.

---

# Talking to people

Every character in the game can be talked to — not a special MCF prefab, every
character, vanilla or modded, in any faction. MCF overrides vanilla
`Character_Base`, so there is nothing to place and nothing to configure before
it works. Walk up to any AI and look at its chest.

Each person carries two numbers about the players, **trust** and **fear**, and
what they are willing to say depends on those numbers. Fear is not stored and
updated by something; it is their personal fear plus the live hostile share of
the area they are in, so a firefight in the village frightens everybody in it
without any node pushing a value around.

A conversation is nodes (what they say) and choices (what you may say back).
Each choice can require a minimum trust, a maximum fear, or a flag set earlier
in the same conversation; can change trust and fear; and can **publish an MCF
event**. That last one is how a conversation causes anything — an informant who
finally gives up the cache location fires `CacheLocationKnown`, and from there
it is an ordinary event you can hang an Intel Source or a Recipe on.

Conversations live in a **library**, not on each person, so you write one and
put it on twenty villagers. Game Master can assign one to a selected character,
edit the library, or create a new entry from the character you have selected.
Edits apply immediately.

A character has two conversation slots: a normal one and an interrogation one.
The interrogation conversation is offered only once somebody is restrained.

⚠️ Conversation flags, trust and fear do not survive a server restart.

# Shouting, surrender and escort

Raise your weapon and press **H** to shout for surrender, or **U** to tell
people to keep their distance. A shout carries 25 metres and everybody inside
that sphere is asked *individually*, so a squad can break unevenly.

Each person weighs how far away you are, whether your weapon is raised, their
own fear, and whether they are armed. Every weight is editable in Game Master on
`MCF_AI_ComplianceComponent`: Fear Weight, Armed Resistance, Weapon Raised
Weight, Fear On Surrender, and a Base Compliance Chance everything else
modifies. Set Armed Resistance high for soldiers and it stops mattering for
civilians, which is how you get "civilians give up, soldiers usually do not"
without two separate systems.

**Punish Unjustified** is on by default: screaming at unarmed civilians for no
reason raises the area's Hostility, which raises everybody else's fear. Turn it
off for anyone who is fair game.

Somebody who gives in drops their weapon and holds position. Walk up and you can
Restrain them, Interrogate them, Release them, or escort them — Follow (they
trail you) or Lead (they walk in front and you steer). An **unrestrained**
escortee rolls every second for a chance to break away, and publishes
`MCF_AI_SubjectEscaped` when they do. A restrained one never rolls. That is the
whole trade.

⚠️ Restraining works but has no animation yet. Arma Reforger ships no surrender
or restrained pose at all, and the obvious workaround crashes the Workbench, so
this needs a purpose-built animation clip.

# The operations board

`MCF_Task_Board` is placeable. It shows taskings and intel, with a read/amend
split — seeing an entry and being allowed to change it are separate rights —
and it shows you what role the system thinks you have, so a wrong answer gets
noticed.

⚠️ Role restrictions are written but currently switched off: everybody can do
everything until roles have been watched in a real session.

Intel carries its provenance ("Handwritten letter", "Radio intercept",
"Captured map") and a faction key. A record with a key is visible only to that
faction; one without is visible to everyone.

`MCF_Intel_SourceComponent` turns any event into intel, so intel becomes a
consequence of play rather than something placed in advance. **DROP** spawns a
document somebody must find and carry back; **SIGNAL** enters it on a faction's
board directly and requires a faction key. Leave **Once** on unless you really
want one document per trigger crossing.

⚠️ Faction-scoped intel needs a two-faction test that has not been run, and
dropped intel objects do not survive a server restart.

---

The wiki at <https://github.com/olmopje/milsim-creator-framework/wiki> covers
all of this at more length, with a troubleshooting page.
