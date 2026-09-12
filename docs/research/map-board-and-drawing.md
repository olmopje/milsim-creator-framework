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

## CORRECTION: two of those obstacles are softer than they looked

### There is no map texture to load

Worth saying plainly, because it is the first thing anybody suggests: the map
is not an image. `SCR_MapConfig` shows it is **drawn from terrain data**, per
zoom layer, at runtime:

```c
MapGridProps gridProps = layer.GetGridProps();
gridProps.SetGridStepSize(m_fGridSquareSize);

MapContourProps contProps = layer.GetContourProps();
contProps.SetContourDensity(m_fContourDensity);
contProps.SetMajorDensity(m_fMajorContourDensity);

MapLegendProps legendProps = layer.GetLegendProps();
legendProps.SetTotalSegmentLength(m_fLegendScaleSize);
```

Contours, grid and legend are generated from the heightmap. There is no file
anywhere that is "the map of Everon". So "just load the texture the game
loads" has nothing to load.

### But the camera does not have to stay off

The line that looked fatal:

```c
if (plc && GetGame().GetCameraManager().CurrentCamera() == plc.GetPlayerCamera())
    plc.SetCharacterCameraRenderActive(false);
```

is one call with a public counterpart — `CloseMap` turns it back on with
`plc.SetCharacterCameraRenderActive(true)`. Nothing stops us calling that
ourselves immediately after opening a map into a board. It is an optimisation
for a fullscreen map, not a requirement of the renderer. **Probably removable,
and cheap to test.**

### And the map widget is an ordinary CanvasWidget

`SCR_MapConstants.MAP_WIDGET_NAME` resolves to a plain `CanvasWidget`. Its
base carries exactly the API `SCR_MapEntity` uses on it:

```c
sealed class CanvasWidgetBase: Widget
{
    proto external float PixelPerUnit();
    proto external vector GetSizeInUnits();
    proto external void SetSizeInUnits(vector newSize);
    proto external void SetZoom(float zoomLevel);
    proto external vector GetOffsetPx();
    ...
}
```

So the widget is a zoomable coordinate space that the map's world is measured
into — `SetSizeInUnits(terrain size in metres)`, then `PixelPerUnit()` back.
Script never issues a draw command on it; the pixels are produced natively.

**Whether those pixels are composited into the widget's place in the UI tree
(capturable by a render target) or drawn straight to the screen cannot be
answered by reading.** Nothing in script passes a widget handle or a screen
rect across to the native side. `SetFrame` takes *world* coordinates and is a
culling frame, not a viewport.

### So the question is down to one experiment

Put the map layout's subtree under an `RTTextureWidget`, call `OpenMap` with
that root, call `SetCharacterCameraRenderActive(true)` straight after, and
look at the board.

- Terrain appears on the board -> the whole live-map idea is on, and only the
  one-map-at-a-time limit remains (the board goes dark while that client has
  their own map open, which is the moment they are not looking at the board).
- Nothing on the board but the map appears over the screen -> it renders to
  the screen and cannot be captured. Fall back to the world render below.
- **The tell either way**: `m_MapWidget.PixelPerUnit()`. If it comes back <= 0
  the widget is not being laid out inside the render target at all, the
  `pixelPerUnit = 0.01; // should never happen` fallback fires, and nothing
  downstream can be right.

Half an hour, and it decides the shape of the feature.

## MEASURED: the map goes in a widget of ours, and a render target holds one

Run on 2026-09-11 with `MCF_Map_ProbeMenu`. Verbatim:

```
A before open: 0 x 0, PixelPerUnit 0, zoom 1
B before open: 0 x 0, PixelPerUnit 0, zoom 1
config built: 6 layer(s), props 1, descriptor defaults 1, descriptor visibility 1
OpenMap called with our own root and a config carrying no modules and no components
character camera switched back on
A after open: 640 x 420, PixelPerUnit 0.15625, zoom 2.13333
B after open: 640 x 420, PixelPerUnit 0.625, zoom 1
map reports open: true, zoom 1
visible world frame <0, 0, 3676> .. <640, 0, 4096>
```

Terrain appeared in the left box. So, settled:

1. **The map draws inside a widget we made**, at a position and size we chose.
   Not full screen, not somewhere else. A board is possible.
2. **A map widget inside an `RTTextureWidget` lays out** — 640 x 420 with a
   real `PixelPerUnit` of 0.625, not the `<= 0` that would have meant no
   layout at all and no board this way.
3. **The character camera can be given straight back.** `OpenMap` switches it
   off, the probe switches it on in the next line, and the map carries on. The
   line that looked fatal is simply removable.
4. **A map with no modules and no components works.** Which is what a board
   wants anyway -- no cursor, no tool menu, no ruler -- and it steps around
   `SetupMapConfig` handing back and mutating the shared `m_ActiveMapCfg`.

Two things the first run got wrong, both mine and both worth remembering:

- **`Anchor` plus a size gives 0 x 0.** Copy `MapMini.layout`: no `Anchor`,
  position and size, offsets as the negative of position+size. A render target
  sized zero then asks for a texture sized zero, which fills the log with
  `Out of memory when requested 0, Type: Video` -- an error that reads like a
  graphics problem and is a layout problem.
- **The gadget map config brings the whole component set**, and
  `SCR_MapCursorModule.InitWidgets` throws once per frame looking for widgets
  a board has no reason to own.

### PROVEN: the map drives a widget inside a render target

Stage two. Two hosts, each holding a widget called `MapWidget`; `OpenMap` does
`FindAnyWidget` for that name on whatever root it is given, so handing it one
host or the other is the entire difference. Verbatim:

```
opened into A (PlainHost)
A: 640 x 420, PixelPerUnit 0.15625, zoom 2.13333
B: 640 x 420, PixelPerUnit 0.625,   zoom 1        <- B untouched, layout default

B pressed
opened into B (Probe)
A: 640 x 420, PixelPerUnit 0.15625, zoom 2.13333
B: 640 x 420, PixelPerUnit 0.15625, zoom 1        <- B moved
```

**B's `PixelPerUnit` went from 0.625 to 0.15625 at the moment the map was
opened into it, and 0.15625 is exactly what the map gives the widget it is
bound to.** 0.625 is the arithmetic of the layout alone — 640 pixels over the
declared `SizeInUnits` of 1024. 0.15625 is the map's own: it calls
`SetSizeInUnits(terrain size in metres)` on its widget and sets the zoom, and
pixels-per-metre falls out of that.

So `SCR_MapEntity` reached inside an `RTTextureWidget`, took the map widget it
found there, and drove it. **A render target can hold the game's own map.**

Repeated twice in the same run, with the same numbers.

### The one thing left

That the pixels land in the render target's *texture*. The map is
demonstrably driving a widget inside one; whether what it draws is captured
needs `RTTextureWidget.SetRenderTarget(entity)` and a material sampling
`$rendertarget` on a board in the world. That is material authoring, not
script, and it is the last unknown in the chain.

Everything before it is now measured rather than hoped for:

| question | answer |
| --- | --- |
| map in a widget we choose, at a size we choose | yes |
| map with no modules and no components | yes, and that is what a board wants |
| character camera can be given straight back | yes |
| a map widget inside a render target lays out | yes |
| the map drives that widget | yes |
| the render target's texture receives the pixels | **not yet tested** |

### What the probe cost to get right

Two mistakes, both mine, both worth not repeating:

- **`Anchor` plus a size gives 0 x 0.** Copy `MapMini.layout`: no `Anchor`,
  position and size, offsets as the negative of position+size. A render target
  sized zero then asks for a texture sized zero, and the log fills with
  `Out of memory when requested 0, Type: Video` — an error that reads like a
  graphics fault and is a layout fault.
- **The gadget map config brings the whole component set**, and
  `SCR_MapCursorModule.InitWidgets` throws once per frame looking for widgets
  a board has no reason to own. Build the configuration by hand: the four
  defaults named in `SCR_MapConstants`, no modules, no components.

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

---

## SETTLED IN GAME: the board works, and what it cost to find out

The last unknown in the chain -- whether the render target's texture actually
receives the map's pixels -- is answered. It does. A board in the world shows
the world's real map, the one with everybody's markers on it.

Three wrong answers were paid for on the way, and each is worth keeping
because each looked right:

**"The board is blank, so the render target does not work."** No. The board
was showing the map widget's ClearColor -- `0.173 0.344 0.62` in linear, which
arrives as a pale steel blue on screen. A blank board that is *the right blue*
is proof the whole chain works and only the content is missing. Check the
colour before blaming the plumbing.

**"Hang the widget tree on the screen and see whether the map draws."** That
test cannot answer anything, and it took a round trip to see why: the thing in
that tree is the `RTTextureWidget`, and an `RTTextureWidget` sends its subtree
to the entity's mesh instead of to the screen. On the screen it is a hole. The
test needs the same `MapWidget` with **no render target around it**.

**"The character camera is the problem."** It is not. With
`SetCharacterCameraRenderActive(false)` the board is still blank, and in Game
Master the map and the world are visibly drawn at the same time -- so "one
scene view at a time" is false. The line in `OpenMap` really is an
optimisation.

### What it actually was: the map is taken away

Every run had it in the log and it was read past three times:

```
15:36:30  map board raised with 6 layer(s)
15:36:31  fitted at zoom 0.170898
15:36:32  dump: open true, frame covers the island
15:36:33  SCR_MapEntity: Attempted opening a map while it is already open
```

Two to four seconds after a board comes up, something else opens a map -- the
spawn screen, the Game Master, a player's own map -- and `SCR_MapEntity`
closes ours to make room. Nothing ever hands it back, so the board is blank
from then on, forever.

The board therefore **watches and reclaims**: four times a second it asks
whether a map is open and whether the open one is the widget in its own tree.
If a map is open and it is not ours, somebody is reading it and we wait. If
none is open, we take it. The board is blank exactly while somebody is reading
a map, which is exactly when nobody is looking at the board.

`GetMapWidget() == m_wMapWidget` is the whole test, and it is also what stops
`OnDelete` from closing a map that belongs to a player.

### Activation distance

A live map on a surface is the most expensive thing MCF draws, and a mission
can have several boards. So each one carries its own range:

| attribute               | default | what it is                               |
| ----------------------- | ------- | ---------------------------------------- |
| `m_fActivationDistance` | 40 m    | past this the map is not drawn at all    |
| `m_fFadeBand`           | 5 m     | metres of fade before that, so 35 -> 40  |

The fade is **a white panel over the map inside the render texture**, walked
from opacity 0 to 1 -- not anything done to the map or the material. A map has
no opacity of its own to turn down, and dimming the board's material would
take the frame with it. At 1 the board reads as a blank white board, which is
what a map looks like from across a field anyway.

At full white the map is closed and the render target drops to 1 FPS, so a
board nobody is near costs a still frame of white. Distance is measured from
the **current camera**, not the character, because a Game Master flying around
has no character where their eyes are.

---

# HOW WE GET THE REST OF THE WAY

Research pass, 2026-09-12, after the board worked but not well. Everything
below is quoted from the game's own scripts. The headline is one line in a
generated file that changes the shape of the whole feature.

## 1. The map widget is a CanvasWidget, and a CanvasWidget takes draw commands

```c
// scripts/Game/generated/UI/MapWidget.c
sealed class MapWidget: CanvasWidget
{
}
```

```c
// scripts/Core/generated/UI/CanvasWidget.c
proto external void SetDrawCommands(array<ref CanvasWidgetCommand> drawCommands);
proto ref ImageDrawCommand CreateCommandFromImageSet(ResourceName resource, string imageName, vector size);
proto void TessellateCircle(vector center, float radius, int segmentCount, out notnull array<float> vertices);
```

and the commands themselves (`scripts/Core/proto/EnWidgets.c`):

```c
class LineDrawCommand : CanvasWidgetCommand
{
    int m_iColor = 0xff000000;
    ref array<float> m_Vertices;   //!< 2D vertices such as [x0, y0, x1, y1, ... xn, yn]
    float m_fWidth;
    float m_fOutlineWidth;
    int m_iOutlineColor;
    ref SharedItemRef m_pTexture;
    vector m_UVScale;
    bool m_bShouldEnclose;
}
class ImageDrawCommand : CanvasWidgetCommand { ... vector m_Position; vector m_Size; float m_fRotation; ... }
class TextDrawCommand : CanvasWidgetCommand { ... string m_sText; vector m_Position; float m_fSize; ... }
class PolygonDrawCommand, TriMeshDrawCommand, CompositeDrawCommand
```

**This is the thing the board has been missing.** Everything we have wanted to
put on the board that the engine does not draw for us -- markers, labels,
freeform strokes, a north arrow, a scale bar -- is a draw command on a canvas,
not a widget per item and not a map entity setting. One array, rebuilt when
something changes, handed over with `SetDrawCommands`.

Two constraints to design around, both stated by the engine:

- `const int CANVAS_COMMAND_VERTICES_LIMIT = 400;` in `SCR_MapConstants` --
  "hardcoded in ENF". A long stroke is several commands.
- "The caller needs to keep the array alive - the callee takes just a pointer
  to it." So the array is a field, not a local.

And the map layout has a canvas reserved for exactly this that **nothing in
vanilla claims**:

```c
const string DRAWING_WIDGET_NAME = "DrawingWidget";   // name of the CanvasWidget for drawing within map layout
```

## 2. So the board draws its own overlay, in its own tree

The board already owns a widget tree that nothing else touches. Adding a
`CanvasWidget` over the `MapWidget` inside it -- same size, same slot form --
gives us a layer the board draws and nobody else can disturb, inside the
render target, for free.

The world-to-pixel conversion does not have to be hand-rolled either.
`CanvasWidgetBase` does it, honouring the widget's own zoom and offset:

```c
// scripts/Core/generated/UI/CanvasWidgetBase.c
proto external float PixelPerUnit();
proto external float GetZoom();
proto external void SetZoom(float zoomLevel);
proto external vector PosToPixels(vector posUnits);   // <- this
proto external vector SizeToPixels(vector sizeUnits);
proto external void ZoomAt(vector posUnits, float zoomLevel);
proto external vector GetOffsetPx();
proto external void SetOffsetPx(vector offsetPx);
proto external vector GetSizeInUnits();
proto external void SetSizeInUnits(vector newSize);
```

`SetSizeInUnits` is already set to the terrain size in metres by the map's own
open, so a world position in metres is a position in units, and `PosToPixels`
is the whole conversion. Mind the Y flip the map uses everywhere:
`SCR_MapEntity.WorldToScreen` starts with `worldY = m_iMapSizeY - worldY;`.

## 3. Markers: the data is readable at any time, the widgets are not

`SCR_MapMarkerManagerComponent` sits on the game mode and has a static
instance:

```c
protected static SCR_MapMarkerManagerComponent s_Instance;
static SCR_MapMarkerManagerComponent GetInstance() { return s_Instance; }

array<SCR_MapMarkerBase> GetStaticMarkers()      // returns a copy
array<SCR_MapMarkerBase> GetDisabledMarkers()    // returns a copy
array<SCR_MapMarkerEntity> GetDynamicMarkers()   // live array
SCR_MapMarkerConfig GetMarkerConfig()
```

**The arrays on a client already contain only what that player may see.** A
marker of another faction is dropped at insert:

```c
if (localFaction && !isMyFaction)
{
    m_aStaticMarkers.RemoveItem(marker);
    return;
}
```

so the board does not have to reimplement channel permissions to show markers
honestly -- it shows what its viewer's client was given.

One trap, and vanilla walks into it deliberately: markers outside the open
map's visible frame are MOVED into `m_aDisabledMarkers`. Read both arrays and
union them, the way `SCR_MapMarkersUI.CreateStaticMarkers()` does.

A marker carries integers, not objects:

```c
void GetWorldPos(out int pos[2])   // world X and world Z, no elevation
int GetIconEntry()                 // index into the config, not an image
int GetColorEntry()                // index into the config, not a Color
string GetCustomText()
int GetRotation()
SCR_EMapMarkerType GetType()
```

and the icon and colour resolve through the config:

```c
bool GetIconEntry(int i, out ResourceName imageset, out ResourceName imagesetGlow, out string imageQuad)
Color GetColorEntry(int i)
```

which is exactly what `CanvasWidget.CreateCommandFromImageSet(imageset, quad,
size)` wants.

**Why we do NOT try to reuse vanilla's marker widgets.** They are created as
children of the map menu's own frame and die with it:

```c
Widget mapFrame = mapRoot.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
m_wRoot = GetGame().GetWorkspace().CreateWidgets(m_ConfigEntry.GetMarkerLayout(), mapFrame);
```

and they are repositioned by `SCR_MapMarkerManagerComponent.Update`, which is
only registered between `OnMapOpen` and `OnMapClose`. A board that is not a
map menu gets none of it. Reading the data and drawing it ourselves is not a
workaround; it is the only route, and it is less code than the alternative.

## 4. Independence: drive the WIDGET, not the entity

This is the part that has cost the most and it may have a one-line answer.

Everything the board writes today goes to the map ENTITY -- `ZoomChange`,
`PosChange`, `SetFrame`, `EnableVisualisation`, `EnableGrid`, `SetLayer` --
and every one of those is shared with whatever else is using the map.

But zoom and offset also exist ON THE WIDGET, per widget: `SetZoom`,
`SetOffsetPx`, `SetSizeInUnits` above. And the observed behaviour fits that:
the board did NOT follow a player panning their own map, which is only
possible if the view state that matters is the widget's.

**So the test, and it is small: set the board's view with
`m_wMapWidget.SetZoom()` and `m_wMapWidget.SetOffsetPx()` and stop calling
`ZoomChange` and `PosChange` at all.** If the board keeps its view while
somebody drives their own map, the board is genuinely independent and the
whole "wait while a map is open" dance can go.

What stays shared no matter what, because it is on the entity:

| call                    | scope   |
| ----------------------- | ------- |
| `EnableVisualisation`   | global  |
| `EnableGrid` / `EnableOverlay` / `EnableLegend` | global |
| `SetLayer`              | global  |
| `SetFrame`              | global  |
| `InitializeLayers`      | global -- and destructive, see the verdict above |

`SetFrame` is the one to watch: it is the world rectangle the engine prepares,
so while a player's map is open theirs wins and the board may be culled to
their rectangle. If that shows up, the answer is to re-state the union of both
rather than to fight for it.

Layer PROPERTIES, unlike the layer selection, are per layer and safe to read:

```c
MapDescriptorProps GetPropsFor(int iFaction, EMapDescriptorType type);
MapGridProps GetGridProps();  MapContourProps GetContourProps();  MapRoadProps GetRoadProps();
```

## 5. Freeform drawing: build it, do not copy vanilla's

Vanilla's map drawing is not a model:

```c
//! Temporary drawing substitute so the protractor can be utilized properly
[Attribute("9", UIWidgets.EditBox, desc: "Max line count")]
protected int m_iLineCount;
```

Nine straight segments, each an `ImageWidget` rotated and stretched between
two world points, stored in a `protected` array with no getter, destroyed on
map close, and -- checked for `[RplProp]`, `[RplRpc]`, `RplComponent` -- **not
replicated at all**. There is nothing to reuse and nothing to mirror onto the
board.

What we build instead falls straight out of section 1: a stroke is a list of
world points; drawing it is one `LineDrawCommand` per 200 points (400 floats);
placing it is `PosToPixels` per point. The same array of strokes drives the
board's canvas and, later, a canvas in the player's own map -- the map layout
already has `"DrawingWidget"` sitting unclaimed for it.

Channels then become what they should be: a property of a stroke on the
server, and a filter when each client builds its command array. Nothing about
the drawing code needs to know about permissions.

## 6. The order to build it in

1. **Move the board's view onto the widget** (`SetZoom` / `SetOffsetPx`).
   Smallest change, and it decides whether the board can be independent. Every
   later item is easier if it is.
2. **Confirm the corrected pan** from the log line already in place, or delete
   the pan maths entirely if step 1 replaces it -- `SetOffsetPx` may make it
   moot.
3. **A canvas over the map inside the board's tree**, and the world-to-pixel
   helper on top of `PosToPixels`. Prove it with something trivial: a dot on
   the board at the viewer's own position.
4. **Markers**, read from the manager and drawn as image commands. This is the
   feature the board is missing, and after step 3 it is a loop.
5. **Strokes**, shared state on the server, drawn with line commands on both
   the board and the player's map.
6. **Channels**, as a filter on step 5.

Steps 3 to 6 are all the same machinery, which is the argument for doing 3
properly.

## 7. Is the board's own position and zoom actually possible? What is proven, and the one thing that is not

Asked directly, so answered directly.

### Proven, and we measured it ourselves before we knew what it meant

Stage two of the original probe put two hosts in the tree, each holding a
widget called `MapWidget`, and opened the map into one of them. The numbers
were written down at the time:

```
opened into A (PlainHost)
A: 640 x 420, PixelPerUnit 0.15625, zoom 2.13333
B: 640 x 420, PixelPerUnit 0.625,   zoom 1        <- B untouched
```

That `zoom` is `CanvasWidgetBase.GetZoom()` read off each widget. **Two
widgets, two different zooms, at the same moment.** Zoom is not a property of
the map -- it is a property of the widget, and opening a map into one widget
left the other one's alone.

`SCR_MapEntity.SetZoom` confirms the direction of travel: it works out a
ratio against the widget and hands it to the entity,

```c
float pixelPerUnit = m_MapWidget.PixelPerUnit();
ZoomChange(targetPPU / pixelPerUnit);
```

and the widget the map is open in is the one whose zoom moves. So
`ZoomChange` and `PosChange` are the entity's way of driving THE WIDGET IT
WAS OPENED INTO -- not a global view. Which is why the board, driving them
while no map was open, worked at all.

**So the board's own position and zoom are possible, and the route is to stop
going through the entity:**

```c
m_wMapWidget.SetZoom(level);
m_wMapWidget.SetOffsetPx(offset);
```

Both are `CanvasWidgetBase`, both are per widget, and neither has anything to
do with `m_bIsOpen` or with anybody else's map. The board stops needing to
wait for a player to finish reading, stops needing to re-state anything four
times a second, and stops being able to disturb anyone.

### Not proven: whether the FRAME culls what the board draws

`SetFrame` is on the entity, so it is one rectangle for everybody, and
vanilla's own comment in `OpenMap` says what it is for:

```c
SetFrame(Vector(0, 0, 0), Vector(0, 0, 0)); // Gamecode starts rendering stuff like descriptors straight away
                                            // instead of waiting a frame - this is a hack to display nothing,
                                            // avoiding the "blink" of icons
```

A zero frame displays nothing. So the frame is not a hint, it gates what is
shown -- at least for descriptors, which is what the comment names.

The open question is how far that reaches: while a player has their map open
and panned into one corner, the entity's frame is theirs, and we do not know
whether the board's terrain is culled to it, only its icons, or nothing at
all. Re-stating our frame does not win that argument -- their map writes it
every frame and the board four times a second.

**But most of it does not matter, because of section 3.** The board is going
to draw its markers itself, on its own canvas, from data it reads directly.
Our overlay is ours and no frame touches it. What is left at risk is the
engine's own descriptors and, possibly, terrain.

### The test that settles it, and it is one build

Drive the board's view with `SetZoom` / `SetOffsetPx`, remove the
"wait while a map is open" guard entirely, then in game:

1. Stand at the board, zoom the board in a few steps.
2. Open your own map with M, pan to the far side of the island, zoom right in.
3. Look at the board.

- Board unchanged -> fully independent, and the feature is done the moment
  markers are drawn.
- Board keeps its view but loses icons or terrain outside your rectangle ->
  independent enough, and the answer is to draw the missing parts ourselves.
- Board follows your map -> zoom is per widget but the render is not, and the
  board can only ever be a second view of one shared map.

## 8. CORRECTION: the widget's zoom is not the map's zoom

Section 7 said the board's own view was one small change away. Tested in
game, and it is wrong. Setting `SetZoom` and `SetOffsetPx` on the board's own
widget moves some detail about and leaves the map where it was, and a player
opening their own map still moves the board.

So the measurement in section 7 was real but it was measuring something else.
`CanvasWidgetBase`'s zoom and offset are the CANVAS transform -- the space
draw commands are placed in. The engine renders the map from the ENTITY,
through `ZoomChange` and `PosChange`. One map entity, one rendered view, and
a board cannot hold a different one at the same instant.

**The thing that saves the feature is that the map entity is per client.**
`SCR_MapEntity` is a client-side singleton; a player opening their map takes
the map over on THEIR machine only. Every other client's board is untouched.

So the only screen a board can be wrong on is the screen of somebody who has
their own map open -- and that person is looking at their map, not at the
board. The board goes white on that one client for as long as their map is
open, and comes back the moment they close it. Everybody standing at the
board sees nothing change at all.

Which means the spec is still met, read the way it should be read:

| requirement | answer |
| --- | --- |
| the player's map behaves exactly as before | yes -- the board writes nothing while a map is open |
| the board has its own zoom and pan | yes, and replicated, so everyone at the board sees the same |
| the board is not disturbed by other players' maps | yes -- theirs is on their client |
| the board is not disturbed by YOUR map | it goes blank for those seconds instead of lying |

And markers, when we draw them ourselves on our own canvas, are not affected
by any of this -- which is the argument for doing them that way rather than
looking for a way to borrow vanilla's.

## 9. CORRECTION to section 5: do not copy vanilla's, but DO live inside it

Section 5 is right that vanilla's map drawing has nothing reusable in it --
nine straight segments, unreplicated, thrown away on map close. It drew the
wrong conclusion from that, which was to build the drawing somewhere else.

The right conclusion is that `SCR_MapDrawingUI` is the wrong IMPLEMENTATION
sitting in exactly the right PLACE, and the place is worth more than the
implementation.

**Why the place is worth so much.** `SCR_MapDrawingUI` is listed in
`Configs/Map/MapFullscreen.conf`, and `MapFullscreen.conf` is what
`SCR_MapConfigComponent.GetGadgetMapConfig()` returns. Both the player's own
M map (`SCR_MapMenuUI`) and MCF's Control map window open with that config.
So a `modded class SCR_MapDrawingUI` is:

- in the player's map and in the board's window, from one file;
- given `OnMapOpen(config)`, `OnMapClose(config)` and `Update(timeSlice)` by
  the map itself, with `config.RootWidgetRef` handed to it -- no menu has to
  own it, no menu has to remember to tick it;
- holding `m_CursorModule` and `m_MapEntity` already;
- able to turn vanilla's straight-line mode off when freehand starts, by
  overriding `SetDrawMode` -- the two modes share the left mouse button and
  one of them has to yield;
- built WITHOUT copying `MapFullscreen.conf` into the mod, which is the other
  way to add a component and which freezes MCF's map at today's vanilla.

The first attempt did own it from a menu (a `MCF_Map_Drawer` class started by
`MCF_Map_BoardControlMenu`, plus a panel layout of our own buttons, plus a
planned `modded class SCR_MapMenuUI` to start a second one for the player's
map). All of that is deleted. It was three files and a layout doing what one
`modded class` does better.

**The controls go in the right-click menu, which is open to anyone.**
`SCR_MapRadialUI.GetInstance()`, then `GetOnMenuInitInvoker()`, which the
radial fires after `ClearEntries()` each time it opens. `AddRadialCategory`
and `AddRadialEntry` return `SCR_SelectionMenuEntry` with `SetId`/`GetId` and
`GetOnPerform()`. This is the extension point the user asked for by name
("de standaard map UI (rechtermuisknop in M)") and it needs no vanilla edit.

**The trap in that, which is not obvious until it bites.** `HandleDraw(true)`
sets `EMapCursorState.CS_DRAW`, and `CS_DRAW` is a member of
`SCR_MapCursorModule.STATE_CTXMENU_RESTRICTED`. While draw mode is on, the
right-click menu will not open. A radial entry that only turns drawing ON is
therefore a door that locks behind you. The shape that works:

- six entries, one per colour; performing one sets the colour AND starts
  drawing, a frame later (the menu is still closing and the cursor's
  `CS_CONTEXTUAL_MENU` has to clear before `HandleDraw` will say yes);
- right-click (`MapContextualMenu` UP) stops drawing, because while drawing
  the right button has nothing else to do -- and putting the pencil down is
  what it should mean anyway.

**One more measured fact, about the wire rather than the map.** An Enfusion
RPC does not carry its sender. Inside an `[RplRpc(..., RplRcver.Server)]`
handler, `GetGame().GetPlayerController()` is the SERVER'S controller, which
on a dedicated server is null. Every stroke was therefore owned by player -1
and "rub out mine" would have rubbed out everybody's. The caller sends its
own player id as an argument instead.
