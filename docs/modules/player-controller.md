# Player Controller improvement — technical sub-project

Standalone document, same reason as the other technical sub-projects: this touches the character controller/input/camera, not the mission-scripting layer of the main framework.

---

## 1. Starting situation — three separate complaints, three separate causes

Not one problem, and therefore not one fix:

1. **Movement responsiveness** (acceleration/deceleration feels sluggish) — a **value tweak**, proven moddable
2. **Mouse turn-speed curve** (fast flicks barely turn, moderate movement turns fast) — a **curve/smoothing setting**, a separate problem from #1
3. **ADS input timing** (the click to aim is missed during the raise-weapon bob animation) — an **input-buffering problem**, fundamentally different in nature from the first two

---

## 2. Proof that #1 is moddable

The workshop mod **"Arcade Movement"** already does exactly this: 2x more responsive acceleration/stopping speed, removes the turn-speed-reduction factor, speeds up sneak mode, unlimited stamina. This proves these parameters are exposed and adjustable.

**Deliberate choice: don't build on this mod, but use the same underlying parameters.** Same consideration as earlier with Scenario Framework/GME/Ci5 — unknown maintenance status, and "arcade" probably goes further than what a milsim unit wants (unlimited stamina, for example, undermines tactical fatigue as a game concept). Goal: **find the same knobs, choose our own, subtler tuning** — more responsive without losing the tactical weight that milsim specifically values.

---

## 3. Mouse turn-speed curve — separate problem, separate approach

Community reports: with a fast mouse flick the character barely turns, with moderate constant movement it turns smoothly — points to a **curve/clamping problem**, not just a sensitivity scale (players report that even 200% sensitivity doesn't fix the underlying behavior, which confirms it's not a simple scale factor). This needs to be identified separately from the movement-acceleration parameters in section 2 — likely a separate curve configuration for camera/aim turn rate.

**First step:** search the camera/input configuration in Workbench for a curve or clamp value for turn rate, separate from the walk/run-speed parameters.

---

## 4. ADS input timing — a fundamentally different kind of problem

This is not a value tweak but an **input-handling problem**: the click to aim is ignored if it happens exactly during the weapon-raise bob animation, instead of being remembered and still executed once the animation allows it.

**Possible anchor point:** earlier engine updates added `CharacterCommandHandlerComponent.IsItemActionLoopTag` and `FinishItemUse()` for continuous context actions (reloading, repairing) — this suggests there are already scriptable hooks in the command handler for this kind of timing problem. **Not yet confirmed whether this specific ADS input problem is solvable via the same hooks** — requires direct inspection in Workbench, don't assume.

**Realistic estimate:** this is considerably more work than section 2/3, since it touches a state-machine/timing problem instead of an exposed value. Building an "input buffer" (remember the ADS intent, execute it once the animation state allows) is the right pattern, but requires first confirming the command handler allows that from script.

---

## 5. What is explicitly out of scope here

- No full controller replacement or custom input system — too risky, too much chance of introducing new bugs in something every player constantly touches
- Head-bob/camera smoothing (separately reported complaint, "nausea" on uneven terrain) — a separate, smaller topic, only take on once sections 2-4 are stable

---

## 6. Proposed first step — iterative, no fixed end profile

**Decision:** the unit doesn't want to choose in advance between "slightly less clunky" and "toward arcade" — this gets play-tested, not locked in at once. This changes the approach: build the parameters so they're **easy to adjust repeatedly**, not as separate hard values you have to look up and manually adjust every time.

**Practical implementation:**
1. **Section 2 first, with three test profiles instead of one number** — e.g. "Profile A: 25% more responsive than vanilla", "Profile B: 50%", "Profile C: 75% (close to Arcade Movement)". Have multiple unit members try all three, not just whoever builds it — one person's personal preference is not a good measure for a whole unit
2. **Gather feedback in a structured way**, not just loose "feels good/bad" — ask specifically: does it still feel "heavy" enough during a long patrol? Does anyone miss the sluggish feeling from fatigue? Does it feel responsive enough during CQB?
3. **Investigate section 3 separately** — a different parameter family, don't assume it comes along with section 2
4. **Section 4 only as a standalone, bigger follow-up track** — acknowledge up front that this costs more time than the first two
5. **Lock the chosen value into the main project's Config-Layer philosophy** (configurable, not hardcoded) once there's a preference — so the unit can still adjust later without having to dive back into code, should taste change after a few months of playing

---

## 7. Open questions to answer early

- Which exact config parameters does Arcade Movement touch — are they directly findable in Workbench's Resource Browser on the character prefab, or is that deeper?
- Is the turn-speed curve (section 3) a separate camera configuration, or tied to the same place as the movement parameters?
- Is `CharacterCommandHandlerComponent` sufficiently accessible from script to build an ADS input buffer (section 4), or does the bob-animation timing sit deeper in closed C++?
- **Decided:** tuning gets iteratively play-tested with multiple profiles (see section 6), no fixed choice up front — the question is now practical: how many test profiles are feasible without burdening the unit with too many separate builds, and who coordinates gathering feedback?
