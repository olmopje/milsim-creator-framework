# Onderzoek: pijnpunten van Arma 3-missiemakers/units bij de overstap naar Reforger

Bronnen: Steam-discussies, Bohemia-forums, BI-feedbacktracker, ACE3-GitHub. Kwalitatief, geen enquête — bedoeld om onze prioriteiten te toetsen, niet als harde data.

## 1. Bevestigd: het project speelt in op de #1 klacht

Vrijwel elke discussie over "waarom is Reforger's mission-making slechter" komt uit bij hetzelfde punt: geen in-game editor zoals Eden. Bohemia zelf (developer Nillers, officiële reactie): *"We know many people are missing the in-game Eden Editor. It's a complex thing we intend to fully focus on our way to Arma 4."* — dus bevestigd: geen plannen voor Reforger zelf.

**Consequentie:** ons hele project (live GM-plaatsbare, missiemaker-vriendelijke logica) valt precies in het gat dat Bohemia zelf erkent en niet gaat vullen vóór Arma 4.

## 2. Waarschuwingssignaal: ACE3 weigerde Reforger-support

Uit de officiële ACE3 GitHub-discussie: *"Due to severe limitations of the Enfusion engine and its scripting language, the lack of an in-game editor and most importantly an active playerbase we currently aren't working on bringing ACE to Arma Reforger."*

Geen specificatie van wélke limitaties. Onze scope is aanzienlijk kleiner dan een volledig medisch/ballistisch systeem zoals ACE, dus dit is geen directe blocker — wel een signaal om alert te blijven op onverwachte scripting-grenzen tijdens Fase 0-testen, in lijn met de "test eerst, bouw daarna"-houding die we al hanteren bij de stealth-module.

## 3. Nieuwe module-kans: Squad Cohesion / C2-laag

De meest specifieke en herhaalde klacht van ervaren spelers, niet over bugs maar over ontwerp:
- *"Even if you're a member of a given team, you can spawn anywhere you like, you have no idea where the rest of the team is... the team leader similarly has absolutely no idea where any of his team members are."*
- *"Being part of a squad ATM has no meaning, besides using the radio respawn, which most people don't even know is a mechanic at all."*

Dit is exact het "milsim-gevoel" dat door Arma 3-veteranen gemist wordt — niet gebrek aan content, maar gebrek aan **gedwongen/betekenisvolle coördinatie**. Zie sectie 5.10 in `ARCHITECTURE.md` voor de uitwerking als nieuwe module.

## 4. Bevestigd probleem, maar buiten ons bereik — expliciet benoemd

AI-commandogedrag wordt door de communit én door BI's eigen feedbacktracker (T190910) als een van de grootste frustraties genoemd: AI die niet uit voertuigen stapt op commando, follow-orders die breken. Dit zit op **engine-pathfinding-niveau** — geen scenario-/scriptinglaag kan dit structureel repareren.

**Update: gedeeltelijke mitigatie wél haalbaar, geen structurele fix.** Zie sectie 5.11 in `ARCHITECTURE.md` — een watchdog-patroon dat vastgelopen AI detecteert en corrigeert kan de symptomen merkbaar verminderen zonder de onderliggende pathfinding aan te pakken. Dit blijft een pleister, geen genezing, en wordt ook zo gecommuniceerd naar de unit.

## 5. Kleine, lage-prioriteit kanttekening

Geen mogelijkheid om AI-teamleden te rekruteren in Conflict/Combat Ops (alleen via Game Master-toegewezen squads) — een kleine QoL-toevoeging (bijv. een "recruit at base"-actie), maar dit wijzigt basisgameplay-systemen eerder dan missielogica. Genoteerd als mogelijke toekomstige losse kleine toevoeging, niet als kernmodule.

## 6. Al gedekt, ter bevestiging

De wens naar meerdere save-states in Game Master (2022-forumklacht: *"if it WOULD have multiple save games, you could at least use it like a sort of..."*) wordt al rechtstreeks geadresseerd door onze Save/Load-architectuur (sectie 8, hoofddocument).

## 7. Buiten scope, geen mod-oplossing

Crossplay-toxiciteit/teamkilling door consolespelers is een communitymanagement-/matchmaking-vraagstuk, geen missie-scriptingprobleem. Niet opgenomen.
