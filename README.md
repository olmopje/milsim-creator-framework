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
- **Readable objects** — a letter, a notepad, a phone and a laptop, each drawn as the thing it is with its own model behind the screen. Locked devices are broken into on the device's own display.

What is not proven: faction-scoped intel and late-join replication both need a two-peer test, and the audience filter on text lines is plumbed but never observed selecting.

The chronological record, including every dead end, is [`docs/architecture/PROJECT_STATUS.md`](docs/architecture/PROJECT_STATUS.md). Where it is going next is [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Structure

MCF is **six addons**, each a folder under `addons/` with its own `addon.gproj`. Every module depends on Core and on nothing else; only the unpublished test addon reaches across.

```
addons/
  MCF/              — Core. Required by everything.
                      Event bus, identity, tick manager, budget manager,
                      persistent store, roles, disposition, hostility,
                      the game mode prefab, and the four vanilla overrides.
  MCF_Ops/          — Tasks, intel, devices, the operations board, squad cohesion
  MCF_Dialogue/     — Conversations: data, library, components, menus
  MCF_AI/           — Ambient life, and the shout / surrender / restrain chain
  MCF_Objectives/   — Logic nodes, objectives, triggers, recipes, sequences
  MCF_Dev/          — Test world, missions, self-test harness. Never published.

docs/
  architecture/     — STRUCTURE.md (what the addons are — the structure of record)
                      ARCHITECTURE.md (the plan and the reasoning)
                      PROJECT_STATUS.md (the record, including dead ends)
  guides/           — MISSION_MAKER_GUIDE.md, plain-language node reference
  modules/          — standalone technical sub-projects
  research/         — design notes and community pain-point research
  ROADMAP.md        — what is verified, what is missing, what comes next

tools/              — measurement scripts, including the one that produced
                      the dependency graph in STRUCTURE.md

.github/            — issue templates, CI workflows
```

**Open `addons/MCF_Dev/addon.gproj` in the Workbench.** It is the only project file that pulls in all six addons, and it carries the test world. See [`docs/architecture/STRUCTURE.md`](docs/architecture/STRUCTURE.md) section 5.

**Anything shared between modules goes in Core.** Not in whichever module needed it first. That rule is why Core is the second-largest addon, and it is deliberate — see [`docs/architecture/STRUCTURE.md`](docs/architecture/STRUCTURE.md) section 1.

## Requirements

- Arma Reforger Tools (Steam) — for Workbench, prefab editing, and local testing
- Git

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for naming conventions, event-contract rules, which addon a new module belongs in, and the PR workflow. **Read this before adding a module** — the integration contract in `ARCHITECTURE.md` section 3.1 is not optional.

## License

Arma Public License (APL) — see [`LICENSE.md`](LICENSE.md).
