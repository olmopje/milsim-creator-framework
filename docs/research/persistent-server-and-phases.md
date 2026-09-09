# Design note: the persistent-server model

Status: **design captured, not started.** Extends
`docs/research/mcf-task-system-design.md`. Grounded in research; sources at the
bottom.

## The proposal

- A unit runs its server all week, not only during operations.
- Command connects to that server and plans **from an in-game headquarters**,
  doing what a web planning tool would let them do — but diegetically.
- The plan **saves on the server**.
- When the operation starts, the plan is shown to everyone in a **briefing
  room**, potentially with a live map display.
- Units define fixed squads, roles and loadouts from Game Master mode; players
  pick a slot on joining and immediately get the right role, squad and kit. The
  GM designates which squads hold commander rights.

## This overturns the earlier recommendation

`mcf-task-system-design.md` concluded: build for execution, not planning,
because units plan out of game and an in-game planner would compete with
TacOps and lose.

That reasoning assumed planning has to be out of game. This proposal removes
that assumption. Planning inside a persistent world, in a physical HQ, that
persists to a briefing room later, is **not the same product as TacOps** — it
is a category a browser tool cannot enter. The differentiator is not the
planning features; it is that the planning is diegetic and persistent.

The earlier advice stands only for the narrow case of replicating map-drawing
in game. It does not stand against this.

## The new architectural concept: phases

One persistent server with distinct phases, each with different permissions,
views and available actions:

- **Planning** — command-level only. HQ is active. Plans are authored and saved.
- **Briefing** — plan is read-only and displayed. Players slot in, see their
  assignment.
- **Operation** — the plan is carried and tracked; changes happen as fragmentary
  orders.
- **Debrief** — outcome recorded against the plan. MCF already has
  `MCF_AAR_DebriefManager`, currently unconnected to any of this.

Phase is mission state, not a mode switch, and determines who may author what.
This is the organising idea the whole system should be built around.

## Persistence: what the engine gives us

Arma Reforger has had built-in persistence since **1.6**. Arkensor's Enfusion
Persistence Framework is deprecated as a result, explicitly because the base
game now covers it.

Two storage tiers:

- **Session storage** — world state (bases, structures, vehicles, environmental
  changes). Survives server restarts, until the mission concludes.
- **Game mode storage** — data that outlives individual sessions: "player
  experience, currencies, unlocks and similar progression."

Operational facts:

- Entirely server-side. Off by default on a new server; enabled with
  `-loadSessionSave`.
- Autosaves every 10 minutes by default, retaining 10 save points. Both
  configurable in `config.json` (`autoSaveInterval` 0-60, `saveRetention`
  1-128).
- Objects are identified by deterministic UUIDs, valid "as long as the map has
  not changed significantly."

### ANSWERED: yes, and not via the engine's save system

Verified on a real dedicated server on 2026-09-09. A mod can persist arbitrary
custom data across restarts using plain file IO:

```c
FileHandle file = FileIO.OpenFile("$profile:MCF_store.txt", FileMode.WRITE);
file.WriteLine("key=value");
file.Close();
```

`FileIO` (`scripts/Core/generated/System/FileIO.c`) exposes `OpenFile`,
`FileExists`, `MakeDirectory`, `DeleteFile`, `CopyFile` and `FindFiles`.
Writable prefixes are `$profile:`, `$logs:` and `$saves:`. `$profile:` resolves
inside the directory passed to the server with `-profile`, so we own it.

Proven by `MCF_Core_PersistentStore`, called from
`MCF_Core_GameModeComponent.OnGameModeStart()`. Two consecutive server runs:

```
run 1:  PersistentStore: no store file yet -- starting empty
        (file written: serverRunCount=1)
run 2:  PersistentStore: loaded 2 entries from $profile:MCF_store.txt
        PersistentStore: this server has started 2 times -- previous runs survived restart
```

This is better than relying on the engine's persistence, because it is entirely
decoupled from world and session saves — which is exactly what the risk below
demands.

Note `GameSessionStorage` (`scripts/Core/generated/System/GameSessionStorage.c`)
is a different thing: a static string map that survives *script and addon
reloading within one executable run*, not restarts. Useful for surviving a
script reload during development, not for cross-session data.

### A risk that shapes the design

Mod changes can break save compatibility — adding, removing or updating mods
that alter placed entities can invalidate a save, making "a large mod change"
effectively a world reset.

A unit running a server all week **while the mod is still being developed** is
exactly the case where this bites. A week of planning must not be wiped by a
mod update.

**Design consequence: store plans as data, not as world entities.** Keep the
plan out of world/session persistence and in its own record store keyed by
mission or operation, so iterating on the mod cannot destroy it. This is a
strong argument for the fallback storage route regardless of what the built-in
system turns out to support.

## Slotting: a confirmed gap

Role-based player slot selection is an **open feature request on Bohemia's
feedback tracker (T177278) with no developer response**. The request describes
exactly this use case: mission creators place playable groups, those groups
appear in the role selection screen, and players pick a specific role within a
group so compositions stay balanced.

The ticket also notes the one existing community mod "introduces custom UI
changes and lacks compatibility with vanilla task creation systems."

That incompatibility is an argument *for* this design rather than against it:
if slotting has to be owned anyway, and the existing solution breaks vanilla
tasks, then owning slotting and tasks together as one coherent system is the
sound choice. It also means MCF is not duplicating something vanilla is about
to ship.

What exists and can be read rather than rebuilt: `SCR_GroupsManagerComponent`
(56 KB) and `SCR_PlayerControllerGroupComponent` (62 KB) already provide
groups, membership, roles and leadership. The 2-7 TSLF framework covers group
role lists, size caps and per-group radio frequencies, and could be a
compatibility target rather than a competitor.

## What this makes MCF

Worth stating plainly, because it changes the project's scope: this is no
longer a mission-scripting framework. It is **unit infrastructure** — a
persistent operations environment covering planning, slotting, briefing,
execution and debrief.

That is a much larger and more valuable thing, and it should be entered
deliberately rather than by drift.

## Open questions to settle before building

1. Can the built-in persistence store arbitrary custom records? (Blocking.)
2. Is the planning phase a separate server session from the operation, or the
   same session in a different phase? Affects whether world state must persist
   or only plan data.
3. Does the HQ need to be a real location on the operation map, or a separate
   planning world? A separate world is cheaper and avoids the command element
   occupying the battlefield during the week.
4. Who may enter the planning phase — is that itself a slotting/permission
   question, and does it reuse the same rights model as commander squads?
5. Does the briefing room display need to be interactive, or is a rendered
   read-only view enough for a first version? Read-only is far cheaper and
   matches how briefings actually run.

## Sources

- Arma Reforger persistence and save games (storage tiers, server flags,
  autosave, mod-change risk):
  https://loafhosts.com/guides/arma-reforger-persistence-and-save-games
- Enfusion Persistence Framework, deprecated as of 1.6 in favour of built-in
  persistence:
  https://github.com/Arkensor/EnfusionPersistenceFramework/blob/armareforger/README.md
- Arma Reforger Persistence System (official wiki):
  https://community.bistudio.com/wiki/Arma_Reforger:Persistence_System
- Role-based player slot selection feature request:
  https://feedback.bistudio.com/T177278
