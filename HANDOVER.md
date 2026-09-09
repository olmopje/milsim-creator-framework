# MCF Handover — 2026-09-09

## Wat is MCF?
Milsim Creator Framework, prefix `MCF_`. Arma Reforger-mod voor missiemakers: herbruikbare bouwstenen (triggers, objectives, AI-hulpcomponenten, dialoog, sequenties) zodat je milsim-scenario's kunt bouwen zonder alles vanaf nul te scripten.

- **Repo:** `G:\MCF` (git, branch `main`), GitHub: https://github.com/olmopje/milsim-creator-framework.git
- **Workbench-project:** `G:\MCF\addons\MCF\addon.gproj`, addon-GUID `6A50E40BA3B94A4F`
- **Testwereld:** `G:\MCF\addons\MCF\worlds\arland\MCFTestworld.ent` (Arland, Game Master-modus)
- **Volledige status:** `docs/architecture/PROJECT_STATUS.md` — lees dit eerst, met name de sectie "MAJOR UPDATE (2026-09-09 session)" bovenaan.
- **Missiemakers-gids:** `docs/guides/MISSION_MAKER_GUIDE.md`
- **Architectuur-overzicht:** `docs/architecture/ARCHITECTURE.md`

## Grootste resultaat van de sessie van vandaag
Na een zeer lang debugtraject: **live in-game Game Master-plaatsing van MCF-prefabs werkt nu volledig.** Alle 12 System-type prefabs zijn zichtbaar en plaatsbaar in de Entity Browser tijdens het spelen. Zie `PROJECT_STATUS.md` voor de exacte, stap-voor-stap bevestigde recept-structuur — dat is nu de te volgen sjabloon voor élk nieuw placeable prefab.

**Kern van de fix (samengevat):** `SCR_EditableEntityComponent` moet erven van `Default_SCR_EditableEntityComponent.ct`, met `m_EntityType SYSTEM`, de juiste combinatie van `EEditableEntityFlag`s (`PLACEABLE|VIRTUAL[|HAS_AREA]`), een `SCR_EditableEntityUIInfo` met `m_aAuthoredLabels { ENTITYTYPE_SYSTEM }`, en een `Hierarchy`-component. Zonder ontbrekend stukje: **geen enkele foutmelding in de log**, maar het item verschijnt gewoon nooit — dat maakte dit zo moeilijk te vinden.

## Tooling-opzet (belangrijk voor nieuwe chat!)
- **Gebruik uitsluitend `enfusion-workbench-mcp`** (Goldwep-fork, 112 tools) voor alle Enfusion/Workbench-taken. De oudere `enfusion-mcp`, `arma-reforger-mcp` en `arma-reforger-api` zijn verwijderd uit de Claude Desktop-config — gebruik ze niet, en stel niet voor ze terug te zetten.
- Lokale build: `G:\enfusion-workbench-mcp` (npm-project, `dist/index.js`)
- **Let op path-config:** `ENFUSION_PROJECT_PATH` staat nu op `G:\MCF\addons\MCF` (de addon-root zelf) voor de `project`-tool (read/write/browse). Voor `game_duplicate`/`wb_entity_duplicate` is dit verkeerd-om (die willen de bovenliggende map + `modName`) — check dit als die tools "addon not found" geven.
- Workbench starten: `enfusion-workbench-mcp:wb_launch` met `gprojPath: "G:\MCF\addons\MCF\addon.gproj"` en `world: "worlds/arland/MCFTestworld.ent"`.
- **Voor een écht verse test na bestandswijzigingen: gebruiker moet Workbench VOLLEDIG sluiten** (Taakbeheer checken) vóórdat je opnieuw `wb_launch` aanroept — "Already Running" reconnects zonder de boel echt te herladen, wat cruciaal bleek voor cache-gevoelige tests.

## Open item voor volgende sessie
**"Scenario properties"-paneel (instelbare waarden na plaatsing in Game Master)** — bijv. bij Arsenal kun je na plaatsen factie/loadout/wapens instellen; onze MCF-prefabs tonen "No properties, this entity has no properties to edit." Dit is een ander systeem dan wat we vandaag hebben opgelost (waarschijnlijk `SCR_AttributesManagerEditorComponent` + een attribuutlijst-config per component). Nog niet onderzocht.

**Startpunt:** lees `Scripts/Game/Editor/Containers/Attributes/SCR_BaseEditorAttribute.c` via `game_read`, en zoek een simpel (niet-Arsenal) vanilla System-type prefab dat wél instelbare eigenschappen heeft na plaatsing — vergelijk zijn componentopbouw net zoals we vandaag deden voor de zichtbaarheids-fix (dupliceren via `game_duplicate`, inspecteren via `wb_resources getInfo`, vergelijken met onze eigen prefabs).

## Werkwijze die vandaag goed werkte (herhaal dit patroon)
1. Zoek een **bewezen werkend vanilla-voorbeeld** dat lijkt op wat je probeert te bouwen.
2. Dupliceer het via `game_duplicate` naar `Prefabs/Test/...` in de mod.
3. Inspecteer het volledig via `wb_resources` (`action: getInfo`) — dit toont de volledig opgeloste JSON-structuur.
4. Vergelijk component-voor-component met je eigen prefab.
5. Pas één verschil per keer toe, commit, en test met een **volledig verse Workbench-herstart** (niet alleen "Reload Game").
6. Ruim testbestanden op zodra de fix bevestigd is.

## Overige losse technische lessen (zie PROJECT_STATUS.md voor details)
- `.et.meta`-bestanden bevatten de GUID die Workbench daadwerkelijk gebruikt — niet het `ID`-veld in de `.et`-tekst zelf. Bij twijfel over "Wrong GUID"-foutmeldingen: check eerst de `.meta`.
- Bij het overriden van een bestaand vanilla-bestand (zoals `EditorModeEdit.et`) in je addon: bewaar **altijd** de volledige originele inhoud, voeg alleen je eigen wijziging toe. Een gedeeltelijke override mist tientallen andere componenten en veroorzaakt catastrofale, moeilijk te herleiden crashes.
- De "Create/Update Selected Editable Prefabs"-plugin crasht met een engine-assertion (`BadFloat` in `EditablePrefabsLabel_Size.GetLabelValid`) op mesh-loze entiteiten zonder fysieke afmeting — dit is een bevestigde native engine-bug, geen oplosbaar probleem via bestandsaanpassingen. Vermijd deze plugin voor pure logica-prefabs; gebruik de handmatige route uit de sectie hierboven.