# Milsim Framework (MF) — Arma Reforger

Een zelfgebouwd, modulair missie-framework voor Arma Reforger. Geeft missiemakers Eden-achtige narratieve diepte, live bruikbaar in Game Master, gebouwd rond een gedeelde Core en van meet af aan performance-bewust voor grote PvE co-op groepen.

Geen dependency op third-party frameworks (Scenario Framework, Ci5, GME) — wel geïnformeerd door hun ontwerp. Zie [`docs/architecture/ARCHITECTURE.md`](docs/architecture/ARCHITECTURE.md) voor de volledige onderbouwing.

## Status

🚧 Fase 0 — Proof of concept. Nog geen speelbare build.

## Structuur

```
addons/MF/          — de daadwerkelijke mod (laadt in-game)
  Prefabs/            — .et prefab-bestanden
  Scripts/Game/Core/  — Event Bus, Object Identity, Module Registry, Tick Manager, etc.
  Scripts/Game/Modules/ — losse modules (Objective, Hostility, Ambient Life, ROE, ...)
  Configs/            — .conf-bestanden, StringTables
  UI/                 — GM-attribuutlayouts

docs/
  architecture/       — het volledige architectuurplan (leidend document)
  modules/            — losstaande technische deelprojecten (bijv. voertuig-schieten)

.github/              — issue templates, CI-workflows
```

## Vereisten

- Arma Reforger Tools (Steam) — voor Workbench, prefab-bewerking en lokaal testen
- Git

## Bijdragen

Zie [`CONTRIBUTING.md`](CONTRIBUTING.md) voor naamgevingsconventies, event-contract-regels en de PR-workflow. **Lees dit voordat je een module toevoegt** — het integratie-contract in `ARCHITECTURE.md` sectie 3.1 is niet optioneel.

## Licentie

Arma Public License (APL) — zie [`LICENSE.md`](LICENSE.md).
