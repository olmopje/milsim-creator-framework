# ACE Anvil compatibility

Standalone document — this is about interoperability with an external, experimental project (ACE Anvil), not about a module of our own. Treat this as a **boundary/bridge**, not part of the Core.

---

## 1. What ACE Anvil is (updated information)

- Open-source realism mod for Arma Reforger by the ACE team (GPLv2), actively maintained
- Modular: 16+ separate addons (Medical, Backblast, Carrying, etc.), each with its own GUID, all depending on `ACE_Core`
- Uses Enfusion's native replication system with **the same server-authoritative pattern** we already defined in `ARCHITECTURE.md` section 3.1 (client validates → RPC request → server decides → state replicates) — a good sign for compatibility at the architecture level
- Its own persistence via `ACE_EditorStruct` (JSON-based), separate from our `SCR_EditableEntityComponent` serialization — different data domain, no direct source of conflict

**Explicit warning, straight from their own documentation:** ACE Anvil calls itself an experimental test platform. Their own README states that feature parity with ACE3 will likely not be reached before the team moves on to Arma 4. **Conclusion: their API can still change. Build thin and isolated, never deeply intertwined.**

---

## 2. Design principle: soft dependency, no hard requirement — and modular per integration point

**Confirmed: the unit already actively uses ACE Medical.** So this is no longer speculative future work — the bridge needs to be reliable from the first implementation, not "figure it out someday".

MCF must **keep working fully without ACE Anvil**. Not every unit member has to run it, and the framework must never crash or degrade if it's absent. When ACE Anvil is present, MCF extends its behavior; when absent, it simply falls back to its own logic.

**Modular here means: every integration point independently toggleable, not one big on/off switch.** The four integration points in section 3 (Compliance/ROE, AAR, Interaction hints, Carrying) each get their own Config-Layer attribute. A mission maker might, for example, want the Compliance/ROE medical check (3.1) but disable the AAR medical enrichment (3.2) because it makes debriefs too long — that must be possible without affecting the other three. This prevents a future ACE breaking change from immediately making the whole bridge unusable instead of just the affected integration point.

**Practical:** a runtime check at initialization (does the `ACE_Core` class/GUID exist?) determines whether the bridge layer can ever become active; the four sub-toggles then determine which integration points actually run. This is the same kind of "optional dependency" pattern that CBA in Arma 3 used successfully for decades for mod interoperability.

**Namespace:** `MCF_ACE_` — all ACE-touching code isolated in one namespace, with every integration point as its own sub-module within it (`MCF_ACE_Compliance_`, `MCF_ACE_AAR_`, `MCF_ACE_Interact_`, `MCF_ACE_Carrying_`). If ACE Anvil's API breaks on an update, the damage is limited to one sub-module, not the whole bridge.

---

## 3. Concrete integration points (where our modules need to be ACE-aware)

### 3.1 Compliance/ROE layer (main document 5.7) ↔ ACE Medical
Our "drop your weapon"/"stand back" actions must **not** be triggerable on a target that's already unconscious/incapacitated via ACE Medical — that's no longer a meaningful compliance action. Check: if ACE Medical is active, query its consciousness status before the UserAction becomes visible; without ACE Medical this falls back to the vanilla damage state.

### 3.2 AAR/Debrief module (5.8) ↔ ACE Medical
Optional enrichment, not a requirement: if ACE Medical is running, the debrief can also include medical events (who got wounded, who was treated) for a richer session summary. Purely additive — the AAR module works identically without ACE.

### 3.3 Interaction hint system (5.6) & Compliance layer (5.7) ↔ ACE's interaction system
ACE Anvil adds its own `UserAction`-based interactions (e.g. on unconscious patients). Risk: **ID collisions** on the same entities if both systems are active at once. Required: explicit verification during a Phase-0-like integration test that our UserAction IDs never overlap with ACE's — no assumption, actually test with both mods loaded at once.

### 3.4 Carrying component ↔ Compliance layer (5.7)
ACE's "Carrying" system (carrying incapacitated units) conceptually overlaps with an arrested/compliant NPC from our ROE module. No direct technical clash expected, but something to test together: can an NPC put into "hands up" state by our module also be picked up by ACE's carry action, and is that the desired behavior?

---

## 4. What is explicitly out of scope here

- No building a custom medical system that duplicates ACE Medical — if the unit wants ACE Medical, use it; we only build the bridge
- No hard dependency in `.gproj` — ACE Anvil always stays optional for the end user

---

## 5. Open questions to answer early

- Is there a stable, documented way to detect at runtime whether `ACE_Core` is loaded, or does this require a fragile class-existence check (which could break on an ACE restructuring)?
- How stable is ACE Anvil's `UserAction` ID assignment between versions — is there a risk of silent breaking changes on an ACE update that lets our `MCF_ACE_` bridge fail unnoticed? If so: is a periodic compatibility test (tied to the Autotest infrastructure from 3.2) advisable, running whenever the unit updates ACE Anvil?
- **Confirmed:** the unit already actively uses ACE Medical, so bridge development is non-speculative and deserves priority once Compliance/ROE (5.7) and AAR (5.8) from the main project exist — that's the blocking dependency, not the ACE side.
