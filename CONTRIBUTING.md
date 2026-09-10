# Contributing to MCF

## Language

All code, comments, commit messages, and documentation are written in English.

## Before you start

Read `docs/architecture/ARCHITECTURE.md` section 3.1 (Integration Contracts) in full, and `docs/architecture/STRUCTURE.md` for where things live. The first is not a style guide but a hard requirement — a module that breaks these rules will not be merged, no matter how well it otherwise works.

**Open `addons/MCF_Dev/addon.gproj` in the Workbench.** MCF is six addons and the Workbench opens one project at a time; MCF_Dev is the only one that depends on all the others, so it is the only one that loads the whole framework.

## Which addon does my code go in?

In order, stop at the first that applies. This is the single most consequential decision in a PR and it is not a matter of taste.

1. **Two modules would both use it → `MCF` (Core).** Anything shared between modules goes in Core, not in whichever module happened to need it first. Something that exists only as a dependency of other things is a library, and libraries live in Core.
2. **It only makes sense together with an existing module's feature → that module.** Device locks went into Ops because a locked laptop is intel you cannot read yet, not a separate subject.
3. **It is a handful of scripts → the nearest existing module.** A new addon costs a `.gproj`, a dependency GUID, a Workshop listing and the open-it-once-in-the-Workbench tax. Earn it.
4. **Otherwise → a new addon**, and Core's four vanilla-override manifests need editing to name its contributions. Say so in the PR.

A module may reference **Core and nothing else**. There is no exception; `MCF_Dev` is the test harness and is not a module. `tools/measure_addon_graph.ps1` counts the real edges — run it if you are unsure whether what you wrote crossed a line.

## Checklist per new module / PR

- [ ] All classes/prefabs use the `MCF_` prefix, with a namespace listed in `STRUCTURE.md` section 4 (or a new one, argued for in the PR)
- [ ] The addon it lands in follows the four rules above
- [ ] `tools/measure_addon_graph.ps1` shows no new cross-module edge
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
- [ ] Any GUID minted from the `6A1C4F0B39D2xxxx` block was checked with `git grep` first — two collisions have already happened

## Documentation

If a change alters the addon layout, the dependency graph, or which addon owns a namespace, **update `docs/architecture/STRUCTURE.md` in the same PR** and re-run the measurement script so the numbers in it are real. That document has been wrong twice: once because nobody re-measured for a week, once because it was updated in a copy and not in the repository.

Research notes and design writing belong in `docs/`, never under `addons/` — anything under an addon is packed into the shipped mod and distributed to every player.

## Commit and branch conventions

- Branches: `module/<name>` for new modules, `fix/<description>` for bugfixes, `docs/<description>` for documentation
- Commits: short imperative title (`Add Hostility decay`, not `Hostility added`)
- One module per PR — no combined "various fixes" PRs, that makes reviewing against the integration contract impossible

## Testing

Automated compile validation via CI is limited without a full Workbench environment (see `.github/workflows/`). Always test locally in Workbench + a Game Master session with at least 2 players before opening a PR, especially for replication-sensitive changes.

A clean compile means very little on its own. Three separate framework-wide defects were found in a single day where "it compiles" turned out to mean nothing at all — say in the PR what you actually watched happen in a session.

## Releases / distribution

New versions are tagged (`vX.Y.Z`) on `main` once the phase goals from `ARCHITECTURE.md` section 9 for that version have been met. See `CHANGELOG.md`.
