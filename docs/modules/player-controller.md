# Player Controller-verbetering — technisch deelproject

Los document, zelfde reden als de andere technische deelprojecten: dit raakt character-controller/input/camera, niet de missie-scriptinglaag van het hoofdframework.

---

## 1. Uitgangssituatie — drie aparte klachten, drie aparte oorzaken

Niet één probleem, en dus niet één fix:

1. **Bewegingsresponsiviteit** (acceleratie/afremming voelt traag) — een **waarde-tweak**, bewezen moddable
2. **Muis-turn-speed-curve** (snel flicken draait nauwelijks, gematigd bewegen draait snel) — een **curve/smoothing-instelling**, apart probleem van #1
3. **ADS-inputtiming** (klik om te richten wordt gemist tijdens de bob-animatie) — een **input-buffering-probleem**, fundamenteel anders van aard dan de eerste twee

---

## 2. Bewijs dat #1 moddable is

De workshop-mod **"Arcade Movement"** doet al precies dit: 2x responsievere acceleratie/stopsnelheid, verwijdert de turn-speed-reductiefactor, versnelt sluipmodus, onbeperkte stamina. Dit bewijst dat deze parameters blootgesteld en aanpasbaar zijn.

**Bewuste keuze: niet op deze mod bouwen, wel dezelfde onderliggende parameters gebruiken.** Zelfde overweging als eerder bij Scenario Framework/GME/Ci5 — onbekende onderhoudsstatus, en "arcade" gaat waarschijnlijk verder dan wat een milsim-unit wil (onbeperkte stamina bijvoorbeeld ondermijnt tactische vermoeidheid als spelconcept). Doel: **dezelfde knoppen vinden, een eigen, subtielere afstelling kiezen** — responsiever zonder de tactische zwaarte te verliezen die milsim juist waardeert.

---

## 3. Muis-turn-speed-curve — apart probleem, aparte aanpak

Community-rapportage: bij een snelle muis-flick draait het personage nauwelijks, bij gematigde constante beweging juist soepel — wijst op een **curve/clamping-probleem**, niet gewoon een sensitivity-schaal (spelers melden dat zelfs 200% sensitivity het onderliggende gedrag niet oplost, wat bevestigt dat het geen simpele schaalfactor is). Dit moet apart geïdentificeerd worden van de bewegingsacceleratie-parameters uit sectie 2 — waarschijnlijk een aparte curve-configuratie voor camera-/aim-turn-rate.

**Eerste stap:** in Workbench de camera-/inputconfiguratie doorzoeken op een curve- of clamp-waarde voor turn-rate, los van de wandel/rensnelheid-parameters.

---

## 4. ADS-inputtiming — fundamenteel ander soort probleem

Dit is geen waarde-tweak maar een **input-handling-probleem**: de klik om te richten wordt genegeerd als hij precies tijdens de bob-animatie van het wapen-omhoog-brengen valt, in plaats van onthouden en alsnog uitgevoerd te worden zodra de animatie het toelaat.

**Mogelijk aanknopingspunt:** eerdere engine-updates voegden `CharacterCommandHandlerComponent.IsItemActionLoopTag` en `FinishItemUse()` toe voor continue context-acties (herladen, repareren) — dit suggereert dat er al scriptbare hooks in de command-handler zitten voor dit soort timing-problemen. **Nog niet bevestigd of dit specifieke ADS-inputprobleem via dezelfde hooks oplosbaar is** — vereist directe inspectie in Workbench, niet aannemen.

**Realistische inschatting:** dit is aanzienlijk meer werk dan sectie 2/3, want het raakt een state-machine/timing-probleem in plaats van een blootgestelde waarde. Een "input-buffer" bouwen (onthoud de ADS-intentie, voer hem uit zodra de animatiestaat het toelaat) is het juiste patroon, maar vereist eerst te bevestigen dat de command-handler dat toelaat vanuit script.

---

## 5. Wat hier expliciet niet bij hoort

- Geen totale controller-vervanging of eigen inputsysteem — te riskant, te veel kans op het introduceren van nieuwe bugs in iets dat elke speler continu raakt
- Head-bob/camera-smoothing (los gemelde klacht, "nausea" bij oneffen terrein) — apart, kleiner onderwerp, pas oppakken als sectie 2-4 stabiel zijn

---

## 6. Voorgestelde eerste stap

1. **Sectie 2 eerst** — laagste risico, bewezen moddable, directe speelervaring-winst. Vind de exacte parameters die Arcade Movement aanpast (niet de mod zelf gebruiken, wel als kaart om te weten waar te zoeken in Workbench) en stel een eigen, mildere afstelling samen
2. **Sectie 3 los onderzoeken** — andere parameter-familie, niet aannemen dat het met sectie 2 meekomt
3. **Sectie 4 pas als losstaand, groter vervolgtraject** — erken vooraf dat dit meer tijd kost dan de eerste twee

---

## 7. Open vragen om vroeg te beantwoorden

- Welke exacte config-parameters raakt Arcade Movement — zijn die direct in Workbench's Resource Browser te vinden op het karakter-prefab, of zit dat dieper?
- Is de turn-speed-curve (sectie 3) een losse camera-configuratie of gekoppeld aan dezelfde plek als de bewegingsparameters?
- Is `CharacterCommandHandlerComponent` vanuit script voldoende toegankelijk om een ADS-inputbuffer (sectie 4) te bouwen, of zit de bob-animatie-timing dieper in gesloten C++?
- Welke afstelling van "responsiever maar niet arcade" voelt goed voor de unit — dit is een gevoelswaarde die alleen via speeltests met meerdere leden vastgesteld kan worden, niet via een formule
