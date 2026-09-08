# Milsim Creator Framework (MCF) — Architecture Plan

## 1. Vision

A self-built, modular framework for Arma Reforger that gives mission makers Eden-like narrative depth — usable live in Game Master, built around a shared Core, and suitable from the start for large PvE co-op groups without performance degradation.

No dependency on existing third-party frameworks (Scenario Framework, Ci5, GME) — informed by their design, but built independently.

---

## 2. Architecture — layer overview

```
┌─────────────────────────────────────────────────┐
│  GM INTEGRATION LAYER                            │
│  (SCR_EditableEntityComponent, live attributes)  │
├─────────────────────────────────────────────────┤
│  CORE                                            │
│  Event Bus · Object Identity · Module Registry   │
│  Config Layer · Replication Helper · Tick Manager│
├─────────────────────────────────────────────────┤
│  NARRATIVE LAYER          │  WORLD SYSTEMS       │
│  Objective Nodes          │  Hostility/Reputation │
│  POI/Observation Nodes    │  Infrastructure Net   │
│  Logic Nodes (AND/OR/CNT) │  Civilian AI Behavior │
│  Voice Line / Comms       │  Waypoint+Animation   │
├─────────────────────────────────────────────────┤
│  PERSISTENCE                                     │
│  Save/Load (native serialization + module state) │
├─────────────────────────────────────────────────┤
│  PERFORMANCE LAYER (cuts across everything above)│
│  Dynamic spawn/despawn · Throttling · Budgets    │
└─────────────────────────────────────────────────┘
```

---

## 3. Core — the foundation

| Component | Function |
|---|---|
| **Event Bus** | Central pub/sub system. Modules never communicate directly — always via events. Prevents tight coupling. |
| **Object Identity** | Generic component on any relevant entity with a unique tag/name field. Modules find each other via tags, not hard references. |
| **Module Registry** | Every module registers itself with version + dependencies. Determines load order, checks compatibility. |
| **Config Layer** | One central resource per scenario: which modules are active, with which settings. The mission maker never has to touch code. |
| **Replication Helper** | Wrapper around RplComponent/RPC boilerplate — modules don't have to reimplement netcode themselves. |
| **Tick Manager** | One central update loop instead of separate per-entity `EOnFrame` — critical for performance (see section 7). |

### 3.1 Integration contracts (highest standard — explicitly defined, never implicitly assumed)

These are rules that apply **once, at the Core level**, so no module has to interpret them itself or, worse, interpret them differently from another module.

- **Naming convention & prefix.** Project name: **Milsim Creator Framework (MCF)**. All classes/prefabs get the fixed prefix `MCF_`, per BI's official *Editor Entity Naming Conventions* ("replace `SCR_` with your own tag"). Within that, every module gets its own sub-namespace, so the class name immediately shows which module is responsible:

  | Namespace | Covers |
  |---|---|
  | `MCF_Core_` | Event Bus, Object Identity, Module Registry, Tick Manager |
  | `MCF_AI_` | Civilian AI behavior (5.3), Ambient Life behavior profiles (5.5), Compliance/ROE logic (5.7), AI Command Watchdog (5.11) |
  | `MCF_Obj_` | Objective/POI/Logic Nodes (4.x) |
  | `MCF_Hostility_` | Hostility/Reputation manager (5.1) |
  | `MCF_Infra_` | Infrastructure network/AI Warning (5.2) |
  | `MCF_Voice_` | Voice Line/Comms (4.4) |
  | `MCF_Interact_` | Interaction hint system (5.6) |
  | `MCF_AAR_` | Debrief module (5.8) |
  | `MCF_Squad_` | Squad Cohesion/C2 layer (5.10) |
  | `MCF_Build_` | Field Construction module — worked out separately in `docs/modules/field-construction.md`, deliberately not included in the phased roadmap (section 9) |
  | `MCF_ACE_` | ACE Anvil compatibility bridge — worked out separately in `docs/modules/ace-anvil-compatibility.md`, always optional (soft dependency), modularly toggleable per integration point (`MCF_ACE_Compliance_`, `MCF_ACE_AAR_`, `MCF_ACE_Interact_`, `MCF_ACE_Carrying_`) — **confirmed actively needed**, not speculative work |
  | `MCF_React_` | Scripted AI Reactions — reusable behavior-recipe catalog (5.12) + Sequence Recorder (5.13), combines existing building blocks, no behavior logic of its own |

  New modules that don't fit this table only get a new namespace after discussion — prevents namespace sprawl.
- **Event contract.** Every event has a fixed namespace pattern (`Module_Action`, e.g. `Objective_Complete`, `Hostility_ThresholdCrossed`) and a documented payload schema. No event is named ad hoc — new events are registered centrally in the Module Registry, not invented loosely per module.
- **Authority policy.** All state mutations (objective status, hostility value, infrastructure status, ROE outcome) are **server-authoritative**. Clients only ever receive replicated results via the Replication Helper — never local predictions that need to be corrected later. This applies to every module, without exception.
- **Faction Alias integration.** The Core reuses SF's proven `SCR_FactionAliasComponent` pattern instead of inventing its own faction abstraction. Any module that needs a faction (Hostility, ROE, Objective conditions) refers to an Alias, not a hard Faction Key — so a scenario is reusable with different faction combinations without a rebuild.
- **GameMode compatibility.** The Core is **additive**: a component alongside `GameMode_Base`, not a replacement for it. This guarantees the Core can run alongside vanilla Conflict/Combat Ops if the unit ever wants that, instead of forcing the framework to be an exclusive scenario type.
- **Validation pass at mission init.** When a scenario starts, a validation pass runs that logs missing tag references, misconfigured conditions, and module dependency conflicts — with the same (W)/(E) severity distinction as SF's debug system. A mission maker's typo must never let a system fail silently.
- **Event Bus lifecycle coupling.** Listeners are automatically unsubscribed when their entity is destroyed/despawned — prevents dangling listeners and memory leaks without every module having to manage this itself.

### 3.2 Test and stress-test infrastructure (mandatory hook, not a per-module side initiative)

Two separate goals, with one central registration so nobody has to invent their own ad-hoc test script:

**Functional tests — builds on Bohemia's official Autotest Framework**
- Every module ships a test suite that inherits from `SCR_AutotestSuiteBase`, running in a dedicated test world (BI's `MpTest` pattern, possibly a dedicated MCF test world)
- Test suites are registered with the Module Registry, alongside the module itself — no loose, undocumented test script somewhere in a subfolder
- **Realistic limit:** this runs via Workbench, not via a plain GitHub Actions runner (see the CI limitation in `CONTRIBUTING.md`) — it automates the "test in Workbench" step in our workflow, it does not replace that step

**Stress tests — its own layer, inspired by an existing community pattern**
- Every module can optionally register a **Stress Profile**: a simple recipe (how many instances of itself to spawn, at what intensity, for how long) — e.g. Ambient Life registers "spawn 150 NPCs across 5 villages", the Infrastructure Network registers "activate 20 graph nodes with varying status"
- **One central Stress Test Controller** (a GM-placeable entity, same pattern as other MCF nodes) can fire individual profiles or a combination — this lets you simulate the "40+ players + a busy village" scenario from the roadmap with real numbers instead of guesswork
- Results (FPS/tick timing per profile) are logged to the Debug Overlay from section 7 — reuse of existing infrastructure, no new reporting system

**Consequence for the checklist:** this becomes a mandatory part of every new module from Phase 0 onward, not a later addition — see the updated `CONTRIBUTING.md`.

---

## 4. Narrative layer

### 4.1 Objective Node
The basic building block for narrative tasks.

**Attributes:**
- Title / description
- **Visible on Map** (yes/no) — toggle for the map marker
- **Condition slot** — connectable to any boolean check (reputation, time, item ownership, another node's status)
- **On Complete → Event Out** — fires an event on the Event Bus once complete, which other nodes can listen for
- **On Fail → Event Out** (newly added — every objective must also be able to trigger a failure path, otherwise your story becomes a straight line)
- **Intel gate** (new) — the objective stays hidden/unclear until an information source (civilian, document, radio intercept) "unlocks" it. This is the generalization of your "intel via a civilian with good reputation" example: any information source can be a gate, not just civilian reputation.

### 4.2 POI / Observation Node
For the "monitor multiple locations" pattern.

- Separate trigger zones that each report activity to one shared Observation listener
- The listener decides branching: whichever POI reports first determines what happens next
- **Added idea: Observation decay** — if a POI isn't visited for too long, the chance of a "missed event" (e.g. a convoy passing unseen) can factor into a debrief score. Gives players a reason to actually divide up patrols — strong for milsim.

### 4.3 Logic Nodes
AND / OR / Counter / Timer nodes — generic boolean and counting systems that Objective and POI nodes plug into. Same concept as SF's LogicCounter, but decoupled so any node type can use them.

### 4.4 Voice Line / Comms Node
- Scripting: node triggers a voice line/sequence on a linked entity (same pattern as SF's Voice Over actions)
- **Note — separate workflow:** this requires an audio pipeline separate from your Core code: record → import into Workbench → link via an `.acp` config to an `SCR_CommunicationSoundComponent`. Plan this early if you want a lot of lines, because it doesn't scale automatically with your scripting speed.
- **Added idea: Voice Line Priority Queue** — with multiple simultaneous triggers (e.g. two objectives completing at once) you need to decide which voice line takes priority and which waits/gets dropped. Without this, lines overlap and it gets messy.

---

## 5. World systems

### 5.1 Hostility / Reputation manager
As designed earlier: a server-side singleton, a decay value per area/faction, fed by impact hooks (civilian casualties, destruction, aid), read by civilian AI behavior profiles and by Objective nodes (intel gates).

### 5.2 Infrastructure network — NEW, for your AI Warning System
This is the generic solution for your comms-tower idea, and reusable for similar puzzles later (power grids, water treatment, radar chains).

**Concept:** a graph of linked nodes with dependencies.

```
[Generator] --(power)--> [Cable segment A] --> [Cable segment B] --> [Comms Tower] --> AI Warning active
```

- Every node has a status (active/inactive) and a list of dependencies
- If a generator is destroyed or a cable segment is cut, the status change propagates through the graph → the tower goes offline → the AI warning system (e.g. QRF call, artillery strike, reinforcement alarm) is delayed or fully disabled
- **Intel coupling:** players need to know *where* the weak link is — tie this back into your intel system (recon, interrogating prisoners, documents), so sabotage rewards good scouting instead of a guess
- **Repairability (new idea):** enemy AI can be scripted to repair a generator or replace cables after some time — this prevents one sabotage action from permanently "turning off" the whole mission, keeping tension alive
- **Performance:** the graph only needs to be recalculated on a status change (event-driven), not continuously polled — lines up directly with the performance principles in section 7

### 5.3 Civilian AI behavior
Utility AI/behavior tree that reads the hostility value and switches between behavior profiles (neutral → fearful → tipping off OPFOR → active resistance). Perception/sensor hooks instead of simple proximity checks.

**See also:** the expanded four-state Alert system (Unaware/Suspicious/Investigating/Engaged) in `docs/modules/stealth-and-suppression.md` section 3.4 — originally worked out for stealth detection, but by now the shared state machine that Compliance/ROE (5.7), AI Command Watchdog (5.11), Scripted AI Reactions (5.12), and Sequence Recorder (5.13) all lean on. This lives in a separate document for historical reasons, not because it's stealth-specific — a candidate to move here during a future doc cleanup.

### 5.4 Waypoint + Animation module
Custom waypoint subclass that triggers a specific animation on arrival via the CharacterAnimationComponent/animgraph.

### 5.5 Ambient Life / Pattern-of-Life module — NEW
Not real A-life simulation (too heavy, too complex) — a light, cheap illusion of a living village, built on top of the already-planned Waypoint+Animation module.

**Core concept: Lifestyle POIs + Roles**
- **Lifestyle POI**: a variant of the POI node (4.2), but tagged with a role instead of an observation function — Shop, House, Market Stall, Checkpoint, Well. Every Lifestyle POI has a "slot" (how many NPCs can be present at once) and a set of idle animations that fit it.
- **Actor archetypes** (behavior profiles, no new AI logic — just a pre-scripted cycle):
  - **Shopkeeper** — stationary at one Lifestyle POI, cycles through idle animations (setting up stock, waiting, tidying)
  - **Customer/Civilian** — roams between 2-4 Lifestyle POIs in a village, a "browse" animation for a random duration per stop, then on to the next — exactly the same pattern as a Patrol waypoint cycle, just framed as civilian
  - **Militia/enemy off-duty** — same roam cycle as Customer, but with an enemy faction: gives the player the sense that the opposition also "just lives" in the village instead of constantly standing in a combat stance
  - **Traffic** — vehicles driving between villages on roads, spawning out of sight, despawning at distance (same pattern as existing Ambient Civilians-style mods)
- **"Fake conversations"** — two NPCs who happen to be at the same POI periodically play a synced pair of idle/talk animations for a few seconds. No dialogue system, no logic — purely timed animation pairs that give the illusion of interaction.

**Integration with existing modules (no new dependencies, everything via the Event Bus):**
- **Hostility manager** — if the hostility value in an area rises, this module listens in and sends shopkeepers "inside", clears civilians off the street, and switches militia NPCs from "shopping" to alert behavior. No new coupling needed, just an event listener.
- **Reputation/intel system** — a shopkeeper or civilian NPC from this module *can* simultaneously be the intel giver from section 4.1. Players can't visually tell a decorative NPC apart from a functional one — exactly the "fake it until you actually need it" idea.

**Performance — this is the heaviest module in terms of NPC count, so extra strict:**
- Fully bound to **Dynamic Spawn/Despawn per Area**: outside player range, Lifestyle NPCs are not simulated or even spawned
- **Global budget** on the number of active Ambient Life NPCs at once, configurable per scenario (ties into the budgets from section 7)
- **Staggered update timers**: not all NPCs recalculate their next animation on the same frame — random offset per NPC to avoid CPU spikes
- **No perception/AI overhead for pure decor NPCs** — a shopkeeper who never needs to detect danger doesn't need to run the same expensive sensor checks as combat AI

### 5.6 Interaction hint system — NEW
Being smart about interaction here mainly means: **most NPCs cost nothing at all**, and only a small subset ever has a chance at a genuinely useful reaction. No dialogue trees, no NLP, no per-NPC scripting work — a layered tier system with text pools.

**Three tiers, increasing in cost:**

| Tier | Who | Interaction | Cost |
|---|---|---|---|
| **0 — Decor** | Majority of Ambient Life NPCs | No interaction possibility at all | Zero — no component needed |
| **1 — Barker** | Small subset, tagged by the mission maker | `UserAction` ("Talk to") shows one line from a generic text pool | One component + shared text pool, no unique content per NPC |
| **2 — Informant-capable** | Server-side, invisibly marked subset of Tier 1 | Same interaction, but with a chance of a "pointer" line instead of generic filler | One extra attribute (chance) + pointer text templates |

**How the "pointer" works (the cryptic, realistic part):**
- On interacting with a Tier 2 NPC, the system rolls a percentage. On a hit, the player gets a **template line** with a filled-in reference, e.g.: *"Maybe %1 can help you."* — where %1 is filled with the role/name of the actual intel giver (section 4.1) or a vague direction ("near the harbor", "the man with the red cap")
- On a miss, the player just gets a generic Tier 1 line — indistinguishable to the player from an NPC who simply doesn't know anything
- This produces **investigation without dialogue trees**: players have to ask around, and most answers are noise — exactly as you asked, "fake it until you make it"

**Chance percentage — fed by existing state, no new system needed:**
- Base chance set by the mission maker per Tier 2 NPC
- **Modified by the hostility/reputation value** of the area (section 5.1) — higher trust, higher chance of a useful pointer
- **Retryable, but with a time cooldown instead of a one-shot** — a missed roll isn't permanently lost; the player can try again later once reputation in the area has risen. To prevent this from degenerating into spam-clicking until a hit, a fixed minimum retry time per NPC applies (e.g. a few in-game minutes), independent of reputation growth. This keeps the realistic "earning trust pays off" feel without the chance mechanic becoming meaningless due to infinite immediate repetition.

**Content authoring stays light:** mission makers write a handful of generic Tier 1 lines and a handful of pointer templates per scenario — no unique text per individual NPC needed. This fits the "not too deep, but alive" goal: content effort scales with the number of interaction *types*, not the number of NPCs.

### 5.7 Compliance / ROE interaction layer — NEW
A Ready or Not-style layer: players can force NPCs into compliance behavior at gunpoint. A strong addition to the milsim goal, since this literally practices escalation-of-force/ROE procedures — not decoration, but training value.

**Core commands:**
- **"Drop your weapon"** — aimed at armed AI (enemy or unclear status). On compliance: the AI drops its weapon (reuses the existing Item Safeguard/inventory-drop mechanism from section 6), enters a "hands up" animation state, becomes arrestable/transportable.
- **"Stand back"** — aimed at unarmed NPCs (civilians). On compliance: the NPC stops moving/doesn't approach further. Simpler and cheaper than the weapon-drop variant, no item interaction needed.

**Trigger mechanism (the feasible part):**
- A `UserAction`-like context action (same pattern as the Tier interactions in 5.6), available once: the player has their weapon raised, the aim vector hits the NPC within range/cone, and line-of-sight is clear
- **On-demand check, no polling** — the aim/LOS condition is only evaluated at the moment of an interaction attempt, not continuously per player-NPC pair. Critical for performance with multiple players/NPCs at once.
- **Optionally linked to a voice line + keybind**, not to actual speech recognition (see the note below)

**Compliance chance — reuses existing systems, no new mechanism:**
- Base chance per faction/unit type, configurable by the mission maker (a seasoned militia member is less quick to comply than a lower-morale unit)
- Modified by the same morale/threat-state factors already planned for AI behavior (suppression, number of nearby enemies, being isolated) — no separate system needed, the same threat-state concept already in 5.3
- On refusal: the AI falls back to normal behavior (fight/flee) — no separate "refusal animation" needed, just falling back to existing behavior

**Consequence coupling with the Hostility manager (section 5.1):**
- Correct use against an actual threat: neutral to positive for reputation
- "Stand back" against an unarmed, non-threatening civilian, or excessive force after compliance: directly negative for the hostility value in the area — reuses the existing "friendly-caused casualties/ROE" distinction from section 5.1. This turns the module into an **escalation-of-force training tool**, not just a fun action button.

**Note — voice command via speech recognition: likely not feasible within normal modding.**
Reforger's VoN audio system does not give mods access to raw audio or speech-to-text — it's purely a pass-through that routes compressed audio between players. A real "player says it out loud → AI reacts" link would require an external process listening outside the game, which is neither stable nor sensible for a unit project (anti-cheat/EULA risk in MP). **Pragmatic alternative with almost the same feel:** a keybind/radial command that simultaneously (a) audibly plays the matching voice line via VoN to other players, and (b) triggers the AI logic. The player really does "shout" the command out loud — only the detection runs via a button, not speech recognition.

### 5.8 After-Action Review (AAR) / Debrief module — RESTORED
Mentioned early in the brainstorm ("milsim units love debriefs"), but not worked out in earlier versions of this document. This is the natural place to summarize a whole session's Event Bus history, and costs little extra build work since every module already puts events on the Bus.

- **Passively listens** on the Event Bus for the whole mission — no separate data collection per module needed, just a central logger that stores all relevant events with a timestamp
- **Aggregates at mission end:** completed/failed objectives, ROE compliance outcomes (correct vs. incorrect "stand back"/"drop weapon" actions), missed POI observations, hostility trend per area over time
- **Output:** a summary debrief screen or exportable text file — valuable for milsim units that discuss sessions afterward
- **Performance:** purely event-driven, no polling — the cheapest module in the whole framework since it never has to initiate anything, only record

### 5.9 Logistics/Supply — BACKLOG, deliberately not in initial scope
Also mentioned early, but deliberately **not** included in the core roadmap to prevent scope creep. Would lean on similar Area/Dynamic-Despawn patterns as the rest of the framework (supply points as Lifestyle-POI-like nodes, convoys as POI/Observation chains), so technically not a new pattern — but a deliberate later extension, not a requirement within the current phased roadmap (section 9).

### 5.10 Squad Cohesion / C2 layer — NEW, from community research
Directly derived from `docs/research/mission-maker-pain-points.md`: the most repeated complaint from experienced milsim players is not lack of content, but that squad membership in vanilla Reforger "has no meaning" — no visibility of team positions, spawn decoupled from squad, no coordination incentive.

- **Squad position overview for the leader**: a light, opt-in map layer that only shows squad members to each other (never the whole army) — purely informational, no new AI logic
- **Muster gate (optional, mission-maker configurable)**: an Objective node (reuses section 4.1) can require, as a condition, that a squad is physically together before releasing the next narrative beat — enforces coordination without forcing a hard "you may not spawn" slot
- **Radio-respawn visibility**: a simple UI hint that makes the existing but little-known radio-respawn mechanism visible, so squads actually use it

**Namespace:** `MCF_Squad_` — new, since this is player coordination, not AI behavior (so deliberately not under `MCF_AI_`) and not a narrative node (so not under `MCF_Obj_`).

**Performance:** purely informational/event-driven (map layer updates only on a squad member's position change), no polling overhead.

**Scope boundary:** this does not fix AI command behavior (see `docs/research/mission-maker-pain-points.md` section 4 — that's engine-level and deliberately out of scope) — this only solves the *information and coordination gap between human players*.

### 5.11 AI Command Watchdog — a patch, not a structural fix
A direct answer to the acknowledged, frequently-mentioned complaint from section 4 of the research — with the limitation stated up front and repeatedly: this **suppresses symptoms**, it does not fix pathfinding/command logic that lives in closed C++.

**Pattern:**
1. **Detection** — a low-priority Tick Manager check monitors AI that received a "get out" or follow command; if the state/position stays unchanged past a configurable threshold, mark it as stuck
2. **First attempt: repeat the command** — often enough to "wake" the AI state machine without intervention. Always try this first, cheapest and safest
3. **Last resort: forced correction** — only if repeating doesn't work: AI stuck in a vehicle gets placed at a **validated, safe exit position**; AI that's no longer following gets a direct position correction toward the group

**Hard requirements, not optional:**
- **Safe-position validation is mandatory** before any forced teleport — prevents an AI from appearing inside geometry/water, which would be a new bug instead of a fix
- **Carefully tuned threshold value** — too short and normal, slow AI activity gets wrongly flagged as "stuck"; too long and the patch feels uselessly slow
- **Log every intervention** (via the validation pass/debug overlay from section 3.1/7) — so you can see how often this is actually needed, and whether the threshold needs adjusting

**Namespace:** falls under `MCF_AI_` — it's generic AI command behavior, not tied to civilian-, Ambient-Life-, or ROE-specific logic.

**Performance:** low-priority tick (see section 7.8) — this doesn't need to check often, getting stuck is by definition a slow-occurring problem.

### 5.12 Scripted AI Reactions — reusable behavior-recipe catalog, NEW
An answer to the wish for "easily extendable AI actions via a visual editor" — with a deliberate scope decision that's explained honestly before we go further.

**Scope decision: a catalog of ready-made recipes, not building a node-graph editor.** A custom drag-and-drop visual scripting tool would essentially mean building our own Eden editor — a much bigger project than the rest of this framework combined, and out of proportion with the rest of the roadmap. Instead: **every behavior is a named, reusable "recipe"** that a mission maker picks from a dropdown (same GM attribute pattern as the rest of the framework, section 6) — no node graph needed to get 90% of the value.

**What a "recipe" technically is:** a trigger (usually a state transition from the Alert system, see `docs/modules/stealth-and-suppression.md` section 3.4) tied to a fixed sequence of already-existing building blocks — Waypoint+Animation (5.4), Voice Line (4.4), existing Actions like Kill Entity/Add Waypoint. **No new Core functionality needed** — a recipe is purely configuration of things that already exist, which is what makes it genuinely "easily extendable": adding new recipes is making content, not writing code.

**Starter catalog (examples from your own question, plus a few common milsim tropes):**

| Recipe | Trigger | Building blocks (all already planned) |
|---|---|---|
| **HVT Flees by Vehicle** | Investigating/Engaged (stealth-and-suppression.md 3.4) | Waypoint to nearest vehicle → Get In action → escape-route waypoint |
| **HVT Flees by Helicopter** | Same | Voice Line ("call for evacuation") → wait → heli waypoint → Get In → extraction waypoint |
| **Hostage Execution on Alarm** | Engaged within X seconds of first contact | Animation (threaten/shoot) → Kill Entity action → linkable to existing hostility consequences (5.1), since this should weigh heavily against the player if it happens due to failed player behavior |
| **Fake Surrender** | Player approaches a "compliant" NPC (reuses the weapon-drop animation from the Compliance/ROE module, 5.7!) | Weapon-drop animation → wait until the player is close → surprise attack. A nice example of reuse: this recipe adds no new animation, it combines an existing one in a new way |
| **Call for Reinforcements** | Suspicious/Investigating (stealth-and-suppression.md 3.4) | Voice Line → trigger the QRF system (already exists) |

**Extensibility in practice:** adding a new recipe = a new row in the catalog config, no new class. Anyone in the unit who knows the building blocks (waypoints, animations, voice lines) can in principle assemble a new recipe without writing Enforce Script — that's the actual "easily extendable" promise, stronger than a visual editor would have been for the investment it would have cost.

**Honest stretch option for later:** once this system exists and the catalog grows, a simple Workbench plugin (same kind as SF's "Game Mode Setup" plugin) could simplify assembling new recipes into a form instead of loose config work. That's not a node-graph editor, but a step toward "more visual" — explicitly a backlog idea, not core scope.

**Namespace:** `MCF_React_` — new, since this is a recipe catalog that combines across multiple existing modules, with no behavior logic of its own.

**Performance:** a recipe costs exactly what its building blocks already cost (a waypoint, an animation, a voice line) — no extra overhead on top of what was already planned.

### 5.13 Sequence Recorder — path-and-cue recording for mini scripted scenarios, NEW
A direct extension of 5.12: a **new kind of recipe input** alongside manually assembled recipes. Instead of picking waypoints and animations one by one, the mission maker acts out the scene themselves and the system records what happened.

**Important distinction, stated up front so expectations are correct:**
- **This is not motion capture.** What gets recorded is *position/orientation over time* plus *moments when an existing action/animation was triggered* (sitting, interacting, getting in, raising a weapon) — no new skeletal animation.
- A completely new gesture that doesn't exist yet requires Reforger's own Animation Editor and an animator — a different discipline than mission scripting, not something this system can replace.

**How it works:**
1. **Recording** — a player (typically the mission maker themselves, while building) walks/performs the desired routine, while a Recorder captures position+rotation at a fixed interval and adds a timestamped "cue" every time an existing action/animation is invoked
2. **Saving** — the result is a **Sequence Asset**: essentially a fine-grained waypoint chain plus cue events, stored as data — no new animation file
3. **Playback** — assigned to an AI character via the Scripted AI Reactions catalog (5.12): the AI follows the recorded path using its normal movement animations, and triggers the same cues at the same relative moment. The AI "moves as itself", but follows your directed route and timing

**Why this enables mini scripted scenarios within a larger whole:** a Sequence Asset combined with a small cluster of Objective/Logic nodes (4.1/4.3) forms a self-contained "vignette" — e.g. a complete checkpoint routine or an ambush setup — that can be dropped into any larger mission as a reusable unit. This is not a new concept alongside the existing node hierarchy, it's that hierarchy simply applied at a smaller scale, exactly as Area→Layer→Slot already supports.

**Namespace:** falls under `MCF_React_` — a Sequence Asset is technically just a new type of recipe input, not a separate system.

**Open questions to answer early:**
- How fine-grained does the recording interval need to be for a smooth result without making the Sequence Asset unnecessarily large — a practical trade-off that can only be determined by testing in Workbench
- Does the playing-back AI react naturally to unexpected obstacles while following a recorded path (e.g. a player accidentally standing in the way), or does it rigidly follow the path regardless of circumstances? Determines whether fallback behavior is needed on top of the literal playback logic

---

## 6. GM integration layer

Every node above becomes a custom prefab with:
- `SCR_EditableEntityComponent` + `PLACEABLE` flag → appears in the GM catalog
- Custom Editor Attributes (sliders, dropdowns) → live configurable by the GM without Workbench
- Automatic inclusion in the mission save serialization (native BI system)

---

## 7. Performance-conscious design principles

This literally touches every layer above, so it's given its own explicit section:

1. **Event-driven over polling.** Every node reacts to events, not its own tick loop. Where a check genuinely needs to be periodic (e.g. hostility decay), it runs through the central **Tick Manager** with a configurable update rate — never per-entity `EOnFrame`.
2. **Dynamic spawn/despawn per Area**, same pattern as SF: content outside player range is not simulated. Areas remember their state (positions, completion) so despawning doesn't cost progress.
3. **Graph updates only on change** (infrastructure network, reputation) — no continuous recalculation.
4. **Bundled replication.** Combine related state into one `RplProp` component instead of replicating it separately — less network overhead, easier to debug.
5. **Configurable budgets.** The mission maker can set, per scenario, a max number of active AI groups, active triggers, and active infrastructure nodes — prevents an enthusiastic GM from bringing the server to its knees.
6. **Voice line queue** (see 4.4) prevents audio stacking but also saves unnecessary simultaneous sound instances.
7. **Debug overlay from day 1** (like SF's debug menu) — live shows which nodes are active, what graph status infrastructure has, what hostility value an area has. Not only handy for you while building, but also a performance diagnostic tool: you immediately see if something keeps running unnecessarily.
8. **Tick priority levels** instead of one flat update rate for the whole Tick Manager: a "critical" tier with a short interval for gameplay-deciding checks (e.g. ROE compliance status), and a "cosmetic" tier with a much longer interval for decor (Ambient Life animation choices). One rate for everything is either too slow for what matters, or needlessly expensive for what nobody notices.
9. **Event Bus lifecycle coupling** — listeners are automatically unsubscribed on entity destruction (see section 3.1), so despawning NPCs and removed nodes don't leave dangling listeners silently holding onto memory.

---

## 8. Persistence / Save-Load

- Base: native `SCR_EditableEntityComponent` serialization (position, state, custom attributes) — already part of the save file
- Additional: a dedicated **Module State Serializer** for data that isn't an entity attribute — reputation values per area, infrastructure network status, Event Bus history (which objectives are already complete)
- Goal: mission makers can pre-build, and live GM sessions can be saved/resumed mid-session without losing progress

---

## 9. Phased roadmap

| Phase | Goal |
|---|---|
| **0 — Proof of concept** | One Trigger Zone entity: GM-placeable, live attributes, full save/load cycle tested, **plus** the event naming contract, authority policy, validation pass, and the Test/Stress registration from section 3.1-3.2 applied from the start — these are not later additions but foundation |
| **1** | Objective Node complete (title, map visibility, condition slot, on-complete/on-fail events, intel gate) |
| **2** | POI/Observation Node + Logic Nodes, Event Bus coupling between them |
| **3** | Hostility/Reputation manager + civilian behavior hook + Faction Alias integration |
| **4** | Infrastructure network (AI Warning System) + intel coupling |
| **5** | Waypoint+Animation module, Voice Line module + priority queue |
| **6** | Ambient Life/Pattern-of-Life module (Lifestyle POIs, actor archetypes, fake conversations) + interaction hint system (tiers, pointer templates) — builds directly on Phase 5 |
| **7** | Compliance/ROE interaction layer (gunpoint, drop weapon, stand back) + coupling to the Hostility manager |
| **8** | AAR/Debrief module — aggregates the Event Bus history of all previous phases, so it only makes sense as a closer |
| **9** | Squad Cohesion/C2 layer — standalone from the rest, can also be picked up in parallel earlier if capacity allows |
| **10** | AI Command Watchdog — only take on once the base AI modules (Phase 3, 6, 7) are running, so there are enough real usage scenarios to test the threshold against |
| **11** | Scripted AI Reactions catalog — only makes sense after Phase 3-7 (Hostility, Waypoint+Animation, Voice Line, Compliance/ROE), since every recipe reuses those building blocks |
| **12** | Sequence Recorder — builds directly on Phase 11, since a recording is technically a new type of recipe input |
| **13** | Performance pass: Tick Manager priority levels, budgets, refine the debug overlay under load (test with 40+ players and a busy village at once) |
| **14** | GM attribute UI polish + documentation for other mission makers in the unit (ongoing from Phase 0, not starting only here) |

---

## 10. Open design questions to prototype early

- How robust is custom attribute serialization in practice with nested node hierarchies? (risk identified in Phase 0)
- How many concurrent infrastructure graph nodes can the Tick Manager handle before your performance target is no longer acceptable?
- How far can "repairable" sabotage (AI repairing a generator) go before it feels frustrating to players instead of tense?
