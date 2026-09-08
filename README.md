# Milsim Creator Framework (MCF) — Arma Reforger

A self-built, modular mission framework for Arma Reforger. Gives mission makers Eden-like narrative depth, usable live in Game Master, built around a shared Core and performance-conscious from the start for large PvE co-op groups.

No dependency on third-party frameworks (Scenario Framework, Ci5, GME) — informed by their design, but built independently. See [`docs/architecture/ARCHITECTURE.md`](docs/architecture/ARCHITECTURE.md) for the full rationale.

## Status

🚧 **Phase 0 in progress.** Architecture baseline locked at v0.1.0. Core foundation (Event Bus, Tag Registry, Object Identity Component) is written and confirmed compiling in Workbench. See [`docs/architecture/PHASE0_PROGRESS.md`](docs/architecture/PHASE0_PROGRESS.md) for exact status and environment notes. No playable build yet.

## Structure

```
addons/MCF/          — the actual mod (loads in-game)
  Prefabs/            — .et prefab files
  Scripts/Game/Core/  — Event Bus, Object Identity, Module Registry, Tick Manager, etc.
  Scripts/Game/Modules/ — individual modules (Objective, Hostility, Ambient Life, ROE, ...)
  Configs/            — .conf files, StringTables
  UI/                 — GM attribute layouts

docs/
  architecture/       — the full architecture plan (leading document)
  modules/            — standalone technical sub-projects (e.g. vehicle shooting, ACE compatibility)
  research/           — research into community/mission-maker complaints that informs the roadmap

.github/              — issue templates, CI workflows
```

## Requirements

- Arma Reforger Tools (Steam) — for Workbench, prefab editing, and local testing
- Git

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for naming conventions, event-contract rules, and the PR workflow. **Read this before adding a module** — the integration contract in `ARCHITECTURE.md` section 3.1 is not optional.

## License

Arma Public License (APL) — see [`LICENSE.md`](LICENSE.md).
