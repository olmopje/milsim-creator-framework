# Schieten vanuit voertuig als passagier — technisch deelproject

Los document, bewust losgekoppeld van het narratieve missie-framework (`reforger-milsim-framework-blueprint.md`). Dit raakt character-animatie en het compartment-systeem, niet de Core/Event Bus-architectuur.

---

## 1. Uitgangssituatie

- **Niet vanilla**, en door Bohemia (Klamacz) expliciet niet gepland — mede omdat community-mods het al proberen op te lossen.
- Bestaande opties zijn risicovol om op te bouwen:
  - **DRIVE-BY** — bekendste mod, maar ontwikkelaar is niet transparant/onderhoudt niet actief. Veel servers draaien hem daarom bewust niet.
  - Een **nieuwere mod** dook op medio 2026 (via een showcase-video) — onbevestigde kwaliteit/onderhoudsstatus, nog niet grondig te beoordelen.
- Conclusie: zelfstandig bouwen is de veiligste keuze, consistent met je eerdere voorkeur om niet op mogelijk-buggy dependencies te leunen.

---

## 2. Waarom dit een ander technisch domein is dan je missie-framework

Je Core/Event Bus/Objective-systeem draait op **scenario-logica**: events, condities, state-management. Dit hier draait op:
- **Character-animatie** (vuurposes per zithouding, blending tijdens voertuigbeweging)
- **Compartment-systeem** (welke stoel laat welk gedrag toe)
- **Hit-detectie vanuit een niet-standaard positie** (raycasts vanuit een zittende pose, door voertuiggeometrie heen)
- **Camera-gedrag** (ADS vanuit een raam voelt anders dan ADS te voet)

Dit hoort dus **niet thuis in je Core** — het is een losstaand gameplay-systeem, met hooguit een simpele aan/uit-toggle in je Config-laag zodat missiemakers kunnen kiezen of een scenario ermee rekening houdt (bijv. ambushes op konvooien minder eenzijdig maken).

---

## 3. Relevante bestaande bouwstenen (officieel, niet third-party)

| Component | Relevantie |
|---|---|
| **`BaseCompartmentManagerComponent`** | Definieert alle zit-slots van een voertuig (PilotCompartment, CargoCompartment, etc.) — hier registreer je welke stoelen "vuur-capabel" worden |
| **`BaseCompartmentSlot`** | Individuele stoel-definitie: bezetting, area-matching voor stoelwissels, exit-alignering |
| **`CompartmentAccessComponent`** | **Belangrijke vondst:** heeft al een methode die teruggeeft *"of we in een compartment zitten met ADS ingeschakeld"* — dit concept (ADS-vanuit-compartment) bestaat dus al in de engine, vermoedelijk gebruikt voor bestaande turret-gunner-posities. Dit is een sterk aanknopingspunt: je bouwt mogelijk een **uitbreiding** van een bestaand mechanisme, geen compleet nieuw systeem. |
| **Weapon Animation-pipeline (Workbench)** | Officiële BI-tutorial + voorbeeldproject (`SampleMod_AnimationWorkshop` op Bohemia's GitHub) voor het bouwen van eigen wapen-/houding-animaties — het juiste startpunt voor de vuurpose-animaties zelf |
| **Get Out/Door Info-systeem** | Recente engine-updates voegden fijnmazige controle toe over uitstap-animaties per deur/stoel — relevant als je wil dat spelers ook *tijdens* het schieten nog soepel kunnen uitstappen |

---

## 4. Realistische scope-inschatting

**Wat waarschijnlijk haalbaar is met redelijke inspanning:**
- Vuren vanuit **open posities** (laadbak van een pick-up/technical, open Ural-bak) — geen raamgeometrie waar doorheen geraycast moet worden, dus het dichtst bij het bestaande turret-ADS-concept
- Eén pilot-voertuig volledig werkend krijgen vóór je opschaalt naar de rest van je wagenpark

**Wat aanzienlijk meer werk is:**
- Vuren **door een raam** in een gesloten voertuig (Humvee, UAZ) — vereist per voertuigmodel controleren of de animatie niet door de deur/het portier clipt, en preciezere hit-detectie
- **Content schaalt met elk voertuigtype** — elke unieke voertuiggeometrie heeft mogelijk eigen aanpassingen nodig aan vuurhoek/animatie-clipping. Dit is geen eenmalige bouwkost maar een terugkerende contentkost per voertuig dat je wil ondersteunen.

**Balans-overweging (expliciet genoemd in de communitydiscussie):** Bohemia's terughoudendheid komt deels voort uit een balansvraag — te makkelijk vanuit een voertuig kunnen vuren ondermijnt het risico van transport. Iets om zelf bewust in te plannen: een cooldown, precisie-penalty, of beperkte vuurhoek vanuit de stoel, in plaats van vuren zonder nadeel toe te staan.

---

## 5. Voorgestelde eerste stap

Een kleine proof-of-concept, net als bij het missie-framework:

1. Kies **één open-laadbak-voertuig** als pilot (minst complexe animatie-casus)
2. Onderzoek hoe de bestaande ADS-in-compartment-vlag voor turret-posities precies werkt (`CompartmentAccessComponent`) — kopieer dat patroon in plaats van vanaf nul te bouwen
3. Bouw één vuurpose-animatie via de officiële Weapon Animation-tutorial/sample-project
4. Test hit-detectie en netwerk-gedrag met 2+ spelers voordat je opschaalt naar meer voertuigen

---

## 6. Open vragen om vroeg te beantwoorden

- Werkt de bestaande ADS-in-compartment-vlag ook op reguliere passagiersstoelen, of is die hard gekoppeld aan turret-specifieke logica?
- Hoeveel animatie-aanpassing per voertuigmodel is nodig om clipping te voorkomen — is dit vooraf in te schatten of alleen per voertuig te testen?
- Welke balans-maatregel (cooldown/precisie-penalty/vuurhoek-beperking) past het beste bij jullie milsim-stijl, gezien de balanszorgen die Bohemia zelf noemt?
