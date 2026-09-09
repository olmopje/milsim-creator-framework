# Idea: overlapping area masks as the AI information layer

Status: **captured, not designed.** Raised by the project owner 2026-09-09 for
the AI interaction layer, explicitly to be looked at later. Written down now so
it is not lost, with the hooks it would connect to.

## The idea

Overlapping area masks, with information flows woven into them. Where masks
overlap they connect, and hierarchies can be used.

## Why it is interesting

It is a **different connection model from what MCF has today**, and that is the
point rather than a problem.

Right now nodes connect by explicit event name: a mission maker types
`VillageAlerted` in two places and that is the link. Every connection is
authored by hand, and the topology is whatever someone typed.

Area masks invert that. Connection becomes **implicit from geography**: two
areas that overlap can pass information to each other, without anyone wiring
them together. A sentry in one sector notices something; it propagates to the
adjacent sector because they overlap, not because a mission maker drew a line
between them.

That is much closer to how information actually moves through a force, and it
scales in a way hand-wiring does not — twenty overlapping areas need no
authoring, twenty hand-wired events need forty correctly-typed names.

The hierarchy part matters too: areas containing sub-areas maps naturally onto
command structure, which is the same hierarchy the audience model needs
(`multiplayer-and-audience.md`) and the same one the task system delegates
along (`mcf-task-system-design.md`). One structure, several uses.

## It is not foreign to what exists

MCF already has a primitive version of this, barely used:

- `MCF_Hostility_Manager` tracks a tension value **per area key**.
- `MCF_AI_CivilianBehaviorHookComponent` takes an Area Key and reads that
  area's tension to report neutral/fearful/hostile.

So "areas carry state that entities read" is already the model — it is just
flat, string-keyed, and has no notion of overlap, propagation or nesting. This
idea is the generalisation of something already half-present.

## The design question to answer first

**How do area masks and the event bus coexist?** They are two different ways
for things to connect, and having both without a clear rule for which does what
would make the framework harder to reason about, not easier.

A plausible split, to be tested rather than assumed: the event bus stays the
explicit, authored spine (this trigger causes that outcome), and area masks
carry ambient, propagating state (what is known here, how tense it is, who has
seen what). Discrete causation versus continuous awareness.

## Open questions for when this is picked up

- What is a mask, concretely? A shape in the world, a config entity, or a
  volume component on an entity?
- What propagates across an overlap: everything, or typed channels
  (intel/alarm/tension) with different rules and rates?
- Does propagation cost time? Instant spread is simpler; delayed spread is far
  more interesting for milsim, since information outrunning the enemy is the
  whole game.
- Does the hierarchy come from geometry (nesting) or is it declared separately
  and merely correlated with geometry?
- How does a mission maker see and edit this? Overlapping invisible volumes are
  hard to author blind — this likely needs the same visualisation work the
  radius indicator needed and did not get.
