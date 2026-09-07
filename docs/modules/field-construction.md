# Field Construction-module (FOB-bouwen buiten Conflict)

Los document, zelfde reden als de andere technische deelprojecten: dit raakt het bestaande supply-/constructiesysteem van het basisspel, niet de narratieve Core/Event Bus-logica an sich — al is de integratie ermee wél waardevol (zie sectie 4).

---

## 1. Uitgangssituatie — niet opnieuw uitvinden

Conflict heeft al een volwaardig bouwsysteem ("Free Roam Building", sinds 0.9.7.85):
- Bouwen overal ter wereld, zolang een speler dicht bij een voertuig staat dat de vereiste bouwmaterialen ("supplies") meedraagt
- **Twee-staps-flow:** blueprint plaatsen (materialen worden direct afgeschreven) → een speler met een schop moet het fysiek afmaken. Blijft een blueprint onbebouwd, zijn de materialen al kwijt — een bewuste risico/beloning-mechaniek die we willen behouden, niet omzeilen
- Bepaalde structuren zijn rank-gated (bijv. mortier-emplacement) en factie-specifiek (FIA-FOB's waren ooit per ongeluk universeel bouwbaar — inmiddels gefixed)

**Kernprincipe voor dit deelproject:** hergebruik de bestaande supply-economie en blueprint-mechaniek. Bouw geen parallelle economie — voeg alleen een **configuratielaag** toe die missiemakers controle geeft over wélke assets vanuit wélk voertuig bouwbaar zijn, buiten de vaste Conflict-facties om.

---

## 2. Wat de missiemaker-wens concreet vraagt

Twee dingen, te vertalen naar één mechanisme:
1. **FOB-bouwen ook in eigen scenario's** (niet alleen vanilla Conflict)
2. **UI-keuze welke assets bouwbaar zijn vanuit welk specifiek voertuig** — dus niet hardcoded per factie, maar per scenario instelbaar

### Voorgesteld concept: Build Catalog + Vehicle-koppeling

- **Build Catalog** — een losse resource (vergelijkbaar met SF's Faction Catalog-concept) die een lijst bouwbare prefabs + hun supply-kosten bevat. Een missiemaker kan meerdere catalogi maken: bijv. "Basis Fortificaties", "Genie-structuren", "Comms-uitrusting"
- **Vehicle-koppeling** — elk voertuig-prefab krijgt een attribuut "Build Catalog Reference" (dropdown/resource-picker, zelfde patroon als de custom Editor Attributes uit sectie 6 van het hoofdproject). Staat dit attribuut gevuld, dan fungeert dat voertuig binnen een straal als bouwbron voor precies die catalogus — niets meer, niets minder
- Verschillende voertuigen in hetzelfde scenario kunnen naar verschillende catalogi verwijzen — bijv. een gewone logistieke truck ontgrendelt alleen basisfortificaties, een "Genie-HEMTT" ontgrendelt geavanceerdere structuren. Geeft missiemakers een narratief instrument (wie mag wat bouwen) zonder harde rank-/factie-koppeling

---

## 3. Waarom dit als configuratielaag i.p.v. nieuw systeem

Als de vanilla vehicle-naar-catalogus-koppeling al via config/data aanpasbaar is (niet hardcoded in gesloten C++), is dit een **dunne laag**: een custom attribuut + een resource-referentie, geen nieuwe economie, geen nieuwe blueprint-logica. Als dat koppelpunt wél hardcoded per factie blijkt te zitten, is er meer werk nodig (een eigen trigger-gebaseerde bouw-zone die de vanilla-flow imiteert in plaats van hergebruikt). **Dit moet eerst in Workbench bevestigd worden — zie open vragen.**

---

## 4. Integratiekans met het hoofdproject (niet alleen decoratief)

Dit hoeft geen geïsoleerd systeem te zijn:
- **Infrastructuur-netwerk (sectie 5.2, hoofddocument)** — in plaats van dat een missiemaker comms-torens/generators alleen vooraf in de wereld plaatst, zouden spelers zelf een comms-relay kunnen *bouwen* via een Genie-catalogus, die zich dan als nieuwe node aan de bestaande infrastructuur-graaf toevoegt. Sabotage én constructie worden zo twee kanten van dezelfde medaille.
- **Squad Cohesion/C2-laag (sectie 5.10)** — een gebouwde FOB zou een nieuw, tijdelijk muster-/spawnpunt kunnen worden, wat de coördinatie-doelen van die module versterkt.

Geen van beide is noodzakelijk voor een eerste versie — wel het vermelden waard zodat dit niet als geïsoleerd eiland gebouwd wordt.

---

## 5. Namespace en performance

- **Namespace:** `MCF_Build_` — nieuw, want dit is noch AI-gedrag, noch een verhalende node, noch spelerscoördinatie
- **Performance:** bouwzone-detectie (is een speler dicht genoeg bij een bouwbron-voertuig) is een trigger-gebaseerde check, event-driven op enter/exit — geen continue polling, consistent met de rest van het framework

---

## 6. Open vragen om vroeg te beantwoorden (vóór er één regel code geschreven wordt)

- **Is de vehicle-naar-build-catalogus-koppeling in vanilla data-gedreven (config/attribuut) of hardcoded per factie in C++?** Dit bepaalt of dit een dunne configuratielaag wordt of een zwaardere eigen implementatie. Eerste stap: dit rechtstreeks in Workbench/Resource Browser inspecteren op een bestaand Conflict-voertuig, niet aannemen.
- Is de blueprint-plus-schop-flow als losstaand, herbruikbaar component beschikbaar buiten de Conflict-gamemode, of zit die logica verweven met Conflict-specifieke managers?
- Hoe wordt rank-gating (zoals bij het mortier-emplacement) idiomatisch toegepast — is er een generiek "vereiste rang"-attribuut op bouwbare items, herbruikbaar voor onze eigen catalogi?
