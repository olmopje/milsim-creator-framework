# Milsim Creator Framework (MCF) — Arma Reforger

A self-built, modular mission framework for Arma Reforger. Gives mission makers Eden-like narrative depth, usable live in Game Master, built around a shared Core and performance-conscious from the start for large PvE co-op groups.

No dependency on third-party frameworks (Scenario Framework, Ci5, GME) — informed by their design, but built independently. See [`docs/architecture/ARCHITECTURE.md`](docs/architecture/ARCHITECTURE.md) for the full rationale.

📖 **[The wiki](https://github.com/olmopje/milsim-creator-framework/wiki) is the place to start** — mission-maker guides, the node reference, and the Enfusion lessons that cost us the most time.

## Status

🚧 **Working, unpolished, pre-release.** There is no tagged build yet and no Workshop publication.

What is proven in a live session, not merely compiling:

- **An event-driven core** — event bus, tick manager, budget caps, cross-restart persistence outside the engine's own saves.
- **Twelve placeable nodes**, every one observed firing: proximity, cone and line-of-sight detection, AND/OR/counter logic, relays, recipes, objectives, observation points, on-screen text.
- **An operations board** — taskings and intel with a read/amend permission split, edited live in Game Master.
- **Conversations with any AI** — trust and fear per character, replies gated on those numbers, authored from a Game Master library. Reaches every character in the game, vanilla or modded, through an override of `Character_Base` rather than an MCF-only prefab.
- **Shout, surrender, restrain, escort** — a real custom keybind (H / U) forces nearby AI to weigh distance, your weapon, and their own fear; those who give in drop their weapon, can be restrained, and can be walked somewhere.

An automated self test, run on two peers on two factions, has watched faction-scoped intel work in both directions, the audience filter on text lines show a line to the faction addressed and withhold it from the other, device profiles reach a client that is not the host, and two clients fetch the same picture independently. Eight checks, none failed.

What is still not proven: **late join** — a client arriving into a mission that already has authored content. And **role restrictions**, which resolve correctly but are not yet enforced: the master switch is still open, so nothing has been refused yet.

The chronological record, including every dead end, is [`docs/architecture/PROJECT_STATUS.md`](docs/architecture/PROJECT_STATUS.md). Where it is going next is [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Structure

```
addons/MCF_Core/      — the framework core (loads in-game)
  Prefabs/              — .et prefab files, including the Character_Base override
  Scripts/Game/Core/    — event bus, identity, tick manager, stores, dialogue data
  Scripts/Game/Modules/ — per-entity components (triggers, dialogue, disposition, escort)
  Scripts/Game/Editor/  — Game Master context actions and editor attributes
  Scripts/Game/UI/      — MCF menus (operations board, dialogue, editors)
  Configs/              — .conf files, StringTables, the input-binding override
  UI/layouts/           — .layout files for those menus

docs/
  architecture/         — ARCHITECTURE.md (the plan), PROJECT_STATUS.md (the record)
  guides/               — MISSION_MAKER_GUIDE.md, plain-language node reference
  modules/              — standalone technical sub-projects
  research/             — design notes and community pain-point research
  ROADMAP.md            — what is verified, what is missing, what comes next

.github/              — issue templates, CI workflows
```

## Requirements

- Arma Reforger Tools (Steam) — for Workbench, prefab editing, and local testing
- Git

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for naming conventions, event-contract rules, and the PR workflow. **Read this before adding a module** — the integration contract in `ARCHITECTURE.md` section 3.1 is not optional.

## License

Arma Public License (APL) — see [`LICENSE.md`](LICENSE.md).
