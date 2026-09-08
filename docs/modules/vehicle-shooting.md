# Shooting from a vehicle as a passenger — technical sub-project

Standalone document, deliberately decoupled from the narrative mission framework (`ARCHITECTURE.md`). This touches character animation and the compartment system, not the Core/Event Bus architecture.

---

## 1. Starting situation

- **Not vanilla**, and explicitly not planned by Bohemia (Klamacz) — partly because community mods are already trying to solve it.
- Existing options are risky to build on:
  - **DRIVE-BY** — the best-known mod, but the developer is not transparent/does not actively maintain it. Many servers deliberately don't run it for that reason.
  - A **newer mod** surfaced mid-2026 (via a showcase video) — unconfirmed quality/maintenance status, not yet thoroughly assessable.
- Conclusion: building it independently is the safest choice, consistent with your earlier preference not to lean on potentially buggy dependencies.

---

## 2. Why this is a different technical domain than your mission framework

Your Core/Event Bus/Objective system runs on **scenario logic**: events, conditions, state management. This one runs on:
- **Character animation** (fire poses per seated posture, blending during vehicle movement)
- **Compartment system** (which seat allows which behavior)
- **Hit detection from a non-standard position** (raycasts from a seated pose, through vehicle geometry)
- **Camera behavior** (ADS from a window feels different from ADS on foot)

So this does **not belong in your Core** — it's a standalone gameplay system, at most with a simple on/off toggle in your Config Layer so mission makers can choose whether a scenario accounts for it (e.g. making convoy ambushes less one-sided).

---

## 3. Relevant existing building blocks (official, not third-party)

| Component | Relevance |
|---|---|
| **`BaseCompartmentManagerComponent`** | Defines all seat slots of a vehicle (PilotCompartment, CargoCompartment, etc.) — this is where you register which seats become "fire-capable" |
| **`BaseCompartmentSlot`** | Individual seat definition: occupancy, area matching for seat swaps, exit alignment |
| **`CompartmentAccessComponent`** | **Important find:** already has a method that returns *"whether we're in a compartment with ADS enabled"* — this concept (ADS-from-compartment) already exists in the engine, presumably used for existing turret-gunner positions. This is a strong anchor point: you may be building an **extension** of an existing mechanism, not a completely new system. |
| **Weapon Animation pipeline (Workbench)** | Official BI tutorial + sample project (`SampleMod_AnimationWorkshop` on Bohemia's GitHub) for building your own weapon/posture animations — the right starting point for the fire-pose animations themselves |
| **Get Out/Door Info system** | Recent engine updates added fine-grained control over exit animations per door/seat — relevant if you want players to still be able to smoothly exit *while* shooting |

---

## 4. Realistic scope estimate

**What's probably feasible with reasonable effort:**
- Firing from **open positions** (pick-up truck/technical bed, open Ural bed) — no window geometry to raycast through, so closest to the existing turret-ADS concept
- Getting one pilot vehicle fully working before scaling up to the rest of your vehicle roster

**What's considerably more work:**
- Firing **through a window** in a closed vehicle (Humvee, UAZ) — requires checking per vehicle model whether the animation doesn't clip through the door/window, plus more precise hit detection
- **Content scales with every vehicle type** — every unique vehicle geometry may need its own adjustments to fire angle/animation clipping. This is not a one-time build cost but a recurring content cost per vehicle you want to support.

**Balance consideration (explicitly mentioned in the community discussion):** Bohemia's reluctance partly stems from a balance question — being able to fire too easily from a vehicle undermines the risk of transport. Something to deliberately plan for yourself: a cooldown, an accuracy penalty, or a limited fire angle from the seat, instead of allowing firing with no downside.

---

## 5. Proposed first step

A small proof of concept, same as with the mission framework:

1. Pick **one open-bed vehicle** as the pilot (least complex animation case)
2. Investigate exactly how the existing ADS-in-compartment flag for turret positions works (`CompartmentAccessComponent`) — copy that pattern instead of building from scratch
3. Build one fire-pose animation via the official Weapon Animation tutorial/sample project
4. Test hit detection and network behavior with 2+ players before scaling up to more vehicles

---

## 6. Open questions to answer early

- Does the existing ADS-in-compartment flag also work on regular passenger seats, or is it hard-tied to turret-specific logic?
- How much animation adjustment per vehicle model is needed to prevent clipping — can this be estimated in advance, or only tested per vehicle?
- Which balance measure (cooldown/accuracy penalty/fire-angle limit) fits your milsim style best, given the balance concerns Bohemia itself raises?
