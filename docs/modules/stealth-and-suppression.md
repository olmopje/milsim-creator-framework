# Stealth & Suppressie-verbetering — technisch deelproject

Los document, zelfde reden als het voertuig-schiet-document: dit raakt wapenstatistieken, character-animatie en (mogelijk) AI-perceptie — niet de Core/Event Bus-architectuur van het missie-framework.

---

## 1. Uitgangssituatie — eerst de aanname corrigeren

"Silencers zijn nutteloos" klopt niet volledig. Bevestigd in vanilla:
- Suppressors dempen geluid en maskeren grotendeels de mondingsflits (niet volledig)
- Spelers melden bruikbare stealth-engagements op 400+ meter, omdat AI zonder zichtbare mondingsflits alleen op geluidsrichting kan terugvuren, met een forse precisie-penalty

**Het echte probleem:** er is geen subsonic-munitie beschikbaar via Arsenal. Kogels blijven supersoon en produceren dus altijd een hoorbare knal ("crack"), los van de suppressor. De suppressor doet zijn werk, de munitie ondermijnt het.

---

## 2. Relevante bestaande bouwstenen (officieel)

| Component | Relevantie |
|---|---|
| **`SCR_WeaponStatsManagerComponent`** | Officieel systeem waarmee attachments (inclusief suppressors) wapenstatistieken wijzigen (bijv. mondingssnelheid-factor). De juiste, gedocumenteerde plek om suppressor-effectiviteit en subsonic-munitie-gedrag te implementeren — geen nieuw systeem nodig. |
| **`InventoryItemComponent` custom attributes** | Basis waarop `WeaponStatsManagerComponent` leunt — nodig om een nieuwe munitie-variant (subsonic) als aparte, moddable entiteit te definiëren |
| **Perception Factor (uit Scenario Framework-acties, sectie 5.3 in het hoofdproject)** | Bevestigd bestaand concept: AI-perceptievermogen is al instelbaar als attribuut. Aanknopingspunt om geluidsdetectie per munitietype te differentiëren, mits het onderliggende systeem dat toelaat (zie open vraag hieronder) |
| **Scroll-wheel stance-systeem (vanilla)** | Community-breed als sterk ervaren t.o.v. Arma 3 — uitbreiden, niet vervangen |

**Belangrijke onzekerheid:** Bohemia documenteert het AI-detectiesysteem zelf expliciet niet ("ongoing development"). De stance/snelheid-naar-camouflage-koppeling die in Arma 3 hard vastligt, mag niet zomaar verondersteld worden 1-op-1 aanwezig te zijn in Reforger — dit moet eerst empirisch getest worden voordat er iets op gebouwd wordt.

---

## 3. Voorgestelde verbeteringen

### 3.1 Subsonic-munitie (grootste hefboom, laagste risico)
- Nieuwe munitie-variant per suppressor-compatibel kaliber, via `WeaponStatsManagerComponent`
- Significant lagere geluidsdetectie-straal dan standaardmunitie, in ruil voor een realistische penalty (lager effectief bereik/stopping power op afstand — subsonic munitie is in werkelijkheid ook zwakker)
- **Balans-eis:** geen "gratis onzichtbaarheid" — de penalty moet voelbaar genoeg zijn dat het een tactische keuze blijft (dichterbij sluipen, minder bereik) in plaats van een strict-betere optie

### 3.2 Stance/camouflage-koppeling verifiëren en zo nodig expliciet maken
- Eerst testen: reageert vanilla-AI daadwerkelijk merkbaar anders op prone+stilstaand vs. crouch+bewegend vs. staand+rennend qua detectieafstand?
- Alleen als dat onvoldoende differentieert: een eigen laag toevoegen die stance+snelheid vertaalt naar een aanpassing op de Perception Factor-achtige waarde — **niet** blind bouwen zonder eerst te bevestigen wat vanilla al doet, anders bouw je iets dat al bestaat of dat conflicteert met de native logica

### 3.3 Fijnere prone/crouch-hoogteregeling
- Uitbreiding van het bestaande scroll-wheel stance-systeem met meer tussenstappen voor hoogte, specifiek gericht op net-over-dekking-mikken
- Dit is animatie-/character-controller-werk, zelfde technische domein als het voertuig-schiet-document — geen scripting-logica

### 3.4 Detectie-feedback voor de speler — Alert-systeem
De speler moet kunnen aanvoelen dat hij bijna ontdekt is (en nog kan ontsnappen) zonder dat het een gamey HUD-meter wordt die de immersie breekt — consistent met de "fake it until je het nodig hebt"-filosofie uit het hoofdproject.

**Kernidee: reageer op AI-staatsovergangen, niet op ruwe detectiewaardes.**
Dit ontkoppelt het systeem volledig van de onzekere interne perceptie-logica uit sectie 3.2 (die nog niet bevestigd is) — je luistert alleen naar *wanneer* de AI van staat wisselt, niet naar *hoe* die beslissing tot stand komt. **Vier staten** (uitgebreid t.o.v. het oorspronkelijke drie-staten-ontwerp, om het dynamischer te laten voelen — AI die eerst wil dichterbij komen in plaats van direct vol alarm te slaan):

| Overgang | Wat er gebeurt | Wat de speler waarneemt (diegetisch, geen HUD) |
|---|---|---|
| **Onwetend → Argwanend** | AI registreert iets ambigus (geluid/glimp), blijft ter plekke | Bark ("Wat was dat?"), hoofd-/lichaamsdraai naar de richting — subtiel, makkelijk te missen |
| **Argwanend → Onderzoekend** | AI verlaat zijn wacht-/ankerpositie om de bron te onderzoeken — hergebruikt de bestaande **Investigation Distance**-instelling (onderdeel van "Set Max Autonomous Distance") zodat de groep tijdelijk verder van zijn normale patrouillegebied mag afwijken | Zichtbare, voorzichtige beweging richting de laatst bekende locatie — wapen geheven, geen vuur. Dit is het spannendste moment: de speler ziet de AI dichterbij komen zonder zekerheid of hij ontdekt is |
| **Onderzoekend → In gevecht** | Visueel/geluidscontact bevestigd tijdens het onderzoeken | Onmiskenbaar: geschreeuw, vuur, radiomelding voor versterking (haakt in op het QRF-systeem) |
| **Onderzoekend → Onwetend (niets gevonden)** | Onderzoekstijd verstrijkt zonder bevestigd contact — AI keert terug naar zijn ankerpositie/patrouille | Aparte "stand-down"-bark/animatie — duidelijk afsluitmoment voor de speler: "ik ben ontsnapt" |
| **In gevecht → Onwetend (verloren contact)** | AI verliest contact tijdens het gevecht zelf | Zelfde stand-down-signaal als hierboven |

**Twee aparte ontsnappingsvensters, niet één:**
- **Tijdens Argwanend** (voor de AI besluit te gaan onderzoeken): blijft de speler stil/uit het zicht, dan valt de AI direct terug naar Onwetend zonder ooit te bewegen — het goedkoopste en meest wenselijke uitkomst
- **Tijdens Onderzoekend** (de AI is al onderweg): een langer venster, want de AI moet fysiek de afstand afleggen — geeft de speler tijd om zich te verplaatsen of dieper weg te duiken terwijl de AI dichterbij komt. Dit is het stuk dat het "dynamischer" maakt: de speler ziet de dreiging naderen in plaats van een binaire schakelaar

**Belangrijke onzekerheid, zelfde soort als in sectie 3.2:** het is nog niet bevestigd of vanilla Reforger's onderliggende Threat State-enum al native vier granulariteitsniveaus kent, of dat "Onderzoekend" een eigen sub-staat is die wij bovenop de bestaande Argwanend/In-gevecht-staten moeten bouwen met de Investigation Distance-instelling als bewegingsmiddel. **Eerst in Workbench inspecteren welke Threat State-waarden daadwerkelijk bestaan**, niet aannemen dat er al vier stappen native aanwezig zijn.

**Missiemaker-configureerbaar, niet hardcoded (past bij Config-laag-filosofie) — twee lagen, geen vaste standaard:**

Geen enkel niveau is "de" standaard — in plaats daarvan een plafond-en-voorkeur-model:

- **Scenario-plafond (missiemaker, server-side, geldt voor iedereen gelijk):** de missiemaker stelt per scenario het *maximaal toegestane* feedback-niveau in — puur diegetisch, diegetisch+cue, of diegetisch+indicator. Dit is de enige laag die de daadwerkelijke mechaniek raakt (het ontsnappingsvenster/timer blijft altijd server-authoritative en identiek voor iedereen, ongeacht dit plafond — eerlijkheid blijft gewaarborgd).
- **Persoonlijke voorkeur (speler, client-side, alleen presentatie):** binnen dat plafond kiest elke speler zelf hoeveel hij ziet/hoort — dit raakt alleen de eigen scherm-/audio-weergave, niet de onderliggende AI-logica of wat andere spelers zien. Een speler die liever puur diegetisch speelt kan dat kiezen, ook als de missiemaker een hogere indicator toestaat; een speler kan nooit méér krijgen dan het scenario-plafond.

Dit lost het "instelbaar"-vraagstuk structureel op: de unit hoeft niet één keuze voor iedereen te maken, en toegankelijkheidsvoorkeur (sommige spelers willen meer duidelijkheid) ondermijnt nooit de eerlijkheid tussen spelers, omdat de mechaniek zelf nooit verandert — alleen wat je er zelf van te zien/horen krijgt.

**Integratie (geen nieuwe Core-onderdelen nodig):**
- Loopt volledig via de Event Bus, binnen de `MCF_AI_`-namespace uit het hoofdproject
- Zuiver event-gedreven — kost niets zolang er geen staatsovergang plaatsvindt, sluit direct aan bij de performance-principes. De Onderzoekend-fase kost wél iets extra: de AI-groep beweegt daadwerkelijk, dus dit valt onder de reguliere AI-simulatiekosten, niet onder "gratis event-overhead" — geen misvatting daarover laten bestaan
- De keuze tussen de drie feedback-niveaus is een Config-laag-attribuut per scenario, niet een globale instelling — verschillende missies binnen dezelfde unit kunnen een ander niveau kiezen

**Afhankelijkheid, expliciet benoemd:** dit systeem werkt ongeacht de uitkomst van het onderzoek in sectie 3.2 (stance/camouflage-koppeling) — het reageert op staatsovergangen, niet op de onderliggende detectieformule. Kan dus onafhankelijk gebouwd en getest worden.

---

## 4. Wat hier expliciet niet bij hoort (scope-bewaking)

- Geen volledige eigen AI-perceptie-engine bouwen — dat zou het "ongoing development"-systeem van Bohemia zelf dupliceren en voortdurend uit sync raken met engine-updates
- Geen ghillie-suit-camouflagesysteem in deze fase — apart, groter onderwerp (texture/materiaal-gebaseerde camouflage-coëfficiënten), pas oppakken als dit fundament staat

---

## 5. Voorgestelde eerste stap

1. **Test eerst, bouw daarna:** meet in Workbench hoe de huidige AI daadwerkelijk reageert op verschillende houding/snelheid-combinaties, voordat er één regel code voor stap 3.2 geschreven wordt
2. Bouw parallel de subsonic-munitie (3.1) — laagste risico, duidelijkste winst, hangt niet af van de uitkomst van de AI-test
3. Stance-hoogteregeling (3.3) als losse, onafhankelijke verbetering — kan altijd, ongeacht uitkomst van 1 en 2
4. **Detectie-feedbacksysteem (3.4) kan meteen parallel starten** — onafhankelijk van 1 t/m 3, omdat het alleen op AI-staatsovergangen reageert, niet op de onderliggende detectiewaarde. Goede kandidaat om als eerste tastbare resultaat te tonen: kost weinig bouwwerk, geeft direct speelbare feedback.

---

## 6. Open vragen om vroeg te beantwoorden

- Reageert vanilla Reforger-AI merkbaar op stance/snelheid voor visuele detectie, of is dit systeem nog te onvolwassen om op te bouwen?
- Is er al een audibleFire-achtige coëfficiënt per munitietype in de huidige game-data aanwezig die hergebruikt kan worden, of moet die volledig nieuw gedefinieerd worden?
- Welke realistische penalty voor subsonic-munitie past het beste bij milsim-balans zonder frustrerend te worden (bereik? stopping power? beide, afgezwakt)?
- Is `AI Threat State`/`On Threat State Changed` rechtstreeks bruikbaar zoals SF het exposet, of moet er een eigen wrapper omheen om de drie feedback-niveaus (puur diegetisch / subtiele cue / expliciete indicator) te ondersteunen?
- **Nieuw:** kent vanilla Reforger's Threat State-enum al vier granulariteitsniveaus (inclusief een apart "onderzoekend"-stadium), of moet dat als eigen sub-staat bovenop de bestaande staten gebouwd worden met Investigation Distance als bewegingsmiddel? Eerste stap: dit in Workbench opzoeken vóór er één regel state-machine-code geschreven wordt.
- Hoe lang moet het Onderzoekend-venster duren om "dynamisch en spannend" aan te voelen zonder frustrerend traag te worden — dit is een gevoelswaarde die alleen via speeltests vastgesteld kan worden, niet via een formule.
- Welk feedback-plafond is een redelijke default voor nieuwe scenario's als de missiemaker niets instelt — of moet dit veld verplicht expliciet gekozen worden bij scenario-setup (aansluitend bij de validatie-pass uit het hoofdproject die nooit stil mag falen)?
