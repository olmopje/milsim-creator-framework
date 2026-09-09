# Multiplayer: MCF has none, and every message currently has no audience

Status: **open, and structural.** Not urgent for single-machine testing, but it
gets more expensive to fix with every node added.

## The finding

A search across every MCF script for `Replication.`, `RplRpc`, `Rpc(`,
`IsServer`, `IsClient`, `Authority` or `Broadcast` returns **nothing**. There
is no network awareness anywhere in the framework.

So the question "is an objective notification broadcast to all players, or only
to the one who completed it, or to their group?" has no designed answer today.
It is neither. It is whatever falls out of running unguarded script on every
machine.

## What that means in a real session

The GameMode entity exists on the server and on every client, and
`MCF_Core_GameLoopComponent` drives the tick wherever it lives. With no server
guard, every machine runs its own independent copy of the whole chain: its own
proximity check, its own trigger fire, its own recipe, its own objective state,
its own popup.

Consequences to expect:

- **Objective state is per-machine, not per-mission.** One player's client can
  consider an objective complete while another's does not.
- **Trigger-once means once per machine.** Not once per mission.
- **The server runs the presentation too**, calling `PopupMsg` where there is no
  screen.
- Each client only evaluates against entities its own world knows about, so
  triggering can differ between clients.

None of this showed up in testing because Workbench play mode is a single
machine acting as both server and client. That hides the problem completely.

## The vanilla pattern to follow

From `scripts/Game/Tasks/Editor/SCR_EditorTask.c`, already read:

```c
Rpc_PopUpNotification(text, true);
Rpc(Rpc_PopUpNotification, text, true);

[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
protected void Rpc_PopUpNotification(string prefix, bool alwaysInEditor)
{
    // ...resolve the local player's faction...
    if (IsTaskAssignedTo(SCR_TaskExecutor.FromLocalPlayer())
        || GetOwnerFactionKeys().Contains(playerFactionKey)
        || (alwaysInEditor && !SCR_EditorManagerEntity.IsLimitedInstance()))
    {
        SCR_PopUpNotification.GetInstance().PopupMsg(...);
    }
}
```

And from `SCR_TriggerTask.EOnInit`, the authority guard:

```c
if (Replication.IsServer()) { ...wire up the trigger... }
```

The shape is: **authoritative logic server-side, presentation broadcast to
everyone, audience decided locally on each client.**

## The design decision to make

The MCF event bus (`MCF_Core_EventManager`) is a plain script singleton — local
to one machine. There are two ways forward:

1. **Keep the bus local, network only the presentation.** Run trigger and logic
   nodes server-side only (`Replication.IsServer()` guard in the tick paths),
   and give the line/notification layer an RPC with an audience filter. Much
   smaller change, matches vanilla, and keeps the bus simple. Recommended.
2. **Network the event bus itself.** Every published event replicates. Far more
   invasive, much more traffic, and it makes every node a networking concern.

Under option 1, the line queue and `MCF_UI_LineDisplayComponent` become the
single place where audience is decided — which is the right seam, because
everything player-facing already funnels through them.

## Audience levels worth supporting

Whatever is built should let a mission maker choose per message:

- everyone in the mission
- one faction
- one group
- only the player who caused it
- Game Master only (useful for debugging and for GM-facing status)

## Recommended sequence when picked up

1. Add a `Replication.IsServer()` guard to the tick/fire paths of the trigger
   nodes, so logic stops running redundantly on clients.
2. Give the line layer an audience enum plus a broadcast RPC that filters
   locally, following the `SCR_EditorTask` pattern above.
3. Only then consider real task-system integration
   (`docs/research/objective-task-system.md`), since vanilla tasks bring their
   own faction/assignment model that the audience work should line up with.

Test properly on a dedicated server. A Workbench play session cannot show any
of these bugs.
