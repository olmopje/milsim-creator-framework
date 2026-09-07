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

---

## 4. Wat hier expliciet niet bij hoort (scope-bewaking)

- Geen volledige eigen AI-perceptie-engine bouwen — dat zou het "ongoing development"-systeem van Bohemia zelf dupliceren en voortdurend uit sync raken met engine-updates
- Geen ghillie-suit-camouflagesysteem in deze fase — apart, groter onderwerp (texture/materiaal-gebaseerde camouflage-coëfficiënten), pas oppakken als dit fundament staat

---

## 5. Voorgestelde eerste stap

1. **Test eerst, bouw daarna:** meet in Workbench hoe de huidige AI daadwerkelijk reageert op verschillende houding/snelheid-combinaties, voordat er één regel code voor stap 3.2 geschreven wordt
2. Bouw parallel de subsonic-munitie (3.1) — laagste risico, duidelijkste winst, hangt niet af van de uitkomst van de AI-test
3. Stance-hoogteregeling (3.3) als losse, onafhankelijke verbetering — kan altijd, ongeacht uitkomst van 1 en 2

---

## 6. Open vragen om vroeg te beantwoorden

- Reageert vanilla Reforger-AI merkbaar op stance/snelheid voor visuele detectie, of is dit systeem nog te onvolwassen om op te bouwen?
- Is er al een audibleFire-achtige coëfficiënt per munitietype in de huidige game-data aanwezig die hergebruikt kan worden, of moet die volledig nieuw gedefinieerd worden?
- Welke realistische penalty voor subsonic-munitie past het beste bij milsim-balans zonder frustrerend te worden (bereik? stopping power? beide, afgezwakt)?
