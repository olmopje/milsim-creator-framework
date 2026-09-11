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

## ANSWERED: the live in-game map cannot be the thing on the board

`SCR_MapEntity.c` was read in full. The good half first:

**Opening a map into a widget tree of your own is fine.** `OpenMap` takes the
root widget straight off the config and does exactly one thing with it:

```c
m_wMapRoot = config.RootWidgetRef;
SetMapWidget(config.RootWidgetRef.FindAnyWidget(SCR_MapConstants.MAP_WIDGET_NAME));
```

It never asks a menu for anything. Per-frame work is driven by the entity's
own `EOnFrame`, not by a menu. All sizing goes through
`m_MapWidget.GetScreenSize()`, so a small widget is measured correctly. A map
in our widget instead of the map menu's is just a different `RootWidgetRef`.

**But there can only ever be one, and it is the player's.** Four things in the
file, each fatal on its own:

- `protected static SCR_MapEntity s_MapInstance;` assigned unconditionally in
  the constructor and nulled in the destructor. A second entity steals the
  global that everything resolves through, and its destruction breaks the
  first.
- All thirteen invokers are `static`, and the destructor calls `.Clear()` on
  every one of them. Events carry no instance, so every existing map module
  would react to our map's pan and zoom as well.
- ```c
  if (m_bIsOpen)
  {
      Print("SCR_MapEntity: Attempted opening a map while it is already open", LogLevel.WARNING);
      CloseMap();
  }
  ```
  Two maps on one entity is impossible: the second open tears the first down.
- `SetupMapConfig` returns and mutates the *shared* `m_ActiveMapCfg` when the
  mode matches, so asking for a config for our widget rewrites the
  `RootWidgetRef` of the config the open map is using.

And the one that ends the discussion even for a board that keeps a map open
permanently -- from `OpenMap`, not gated on fullscreen:

```c
if (plc && GetGame().GetCameraManager().CurrentCamera() == plc.GetPlayerCamera())
    plc.SetCharacterCameraRenderActive(false);
```

**Opening a map switches off the character camera.** A board holding a map
open would black out the player's own view of the world.

So "the board shows the live in-game map, markers and all" is not available.
Not hard -- unavailable, unless the class itself is replaced.

## What is available, and is arguably better

Build the board out of the two things that are proven, and own the data:

- **`RenderTargetWidget.SetWorld(world, camera)`** -- a camera looking
  straight down at the real world, rendered into a texture. This is exactly
  what `SCR_MapRTWBaseUI` does for the compass and the watch, pointed at the
  world instead of a preview. A live aerial view, which for a whiteboard or a
  beamer board is the right look anyway.
- **`RTTextureWidget.SetRenderTarget(entity)`** -- that widget tree becomes
  `$rendertarget` in the board's material.
- **Our own overlay** in the same widget tree: markers and freeform drawings
  as widgets on top of the render.

`RTTextureWidget` carries `SetMaxFPS` and `SetResolutionScale` (with FSR),
which says BI expected this to be used for exactly this kind of thing and
gives the two dials needed to keep it cheap. Render only when somebody is
near, cap the frame rate, drop the resolution.

**And this is what the next step needs anyway.** A map of the enemy's, found
in a house, carrying *their* markings, must not be the live map -- it is a
snapshot with somebody else's information on it. Owning the overlay is the
whole feature, not a workaround for one.

## The order that carries the least risk

## Open questions before building

- Does `RenderTargetWidget.SetWorld` accept the **real** world with a second
  camera index, or does it want a world of its own the way the compass tool
  builds one? The compass creates a preview `BaseWorld`; nothing says the
  real one is refused. Try it first -- it decides whether a board is cheap or
  needs its own world.
- What a board costs with several of them in a command post. `SetMaxFPS` and
  `SetResolutionScale` are the dials; "render only while somebody is within
  N metres" is the third.
- Where the cartographic imagery lives, if a paper frame should look like a
  map rather than an aerial photograph. The map layers are native
  (`InitializeLayers`, `SetImagesetMapping`) and not visible from script.
- Whether a material can be written that samples `$rendertarget` per entity
  instance, or whether every board needs its own material.
