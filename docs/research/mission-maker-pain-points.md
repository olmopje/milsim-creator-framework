# Research: pain points of Arma 3 mission makers/units when moving to Reforger

Sources: Steam discussions, Bohemia forums, BI feedback tracker, ACE3 GitHub. Qualitative, not a survey — meant to validate our priorities, not as hard data.

## 1. Confirmed: the project addresses the #1 complaint

Virtually every discussion about "why is Reforger's mission-making worse" arrives at the same point: no in-game editor like Eden. Bohemia itself (developer Nillers, official response): *"We know many people are missing the in-game Eden Editor. It's a complex thing we intend to fully focus on our way to Arma 4."* — so confirmed: no plans for Reforger itself.

**Consequence:** our whole project (live GM-placeable, mission-maker-friendly logic) falls exactly into the gap that Bohemia itself acknowledges and won't fill before Arma 4.

## 2. Warning signal: ACE3 declined Reforger support

From the official ACE3 GitHub discussion: *"Due to severe limitations of the Enfusion engine and its scripting language, the lack of an in-game editor and most importantly an active playerbase we currently aren't working on bringing ACE to Arma Reforger."*

No specification of which limitations. Our scope is considerably smaller than a full medical/ballistic system like ACE, so this isn't a direct blocker — but it is a signal to stay alert for unexpected scripting limits during Phase 0 testing, in line with the "test first, build second" attitude we already apply to the stealth module.

## 3. New module opportunity: Squad Cohesion / C2 layer

The most specific and repeated complaint from experienced players, not about bugs but about design:
- *"Even if you're a member of a given team, you can spawn anywhere you like, you have no idea where the rest of the team is... the team leader similarly has absolutely no idea where any of his team members are."*
- *"Being part of a squad ATM has no meaning, besides using the radio respawn, which most people don't even know is a mechanic at all."*

This is exactly the "milsim feeling" that Arma 3 veterans miss — not a lack of content, but a lack of **forced/meaningful coordination**. See section 5.10 in `ARCHITECTURE.md` for the write-up as a new module.

## 4. Confirmed problem, but out of our reach — explicitly stated

AI command behavior is named by the community and by BI's own feedback tracker (T190910) as one of the biggest frustrations: AI that doesn't exit vehicles on command, follow orders that break. This sits at the **engine-pathfinding level** — no scenario/scripting layer can structurally fix this.

**Update: partial mitigation is feasible, no structural fix.** See section 5.11 in `ARCHITECTURE.md` — a watchdog pattern that detects and corrects stuck AI can noticeably reduce the symptoms without addressing the underlying pathfinding. This remains a patch, not a cure, and is communicated to the unit as such.

## 5. Small, low-priority note

No ability to recruit AI teammates in Conflict/Combat Ops (only via Game-Master-assigned squads) — a small QoL addition (e.g. a "recruit at base" action), but this changes base gameplay systems rather than mission logic. Noted as a possible future standalone small addition, not a core module.

## 6. Already covered, for confirmation

The wish for multiple save states in Game Master (2022 forum complaint: *"if it WOULD have multiple save games, you could at least use it like a sort of..."*) is already directly addressed by our Save/Load architecture (section 8, main document).

## 7. Out of scope, no mod solution

Crossplay toxicity/team-killing by console players is a community-management/matchmaking issue, not a mission-scripting problem. Not included.
