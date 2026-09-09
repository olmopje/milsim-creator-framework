# MCF — Handover voor nieuwe chatsessie (2026-09-09)

**Lees dit eerst als je een nieuwe Claude-chat/project start voor dit project.**

## Wat is MCF?
Milsim Creator Framework — een Arma Reforger mod-framework (prefix `MCF_`) met herbruikbare
bouwstenen voor missiemakers: triggers, objectives, AI-gedrag, hostility-systeem, interactie,
voice-lines, "recipe"-scripting, squad-cohesie en AAR. Volledige architectuur en scope staan in
`docs/architecture/ARCHITECTURE.md`.

- **Repo:** https://github.com/olmopje/milsim-creator-framework.git (branch `main`)
- **Lokaal pad:** `G:\MCF`
- **Workbench-project:** `G:\MCF\addons\MCF\addon.gproj` (addon-GUID `6A50E40BA3B94A4F`)
- **Testwereld:** `G:\MCF\addons\MCF\worlds\arland\MCFTestworld.ent`

## Status op dit moment
Alle 14 fases van de architectuur zijn geïmplementeerd en compileren schoon (zie CHANGELOG.md
v0.2.0). Alle 12 placeable prefabs zijn nu **zichtbaar en plaatsbaar in Game Master** — dit was
het onderwerp van een zeer lange, diepgaande debug-sessie op 2026-09-09 (zie hieronder).

## MCP-tooling — gebruik ALLEEN dit
We zijn overgestapt op **`enfusion-workbench-mcp`** (Goldwep, github.com/Goldwep/enfusion-workbench-mcp),
112 tools, veruit superieur aan alle eerder geprobeerde alternatieven (enfusion-mcp/Articulated7,
arma-reforger-mcp, ReforgerForge — die zijn allemaal verwijderd uit de Claude Desktop-config).

Belangrijkste tools die je zult gebruiken:
- `project` (action: browse/read/write) — bestanden in JOUW addon lezen/schrijven
- `game_read` / `game_browse` / `asset_search` — vanilla basisspel-content lezen/doorzoeken
- `prefab` (action: inspect) — **volledige overervingsketen tonen, inclusief class-level defaults**
  — cruciaal om te zien wat een prefab ECHT bevat, niet alleen wat er letterlijk in het bestand staat
- `game_duplicate` — vanilla prefab dupliceren met volledige overervingsketen geïnjecteerd
- `wb_launch` — Workbench starten/verbinden (geef altijd `gprojPath` en `world` mee)
- `wb_resources` (getInfo/register/rebuild) — LET OP: `getInfo` kan soms een fractie achterlopen
  op zeer recente schrijfacties; vertrouw bij twijfel op een echte in-game test, niet alleen op getInfo
- `wb_entity_modify` (setProperty/getProperty/listProperties/listArrayItems/removeArrayItem) —
  properties op WERELD-entiteiten aanpassen via Workbench's eigen live API
- `resolve_guid` — GUID opzoeken in de project-index

**Config-valkuil:** `ENFUSION_PROJECT_PATH` moet exact `G:\MCF\addons\MCF` zijn (de addon-root zelf,
niet de bovenliggende map). Als je per ongeluk `G:\MCF\addons` instelt, schrijft `project` naar de
verkeerde plek (`addons/Prefabs/...` in plaats van `addons/MCF/Prefabs/...`). `game_duplicate` wil
juist wél de bovenliggende map + `modName` — deze twee tools zijn inconsistent hierin, wees alert.

**Workbench-herstart-discipline:** cache-problemen zijn hardnekkig. Bij twijfel: vraag de gebruiker
om Workbench **volledig af te sluiten** (niet alleen "Reload Game" binnen de app) en gebruik dan
`wb_launch` opnieuw. Een simpele "Reload Game" laadt de resource-database NIET opnieuw.

## DE GROTE DOORBRAAK: Game Master zichtbaarheid (2026-09-09)
Dit kostte een buitengewoon lange sessie. Het probleem: 12 custom placeable prefabs (logica-only,
geen mesh) verschenen niet in de Game Master Entity Browser, ondanks foutloze logs.

### Wat NIET de oorzaak was (maar wel gefixt moest worden onderweg)
1. Verkeerde/verouderde GUID's in `.et.meta`-bestanden (Workbench resolvet resource-GUID's via het
   `.meta`-bestand, NIET via het `ID`-veld in de `.et`-inhoud zelf!)
2. Onvolledige `EditorModeEdit.et`-override (miste ~95% van de vanilla-inhoud na een eerdere
   handmatige bewerking — override-bestanden moeten de VOLLEDIGE inhoud van het origineel bevatten)
3. Verkeerde UIInfo-klasse (`SCR_UIInfo` i.p.v. `SCR_EditableEntityUIInfo`)
4. Ontbrekend `.meta`-bestand voor onze eigen `MCF_PlaceableEntities.conf`
5. Kapotte Entity Catalog-toevoeging (los systeem van Placeable Registry, hoort niet bij SYSTEM-type items)
6. `Create/Update Selected Editable Prefabs`-plugin crasht native (BadFloat-assertion in
   `EditablePrefabsLabel_Size.GetLabelValid`) op mesh-loze entiteiten — **vermijd deze plugin voor
   pure logica-prefabs, gebruik de handmatige route hieronder**

### De ECHTE, volledige oplossing (bevestigd werkend)
Elke placeable prefab moet **exact dit patroon** volgen op zijn `SCR_EditableEntityComponent`:

```
SCR_EditableEntityComponent "{GUID}" : "{996046FE206C699A}Prefabs/Editor/Components/Default_SCR_EditableEntityComponent.ct" {
 m_EntityType SYSTEM
 m_Flags {
  PLACEABLE
  VIRTUAL
  HAS_AREA        // alleen voor trigger/zone-achtige entiteiten
 }
 m_UIInfo SCR_EditableEntityUIInfo "{GUID}" {
  Name "Weergavenaam"
  m_aAuthoredLabels {
   ENTITYTYPE_SYSTEM
  }
 }
}
```

Plus: een `Hierarchy`-component naast de andere componenten (RplComponent, etc.) op root-niveau.

**De twee cruciale, niet-vanzelfsprekende stukjes waren:**
1. **`m_aAuthoredLabels { ENTITYTYPE_SYSTEM }` binnen `m_UIInfo`** — dit is een LABELS-array,
   apart van het `m_EntityType`-enum-veld. Dit is waarschijnlijk waar de Entity Browser-filter
   daadwerkelijk op filtert. Gevonden door drie verschillende werkende vanilla System-entiteiten
   te vergelijken (RestrictionZone, SpawnPoint, EffectModule_MineField) — alle drie hadden dit.
2. **Erven van `Default_SCR_EditableEntityComponent.ct`** (GUID `{996046FE206C699A}`) i.p.v. het
   component los te declareren — bevestigd via de officiële BI-wiki-tekst: "All editable entities
   which use component prefab Default_SCR_EditableEntityComponent.ct are already flagged as PLACEABLE."

### Werkwijze om dit te verifiëren/reproduceren
1. `game_duplicate` een bekend-werkend vanilla System-item (bijv.
   `{EF72FA7CD87618D5}PrefabsEditable/RestrictionZone/E_EditorRestrictionZoneSmall.et` of
   `{CEA2B24051A44525}PrefabsEditable/SpawnPoints/E_SpawnPoint_US.et`)
2. `prefab` (action: inspect, `include_raw: true`) erop om de RAUWE bestandsinhoud van elk niveau
   in de overervingsketen te zien — dit toont het EXACTE patroon dat werkt
3. Vergelijk met je eigen prefab en kopieer het patroon

### Registry-koppeling (dit deel werkte na de hoofdfix meteen goed)
- `Configs/Editor/MCF_PlaceableEntities.conf` (`SCR_PlaceableEntitiesRegistry`, GUID
  `{20157029BFE49D1A}`, met `.meta`-bestand met `CONFResourceClass`-structuur) bevat de 12 prefab-paden
- Toegevoegd aan `Prefabs/Editor/Modes/EditorModeEdit.et` (onze override) →
  `SCR_PlacingEditorComponent.m_Registries` array
- **Deze eigen, aparte registry-aanpak (i.p.v. een vanilla-bestand overschrijven) werkt nu prima**
  — geen noodzaak om `Configs/Editor/PlaceableEntities/Systems/Systems.conf` te overriden

## Volgende openstaande vraag: Editor Attributes (Scenario properties-paneel)
Wanneer je in Game Master een geplaatst item selecteert en "Edit" kiest, toont vanilla-content
(bijv. Arsenal) een configuratiepaneel ("Set faction", "Enable arsenal", etc.) — onze eigen
prefabs tonen "No properties, this entity has no properties to edit."

Dit is een **apart systeem** van wat we vandaag gefixt hebben (niet Placeable Registry, maar de
"Editor Attributes"-laag, vermoedelijk gekoppeld via `SCR_AttributesManagerEditorComponent` /
attribuutlijst-configs, zoals gezien in `EditorModeEdit.et`'s `m_AttributeLists`). Dit is **nog niet
onderzocht** — dit is de logische volgende stap voor een nieuwe sessie. Onze scriptcomponenten
hebben wél gewone `[Attribute(...)]`-tags (zichtbaar in Workbench's Object Properties), maar dat is
blijkbaar een ander mechanisme dan wat dit specifieke in-game paneel gebruikt.

**Aanbevolen aanpak voor de volgende sessie:** gebruik dezelfde bewezen methode als vandaag — zoek
een simpel vanilla System-item met een WERKEND Editor Attributes-paneel, dupliceer het met
`game_duplicate`, inspecteer met `prefab` (`include_raw: true`), en vergelijk met onze eigen
componenten om het ontbrekende patroon te vinden.

## Overige losse eindjes
- `docs/architecture/PROJECT_STATUS.md` heeft nog niet alle bevindingen van vandaag verwerkt in
  detail (deze handover is de primaire bron voor nu — kopieer relevante stukken over bij gelegenheid)
- Enkele bekende, niet-blokkerende vanilla-foutmeldingen blijven verschijnen in de log
  (Multiple map entities, verouderde vanilla vehicle-bestandssyntax) — genegeerd, niet MCF-gerelateerd