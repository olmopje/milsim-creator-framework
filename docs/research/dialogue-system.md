# Open design note: dialogue is a separate channel from notifications

Status: **parked** — explicitly not a now-issue. The current popup text
display is fine for test messages and system feedback; this note records what
dialogue needs so the two do not get conflated later.

## Two channels, not one

What exists today is a **notification channel**:

`MCF_Voice_LineQueueManager` -> `MCF_Voice_LineDisplayed` ->
`MCF_UI_LineDisplayComponent` -> vanilla `SCR_PopUpNotification`

One-way, transient, no player input, does not take focus. Correct for
"Contact! Sentries are alerted", objective updates, hints, and any system
message. Keep it for that.

**Dialogue** is a different thing: an NPC line, a set of player response
options, a branch chosen by the player, and repeat. Reference point named by
the project owner: Kingdom Come Deliverance. It takes input focus, it blocks,
and the player's choice has consequences.

Trying to serve both from the popup channel would ruin both. They should stay
separate systems that happen to share the event bus.

## What dialogue actually requires

Materially more than anything MCF has built so far:

- **A real UI layout.** A `.layout` file with the NPC line and a list of
  selectable response widgets. This is Workbench GUI work, not just script —
  the first layout work in this project. (`layout_create` exists in the MCP
  tooling.)
- **A conversation data model.** Nodes of: NPC line, list of player options,
  and per option a next node plus optional conditions and effects. This is the
  first thing in MCF that genuinely needs a graph rather than a flat list, so
  the "no node-graph editor" scope decision from ARCHITECTURE.md 5.12 will need
  revisiting for this system specifically.
- **Input focus / menu context.** Opening and closing a conversation, capturing
  input while it is open, releasing it afterwards.
- **Effects on choice.** Each option should be able to publish an event on the
  MCF bus. That is the clean seam: dialogue produces events, and everything
  already built (Logic nodes, Recipes, Alarm triggers, Objectives) reacts to
  them without knowing dialogue exists.

## What it hangs off

MCF already has the entry point: `MCF_Interact_TalkAction` is a
`ScriptedUserAction` that shows a "Talk" prompt on an NPC and calls
`MCF_Interact_HintComponent.Interact()`. A dialogue system replaces or extends
what that call opens, rather than needing a new player-facing trigger.

Per the placeable/carrier-bound split in
`docs/research/placeable-vs-attached-nodes.md`, dialogue is clearly
**carrier-bound**: it belongs on an NPC, not as a node dropped on the terrain.
The conversation data itself is a resource the NPC points at.

## Open questions for when this is picked up

- Author conversations as a config resource (`.conf`) referenced by the NPC, or
  as component attributes on the NPC itself? A `.conf` scales better past a few
  lines and is reusable across NPCs.
- Does a vanilla dialogue/conversation system already exist in Arma Reforger
  that can be reused or subclassed, rather than building the UI from scratch?
  Worth researching before writing any layout — same lesson as the `m_Flags`
  and `EOnInit` bugs, where the vanilla source held the answer.
- Conditions on options (skill checks, prior choices, faction state) — in scope
  or explicitly out for a first version?

No decision made. The notification channel stays as it is either way.
