# Bijdragen aan MCF

## Voordat je begint

Lees `docs/architecture/ARCHITECTURE.md` sectie 3.1 (Integratie-contracten) volledig. Dit is geen stijlgids maar een harde eis — een module die deze regels breekt wordt niet gemerged, ongeacht hoe goed hij verder werkt.

## Checklist per nieuwe module / PR

- [ ] Alle classes/prefabs gebruiken de `MCF_`-prefix
- [ ] Alle nieuwe events volgen het `Module_Actie`-patroon en zijn gedocumenteerd (payload-schema) in `docs/modules/<modulenaam>.md`
- [ ] Geen directe module-naar-module-referenties — alles loopt via de Event Bus of Object Identity-tags
- [ ] State-mutaties zijn server-authoritative; client-kant bevat alleen leeslogica op gerepliceerde waarden
- [ ] Factie-referenties gaan via Faction Alias, nooit via een harde Faction Key
- [ ] Nieuwe UserActions/attributen zijn getest in Game Master, niet alleen in Workbench-preview
- [ ] Teksten staan in de StringTable, niet hardcoded
- [ ] Event Bus-listeners worden opgeruimd bij entity-destructie (geen dangling listeners)
- [ ] Validatie-pass logt (W)/(E)-meldingen bij foutieve configuratie, faalt niet stil
- [ ] Module levert een Autotest-suite (`SCR_AutotestSuiteBase`), geregistreerd bij de Module Registry
- [ ] Indien de module iets herhaaldelijk spawnt/simuleert: een Stress Profile geregistreerd (zie `ARCHITECTURE.md` 3.2) — geen uitzondering "doen we later"

## Commit- en branchconventie

- Branches: `module/<naam>` voor nieuwe modules, `fix/<omschrijving>` voor bugfixes, `docs/<omschrijving>` voor documentatie
- Commits: korte imperatieve titel (`Voeg Hostility-decay toe`, niet `Hostility toegevoegd`)
- Eén module per PR — geen gecombineerde "diverse fixes"-PR's, dat maakt reviewen tegen het integratie-contract onmogelijk

## Testen

Automatische compile-validatie via CI is beperkt mogelijk zonder een volledige Workbench-omgeving (zie `.github/workflows/`). Test daarom altijd lokaal in Workbench + een Game Master-sessie met minimaal 2 spelers voordat je een PR opent, zeker voor replicatie-gevoelige wijzigingen.

## Releases / verspreiden

Nieuwe versies worden getagd (`vX.Y.Z`) op `main` nadat de Fase-doelen uit `ARCHITECTURE.md` sectie 9 voor die versie zijn gehaald. Zie `CHANGELOG.md`.
