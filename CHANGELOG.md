# Changelog

Alle noemenswaardige wijzigingen aan dit project worden hier bijgehouden.
Formaat gebaseerd op [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

## [0.1.0] - 2026-09-08 — Architectuurbaseline
Volledig architectuurplan afgerond en gereviewd vóór start van Fase 0-implementatie. Geen code, uitsluitend ontwerp.

### Toegevoegd
- Player Controller: iteratieve testmethode met drie profielen i.p.v. vast eindresultaat vastgelegd
- Losstaand deelproject: Player Controller-verbetering (`docs/modules/player-controller.md`) — bewegingsresponsiviteit, turn-speed-curve, ADS-inputtiming
- Sequence Recorder toegevoegd (5.13): pad-en-cue-opname, nieuw type recept-input binnen MCF_React_
- Nieuwe module: Scripted AI Reactions-catalogus (5.12), namespace MCF_React_, herbruikbare gedragsrecepten i.p.v. visuele node-editor
- Alert-systeem uitgebreid naar vier stadia (Onwetend/Argwanend/Onderzoekend/In gevecht), hergebruikt Investigation Distance-attribuut
- Losstaand deelproject: ACE Anvil-compatibiliteitsbrug (`docs/modules/ace-anvil-compatibility.md`), namespace `MCF_ACE_`, soft-dependency-patroon
- Test- en stress-testinfrastructuur (3.2): Autotest Framework-integratie + Stress Profile-registratie, verplicht vanaf Fase 0
- Losstaand deelproject: Field Construction/FOB-module (`docs/modules/field-construction.md`), namespace `MCF_Build_`
- Nieuwe module: AI Commando-Watchdog (5.11), pleister voor bekend engine-niveau AI-commandoprobleem
- Onderzoeksdocument mission-maker-klachten (`docs/research/mission-maker-pain-points.md`)
- Nieuwe module: Squad Cohesion/C2-laag (5.10), namespace `MCF_Squad_`
- Repository-structuur opgezet
- Architectuurplan (`docs/architecture/ARCHITECTURE.md`)
- Losstaand deelproject: voertuig-schieten als passagier (`docs/modules/vehicle-shooting.md`)
- Losstaand deelproject: stealth & suppressie-verbetering (`docs/modules/stealth-and-suppression.md`)
### Gewijzigd
- Projectnaam vastgelegd: Milsim Creator Framework (MCF), prefix `MCF_`
### Gefixed (documentreview 2026-09-08)
- Structurele fout: sectie 3.2 stond per ongeluk midden in sectie 3.1's opsomming geknipt
- Namespace-tabel bijgewerkt: `MCF_AI_` miste 5.11, `MCF_React_` miste 5.13
- Verouderde "fase-0-t/m-9"-verwijzing gecorrigeerd (roadmap is gegroeid tot Fase 14)
- Dubbelzinnige "kernroadmap sectie 9"-verwijzing verduidelijkt (verward met Fase 9 in de tabel)
- Bare "sectie 3.4"-verwijzingen naar het Alert-systeem file-qualified (leeft in `stealth-and-suppression.md`, niet in dit document)
- Cross-reference toegevoegd vanuit 5.3 naar het Alert-systeem, met notitie dat dit een verplaatsingskandidaat is bij toekomstige opschoning
- README's structuurschema aangevuld met `docs/research/`

### Deferred
- Existing Dutch documentation (ARCHITECTURE.md, docs/modules/*, docs/research/*) not yet translated to English -- deferred, not blocking Phase 0. All NEW code, comments, and documentation from this point forward are in English.

