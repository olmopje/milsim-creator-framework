# MCF Mission Maker Guide

This guide explains every placeable MCF node in plain language -- what it does, what its settings mean, and a quick example. No scripting knowledge needed. For the technical design behind all of this, see `docs/architecture/ARCHITECTURE.md`.

**Current status:** everything below is a working first version, not a polished final feature. Some things you'd expect to happen automatically (an NPC actually walking to a waypoint, an animation actually playing) still need a manual trigger from a future piece we haven't built yet. Where that applies, it's called out explicitly.

**One rule that matters everywhere:** many nodes link to each other by typing an *event name* into a text field. If two nodes are supposed to talk to each other, the event name must be spelled **exactly the same** on both sides. If you get it wrong, check the log at mission start -- MCF warns you there instead of failing silently.

---

## Objective (`MCF_Obj_Objective`)
A narrative task. Give it a title and description. Toggle "Visible on Map" to show/hide its marker. If you set "Intel Gate Event," the objective stays hidden until that event fires somewhere else in your mission -- useful for "you need to talk to someone first" setups.

## Trigger Zone (`MCF_Obj_TriggerZone`)
The simplest building block: fires a named event when triggered. Use it as a generic "something happened here" marker that other nodes react to.

## Logic Node (`MCF_Obj_Logic`)
Combines multiple events into one outcome. Set Mode to:
- **"OR"** -- fires as soon as any listed input event happens
- **"COUNTER"** -- fires after a set number of input events, from any of the listed ones, have happened in total
- **"AND"** -- fires once every one of up to 4 fixed input slots (`And Input 1-4`, leave unused ones empty) has happened at least once

Useful for "any of these 3 things ends the mission" (OR), "clear 5 checkpoints before the convoy leaves" (COUNTER), or "only proceed once both the bridge is down AND the radio tower is destroyed" (AND).

## Observation Node (`MCF_Obj_Observation`)
A point of interest that reports when something happens there. Feed several of these into one Logic Node (OR mode) to build a "watch all these locations, react to whichever gets used first" setup.

---

## Hostility (background system, not a node you place)
Tracks a 0-100 tension value per area. Other systems (Civilian Behavior, Compliance) read it. There's no placeable "Hostility node" yet -- raising/lowering it currently has to be triggered from another system. Decay toward 0 over time can be turned on with `StartAutoDecay(ratePerSecond)` -- off by default, and needs a Tick Manager + Game Loop in the scenario to actually run once enabled.

## Civilian Behavior Hook (`MCF_AI_CivilianBehaviorHookComponent`)
Put this on a civilian. Give it an Area Key matching the area you want it to react to. It reports "neutral," "fearful," or "hostile" based on that area's current tension -- but does not yet actually change the civilian's animation or behavior on its own. Something has to read this and act on it.

## Compliance (`MCF_AI_ComplianceComponent`)
Put this on an NPC you want players to be able to force compliant at gunpoint. Toggle "Armed" for a "drop weapon" NPC vs an unarmed "stand back" one. Set the base chance they comply, plus the max distance/aim-angle for `IsBeingAimedAt()` to count a player as aiming at this NPC (a distance+angle approximation, not a true line-of-sight check -- it won't detect a wall between player and NPC). Something still needs to call `IsBeingAimedAt()` then `AttemptCompliance()` when a player presses the interact key. If you mark the attempt "unjustified" (e.g. forcing an unarmed, non-threatening civilian to comply), it automatically dings the area's Hostility.

## AI Command Watchdog (`MCF_AI_CommandWatchdogComponent`)
A patch for AI that gets stuck (won't exit a vehicle, stops following). Runs automatically once placed (as long as one Tick Manager + Game Loop exist in the scenario, see below) -- detects "hasn't moved in X seconds," asks for a retry, then force-corrects to the nearest **Safe Fallback Point** (see next entry) if that doesn't work.

## Safe Fallback Point (`MCF_AI_SafeFallbackPointComponent`)
Place one of these near a spot known to be safe for AI to stand -- e.g. right next to a vehicle door where AI tends to get stuck. The Command Watchdog automatically picks the nearest one when it needs to force-correct. Place a few of these around your mission wherever AI commonly gets stuck; the Watchdog does nothing special if none exist nearby.

---

## Lifestyle POI (`MCF_AI_LifestylePOIComponent`)
A location with a role ("Shop," "Well," etc.) and a limited number of slots. Ambient Actors try to occupy it. Doesn't do anything visual by itself -- it's the "parking spot," not the person parked there.

## Ambient Actor (`MCF_AI_AmbientActorComponent`)
Give it a list of Lifestyle POI tags to cycle between and a free-text archetype label ("Shopkeeper," "Customer," anything you like). Add a Simple Mover component (see below) to the same entity and it will actually walk there (in a straight line, not around obstacles) each time you advance it to the next POI.

## Simple Mover (`MCF_AI_SimpleMoverComponent`)
Moves the entity toward a target position in a straight line at a fixed speed -- our own stand-in for AI pathfinding, since no pathfinding API was confirmed. Put it on the same entity as an Ambient Actor or anything else that needs to walk somewhere. It won't go around obstacles, so keep target positions in open, simple terrain for now.

## Interaction Hint (`MCF_Interact_HintComponent`)
Put this on an NPC players can talk to. Fill in a few generic lines. Optionally set a "Pointer Chance" above 0 to sometimes give a more useful hint instead (fill in the template and target name) -- there's a cooldown so players can't just spam-click for a hit.

## Waypoint Animation (`MCF_AI_WaypointAnimationComponent`)
Marks that an animation should play on arrival. Publishes an event saying which animation was requested -- actually playing it on a character isn't wired up yet.

---

## Text Line (`MCF_Voice_TextLineComponent`)
Displays a line of text (a stand-in for voice lines, which are parked for now). Set a priority -- higher-priority lines jump ahead of ones already waiting.

## Recipe (`MCF_React_RecipeComponent`)
The easiest way to chain a few things together without any scripting. Set a trigger event, then list steps in order, each written as `TYPE:value`:
- `PUBLISH_EVENT:SomeEventName` -- fires another event, so recipes can chain into each other
- `PLAY_TEXT_LINE:Some text here` -- shows a text line
- `REQUEST_ANIMATION:` -- requests the waypoint animation event

Example: an HVT-flees-by-vehicle recipe might be triggered by `"Suspicious"` and have steps `PLAY_TEXT_LINE:Get to the car!` then `PUBLISH_EVENT:HVT_FleeStarted`.

## Sequence Recorder / Playback (`MCF_React_SequenceRecorderComponent` / `...PlaybackComponent`)
Records a path and a list of timed cues (using the same `TYPE:value` steps as Recipes), so you can act out a small scene once and reuse it. Recording still needs to be driven manually (call `RecordSample`/`RecordCue` while recording) -- but playback now runs itself automatically once you call `LoadSequence()`, and it actually moves the entity along the recorded path (straight-line between samples, not a real walk animation) while firing cues at the right moment.

---

## Squad Cohesion (`MCF_Squad_CohesionComponent`)
Everything here is off by default -- you choose what to turn on:
- **Position Sharing:** squad members can see each other's position (never the whole team/army)
- **Muster Gate:** checks whether everyone in the squad is within a set radius of a point -- good for "don't proceed until the squad regroups"
- **Radio Respawn Hint:** fires an event to remind players the radio-respawn mechanic exists

## Budget Config (`MCF_Core_BudgetConfigComponent`)
Set limits like `AIGroup:20` to cap how many of a category can be active at once, so a scenario doesn't accidentally overload the server. Anything that wants to respect a budget has to call into it -- this component only sets the limit, it doesn't enforce it by itself yet.

## Tick Manager (`MCF_Core_TickManagerComponent`) + Game Loop (`MCF_Core_GameLoopComponent`)
Place exactly one of **each** per scenario, on the same entity. Together they're the "heartbeat" other systems listen to instead of checking things every single frame -- and they now run themselves automatically (Game Loop drives the Tick Manager every frame). Set how often the critical and cosmetic ticks fire on the Tick Manager. Several other components (the Command Watchdog, Hostility decay, Sequence Playback) depend on this pair existing somewhere in your scenario to actually run.
