# Milsim Creator Framework (MCF) — Architectuurplan

## 1. Visie

Een zelfgebouwd, modulair framework voor Arma Reforger dat missiemakers Eden-achtige narratieve diepte geeft — live bruikbaar in Game Master, opgebouwd rond een gedeelde Core, en van meet af aan geschikt voor grote PvE co-op groepen zonder performance-verval.

Geen dependency op bestaande third-party frameworks (Scenario Framework, Ci5, GME) — wel geïnformeerd door hun ontwerp.

---

## 2. Architectuur — lagenoverzicht

```
┌─────────────────────────────────────────────────┐
│  GM-INTEGRATIELAAG                               │
│  (SCR_EditableEntityComponent, live attributen)  │
├─────────────────────────────────────────────────┤
│  CORE                                            │
│  Event Bus · Object Identity · Module Registry   │
│  Config-laag · Replicatie-helper · Tick Manager  │
├─────────────────────────────────────────────────┤
│  NARRATIEVE LAAG          │  WERELD-SYSTEMEN     │
│  Objective Nodes          │  Hostility/Reputatie │
│  POI/Observation Nodes    │  Infrastructuur-net  │
│  Logic Nodes (AND/OR/CNT) │  Civiele AI-gedrag   │
│  Voice Line / Comms       │  Waypoint+Animatie   │
├─────────────────────────────────────────────────┤
│  PERSISTENTIE                                    │
│  Save/Load (native serialisatie + module-state)  │
├─────────────────────────────────────────────────┤
│  PERFORMANCE-LAAG (doorsnijdt alle bovenstaande) │
│  Dynamic spawn/despawn · Throttling · Budgetten  │
└─────────────────────────────────────────────────┘
```

---

## 3. Core — de fundering

| Component | Functie |
|---|---|
| **Event Bus** | Centraal pub/sub-systeem. Modules communiceren nooit direct — altijd via events. Voorkomt tight coupling. |
| **Object Identity** | Generiek component op elke relevante entity met een uniek tag/naam-veld. Modules vinden elkaar via tags, niet via harde references. |
| **Module Registry** | Elke module meldt zich aan met versie + dependencies. Bepaalt load-order, checkt compatibiliteit. |
| **Config-laag** | Eén centrale resource per scenario: welke modules actief zijn, met welke instellingen. Missiemaker hoeft nooit code te raken. |
| **Replicatie-helper** | Wrapper om RplComponent/RPC-boilerplate — modules hoeven netcode niet zelf te herimplementeren. |
| **Tick Manager** | Eén centrale update-loop i.p.v. losse `EOnFrame` per entity — cruciaal voor performance (zie sectie 7). |

### 3.1 Integratie-contracten (hoogste standaard — expliciet vastgelegd, niet impliciet verondersteld)

Dit zijn regels die **eenmalig op Core-niveau** gelden, zodat geen enkele module ze zelf hoeft te interpreteren of, erger, anders interpreteert dan een andere module.

- **Naamgevingsconventie & prefix.** Projectnaam: **Milsim Creator Framework (MCF)**. Alle classes/prefabs krijgen de vaste prefix `MCF_`, volgens BI's officiële *Editor Entity Naming Conventions* ("vervang `SCR_` door je eigen tag"). Daarbinnen krijgt elke module een eigen sub-namespace, zodat je aan de class-naam meteen ziet welke module verantwoordelijk is:

  | Namespace | Dekt |
  |---|---|
  | `MCF_Core_` | Event Bus, Object Identity, Module Registry, Tick Manager |
  | `MCF_AI_` | Civiele AI-gedrag (5.3), Ambient Life-gedragsprofielen (5.5), Compliance/ROE-logica (5.7) |
  | `MCF_Obj_` | Objective/POI/Logic Nodes (4.x) |
  | `MCF_Hostility_` | Hostility/Reputatie-manager (5.1) |
  | `MCF_Infra_` | Infrastructuur-netwerk/AI Warning (5.2) |
  | `MCF_Voice_` | Voice Line/Comms (4.4) |
  | `MCF_Interact_` | Interactie-hint-systeem (5.6) |
  | `MCF_AAR_` | Debrief-module (5.8) |

  Nieuwe modules die niet in deze tabel passen, krijgen pas een nieuwe namespace na overleg — voorkomt namespace-wildgroei.
- **Event-contract.** Elk event heeft een vast namespace-patroon (`Module_Actie`, bijv. `Objective_Complete`, `Hostility_ThresholdCrossed`) en een gedocumenteerd payload-schema. Geen enkel event wordt ad-hoc genaamd — nieuwe events worden centraal geregistreerd in de Module Registry, niet losjes verzonnen per module.
- **Authority-beleid.** Alle state-mutaties (objective-status, hostility-waarde, infrastructuur-status, ROE-uitkomst) zijn **server-authoritative**. Clients ontvangen uitsluitend gerepliceerde resultaten via de Replicatie-helper — nooit lokale voorspellingen die later gecorrigeerd moeten worden. Dit geldt voor élke module, zonder uitzondering.
- **Faction Alias-integratie.** De Core hergebruikt SF's bewezen `SCR_FactionAliasComponent`-patroon in plaats van een eigen factie-abstractie te verzinnen. Elke module die een factie nodig heeft (Hostility, ROE, Objective-condities) verwijst naar een Alias, niet naar een harde Faction Key — zo is een scenario herbruikbaar met andere factie-combinaties zonder herbouw.
- **GameMode-compatibiliteit.** De Core is **additief**: een los component naast `GameMode_Base`, geen vervanging ervan. Dit garandeert dat de Core naast vanilla Conflict/Combat Ops kan draaien als de unit dat ooit wil, in plaats van het framework te dwingen als exclusief scenario-type.
- **Validatie-pass bij missie-init.** Bij het starten van een scenario loopt een validatieronde die ontbrekende tag-referenties, verkeerd geconfigureerde condities en module-dependency-conflicten logt — met hetzelfde (W)/(E)-severity-onderscheid als SF's debug-systeem. Een missiemaker-typfout mag nooit stilzwijgend een systeem laten falen.
- **Event Bus-lifecycle-koppeling.** Listeners worden automatisch uitgeschreven wanneer hun entity wordt vernietigd/despawned — voorkomt dangling listeners en geheugenlekken zonder dat elke module dit zelf hoeft te beheren.

---

## 4. Narratieve laag

### 4.1 Objective Node
De basisbouwsteen voor verhalende taken.

**Attributen:**
- Titel / beschrijving
- **Visible on Map** (ja/nee) — toggle voor kaartmarker
- **Conditie-slot** — koppelbaar aan willekeurige boolean-check (reputatie, tijd, item-bezit, andere node-status)
- **On Complete → Event Out** — vuurt een event op de Event Bus af zodra voltooid, waar andere nodes op kunnen luisteren
- **On Fail → Event Out** (nieuw toegevoegd — elke objective moet ook een faalpad kunnen triggeren, anders wordt je verhaal een rechte lijn)
- **Intel-gate** (nieuw) — objective blijft verborgen/onduidelijk totdat een informatiebron (civiel, document, radio-intercept) hem "unlockt". Dit is de generalisatie van je "intel via civiel met goede reputatie"-voorbeeld: elke informatiebron kan een gate zijn, niet alleen civiele reputatie.

### 4.2 POI / Observation Node
Voor het "meerdere locaties monitoren"-patroon.

- Losse trigger-zones die elk activiteit rapporteren aan één gedeelde Observation-listener
- De listener bepaalt branching: welke POI het eerst rapporteert, bepaalt het vervolg
- **Toegevoegd idee: Observation-decay** — als een POI te lang niet bezocht wordt, kan de kans op een "gemiste gebeurtenis" (bijv. konvooi passeert ongezien) meetellen in een debrief-score. Geeft spelers een reden om patrouilles serieus te verdelen — sterk voor milsim.

### 4.3 Logic Nodes
AND / OR / Counter / Timer-nodes — generieke boolean- en telsystemen waar Objective- en POI-nodes op inpluggen. Zelfde concept als SF's LogicCounter, maar losgekoppeld zodat elk nodetype ze kan gebruiken.

### 4.4 Voice Line / Comms Node
- Scripting: node triggert een voice line/sequence op een gekoppelde entity (zelfde patroon als SF's Voice Over-acties)
- **Let op — apart werkproces:** dit vereist een audio-pipeline los van je Core-code: opnemen → importeren in Workbench → koppelen via `.acp`-config aan een `SCR_CommunicationSoundComponent`. Plan dit vroeg als je veel lijnen wil, want het schaalt niet automatisch mee met je scripting-snelheid.
- **Toegevoegd idee: Voice Line Priority Queue** — bij meerdere gelijktijdige triggers (bijv. twee objectives voltooien tegelijk) moet je bepalen welke voice line voorrang krijgt en welke wacht/vervalt. Zonder dit systeem overlappen lijnen elkaar en wordt het rommelig.

---

## 5. Wereld-systemen

### 5.1 Hostility / Reputatie-manager
Zoals eerder ontworpen: server-side singleton, per gebied/factie een vervalwaarde, gevoed door impact-hooks (burgerslachtoffers, destructie, hulp), gelezen door civiele AI-gedragsprofielen en door Objective-nodes (intel-gates).

### 5.2 Infrastructuur-netwerk — NIEUW, voor je AI Warning System
Dit is de generieke oplossing voor je communicatietoren-idee, en herbruikbaar voor vergelijkbare puzzels later (stroomnetten, waterzuivering, radar-ketens).

**Concept:** een graaf van gekoppelde nodes met afhankelijkheden.

```
[Generator] --(stroom)--> [Kabel-segment A] --> [Kabel-segment B] --> [Comms Tower] --> AI Warning actief
```

- Elke node heeft een status (actief/inactief) en een lijst afhankelijkheden
- Als een generator vernietigd wordt of een kabelsegment doorgesneden, propageert de statuswijziging door de graaf → de toren valt uit → het AI warning-systeem (bijv. QRF-oproep, artillerie-inzet, versterkingsalarm) wordt vertraagd of volledig uitgeschakeld
- **Intel-koppeling:** spelers moeten weten *waar* de zwakke schakel zit — dit koppel je terug aan je intel-systeem (recon, gevangenen ondervragen, documenten), zodat sabotage een beloning is voor goede verkenning, niet een gok
- **Herstelbaarheid (nieuw idee):** vijandelijke AI kan gescript worden om een generator te repareren of kabels te vervangen na verloop van tijd — dit voorkomt dat één sabotage-actie de hele missie permanent "uitzet" en houdt spanning erin
- **Performance:** de graaf hoeft alleen herberekend te worden bij een statuswijziging (event-driven), niet continu gepolld — sluit direct aan bij de performance-principes in sectie 7

### 5.3 Civiele AI-gedrag
Utility AI/behavior tree die hostility-waarde uitleest en schakelt tussen gedragsprofielen (neutraal → angstig → tippend aan OPFOR → actief verzet). Perception/sensor-hooks i.p.v. simpele proximity-checks.

### 5.4 Waypoint + Animatie-module
Custom waypoint-subklasse die bij aankomst een specifieke animatie triggert via de CharacterAnimationComponent/animgraph.

### 5.5 Ambient Life / Pattern-of-Life-module — NIEUW
Geen echte A-life-simulatie (te zwaar, te complex) — een lichte, goedkope illusie van een levend dorp, gebouwd bovenop de Waypoint+Animatie-module die al gepland staat.

**Kernconcept: Lifestyle-POI's + Rollen**
- **Lifestyle-POI**: een variant op de POI-node (4.2), maar dan getagd met een rol in plaats van een observatiefunctie — Winkel, Huis, Marktkraam, Checkpoint, Put. Elke Lifestyle-POI heeft een "slot" (hoeveel NPC's er tegelijk kunnen zijn) en een set idle-animaties die daar passen.
- **Actor-archetypes** (gedragsprofielen, geen nieuwe AI-logica — gewoon een voorgeprogrammeerde cyclus):
  - **Shopkeeper** — stationair op één Lifestyle-POI, cyclet door idle-animaties (uitstallen, wachten, opruimen)
  - **Klant/Civiel** — roamt tussen 2-4 Lifestyle-POI's in een dorp, "browse"-animatie voor een willekeurige duur per stop, dan door naar de volgende — exact hetzelfde patroon als een Patrol-waypoint-cyclus, alleen civiel geframed
  - **Militie/vijand off-duty** — zelfde roam-cyclus als Klant, maar met vijandelijke factie: geeft de speler het gevoel dat de tegenstander ook "gewoon leeft" in het dorp i.p.v. constant in gevechtshouding te staan
  - **Verkeer** — voertuigen die tussen dorpen rijden op wegen, spawnen buiten zichtlijn, despawnen op afstand (zelfde patroon als bestaande Ambient Civilians-achtige mods)
- **"Fake conversations"** — twee NPC's die toevallig op dezelfde POI staan, spelen periodiek een gesynchroniseerd paar idle/praat-animaties voor een paar seconden. Geen dialoogsysteem, geen logica — puur getimede animatie-paren die de illusie van interactie geven.

**Integratie met bestaande modules (geen nieuwe dependencies, alles via de Event Bus):**
- **Hostility-manager** — stijgt de hostility-waarde in een gebied, dan luistert deze module mee en stuurt shopkeepers "naar binnen", laat civielen de straat verlaten, en zet militie-NPC's van "winkelen" naar alert-gedrag. Geen nieuwe koppeling nodig, gewoon een event-listener.
- **Reputatie/intel-systeem** — een shopkeeper- of civiel-NPC uit deze module kán tegelijk de intel-gever zijn uit sectie 4.1. Spelers kunnen visueel geen onderscheid maken tussen een decoratieve en een functionele NPC — precies de "fake it until je het echt nodig hebt"-gedachte.

**Performance — dit is de zwaarste module qua NPC-aantal, dus extra streng:**
- Volledig gebonden aan **Dynamic Spawn/Despawn per Area**: buiten spelersbereik worden Lifestyle-NPC's niet gesimuleerd of zelfs gespawnd
- **Global budget** op aantal actieve Ambient Life-NPC's tegelijk, instelbaar per scenario (sluit aan op de budgetten uit sectie 7)
- **Gespreide update-timers**: niet alle NPC's herberekenen hun volgende animatie in hetzelfde frame — willekeurige offset per NPC om CPU-pieken te voorkomen
- **Geen perception/AI-overhead voor pure decor-NPC's** — een shopkeeper die nooit gevaar hoeft te detecteren, hoeft niet dezelfde dure sensor-checks te draaien als een gevechts-AI

### 5.6 Interactie-hint-systeem — NIEUW
Slim omgaan met interactie betekent hier vooral: **de meeste NPC's kosten helemaal niets**, en alleen een kleine subset heeft ooit een kans op een écht bruikbare reactie. Geen dialoogbomen, geen NLP, geen per-NPC scriptwerk — een gelaagd tier-systeem met tekstpools.

**Drie tiers, oplopend in kosten:**

| Tier | Wie | Interactie | Kosten |
|---|---|---|---|
| **0 — Decor** | Meerderheid van Ambient Life-NPC's | Geen enkele interactiemogelijkheid | Nul — geen component nodig |
| **1 — Barker** | Kleine subset, door missiemaker getagd | `UserAction` ("Aanspreken") toont één regel uit een generieke tekstpool | Eén component + gedeelde tekstpool, geen unieke content per NPC |
| **2 — Informant-capable** | Server-side onzichtbaar gemarkeerde subset van Tier 1 | Zelfde interactie, maar met een kanspercentage op een "pointer"-regel i.p.v. generieke filler | Eén extra attribuut (kanspercentage) + pointer-tekstsjablonen |

**Hoe de "pointer" werkt (het cryptische, realistische deel):**
- Bij interactie met een Tier 2-NPC rolt het systeem een percentage. Bij een treffer krijgt de speler een **sjabloon-regel** met een ingevulde referentie, bijv.: *"Misschien kan %1 je helpen."* — waarbij %1 wordt ingevuld met de rol/naam van de daadwerkelijke intel-gever (sectie 4.1) of een vage richting ("bij de haven", "de man met de rode pet")
- Bij een misser krijgt de speler gewoon een Tier 1-generieke regel — voor de speler niet te onderscheiden van een NPC die toevallig niets weet
- Dit levert **speurwerk zonder dialoogbomen**: spelers moeten rondvragen, en de meeste antwoorden zijn ruis — precies zoals je vroeg, "fake it until you make it"

**Kanspercentage — gevoed door bestaande state, geen nieuw systeem nodig:**
- Basiskans ingesteld door missiemaker per Tier 2-NPC
- **Gemodificeerd door de hostility/reputatie-waarde** van het gebied (sectie 5.1) — hoger vertrouwen, hogere kans op een bruikbare pointer
- **Herprobeerbaar, maar met tijd-cooldown i.p.v. eenmaligheid** — een gemiste roll is niet permanent verloren; de speler kan het later opnieuw proberen als de reputatie in het gebied is gestegen. Om te voorkomen dat dit ontaardt in spam-klikken tot een treffer, geldt een vaste minimale hertry-tijd per NPC (bijv. enkele minuten in-game), losstaand van reputatie-groei. Dit behoudt het realistische "vertrouwen winnen loont" zonder dat de kanspercentage-mechaniek zinloos wordt door oneindige directe herhaling.

**Content-authoring blijft licht:** missiemakers schrijven een handvol generieke Tier 1-regels en een handvol pointer-sjablonen per scenario — geen unieke tekst per individuele NPC nodig. Dit past bij de "niet te diep, wel levend" doelstelling: de content-inspanning schaalt met het aantal *soorten* interactie, niet met het aantal NPC's.

### 5.7 Compliance / ROE-interactielaag — NIEUW
Een Ready or Not-achtige laag: spelers kunnen NPC's onder schot dwingen tot nalevingsgedrag. Sterke aanvulling op het milsim-doel, want dit oefent letterlijk escalation-of-force/ROE-procedures — geen decoratie, maar trainingswaarde.

**Kerncommando's:**
- **"Drop your weapon"** — gericht op gewapende AI (vijandelijk of onduidelijke status). Bij naleving: AI laat wapen vallen (hergebruikt het bestaande Item Safeguard/inventory-drop-mechanisme uit sectie 6), gaat in een "hands up"-animatiestaat, wordt arresteerbaar/vervoerbaar.
- **"Stand back"** — gericht op ongewapende NPC's (civiel). Bij naleving: NPC stopt met bewegen/nadert niet verder. Simpeler en goedkoper dan de wapen-drop-variant, geen item-interactie nodig.

**Trigger-mechanisme (het haalbare deel):**
- Een `UserAction`-achtige context-actie (zelfde patroon als de Tier-interacties in 5.6), beschikbaar zodra: speler wapen geheven heeft, AIM-vector de NPC binnen bereik/kegel raakt, en line-of-sight vrij is
- **On-demand check, geen polling** — de aim/LOS-conditie wordt alleen geëvalueerd op het moment van interactie-poging, niet continu per NPC-speler-paar. Cruciaal voor performance bij meerdere spelers/NPC's tegelijk.
- **Optioneel gekoppeld aan een voice line + keybind**, niet aan echte spraakherkenning (zie kanttekening hieronder)

**Nalevingskans — hergebruikt bestaande systemen, geen nieuw mechanisme:**
- Basiskans per factie/eenheidstype, instelbaar door missiemaker (een doorgewinterde militielid is minder snel compliant dan een lagere-moraal-eenheid)
- Gemodificeerd door dezelfde morele/threat-state-factoren die al gepland staan voor AI-gedrag (suppressie, aantal vijanden in de buurt, geïsoleerd zijn) — geen apart systeem nodig, hetzelfde threat-state-concept dat al in 5.3 zit
- Bij weigering: AI vervalt naar normaal gedrag (vecht/vlucht) — geen aparte "weigerings-animatie" nodig, gewoon terugvallen op bestaand gedrag

**Consequentie-koppeling met Hostility-manager (sectie 5.1):**
- Correct gebruik op een daadwerkelijke dreiging: neutraal tot positief voor reputatie
- "Stand back" tegen een ongewapende civiel die niet dreigt, of excessief geweld na naleving: rechtstreeks negatief voor de hostility-waarde in het gebied — hergebruikt de bestaande "friendly-caused casualties/ROE"-onderscheiding uit sectie 5.1. Zo wordt deze module een **oefenmiddel voor escalation-of-force**, niet alleen een leuke actie-knop.

**Kanttekening — voice-commando via spraakherkenning: waarschijnlijk niet haalbaar binnen normale modding.**
Reforger's VoN-audiosysteem geeft mods geen toegang tot ruwe audio of spraak-naar-tekst — het is puur een doorgeefluik dat gecomprimeerde audio tussen spelers routeert. Een echte "speler zegt het hardop → AI reageert"-koppeling zou een extern proces vereisen dat buiten het spel om luistert, wat noch stabiel noch verstandig is voor een unit-project (anti-cheat/EULA-risico in MP). **Pragmatisch alternatief met vrijwel hetzelfde gevoel:** een keybind/radiaal-commando dat gelijktijdig (a) de bijpassende voice line hoorbaar afspeelt via VoN naar andere spelers, en (b) de AI-logica triggert. De speler "roept" het dus echt hardop het commando, alleen de detectie loopt via een knop, niet via spraakherkenning.

### 5.8 After-Action Review (AAR) / Debrief-module — HERSTELD
Vroeg in de brainstorm genoemd ("milsim-eenheden zijn dol op debriefs"), maar in eerdere versies van dit document niet uitgewerkt. Dit is de natuurlijke plek om de Event Bus-geschiedenis van een hele sessie samen te vatten, en kost weinig extra bouwwerk omdat elke module al events op de Bus zet.

- **Luistert passief mee** op de Event Bus gedurende de hele missie — geen aparte data-verzameling per module nodig, alleen een centrale logger die alle relevante events opslaat met tijdstempel
- **Aggregeert bij missie-einde:** voltooide/gefaalde objectives, ROE-compliance-uitkomsten (correcte vs. onterechte "stand back"/"drop weapon"-acties), gemiste POI-observaties, hostility-verloop per gebied over tijd
- **Output:** een samenvattend debrief-scherm of exporteerbaar tekstbestand — waardevol voor milsim-eenheden die sessies naderhand bespreken
- **Performance:** puur event-gedreven, geen polling — de goedkoopste module in het hele framework omdat hij nooit iets hoeft te initiëren, alleen te registreren

### 5.9 Logistiek/Supply — BACKLOG, bewust niet in eerste scope
Ook vroeg genoemd, maar bewust **niet** in de kernroadmap opgenomen om scope-kruip te voorkomen. Zou leunen op vergelijkbare Area/Dynamic-Despawn-patronen als de rest van het framework (supply-punten als Lifestyle-POI-achtige nodes, konvooien als POI/Observation-ketens), dus technisch geen nieuw patroon — wel een bewuste latere uitbreiding, geen fase-0-t/m-9-verplichting.

---

## 6. GM-integratielaag

Elke node hierboven wordt een custom prefab met:
- `SCR_EditableEntityComponent` + `PLACEABLE`-flag → verschijnt in GM's catalogus
- Custom Editor Attributes (sliders, dropdowns) → live configureerbaar door de GM zonder Workbench
- Automatische opname in de mission save-serialisatie (native BI-systeem)

---

## 7. Performance-bewuste ontwerpprincipes

Dit raakt letterlijk elke laag hierboven, dus expliciet als eigen sectie:

1. **Event-driven boven polling.** Elke node reageert op events, niet op een eigen tick-loop. Waar een check echt periodiek moet (bijv. hostility-decay), loopt die via de centrale **Tick Manager** met een instelbare update-rate — nooit per-entity `EOnFrame`.
2. **Dynamic spawn/despawn per Area**, zelfde patroon als SF: content buiten spelersbereik wordt niet gesimuleerd. Areas onthouden hun state (posities, voltooiing) zodat despawn geen voortgang kost.
3. **Graaf-updates alleen bij wijziging** (infrastructuur-netwerk, reputatie) — geen continue herberekening.
4. **Gebundelde replicatie.** Combineer gerelateerde state in één `RplProp`-component in plaats van los te repliceren — minder netwerkoverhead, makkelijker te debuggen.
5. **Configureerbare budgetten.** Missiemaker kan per scenario een max. aantal actieve AI-groepen, actieve triggers en actieve infrastructuur-nodes instellen — voorkomt dat een enthousiaste GM de server op de knieën krijgt.
6. **Voice line-queue** (zie 4.4) voorkomt audio-stacking maar bespaart ook onnodige gelijktijdige sound-instanties.
7. **Debug-overlay vanaf dag 1** (zoals SF's debug menu) — laat live zien welke nodes actief zijn, welke graaf-status infrastructuur heeft, welke hostility-waarde een gebied heeft. Dit is niet alleen voor jullie handig tijdens bouwen, maar ook een performance-diagnosetool: je ziet meteen als iets onnodig blijft draaien.
8. **Tick-prioriteitsniveaus** i.p.v. één vlakke update-rate voor de hele Tick Manager: een "kritiek"-laag met korte interval voor gameplay-bepalende checks (bijv. ROE-nalevingsstatus), en een "cosmetisch"-laag met een veel langere interval voor decor (Ambient Life-animatiekeuzes). Eén rate voor alles is ofwel te traag voor wat ertoe doet, ofwel nodeloos duur voor wat niet opvalt.
9. **Event Bus-lifecycle-koppeling** — listeners worden automatisch uitgeschreven bij entity-destructie (zie sectie 3.1), zodat despawnende NPC's en verwijderde nodes geen dangling listeners achterlaten die stil geheugen blijven vasthouden.

---

## 8. Persistentie / Save-Load

- Basis: native `SCR_EditableEntityComponent`-serialisatie (positie, staat, custom attributen) — al onderdeel van het save-bestand
- Aanvullend: een eigen **Module State Serializer** voor data die geen entity-attribuut is — reputatiewaarden per gebied, infrastructuur-netwerkstatus, Event Bus-geschiedenis (welke objectives al voltooid zijn)
- Doel: missiemakers kunnen prebuilden én live GM-sessies kunnen tussentijds opslaan/hervatten zonder voortgang te verliezen

---

## 9. Gefaseerde roadmap

| Fase | Doel |
|---|---|
| **0 — Proof of concept** | Eén Trigger Zone-entity: GM-plaatsbaar, live attributen, volledige save/load-cyclus getest, **plus** het event-naamgevingscontract, authority-beleid en validatie-pass uit sectie 3.1 vanaf het begin toegepast — dit zijn geen latere toevoegingen maar fundament |
| **1** | Objective Node volledig (titel, map-visibility, conditie-slot, on-complete/on-fail events, intel-gate) |
| **2** | POI/Observation Node + Logic Nodes, Event Bus-koppeling tussen beide |
| **3** | Hostility/Reputatie-manager + civiele gedragshaak + Faction Alias-integratie |
| **4** | Infrastructuur-netwerk (AI Warning System) + intel-koppeling |
| **5** | Waypoint+Animatie-module, Voice Line-module + priority queue |
| **6** | Ambient Life/Pattern-of-Life-module (Lifestyle-POI's, actor-archetypes, fake conversations) + interactie-hint-systeem (tiers, pointer-sjablonen) — bouwt direct op Fase 5 |
| **7** | Compliance/ROE-interactielaag (gunpoint, drop weapon, stand back) + koppeling aan Hostility-manager |
| **8** | AAR/Debrief-module — aggregeert de Event Bus-geschiedenis van alle voorgaande fases, dus pas zinvol als afsluiter |
| **9** | Performance-pass: Tick Manager-prioriteitsniveaus, budgetten, debug-overlay verfijnen onder belasting (test met 40+ spelers én druk bevolkt dorp tegelijk) |
| **10** | GM-attribuut-UI polish + documentatie voor andere missiemakers in de unit (doorlopend vanaf Fase 0, niet pas hier beginnen) |

---

## 10. Openstaande ontwerpvragen om vroeg te prototypen

- Hoe robuust is custom-attribuut-serialisatie in de praktijk bij geneste node-hiërarchieën? (risico geïdentificeerd in fase 0)
- Hoeveel gelijktijdige infrastructuur-graafnodes kan de Tick Manager aan voor je performance-doel acceptabel blijft?
- Hoe ver kun je "herstelbare" sabotage (generator-reparatie door AI) laten gaan voordat het voor spelers frustrerend aanvoelt i.p.v. spannend?
