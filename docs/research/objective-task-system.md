# Objective node: what is done, and what integrating vanilla tasks would take

Status: **partly done**. The node is functional and gives the player feedback.
Real task-list and map integration is scoped below but deliberately not built.

## What was wrong

`MCF_Obj_ObjectiveComponent` was inert in both directions:

- **No input.** `Complete()` and `Fail()` were only reachable from script, and
  nothing in the framework called them. A mission maker had no way to finish an
  objective at all.
- **No output.** `m_sTitle`, `m_sDescription` and `m_bVisibleOnMap` were stored
  and never rendered. `IsVisibleOnMap()` was a getter nobody called.

Same shape as the missing line-display consumer found earlier the same day.

## What was fixed

- New `m_sCompleteEvent` and `m_sFailEvent` attributes. The objective now
  subscribes to them, so it is wired the same way as every other MCF node:
  by event name, no entity references.
- New `m_bAnnounce` (default on). Unlock, completion and failure are announced
  through `MCF_Voice_LineQueueManager`, which surfaces on screen via
  `MCF_UI_LineDisplayComponent`.
- `Complete()` now respects the intel gate: a locked objective cannot be
  completed. A gate that could be completed through would not be a gate.

An objective is therefore **announced, not tracked**. `m_bVisibleOnMap` still
does nothing.

## What the vanilla task system actually is

Researched from base game source, so this does not need re-reading:

- `scripts/Game/Tasks/` — `SCR_TaskSystem.c` (77 KB), `SCR_Task.c` (59 KB),
  `SCR_ExtendedTask.c` (32 KB), plus UI and network components.
- **Tasks are entities**, not components. `SCR_EditorTask : SCR_ExtendedTask`,
  declared with `EntityEditorProps`, spawned from a task prefab.
- State is driven through `SCR_TaskSystem.GetInstance().SetTaskState(task, state)`
  with `SCR_ETaskState.COMPLETED` / `FAILED` / `CANCELLED` / `CREATED`.
  Confirmed in `SCR_TriggerTask.OnTriggerActivate()`.
- `SCR_EditorTask.SetTaskState()` overrides the base to fire notifications and
  a popup, gated on faction and on `EEditorTaskCompletionType.AUTOMATIC`.
- A task carries: a task ID (`SetTaskID`), owner faction keys, an
  `SCR_EditableEntityComponent`, replication (`Replication.IsServer()` guards,
  `[RplRpc]` broadcasts), and optionally a location name.
- The GameMode entity already has the infrastructure:
  `SCR_TaskManagerUIComponent`, `SCR_TextsTaskManagerComponent`,
  `SCR_TaskFinishHistoryManagerComponent`.

## Options for real integration, when picked up

1. **MCF task entity.** An `MCF_Obj_Task : SCR_EditorTask` entity class with its
   own prefab, whose state MCF events drive via `SCR_TaskSystem.SetTaskState`.
   Most idiomatic and gives the full task list, map marker, waypoints and
   networking for free. Cost: faction ownership, task IDs, prefab inheritance
   and replication all have to be got right — this is the heavyweight option
   and carries real risk of destabilising things, which is why it was not done
   in the same pass as the fix above.

2. **Drive a placed vanilla task.** Let a mission maker place a normal editor
   task and have the MCF objective point at it. Cheaper, reuses everything, but
   needs an entity-reference mechanism, which MCF does not have — every node so
   far is wired purely by event name. Introducing entity references is its own
   design decision, not a free move.

3. **Map marker only.** Skip tasks, use `SCR_MapMarkerManagerComponent` (already
   on the GameMode entity) to honour `m_bVisibleOnMap`. Much smaller than either
   option above and would deliver the single most visible missing piece.

Option 3 is the recommended next step if the goal is player-visible progress
for low risk. Option 1 is the right end state.

## Note for whoever picks this up

Both major bugs found today — the `m_Flags` parse failure and the missing
`EntityEvent.INIT` mask — had their answer sitting in the vanilla source.
Read the base game before writing anything here.
