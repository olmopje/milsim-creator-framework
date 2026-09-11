# A map you can put on a table, and drawing on the one in your hands

Research, 2026-09-11. Nothing here is built yet. Everything below was read out
of the game's own scripts with `game_read`, which reaches them inside the
.pak -- see HANDOVER on why that beats guessing.

## What was asked for

1. A map object a Game Master can place or hang.
2. Scalable, so it can go on a wall as well as on a table.
3. Several frames: paper map on a table, a whiteboard, a beamer-board.
4. The frame is blank and loads the map of whatever world it is in, so it
   works on every terrain without being told which.
5. What it shows is the real in-game map -- the one you can open -- so the
   markers that are already on it are on it here too.
6. Freeform drawing in the normal map, per channel: a commander draws for
   everyone, a squad leader into squad and command, a soldier into his squad.
   Permissions can follow later if they are the hard part.

## 1-3 are ordinary work

MCF already places intel props, and a prefab variant per frame is how the
letter and the notebook are already done. Scale is an entity property; a wall
version is a second prefab with a different mount. Nothing here needs
research, only building.

## 4-5: what the map can be drawn into

**`RTTextureWidget.SetRenderTarget(IEntity ent)`** is the mechanism, and the
engine's own comment says what it is for:

> Sets this widget instance as render resource which can be referenced in
> material as `$rendertarget`.

So a widget tree rendered into a texture, sampled by a material on a mesh in
the world. That is a map on a wall, and `RemoveRenderTarget` is the other half
-- it must be called when the widget goes away.

The reverse direction already ships: `SCR_MapRTWBaseUI` draws the compass and
the watch by creating a whole preview `BaseWorld`, spawning a prefab in it and
pointing a `RenderTargetWidget` at it with `SetWorld`. Useful as proof that
render targets in this engine work and are cheap enough to sit in the map UI,
but it is 3D-into-UI, and this is UI-into-3D.

**The unknown worth naming.** `SCR_MapEntity` is a world entity whose UI is
built when the map menu opens (`OnMapOpen(MapConfiguration)`), and every map
component hangs off that. Whether a second map instance can be built into an
RTTextureWidget outside the menu is not answered anywhere in the scripts, and
it is the one thing the whole "live map on the wall" idea rests on. It has to
be tried before anything is designed around it.

**The cheap version that certainly works** is the MCF pattern: walk up, the
action opens the real map menu. Universal across terrains for free, markers
for free, no render target, no material work, no performance question. It is
what the phone, the laptop and the letter already do.

Going for the rendered board and falling back to this is a sound order: the
prop, the frames and the placement are the same work either way.

## 6: drawing

**Vanilla's drawing is a stopgap and says so.** `SCR_MapDrawingUI` opens with
"Temporary drawing substitute so the protractor can be utilized properly". It
gives nine straight lines, each a rotated `ImageWidget` with a delete button,
held in a local array, never replicated, and thrown away when the map closes.
Not a foundation. Its mechanics are still the ones to copy:

- `m_MapEntity.GetMapCursorWorldPosition(x, y)` -- where the cursor is in the
  world, not on the screen.
- `m_MapEntity.WorldToScreen(worldX, worldY, screenX, screenY, true)` -- the
  way back.
- Lines live in `SCR_MapConstants.DRAWING_CONTAINER_WIDGET_NAME`.
- `OnMapPan` and `OnMapPanEnd` re-place everything, and the end handler waits
  a frame because a size cannot always be set correctly in the same one.

**So: store strokes in world coordinates and redraw them.** A stroke that is
stored in screen space is wrong the moment anybody pans. `CanvasWidget` is in
the generated API and draws lines directly, which is one widget per drawing
rather than one per segment -- the right shape for freeform.

**Channels have a precedent.** The marker system already does networked map
content that only some people can see: `SCR_MapMarkerManagerComponent`,
`SCR_MapMarkerSyncComponent`, and a `SCR_MapMarkerSquadLeader` whose markers
are squad-scoped. Worth reading before inventing a scheme. MCF also has its
own permission system (`MCF_Task_Permissions`) and its own RPC route, which is
where the commander / squad leader / soldier split belongs.

## The order that carries the least risk

1. The prop: frames, scale, placement. Certain work, useful on its own.
2. The interact route -- walk up, the real map opens. Delivers 4 and 5 in full.
3. Try `RTTextureWidget` on the board. If it renders, the board becomes live
   and step 2 stays as the fallback for anyone who wants the full screen.
4. Freeform drawing, one channel, everyone sees it.
5. Channels and permissions.

Each step is worth having on its own, and no step throws away the one before.
