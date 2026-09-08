# Stealth & Suppression improvement — technical sub-project

Standalone document, same reason as the vehicle-shooting document: this touches weapon stats, character animation, and (possibly) AI perception — not the Core/Event Bus architecture of the mission framework.

---

## 1. Starting situation — correcting the assumption first

"Silencers are useless" is not fully correct. Confirmed in vanilla:
- Suppressors dampen sound and largely mask the muzzle flash (not entirely)
- Players report usable stealth engagements at 400+ meters, since AI without a visible muzzle flash can only return fire based on sound direction, with a heavy accuracy penalty

**The real problem:** there is no subsonic ammunition available via the Arsenal. Bullets stay supersonic and therefore always produce an audible "crack", independent of the suppressor. The suppressor does its job, the ammunition undermines it.

---

## 2. Relevant existing building blocks (official)

| Component | Relevance |
|---|---|
| **`SCR_WeaponStatsManagerComponent`** | Official system through which attachments (including suppressors) modify weapon stats (e.g. muzzle velocity factor). The right, documented place to implement suppressor effectiveness and subsonic ammo behavior — no new system needed. |
| **`InventoryItemComponent` custom attributes** | Basis that `WeaponStatsManagerComponent` leans on — needed to define a new ammo variant (subsonic) as a separate, moddable entity |
| **Perception Factor (from Scenario Framework actions, section 5.3 in the main project)** | Confirmed existing concept: AI perception ability is already configurable as an attribute. An anchor point to differentiate sound detection per ammo type, provided the underlying system allows it (see open question below) |
| **Scroll-wheel stance system (vanilla)** | Widely regarded by the community as stronger than Arma 3 — extend, don't replace |

**Important uncertainty:** Bohemia itself does not explicitly document the AI detection system ("ongoing development"). The stance/speed-to-camouflage coupling that's hard-coded in Arma 3 must not simply be assumed to be present 1:1 in Reforger — this needs to be empirically tested first before anything is built on it.

---

## 3. Proposed improvements

### 3.1 Subsonic ammunition (biggest lever, lowest risk)
- New ammo variant per suppressor-compatible caliber, via `WeaponStatsManagerComponent`
- Significantly lower sound-detection radius than standard ammunition, in exchange for a realistic penalty (lower effective range/stopping power at distance — subsonic ammunition is also weaker in reality)
- **Balance requirement:** no "free invisibility" — the penalty must be significant enough that it stays a tactical choice (sneak closer, less range) rather than a strictly-better option

### 3.2 Verify stance/camouflage coupling and make it explicit if needed
- Test first: does vanilla AI actually noticeably react differently to prone+stationary vs. crouch+moving vs. standing+running in terms of detection range?
- Only if that doesn't differentiate enough: add a dedicated layer that translates stance+speed into an adjustment on the Perception-Factor-like value — **don't** build blind without first confirming what vanilla already does, otherwise you build something that already exists or that conflicts with the native logic

### 3.3 Finer prone/crouch height control
- Extension of the existing scroll-wheel stance system with more intermediate steps for height, specifically aimed at aiming just over cover
- This is animation/character-controller work, same technical domain as the vehicle-shooting document — no scripting logic

### 3.4 Detection feedback for the player — Alert system
The player needs to be able to sense that they're close to being spotted (and can still escape) without it becoming a gamey HUD meter that breaks immersion — consistent with the "fake it until you need it" philosophy from the main project.

**Core idea: react to AI state transitions, not raw detection values.**
This fully decouples the system from the uncertain internal perception logic in section 3.2 (not yet confirmed) — you only listen for *when* the AI changes state, not *how* that decision came about. **Four states** (expanded from the original three-state design, to make it feel more dynamic — AI that wants to get closer first instead of immediately going to full alarm):

| Transition | What happens | What the player perceives (diegetic, no HUD) |
|---|---|---|
| **Unaware → Suspicious** | AI registers something ambiguous (sound/glimpse), stays in place | Bark ("What was that?"), head/body turn toward the direction — subtle, easy to miss |
| **Suspicious → Investigating** | AI leaves its guard/anchor position to investigate the source — reuses the existing **Investigation Distance** setting (part of "Set Max Autonomous Distance") so the group may temporarily deviate further from its normal patrol area | Visible, cautious movement toward the last known location — weapon raised, no fire. This is the tensest moment: the player sees the AI getting closer with no certainty whether they've been spotted |
| **Investigating → Engaged** | Visual/sound contact confirmed while investigating | Unmistakable: shouting, gunfire, radio call for reinforcements (hooks into the QRF system) |
| **Investigating → Unaware (nothing found)** | Investigation time elapses without confirmed contact — AI returns to its anchor position/patrol | Separate "stand-down" bark/animation — a clear closing moment for the player: "I got away" |
| **Engaged → Unaware (lost contact)** | AI loses contact during the fight itself | Same stand-down signal as above |

**Two separate escape windows, not one:**
- **During Suspicious** (before the AI decides to investigate): if the player stays still/out of sight, the AI falls straight back to Unaware without ever moving — the cheapest and most desirable outcome
- **During Investigating** (the AI is already on its way): a longer window, since the AI has to physically cover the distance — gives the player time to relocate or duck deeper into cover while the AI approaches. This is the part that makes it "more dynamic": the player sees the threat approaching instead of a binary switch

**Important uncertainty, same kind as in section 3.2:** it is not yet confirmed whether vanilla Reforger's underlying Threat State enum already natively has four granularity levels, or whether "Investigating" is a dedicated sub-state we need to build on top of the existing Suspicious/Engaged states with the Investigation Distance setting as the movement mechanism. **First inspect in Workbench which Threat State values actually exist**, don't assume four native steps are already present.

**Mission-maker configurable, not hardcoded (fits the Config-Layer philosophy) — two layers, no fixed default:**

No single level is "the" default — instead, a ceiling-and-preference model:

- **Scenario ceiling (mission maker, server-side, applies equally to everyone):** the mission maker sets, per scenario, the *maximum allowed* feedback level — purely diegetic, diegetic+cue, or diegetic+indicator. This is the only layer that touches the actual mechanic (the escape window/timer always stays server-authoritative and identical for everyone, regardless of this ceiling — fairness stays guaranteed).
- **Personal preference (player, client-side, presentation only):** within that ceiling, every player chooses for themselves how much they see/hear — this only affects their own screen/audio display, not the underlying AI logic or what other players see. A player who prefers to play purely diegetically can choose that, even if the mission maker allows a higher indicator; a player can never get more than the scenario ceiling.

This resolves the "configurable" question structurally: the unit doesn't have to make one choice for everyone, and accessibility preference (some players want more clarity) never undermines fairness between players, because the mechanic itself never changes — only what you personally get to see/hear from it.

**Integration (no new Core components needed):**
- Runs entirely through the Event Bus, within the `MCF_AI_` namespace from the main project
- Purely event-driven — costs nothing as long as no state transition occurs, lining up directly with the performance principles. The Investigating phase *does* cost something extra: the AI group actually moves, so this falls under regular AI simulation cost, not "free event overhead" — no misconception about that should be allowed to stand
- The choice between the three feedback levels is a Config-Layer attribute per scenario, not a global setting — different missions within the same unit can choose a different level

**Dependency, explicitly stated:** this system works regardless of the outcome of the research in section 3.2 (stance/camouflage coupling) — it reacts to state transitions, not the underlying detection formula. Can therefore be built and tested independently.

---

## 4. What is explicitly out of scope here (scope guarding)

- No building a full custom AI perception engine — that would duplicate Bohemia's own "ongoing development" system and continuously fall out of sync with engine updates
- No ghillie-suit camouflage system at this stage — a separate, bigger topic (texture/material-based camouflage coefficients), only take on once this foundation is in place

---

## 5. Proposed first step

1. **Test first, build second:** measure in Workbench how the current AI actually reacts to different stance/speed combinations, before a single line of code for step 3.2 is written
2. Build the subsonic ammunition (3.1) in parallel — lowest risk, clearest win, doesn't depend on the outcome of the AI test
3. Stance height control (3.3) as a separate, independent improvement — can happen anytime, regardless of the outcome of 1 and 2
4. **The detection feedback system (3.4) can start in parallel right away** — independent of 1-3, since it only reacts to AI state transitions, not the underlying detection value. A good candidate to show as the first tangible result: costs little build work, gives immediately playable feedback.

---

## 6. Open questions to answer early

- Does vanilla Reforger AI noticeably react to stance/speed for visual detection, or is this system still too immature to build on?
- Is there already an audibleFire-like coefficient per ammo type present in the current game data that can be reused, or does it need to be fully newly defined?
- What realistic penalty for subsonic ammunition fits milsim balance best without becoming frustrating (range? stopping power? both, toned down)?
- Is `AI Threat State`/`On Threat State Changed` directly usable as SF exposes it, or does it need its own wrapper to support the three feedback levels (purely diegetic / subtle cue / explicit indicator)?
- **New:** does vanilla Reforger's Threat State enum already have four granularity levels (including a separate "investigating" stage), or does that need to be built as a dedicated sub-state on top of the existing states with Investigation Distance as the movement mechanism? First step: look this up in Workbench before a single line of state-machine code is written.
- How long should the Investigating window last to feel "dynamic and tense" without becoming frustratingly slow — this is a feel value that can only be determined through playtesting, not via a formula.
- What feedback ceiling is a reasonable default for new scenarios if the mission maker doesn't configure anything — or should this field be mandatory to explicitly choose during scenario setup (in line with the validation pass from the main project that must never fail silently)?
