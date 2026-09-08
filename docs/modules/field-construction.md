# Field Construction module (building FOBs outside Conflict)

Standalone document, same reason as the other technical sub-projects: this touches the base game's existing supply/construction system, not the narrative Core/Event Bus logic itself — although integrating with it is valuable (see section 4).

---

## 1. Starting situation — don't reinvent it

Conflict already has a fully-fledged building system ("Free Roam Building", since 0.9.7.85):
- Build anywhere in the world, as long as a player is near a vehicle carrying the required building materials ("supplies")
- **Two-step flow:** place a blueprint (materials are deducted immediately) → a player with a shovel has to physically finish it. If a blueprint stays unbuilt, the materials are already gone — a deliberate risk/reward mechanic we want to keep, not bypass
- Certain structures are rank-gated (e.g. mortar emplacement) and faction-specific (FIA FOBs were once accidentally universally buildable — since fixed)

**Core principle for this sub-project:** reuse the existing supply economy and blueprint mechanic. Don't build a parallel economy — only add a **configuration layer** that gives mission makers control over which assets are buildable from which vehicle, outside the fixed Conflict factions.

---

## 2. What the mission-maker request concretely asks for

Two things, to translate into one mechanism:
1. **Building FOBs in your own scenarios too** (not just vanilla Conflict)
2. **UI choice for which assets are buildable from which specific vehicle** — so not hardcoded per faction, but configurable per scenario

### Proposed concept: Build Catalog + Vehicle linkage

- **Build Catalog** — a standalone resource (comparable to SF's Faction Catalog concept) that holds a list of buildable prefabs + their supply costs. A mission maker can create multiple catalogs: e.g. "Basic Fortifications", "Engineer Structures", "Comms Equipment"
- **Vehicle linkage** — every vehicle prefab gets a "Build Catalog Reference" attribute (dropdown/resource picker, same pattern as the custom Editor Attributes from section 6 of the main project). If this attribute is filled in, that vehicle acts within a radius as a build source for exactly that catalog — nothing more, nothing less
- Different vehicles in the same scenario can reference different catalogs — e.g. a regular logistics truck only unlocks basic fortifications, an "Engineer HEMTT" unlocks more advanced structures. Gives mission makers a narrative tool (who's allowed to build what) without a hard rank/faction coupling

---

## 3. Why this is a configuration layer instead of a new system

If the vanilla vehicle-to-catalog linkage is already adjustable via config/data (not hardcoded in closed C++), this is a **thin layer**: a custom attribute + a resource reference, no new economy, no new blueprint logic. If that link point turns out to be hardcoded per faction, more work is needed (a dedicated trigger-based build zone that imitates the vanilla flow instead of reusing it). **This needs to be confirmed in Workbench first — see open questions.**

---

## 4. Integration opportunity with the main project (not purely decorative)

This doesn't have to be an isolated system:
- **Infrastructure network (section 5.2, main document)** — instead of a mission maker only placing comms towers/generators in the world in advance, players could *build* their own comms relay via an Engineer catalog, which then adds itself as a new node to the existing infrastructure graph. Sabotage and construction thus become two sides of the same coin.
- **Squad Cohesion/C2 layer (section 5.10)** — a built FOB could become a new, temporary muster/spawn point, reinforcing that module's coordination goals.

Neither is necessary for a first version — but worth mentioning so this isn't built as an isolated island.

---

## 5. Namespace and performance

- **Namespace:** `MCF_Build_` — new, since this is neither AI behavior, nor a narrative node, nor player coordination
- **Performance:** build-zone detection (is a player close enough to a build-source vehicle) is a trigger-based check, event-driven on enter/exit — no continuous polling, consistent with the rest of the framework

---

## 6. Open questions to answer early (before a single line of code is written)

- **Is the vehicle-to-build-catalog linkage in vanilla data-driven (config/attribute) or hardcoded per faction in C++?** This determines whether this becomes a thin configuration layer or a heavier custom implementation. First step: inspect this directly in Workbench/Resource Browser on an existing Conflict vehicle, don't assume.
- Is the blueprint-plus-shovel flow available as a standalone, reusable component outside the Conflict game mode, or is that logic interwoven with Conflict-specific managers?
- How is rank-gating (as with the mortar emplacement) idiomatically applied — is there a generic "required rank" attribute on buildable items, reusable for our own catalogs?
