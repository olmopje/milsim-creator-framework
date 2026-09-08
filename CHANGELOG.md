# Changelog

All notable changes to this project are tracked here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

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

