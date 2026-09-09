# Project Status

Supersedes `PHASE0_PROGRESS.md` (kept below for historical environment notes). Last updated after completing the full phased roadmap (ARCHITECTURE.md section 9, Phases 0-14) plus a follow-up session closing several "manual driver" gaps.

## Where things stand

Every phase in the roadmap has a working, confirmed-compiling implementation. See `CHANGELOG.md` [0.2.0] for the full file-by-file list. This is still first-version work throughout -- crude but functional, not polished -- consistent with how the project was built: get something real and testable first, refine later.

**What now runs automatically** (no longer needs a manual driver call):
- `MCF_Core_TickManagerComponent`, via `MCF_Core_GameLoopComponent`'s `EOnFrame`
- `MCF_AI_CommandWatchdogComponent`, via `MCF_Core_TickCritical`
- `MCF_Hostility_Manager` decay, via `MCF_Core_TickCosmetic` (once `StartAutoDecay()` is called)
- `MCF_React_SequencePlaybackComponent`, via its own `EOnFrame` -- and it now actually relocates its owner along the recorded path, not just exposes position data
- `MCF_AI_SimpleMoverComponent`, via its own `EOnFrame` while `MoveTo()` is active
- `MCF_AAR_DebriefManager` and `MCF_Hostility_Manager` decay can now both be started automatically at mission start by adding `MCF_Core_GameModeComponent` to the GameMode entity, instead of needing a manual kickoff call from somewhere
- `MCF_Obj_ProximityTriggerComponent` and `MCF_Obj_SpottedByPlayerComponent` auto-register every newly spawned controllable entity as a watcher, via `MCF_Core_AutoWatcherRegistry` + `MCF_Core_GameModeComponent.OnControllableSpawned()` -- no more manual `RegisterWatchedEntity()`/`RegisterWatcher()` calls needed for the common case. Note: this fires for AI-controlled entities too, not confirmed player-only.
- `MCF_Interact_HintComponent` now has a real player-facing trigger: `MCF_Interact_TalkAction`, a `ScriptedUserAction` that shows a "Talk" prompt and calls `Interact()` -- this was the missing link between a placed NPC and an actual player pressing a button.

**What still needs a manual driver:**
- `MCF_React_SequenceRecorderComponent.RecordSample()`/`RecordCue()` (recording is an authoring-time action, not live gameplay -- lower priority than the pieces above)

## Confirmed-working engine APIs (no longer need re-verifying)

- `EOnInit`, `OnDelete`, `EOnFrame` + `SetEventMask(owner, EntityEvent.FRAME)`
- `GetGame().GetWorld().GetWorldTime()`
- `Math.RandomFloat01()`, `Math.RandomInt()`, `Math.Clamp()`, `Math.Acos()`, `Math.RAD2DEG`
- `vector.Distance()`, `vector.Dot()`, `.Normalize()`
- `ScriptInvoker` (via `GetInvoker().Insert()/.Remove()` -- **not** `ScriptInvokerBase` as a bare parameter type, that does not compile)
- `string.Split()`, `string.Format()`, `.ToInt()`
- `FindComponent()`, `Cast()`
- `SCR_EditableEntityComponent` requires a `RplComponent` with its own GUID to avoid a "missing RplComponent" error on placeable prefabs

## Confirmed native systems found via research (not yet integrated)

- **`SCR_AIGroup.AddWaypoint(waypoint)`** -- the real way vanilla AI groups move, confirmed via actual base-game source (`SCR_AmbientPatrolSpawnPointComponent.c`). Our `MCF_AI_SimpleMoverComponent` predates this discovery and is a straight-line `SetOrigin()` fallback for entities that are **not** in a real `SCR_AIGroup` (e.g. a standalone civilian) -- for anything with a proper AI group, use native waypoints instead of our mover.
- **`SCR_AIAnimationWaypoint`** -- a native `SCR_AIWaypoint` subclass specifically for triggering an animation on arrival, confirmed to exist alongside `SCR_DefendWaypoint`, `SCR_SuppressWaypoint`, etc. This is the correct tool for "AI plays an animation here", not something we need to build ourselves. `MCF_AI_WaypointAnimationComponent` remains the event-only fallback for entities outside an AI group.
- A speculative guess at `CharacterControllerComponent.PlayGesture()`/`CanPlayGesture()` for directly triggering a gesture from script did **not** compile ("Undefined function") -- reverted rather than guessed further. `CharacterControllerComponent`, `CharacterAnimationComponent`, and `AITaskPlayGesture` are all confirmed to exist and gestures are a real, working system in the base game (used in the training mission), but the exact current script-callable entry point was not found through available research tools. Next lead, if picked up again: check the Script Editor's live autocomplete on `GetAnimationComponent()`, or look at how `AITaskPlayGesture` (an `AITaskScripted`-style node) is wired into a behavior tree in the Behavior Editor -- that's a Workbench GUI task, not pure script.
- **`Prefabs/Systems/Milsim.et`** -- our own GameMode prefab, inheriting from the confirmed vanilla base `{1B76F75A3175E85C}Prefabs/MP/Modes/Plain/GameMode_Plain.et`, with `MCF_Core_GameModeComponent`/`MCF_Core_TickManagerComponent`/`MCF_Core_GameLoopComponent` added. Building it this way (a `prefab_create` with the correct `parentPrefab` GUID) was necessary -- using `prefabType: "gamemode"` with no parent produces a `GenericEntity`-rooted prefab that fails at runtime ("required type=SCR_BaseGameMode! This is not allowed!").
  - **Two Workbench crashes (access violations) happened testing this live in-world**, both while creating/loading an entity from this prefab. Root cause: `prefab_create` had also emitted an empty `SCR_RespawnSystemComponent {}` override, which reset the parent's already-correct spawn configuration instead of leaving it alone, producing "SCR_RespawnSystemComponent is missing SCR_SpawnLogic!" followed by a crash. Fixed by removing that empty override (and an invalid `m_sDisplayName` block) from the file.
  - **A world only ever has one GameMode entity, and Workbench's Resource Browser correctly refuses to let you drag a second one in** -- this is not a bug, it's what stopped us from creating the duplicate-gamemode conflict a second time. Placing a whole separate GameMode prefab (ours or vanilla) into a world is the wrong mental model entirely.
  - **The correct, confirmed-working approach: use Workbench's own "Game Mode Setup" plugin** (Plugins menu), which runs a wizard that scans the world, auto-generates every required entity (`SCR_FactionManager`, `SCR_LoadoutManager`, `SCR_AIWorld`, `PerceptionManager`, etc. -- the same list BI's own General Game Mode Setup wiki page describes) in one step, and creates a mission header (`.conf`) pointing at a complete, working GameMode entity (e.g. `SCR_GameModeEditor` for a Game Master scenario). **Then add our three MCF components directly to that wizard-generated GameMode entity** (`wb_component add`, same as any other component) instead of trying to swap in a separate prefab. This is what actually worked, confirmed zero errors, world saved successfully. `Prefabs/Systems/Milsim.et` is consequently no longer the recommended path for a real scenario -- kept for reference/reuse in a from-scratch prefab-based setup if ever needed, but the wizard+add-components route is simpler and proven.
  - A leftover "Multiple game mode entities present!" warning citing a phantom `oldEntity: ENTITY:0 ('SCR_BaseGameMode') at 0,0,0` appeared consistently across every world tested, including brand new ones, **without causing a crash** when the rest of the setup was correct -- this looks like a benign Workbench-internal editor artifact, not a real duplicate, based on it appearing even when only one real GameMode entity existed.
  - The wizard's auto-generated `MapEntity` ships with empty "Map Geometry Data" and "Satellite background" fields, which the wizard itself flags ("MapEntity: Set up map textures") but doesn't block on. Leaving them empty causes a **NULL pointer Virtual Machine Exception in native `SCR_MapEntity::UpdateViewPort`** the moment a player opens the in-game map -- not an MCF bug, confirmed by the stack trace being entirely inside `Scripts/Game/Map/`. Fix: select the MapEntity, and fill in both fields via their `..` browse button with the map's own data (e.g. Arland-specific map geometry/satellite resources for an Arland-based world).
- **"Spotted" detection (an AI noticing the player via its own perception, not a distance/cone approximation) is not built.** The native `PerceptionManager`/`EAIDangerEventType` system (seen in the Game Mode Setup wizard's auto-generated entities and the AI script API's enum group) is presumably the real hook for this, but the exact script-callable entry point hasn't been researched/confirmed yet -- same category of gap as the gesture-trigger API. `MCF_Obj_ProximityTriggerComponent`/`MCF_Obj_ConeDetectionTriggerComponent` cover the geometric approximations (distance, facing direction); genuine AI-perception-based detection is a distinct, still-open follow-up.
- **All hand-written `SCR_EditableEntityComponent { m_EditableEntityFlags "PLACEABLE" m_sDisplayName "..." }` blocks in our prefabs are wrong and silently do nothing** -- confirmed via `WORLD (E): Unknown keyword/data 'm_EditableEntityFlags'`/`'m_sDisplayName'` on placement. This affects every prefab we've hand-written this way (all 8+ `.et` files under `Prefabs/`), not just new ones -- earlier ones just happened not to surface this specific error message when we checked them.
  - **This does NOT block using our nodes at all** -- every prefab still places and works fine via Workbench (`wb_entity_create`, or dragging in the World Editor) despite the flag being silently ignored. The flag only matters for a player using the **in-game, live** Game Master placement menu during actual gameplay to spawn a new instance from scratch -- Workbench-time placement while building a scenario is unaffected.
  - Attempted the BI-documented fix (Workbench's "Create/Update Selected Editable Prefabs" plugin, which needs an `EditablePrefabsConfig.conf` folder-rule mapping our `Prefabs/` source to a `PrefabsEditable/` target) but could not get it working: the base game's own config (at `ArmaReforger:Configs/Workbench/EditablePrefabs/EditablePrefabsConfig.conf`) has no MCF entry, and creating a local override in `MCF:Configs/Workbench/EditablePrefabs/EditablePrefabsConfig.conf` with a guessed `m_aFolderRules { SCR_EditablePrefabsFolderRule { m_SourceDirectory ... m_TargetDirectory ... } }` structure failed with "Unknown keyword/data 'm_aFolderRules'" -- the guessed class/field names are wrong, and no source confirms the correct ones. **Reverted the override to empty** rather than leave a broken guess in the repo.
  - **Open follow-up, not currently blocking:** find the correct `EditablePrefabsConfig`/`SCR_EditablePrefabsFolderRule`-equivalent schema (Script Editor's live autocomplete on a fresh `EditablePrefabsConfig {}` block would likely reveal the real field names immediately -- faster than more external research) so our prefabs can eventually support live in-game Game Master placement too.

## Deliberately unsolved, by design (not oversights)

- **No raycast/navmesh API was ever confirmed.** Rather than guess, self-built workarounds were built instead of the missing pieces we could reasonably approximate:
  - `MCF_AI_ComplianceComponent.IsBeingAimedAt()` -- distance + angle approximation, not true line-of-sight
  - `MCF_AI_FallbackPointRegistry` -- mission-maker-placed safe points instead of a geometry query
  - `MCF_AI_SimpleMoverComponent` -- straight-line movement via `SetOrigin()` each frame, not real pathfinding; used by Sequence Playback and Ambient Actor so things actually move now instead of just publishing an event. **Superseded by native `SCR_AIGroup` waypoints for anything with a real AI group** -- see above.
- **AND logic** (`MCF_Obj_LogicComponent`) is capped at 4 fixed input slots, not an arbitrary list -- Enforce Script has no closures to generate per-input callbacks dynamically.
- **Animation is still not wired up from our own components** (`MCF_AI_WaypointAnimationComponent` still only publishes a "this animation was requested" event) -- see the native `SCR_AIAnimationWaypoint` note above for the actual path forward with real AI groups.

## Reload/compile-check workflow reminder

`enfusion-mcp:wb_reload` is frequently flaky -- it often reports success without a fresh compile actually appearing in the log. When that happens: retry a few times with 15-20s waits, and if it stays stuck, ask whoever is at the PC to click into the Workbench window (giving it focus reliably un-sticks it). Always confirm via the log's `Compiling Game scripts took` / `SCRIPT (E)` lines, never trust the tool's "Reload Complete" message alone.

After many reload cycles in one long session, Workbench can pop an Enfusion engine "Assertion failed: Resources are leaking!" dialog (in `GameApp.cpp`, unrelated to any of our own script files). Clicking **Retry** has been observed to resolve it and let Workbench continue working normally -- this is an engine-internal resource-tracking assertion, not something caused by MCF code.

---

# Historical: original Phase 0 handoff notes

The section below is the original handoff document written after Phase 0 alone. Kept for the environment-setup context (Workbench launch quirks, MCP tool history); the "what compiles" and "not yet started" sections are outdated -- see above instead.

## Environment, confirmed working

- Repo: `G:\MCF` (git, `main` branch)
- Workbench project: `G:\MCF\addons\MCF\addon.gproj`
- **Launching Workbench reliably only works when the user starts it manually** (Steam UI or a desktop shortcut). Scripted launches (`wb_launch`, direct .exe, `steam.exe -applaunch`) were all unreliable for resolving the base-game addon path.
- Once running (World Editor mode, Net API enabled), `enfusion-mcp` tools work normally.
- `enfusion-mcp:mod_build` also spawns its own process and hits the same launch issue -- use `wb_reload` against an already-running, user-launched instance instead.

---

# MAJOR UPDATE (2026-09-09 session): Game Master live placement -- SOLVED

The `m_EditableEntityFlags`/`m_sDisplayName` issue described below under "Deliberately unsolved" was investigated to full resolution in a marathon debugging session. **In-game Game Master placement of MCF prefabs now works.** The old notes below are kept for historical context but are superseded by this section wherever they conflict.

## The complete, confirmed-working recipe for a placeable "System"-type MCF prefab

Every one of the required pieces below was independently confirmed necessary by controlled A/B testing (adding one piece at a time, or duplicating a known-working vanilla prefab and comparing byte-for-byte). Missing any one of them results in the prefab loading with **zero errors in the log** but never appearing in the Game Master Entity Browser -- this silent-failure mode is what made it so hard to find.

```
GenericEntity {
 ID "<GUID>"
 components {
  <YourGameplayComponent> "<GUID>" {
   ...
  }
  SCR_EditableEntityComponent "<GUID>" : "{996046FE206C699A}Prefabs/Editor/Components/Default_SCR_EditableEntityComponent.ct" {
   m_EntityType SYSTEM
   m_Flags {
    PLACEABLE
    VIRTUAL
    HAS_AREA
   }
   m_UIInfo SCR_EditableEntityUIInfo "<GUID>" {
    Name "Your Display Name"
    m_aAuthoredLabels {
     ENTITYTYPE_SYSTEM
    }
   }
  }
  RplComponent "<GUID>" {
  }
  Hierarchy "<GUID>" {
  }
 }
}
```

**Every piece and why it's required:**

1. **`SCR_EditableEntityComponent` MUST inherit from `Default_SCR_EditableEntityComponent.ct`** (`{996046FE206C699A}Prefabs/Editor/Components/Default_SCR_EditableEntityComponent.ct`), not be declared bare. Confirmed via the official BI wiki + `Configs/Workbench/EditablePrefabs/EditablePrefabsComponent_EditableEntity.conf`, which states entities using this template are "already flagged as PLACEABLE".
2. **`m_EntityType SYSTEM`** must be set explicitly. Default is `GENERIC` (index 0) -- confirmed via `Scripts/Game/Editor/Enums/EEditableEntityType.c`. `GENERIC` items do not show under any Entity Browser filter tab we could find.
3. **`m_Flags` needs more than just `PLACEABLE`.** Decoded from three different working vanilla examples (`E_EditorRestrictionZoneSmall.et`, `E_SpawnPoint_US.et`, `EffectModule_MineField_Small_US.et`) via `game_duplicate` + `wb_resources getInfo`: all three use `PLACEABLE | VIRTUAL` at minimum, with `HAS_AREA` added for anything that represents a zone/trigger radius. `VIRTUAL` ("Entity is represented by virtual objects that have to be updated" -- `Scripts/Game/Editor/Enums/EEditableEntityFlag.c`) is very likely the single most important addition for mesh-less logic entities like ours.
4. **`m_UIInfo` must be `SCR_EditableEntityUIInfo`, not generic `SCR_UIInfo`.** Confirmed by the crash stack trace when it's missing/wrong (`SCR_EditableEntityUIInfo.c:245 Function ExtractEditableUIInfoFromPrefab`).
5. **`m_aAuthoredLabels { ENTITYTYPE_SYSTEM }` inside `m_UIInfo` -- this was the actual missing piece, found last, after everything else above was already correct and the entity still didn't appear.** All three vanilla comparison examples consistently include this label array. It appears the Entity Browser's category filtering reads this label array, not (only) the `m_EntityType` enum value. This is separate from `m_aAutoLabels`, which the wiki explicitly warns not to hand-edit (auto-generated by tooling).
6. **A `Hierarchy` component must be present** on the prefab, even though nothing in our own gameplay logic uses it. All three vanilla comparison examples include one. Omitting it was not individually isolated as a hard blocker, but it was present in every known-working example so keep it.
7. **`.et.meta` file's `Name` field must have the correct, current GUID matching the prefab's actual `ID`.** If you ever change a prefab's GUID (e.g. regenerating it), the `.meta` file does NOT auto-update -- Workbench resolves the resource's true GUID from `.meta`, not from the `ID` field inside the `.et` text. A stale `.meta` produces persistent "Wrong GUID for resource" errors that survive full Workbench restarts and resource-database rebuilds until the `.meta` itself is fixed.
8. **The registry `.conf` (`SCR_PlaceableEntitiesRegistry`) also needs its own correct `.meta` file** (`CONFResourceClass` structure, matching the pattern in any wizard-generated `.conf.meta`, e.g. `Missions/*.conf.meta`). A registry `.conf` with no `.meta` at all produces "Wrong GUID for resource ... in property inherited-name" when `EditorModeEdit.et` tries to reference it.
9. **The full original content of `EditorModeEdit.et` must be preserved when overriding it in your addon.** Using Workbench's "Override in addon" + editing only the one component you care about (e.g. just adding your registry to `SCR_PlacingEditorComponent.m_Registries`) can result in Workbench saving back an incomplete file that's missing dozens of other unrelated vanilla components (`SCR_CameraEditorComponent`, `SCR_ContentBrowserEditorComponent`, etc.) -- this alone causes catastrophic, hard-to-diagnose editor UI crashes ("Cannot find editor component 'SCR_PlacingEditorComponent', local instance of editor manager not found!" plus dozens of `NULL pointer to instance. Variable 'menu'` exceptions across every editor toolbar). Fix: read the real vanilla file via `game_read`, and write back its **full** content plus your one addition, never a partial diff.

## Debugging techniques that got us there

- **`game_duplicate`** (from the `enfusion-workbench-mcp`/Goldwep fork -- see below) to pull a known-working vanilla editable prefab into the mod folder unmodified, then `wb_resources getInfo` on it to see its fully-resolved JSON representation (flags, entity type, UI info structure) side by side with our own. This "find a working example, diff against it" approach succeeded where guessing from documentation alone had failed for hours.
- **A/B control test:** add the *unmodified* vanilla duplicate to our own registry and confirm it also doesn't show up before concluding our components were the problem -- this correctly ruled out registry-wiring issues at one point and redirected effort to the component definitions themselves.
- **`resolve_guid`** to check whether a suspicious/stale GUID seen in an error message corresponds to any real, currently-indexed resource (it didn't -- confirming it really was stale data, not a legitimate conflict).
- **Reading the actual `.c` source of `SCR_EditableEntityComponent`/`SCR_EditableEntityComponentClass`/`EEditableEntityType`/`EEditableEntityFlag`** directly via `game_read` was far more reliable than any wiki page -- e.g. the wiki never mentions `m_aAuthoredLabels` needing a value for the browser filter to work; the vanilla `.et` examples were what actually revealed it.

## MCP tooling change

Switched from the original `enfusion-mcp` (Articulated7, npx-based) to **`enfusion-workbench-mcp`** (Goldwep fork, cloned from `https://github.com/Goldwep/enfusion-workbench-mcp.git`, built locally at `G:\enfusion-workbench-mcp`). 112 tools vs. the original's much smaller set. Key additional tools that were decisive this session: `prefab` (action=inspect, shows fully-merged inheritance chain with per-value provenance), `game_duplicate`, `resolve_guid`, `wb_diagnose`, `script_class_hierarchy`, richer `api_search`/`component_search`. The other two previously-configured servers (`arma-reforger-mcp`, `arma-reforger-api`) were never actually connected/used and have been removed from `claude_desktop_config.json`.

**Config gotcha:** `ENFUSION_PROJECT_PATH` needs to point directly at the addon root (`G:\MCF\addons\MCF`) for the `project` (read/write/browse) tool family, but `game_duplicate`/`wb_entity_duplicate` want the *parent* of addon folders plus an explicit `modName`. If both are needed in the same session, be aware a write to a `project`-relative path while the env var is set to the wrong level will silently create files in the wrong location (this happened once this session -- files landed in `addons/Prefabs/` instead of `addons/MCF/Prefabs/` -- caught via `git status` showing an unexpected new top-level directory).

## Still open (separate topic, not started)

**In-game Game Master "Scenario properties" panel (per-instance configurable attributes after placement)** -- e.g. Arsenal's placed-instance editor shows "Set faction", "Enable arsenal", weapon toggles etc.; our placed MCF prefabs currently show "No properties, this entity has no properties to edit." This is a distinct system from everything solved above (likely `SCR_AttributesManagerEditorComponent` + a per-component attribute-list config), not yet investigated. Next session starting point: read `Scripts/Game/Editor/Containers/Attributes/SCR_BaseEditorAttribute.c` and find a simple (non-Arsenal) vanilla System-type prefab that has editable instance properties, and diff its component setup the same way the placement fix was found above.