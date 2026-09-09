# Changelog

All notable changes to this project are tracked here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

## [0.3.0] - 2026-09-10 - Talking to, subduing and escorting AI
The framework stops being only a trigger-and-event system and starts being something
the player interacts with directly. Every character in the game -- vanilla or modded,
any faction -- can now be talked to, shouted at, made to surrender, restrained and
walked somewhere.

### Added -- Dialogue
- `MCF_Dialogue_Data`, `_Script`, `_View`, `_Library` -- a server-authoritative
  conversation graph, a text serializer (three separator levels with newline escaping),
  and a runtime-extensible library that persists across restarts
- `MCF_Dialogue_Component` -- drives one conversation per character. Replicates the
  conversation *id*, not its text, so a client raises the interaction prompt from the
  id alone; the server owns every state transition
- `MCF_AI_DispositionComponent` -- trust and fear per character. Fear is a personal
  component plus the live hostile share of the surrounding area, so it moves during a
  fight without anything having to push it
- Replies are gated on those two numbers; outcomes publish MCF events, so a conversation
  can drive the same logic nodes a trigger can
- `MCF_Dialogue_Menu` and `MCF_Dialogue.layout` -- a running-chat screen, newest line at
  the top, deliberately not matching the placeholder styling of the other MCF menus

### Added -- Reaching every faction
- `Prefabs/Characters/Core/Character_Base.et` overrides vanilla `{37578B1666981FCE}`,
  generated mechanically via `game_duplicate` rather than hand-written. Every character
  inherits the dialogue, disposition, compliance and subject-control components, a
  `MCF_Talk` UserActionContext on Spine5, and the Talk / Restrain / Release / Escort
  actions. This replaced a per-faction approach that could never cover modded factions

### Added -- Game Master authoring
- `MCF_ContextActions.conf` and `MCF_EditorAttributes.conf` with the menus under
  `Scripts/Game/UI` -- assign a conversation to a character, edit the library itself,
  and create a new library entry from the character you have selected
- Editor attributes carry 12 bytes and cannot hold text, so a pick travels as a number
  and the text travels by RPC through a custom menu

### Added -- Shout, surrender, restrain, escort
- `Configs/System/chimeraInputCommon.conf` appends an MCF action context to vanilla's
  bindings. A mod adding its own keybind was previously unproven here; it is now
  verified in play. H shouts surrender, U shouts stay back
- `MCF_AI_Shout` -- queries a 25 m sphere and asks each AI in it individually
- `MCF_AI_ComplianceComponent.WillSurrender` weighs distance, whether the shouter's
  weapon is raised, and the subject's own fear, with civilians dialled likelier to give
  in than soldiers. All weights are Game Master configurable
- A subject who gives in drops its weapon and holds position, and can then be restrained
- `MCF_AI_SubjectControlComponent` -- standing orders (follow, lead, stand off), an
  escape roll for the unrestrained, and a marching pace matched to the escort's own velocity
- `MCF_AI_MarchBehavior` -- re-issues combat-move requests without tearing the behaviour
  down, which is what finally removed the arrive-stop-repeat stutter of pathfinding to a
  point four metres ahead

### Changed -- Intel
- `MCF_Intel_CarrierComponent` moved from a broadcast to an `RplProp`, so Game Master
  edits survive late join and streaming instead of being missed by anyone not present
  at the moment of the edit
- `MCF_Intel_Record` gained a faction key, and `MCF_Core_IntelStore` filters on it: a
  board shows only what that faction knows. An empty key on a record means visible to
  all; an empty faction on a player means they see nothing
- `MCF_Intel_SourceComponent` -- turns a trigger into intel, either dropped into the
  world as an object or signalled straight to a faction's board
- `MCF_Task_Permissions.ResolveRole` is real rather than stubbed: Game Master rights,
  then faction commander, then group leader, then soldier. The master override
  `m_bEveryoneMayDoEverything` is still on until roles have been watched in a session

### Removed
- `MCF_AI_SurrenderAction` and `MCF_AI_StandOffAction` -- superseded by the shout path,
  which decides from the shouter's position and the subject's fear rather than from a
  menu entry on each person
- `MCF_AI_ComplianceComponent.AttemptCompliance` -- a second surrender roll that nothing
  ever called, predating `WillSurrender`

### Repository hygiene
- Research notes moved from `addons/MCF/docs/` to `docs/research/`; anything under
  `addons/MCF/` is packed into the shipped addon, so those notes were being distributed
  to every player
- `.gitignore` rewritten in English, `*.bak` added, and the local `server/` folder
  untracked -- it held an admin password and pure runtime logs

### Known gaps
- The restrained pose is deliberately empty. Mounting the 23 KB narrative animation
  graph as a loiter attachment crashes the Workbench natively; vanilla's own officer
  graph is 475 bytes, so this needs a purpose-built one-clip graph
- No script API can raise a noise the AI hears -- `EarsSensor` and danger events are
  engine-raised only, so a shout is a query, not a sound the AI perceives
- Characters cannot be physically coupled: `Character_Base` lists itself under
  "Forbidden linking". Carry mods work only because their subject is unconscious
- Faction-scoped intel and late-join replication still need a two-peer test


## [0.2.1] - 2026-09-09 - Game Master placeable-entity visibility fix
After an extensive debugging session, resolved all 12 placeable prefabs not appearing in the
Game Master Entity Browser despite compiling and loading with zero errors.

### Fixed
- All 12 prefabs .et.meta files had stale GUIDs in the Name field, out of sync with the
  .et file's own internal ID -- Workbench resolves resource GUIDs from .meta, not from
  the .et content
- Prefabs/Editor/Modes/EditorModeEdit.et override was missing ~95% of vanilla content after
  an earlier partial edit -- restored full content, keeping our MCF_PlaceableEntities.conf
  registry addition
- m_UIInfo used the generic SCR_UIInfo class instead of SCR_EditableEntityUIInfo
- Configs/Editor/MCF_PlaceableEntities.conf had no .meta file at all
- Removed a broken experimental Entity Catalog entry on GameMode_Editor_Full (unrelated
  system to Placeable Registry, doesn't apply to SYSTEM-type entities)
- Root cause, found by comparing three working vanilla System-type entities
  (RestrictionZone, SpawnPoint, EffectModule_MineField) via game_duplicate + prefab inspect:
  all 12 SCR_EditableEntityComponent blocks needed to (1) inherit from
  Default_SCR_EditableEntityComponent.ct ({996046FE206C699A}) instead of being declared bare,
  (2) set m_EntityType SYSTEM, (3) set flags to PLACEABLE VIRTUAL (+ HAS_AREA for
  trigger/zone components), (4) include m_aAuthoredLabels { ENTITYTYPE_SYSTEM } inside
  m_UIInfo -- a labels array separate from m_EntityType, which the Entity Browser filter
  actually reads for categorization, and (5) include a Hierarchy component

### Known non-fix
- The native Create/Update Selected Editable Prefabs Workbench plugin (Ctrl+Shift+U) crashes
  with a BadFloat assertion in EditablePrefabsLabel_Size.GetLabelValid when run against our
  mesh-less logic prefabs (confirmed native engine bug, not addon-side). Use the manual
  component pattern above instead of this plugin for zero-size prefabs.

### Open follow-up
- Placed MCF entities show "No properties" in Game Master's in-game edit panel, while vanilla
  entities (e.g. Arsenal) show a configuration UI. This is a separate system (Editor Attributes,
  likely SCR_AttributesManagerEditorComponent-adjacent) from Placeable Registry visibility and
  has not yet been investigated. See HANDOVER.md for the recommended approach.

### Tooling change
- Switched from enfusion-mcp (Articulated7/npx) to enfusion-workbench-mcp (Goldwep,
  112 tools) -- far more capable, notably prefab inspect (full inheritance-chain resolution),
  game_duplicate, resolve_guid. Removed enfusion-mcp, arma-reforger-mcp, and
  arma-reforger-api from the Claude Desktop MCP config.

## [0.2.0] - 2026-09-08 — Full roadmap implementation (Phases 0-14) + gap closures
Implements the entire phased roadmap from ARCHITECTURE.md section 9, plus closes several "manual driver" gaps that were left open during initial implementation. Every file listed below is confirmed compiling clean in Workbench.

### Added -- Core
- `MCF_Core_EventManager` -- Event Bus (GetInvoker/Publish pattern)
- `MCF_Core_TagRegistry` + `MCF_Core_ObjectIdentityComponent` -- Eden-init-box-equivalent tagging
- `MCF_Core_ValidationRegistry` -- warns on unlinked/typo'd event names at validation time
- `MCF_Core_TickManagerComponent` -- publishes MCF_Core_TickCritical/TickCosmetic on configurable intervals
- `MCF_Core_GameLoopComponent` -- drives the Tick Manager automatically via EOnFrame (closes the original "needs manual Update() call" gap)
- `MCF_Core_BudgetManager` + `MCF_Core_BudgetConfigComponent` -- per-category active-instance caps
- `MCF_Core_DebugOverlay` -- aggregates manager counts into one text report

### Added -- Narrative (Phases 0-2)
- `MCF_Obj_TriggerZoneComponent` -- Phase 0 proof of concept
- `MCF_Obj_ObjectiveComponent` -- title/description/map-visibility, intel gate, Complete/Fail events
- `MCF_Obj_LogicComponent` -- OR, COUNTER, and AND modes (AND uses 4 fixed named slots, not a dynamic array, due to an Enforce Script closure limitation -- documented in the file)
- `MCF_Obj_ObservationNode` -- POI reporting to a shared listener

### Added -- World systems (Phases 3-4)
- `MCF_Hostility_Manager` -- per-area 0-100 value, `AddImpact`, auto-decay via `StartAutoDecay()` (drives off MCF_Core_TickCosmetic)
- `MCF_Infra_NodeComponent` -- dependency graph (generator/cable/tower), Sabotage/Repair, cascades via a shared status event

### Added -- AI (Phases 3, 5-7, 10)
- `MCF_AI_CivilianBehaviorHookComponent` -- neutral/fearful/hostile classification from Hostility
- `MCF_AI_WaypointAnimationComponent` -- requests an animation on arrival (event-only, not wired to actually play)
- `MCF_AI_LifestylePOIComponent` + `MCF_AI_AmbientActorComponent` -- Ambient Life slot/role tracking
- `MCF_AI_ComplianceComponent` -- gunpoint compliance (drop weapon/stand back), with a self-built distance+angle aim approximation (`IsBeingAimedAt`) instead of an unconfirmed raycast API
- `MCF_AI_CommandWatchdogComponent` -- detects stuck AI, retries, then force-corrects to the nearest `MCF_AI_SafeFallbackPointComponent`; self-drives via MCF_Core_TickCritical
- `MCF_AI_FallbackPointRegistry` + `MCF_AI_SafeFallbackPointComponent` -- mission-maker-placed safe positions, built instead of relying on an unconfirmed navmesh/geometry query

### Added -- Interaction & Voice (Phases 5-6)
- `MCF_Interact_HintComponent` -- Tier 1/2 hint lines with chance-based pointer and retry cooldown
- `MCF_Voice_TextLineComponent` + `MCF_Voice_LineQueueManager` -- priority text queue (voice audio parked, text stand-in per explicit decision)

### Added -- React (Phases 11-12)
- `MCF_React_StepRunner` -- shared step execution ("TYPE:value"), used by both Recipes and recorded Sequences
- `MCF_React_RecipeComponent` -- trigger event + ordered step list, no node-graph editor (deliberate scope decision)
- `MCF_React_SequenceRecorderComponent` + `MCF_React_SequencePlaybackComponent` -- records position/cue samples, playback self-drives via its own EOnFrame

### Added -- Squad (Phase 9)
- `MCF_Squad_CohesionComponent` -- position sharing, muster gate, radio-respawn hint, each individually toggleable (off by default)

### Added -- AAR (Phase 8)
- `MCF_AAR_DebriefManager` -- passive Event Bus listener, builds a plain-text session summary

### Added -- Documentation
- `docs/guides/MISSION_MAKER_GUIDE.md` -- plain-language reference for every placeable node (Phase 14)

### Fixed (compile errors found and corrected during implementation)
- `ScriptInvokerBase` used bare as a parameter type does not compile -- fixed by using the `GetInvoker().Insert()/.Remove()` pattern everywhere instead of a centralized Subscribe/Unsubscribe wrapper (repeated twice: EventManager, then again in LogicComponent's AND mode)
- `IsActive()` collided with an existing `ScriptComponent` base method -- renamed to `IsNodeActive()` in `MCF_Infra_NodeComponent`
- A custom no-arg constructor on `MCF_AI_AmbientActorComponent` was incompatible with `ScriptComponent`'s expected signature -- moved initialization into `EOnInit`
- All four placeable prefabs were missing a GUID on their `RplComponent`, which `SCR_EditableEntityComponent` requires for replication -- fixed on all four

### Known gaps, explicitly documented in the affected files
- Several components (`MCF_AI_WaypointAnimationComponent`, `MCF_AI_AmbientActorComponent`) publish an event but do not yet trigger an actual animation/movement -- that hookup is separate follow-up work
- `MCF_React_SequenceRecorderComponent`'s `RecordSample`/`RecordCue` still need a manual driver (recording is an authoring-time action, judged lower priority than the live-gameplay pieces that were auto-driven)
- `MCF_Obj_LogicComponent` AND mode is capped at 4 input slots

## [0.1.0] - 2026-09-08 — Architecture baseline
Full architecture plan completed and reviewed before starting Phase 0 implementation. No code, design only.

### Added
- Player Controller: iterative test method with three profiles instead of a fixed end result
- Standalone sub-project: Player Controller improvement (`docs/modules/player-controller.md`) — movement responsiveness, turn-speed curve, ADS input timing
- Sequence Recorder added (5.13): path-and-cue recording, new recipe-input type within MCF_React_
- New module: Scripted AI Reactions catalog (5.12), namespace MCF_React_, reusable behavior recipes instead of a visual node editor
- Alert system extended to four stages (Unaware/Suspicious/Investigating/Engaged), reuses the Investigation Distance attribute
- Standalone sub-project: ACE Anvil compatibility bridge (`docs/modules/ace-anvil-compatibility.md`), namespace `MCF_ACE_`, soft-dependency pattern
- Test and stress-test infrastructure (3.2): Autotest Framework integration + Stress Profile registration, mandatory from Phase 0
- Standalone sub-project: Field Construction/FOB module (`docs/modules/field-construction.md`), namespace `MCF_Build_`
- New module: AI Command Watchdog (5.11), a patch for a known engine-level AI command problem
- Research document on mission-maker complaints (`docs/research/mission-maker-pain-points.md`)
- New module: Squad Cohesion/C2 layer (5.10), namespace `MCF_Squad_`
- Repository structure set up
- Architecture plan (`docs/architecture/ARCHITECTURE.md`)
- Standalone sub-project: shooting from a vehicle as a passenger (`docs/modules/vehicle-shooting.md`)
- Standalone sub-project: stealth & suppression improvement (`docs/modules/stealth-and-suppression.md`)
### Changed
- Project name locked in: Milsim Creator Framework (MCF), prefix `MCF_`
### Fixed (document review 2026-09-08)
- Structural bug: section 3.2 was accidentally spliced into the middle of section 3.1's bullet list
- Namespace table updated: `MCF_AI_` was missing 5.11, `MCF_React_` was missing 5.13
- Outdated "phase-0-through-9" reference corrected (the roadmap has grown to Phase 14)
- Ambiguous "core roadmap section 9" reference clarified (was being confused with Phase 9 in the table)
- Bare "section 3.4" references to the Alert system made file-qualified (it lives in `stealth-and-suppression.md`, not this document)
- Cross-reference added from 5.3 to the Alert system, noting it's a relocation candidate for a future cleanup
- README's structure diagram updated to include `docs/research/`

