# Contributing to MCF

## Language

All code, comments, commit messages, and documentation are written in English.

## Before you start

Read `docs/architecture/ARCHITECTURE.md` section 3.1 (Integration Contracts) in full. This is not a style guide but a hard requirement — a module that breaks these rules will not be merged, no matter how well it otherwise works.

## Checklist per new module / PR

- [ ] All classes/prefabs use the `MCF_` prefix
- [ ] All new events follow the `Module_Action` pattern and are documented (payload schema) in `docs/modules/<module-name>.md`
- [ ] No direct module-to-module references — everything goes through the Event Bus or Object Identity tags
- [ ] State mutations are server-authoritative; client side contains only read logic on replicated values
- [ ] Faction references go through Faction Alias, never a hard Faction Key
- [ ] New UserActions/attributes are tested in Game Master, not only in Workbench preview
- [ ] Text lives in the StringTable, not hardcoded
- [ ] Event Bus listeners are cleaned up on entity destruction (no dangling listeners)
- [ ] Validation pass logs (W)/(E) messages on misconfiguration, never fails silently
- [ ] Module ships an Autotest suite (`SCR_AutotestSuiteBase`), registered with the Module Registry
- [ ] If the module repeatedly spawns/simulates something: a Stress Profile is registered (see `ARCHITECTURE.md` 3.2) — no "we'll do that later" exception

## Commit and branch conventions

- Branches: `module/<name>` for new modules, `fix/<description>` for bugfixes, `docs/<description>` for documentation
- Commits: short imperative title (`Add Hostility decay`, not `Hostility added`)
- One module per PR — no combined "various fixes" PRs, that makes reviewing against the integration contract impossible

## Testing

Automated compile validation via CI is limited without a full Workbench environment (see `.github/workflows/`). Always test locally in Workbench + a Game Master session with at least 2 players before opening a PR, especially for replication-sensitive changes.

## Releases / distribution

New versions are tagged (`vX.Y.Z`) on `main` once the phase goals from `ARCHITECTURE.md` section 9 for that version have been met. See `CHANGELOG.md`.
