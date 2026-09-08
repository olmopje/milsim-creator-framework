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

## 6. Voorgestelde eerste stap — iteratief, geen vast eindprofiel

**Beslissing:** de unit wil niet vooraf kiezen tussen "licht minder clunky" en "richting arcade" — dit wordt uitgetest, niet in één keer vastgelegd. Dat verandert de aanpak: bouw de parameters zo dat ze **makkelijk herhaaldelijk bij te stellen zijn**, niet als losse harde waarden die je telkens moet opzoeken en handmatig aanpassen.

**Praktische invulling:**
1. **Sectie 2 eerst, met drie testprofielen i.p.v. één getal** — bijv. "Profiel A: 25% responsiever dan vanilla", "Profiel B: 50%", "Profiel C: 75% (dicht bij Arcade Movement)". Laat meerdere unit-leden alle drie proberen, niet alleen degene die het bouwt — persoonlijke voorkeur van één persoon is geen goede maatstaf voor een hele unit
2. **Verzamel feedback gestructureerd**, niet los "voelt goed/slecht" — vraag specifiek: voelt het nog "zwaar" genoeg tijdens een lange patrol? Mist iemand het sukkelgevoel bij vermoeidheid? Voelt het responsief genoeg tijdens CQB?
3. **Sectie 3 los onderzoeken** — andere parameter-familie, niet aannemen dat het met sectie 2 meekomt
4. **Sectie 4 pas als losstaand, groter vervolgtraject** — erken vooraf dat dit meer tijd kost dan de eerste twee
5. **Leg de gekozen waarde vast in de Config-laag-filosofie van het hoofdproject** (instelbaar, niet hardcoded) zodra er een voorkeur is — zo kan de unit later alsnog bijstellen zonder opnieuw in code te hoeven duiken, mocht de smaak na een paar maanden spelen veranderen

---

## 7. Open vragen om vroeg te beantwoorden

- Welke exacte config-parameters raakt Arcade Movement — zijn die direct in Workbench's Resource Browser te vinden op het karakter-prefab, of zit dat dieper?
- Is de turn-speed-curve (sectie 3) een losse camera-configuratie of gekoppeld aan dezelfde plek als de bewegingsparameters?
- Is `CharacterCommandHandlerComponent` vanuit script voldoende toegankelijk om een ADS-inputbuffer (sectie 4) te bouwen, of zit de bob-animatie-timing dieper in gesloten C++?
- **Beslist:** afstelling wordt iteratief uitgetest met meerdere profielen (zie sectie 6), geen vaste keuze vooraf — de vraag is nu praktisch: hoeveel testprofielen zijn haalbaar zonder de unit met te veel losse builds te belasten, en wie coördineert het verzamelen van feedback?
