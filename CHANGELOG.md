# Changelog

All notable changes to this project are tracked here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]
### Changed
- Translated all existing documentation (README, CONTRIBUTING, ARCHITECTURE, module docs, research doc, issue templates, CI workflow comments) from Dutch to English

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

