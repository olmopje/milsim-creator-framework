# ACE Anvil-compatibiliteit

Los document — dit gaat over interoperabiliteit met een extern, experimenteel project (ACE Anvil), niet over een eigen module. Behandel dit als een **grens/brug**, niet als onderdeel van de Core.

---

## 1. Wat ACE Anvil is (bijgewerkte informatie)

- Open-source realisme-mod voor Arma Reforger door het ACE-team (GPLv2), actief onderhouden
- Modulair: 16+ losse addons (Medical, Backblast, Carrying, etc.), elk met eigen GUID, allemaal afhankelijk van `ACE_Core`
- Gebruikt Enfusion's native replicatiesysteem met **hetzelfde server-authoritative patroon** dat wij al vastgelegd hebben in `ARCHITECTURE.md` sectie 3.1 (client valideert → RPC-verzoek → server beslist → state repliceert) — goed teken voor compatibiliteit op architectuurniveau
- Eigen persistentie via `ACE_EditorStruct` (JSON-gebaseerd), losstaand van onze `SCR_EditableEntityComponent`-serialisatie — verschillend datadomein, geen directe conflictbron

**Expliciete waarschuwing, rechtstreeks uit hun eigen documentatie:** ACE Anvil noemt zichzelf een experimenteel testplatform. Hun eigen README stelt dat feature-pariteit met ACE3 waarschijnlijk niet gehaald wordt vóór het team overstapt naar Arma 4. **Conclusie: hun API kan nog wijzigen. Bouw dun en geïsoleerd, nooit diep verweven.**

---

## 2. Ontwerpprincipe: soft dependency, geen harde vereiste

MCF moet **volledig blijven werken zonder ACE Anvil**. Niet elk unit-lid hoeft het te draaien, en het framework mag nooit crashen of degraderen als het ontbreekt. Bij aanwezigheid van ACE Anvil breidt MCF zijn gedrag uit; bij afwezigheid valt het simpelweg terug op eigen logica.

**Praktisch:** een runtime-check bij initialisatie (bestaat de `ACE_Core`-klasse/GUID?) bepaalt of de bridge-laag actief wordt. Dit is hetzelfde soort "optional dependency"-patroon dat CBA in Arma 3 decennialang succesvol hanteerde voor mod-interoperabiliteit.

**Namespace:** `MCF_ACE_` — alle ACE-aanrakende code geïsoleerd in één namespace. Als ACE Anvil's API breekt bij een update, is de schade beperkt tot dit ene mapje, niet verspreid door de hele codebase.

---

## 3. Concrete integratiepunten (waar onze modules ACE-bewust moeten zijn)

### 3.1 Compliance/ROE-laag (hoofddocument 5.7) ↔ ACE Medical
Onze "drop your weapon"/"stand back"-acties moeten **niet** geactiveerd kunnen worden op een doelwit dat via ACE Medical al bewusteloos/incapacitated is — dat is geen zinvolle nalevingsactie meer. Check: is ACE Medical actief, vraag dan zijn bewustzijnsstatus op vóór de UserAction zichtbaar wordt; zonder ACE Medical valt dit terug op de vanilla damage-state.

### 3.2 AAR/Debrief-module (5.8) ↔ ACE Medical
Optionele verrijking, geen vereiste: als ACE Medical draait, kan de debrief ook medische gebeurtenissen meenemen (wie raakte gewond, wie werd behandeld) voor een rijkere sessie-samenvatting. Puur additief — de AAR-module werkt identiek zonder ACE.

### 3.3 Interactie-hint-systeem (5.6) & Compliance-laag (5.7) ↔ ACE's interactiesysteem
ACE Anvil voegt eigen `UserAction`-gebaseerde interacties toe (bijv. op onbewuste patiënten). Risico: **ID-botsingen** op dezelfde entiteiten als beide systemen tegelijk actief zijn. Vereist: expliciete controle bij Fase 0-achtige integratietest dat onze UserAction-ID's nooit overlappen met ACE's — geen aanname, echt testen met beide mods tegelijk geladen.

### 3.4 Carrying-component ↔ Compliance-laag (5.7)
ACE's "Carrying"-systeem (incapacitated units dragen) overlapt conceptueel met een gearresteerde/compliant NPC uit onze ROE-module. Geen directe technische botsing verwacht, maar wel iets om samen te testen: kan een via onze module "hands up"-gezette NPC ook door ACE's carry-actie opgepakt worden, en is dat gewenst gedrag?

---

## 4. Wat hier expliciet niet bij hoort

- Geen eigen medisch systeem bouwen dat ACE Medical dupliceert — als de unit ACE Medical wil, gebruik die; wij bouwen alleen de brug
- Geen harde dependency in `.gproj` — ACE Anvil blijft altijd optioneel voor de eindgebruiker

---

## 5. Open vragen om vroeg te beantwoorden

- Is er een stabiele, gedocumenteerde manier om runtime te detecteren of `ACE_Core` geladen is, of moet dit met een fragiele class-existence-check (die bij een ACE-herstructurering kan breken)?
- Hoe stabiel is ACE Anvil's `UserAction`-ID-toewijzing tussen versies — is er kans op stille breaking changes bij een ACE-update die onze `MCF_ACE_`-brug ongemerkt laat falen? Zo ja: is een periodieke compatibiliteitstest (gekoppeld aan de Autotest-infrastructuur uit 3.2) verstandig, draaiend telkens als de unit ACE Anvil update?
- Speelt de unit daadwerkelijk met ACE Medical, of is dit voorbereidend werk voor een mogelijke toekomstige keuze? Bepaalt of Fase-inplanning nu al zinvol is of beter wacht tot de keuze vaststaat.
