# Open design question: placeable entities vs. attached conditions

Status: **open** — deliberately deferred. Not blocking; raised while getting
all 12 prefabs visible in Game Master.

## The concern

Everything currently gets a prefab with `SCR_EditableEntityComponent`, which
makes it show up in the Game Master placement browser. That was the right
call to get the building blocks usable, but not everything we build is
actually a *thing you place in the world*. Some are conditions or behaviour
that should attach to a carrier — usually an AI unit or the player.

If we keep making everything placeable, the Game Master catalog turns into a
list of nodes where most entries make no sense to drop on the terrain.

## The discriminator

Not "spatial vs. logic". The useful question is:

> Does this node depend on its owner's transform, and does that transform
> need to be alive and changing?

That splits the current 12 into three groups.

### 1. Standalone spatial — position is the whole story, static is correct

- MCF Trigger Zone
- MCF Proximity Trigger
- MCF Objective Node
- MCF Lifestyle POI
- MCF Safe Fallback Point
- MCF Observation Node

Proximity is the clean example: the check is radially symmetric, so it does
not care about facing. A fixed position fully specifies it.

### 2. Carrier-bound — reads the owner's facing or state

- MCF Cone Detection Trigger
- MCF Spotted By Player

`MCF_Obj_ConeDetectionTriggerComponent.OnTickCritical()` calls
`m_Owner.GetTransformAxis(2)` every tick, so the cone direction *is* the
owner entity's forward axis. Placed standalone, you get a cone frozen in
whatever direction it happened to be dropped. That is legitimate for a fixed
sentry position, a camera or a firing port — but it is the narrow case, not
the main one. The primary use is bolted onto something that turns.

Note that watch-registration is *not* the discriminator: Proximity also has
`RegisterWatchedEntity` and hooks the auto-watcher registry. Transform
dependency is what separates these.

### 3. Pure logic — transform is meaningless

- MCF Alarm Trigger (event relay)
- MCF Logic Node (event logic)
- MCF Recipe (behaviour sequence)
- MCF Text Line (attaches to a speaker)

## Constraint: Game Master cannot attach components at runtime

Game Master can place entities and edit exposed attributes on them, but it
cannot rewrite an existing character's component list. Components are fixed
at instantiation. So "let the mission maker add this component to the player
controller from GM" is not available as a design option.

### This is already solved, by inverting the reference

`MCF_Core_AutoWatcherRegistry` sidesteps the constraint entirely. Nodes
register *themselves* into the registry on init, and
`MCF_Core_GameModeComponent.OnControllableSpawned()` then registers every
newly spawned controllable **into those nodes**:

- `MCF_Obj_ProximityTriggerComponent.RegisterWatchedEntity(entity)`
- `MCF_Obj_SpottedByPlayerComponent.RegisterWatcher(entity)`

The player never receives a component. The node holds the reference to the
player, not the other way around. Nothing has to be baked into character
prefabs.

### What this implies for Cone Detection

Cone Detection is the one that does not fit the pattern yet, because it reads
its *own* forward axis rather than a referenced carrier's. Making the carrier
a reference — defaulting to the owner when unset — would give both behaviours
from one component:

- no carrier set → uses its own transform (static sentry, today's behaviour)
- carrier registered → follows that entity's facing

That is a small contained change inside one component, not a prefab or
catalog restructure.

### The one thing that genuinely must be pre-built

Not the character prefabs — the **game mode**. `MCF_Core_GameModeComponent`
owns the `OnControllableSpawned` hook that feeds the registry, so a mission
must use MCF's game mode for any of this to work. That is a single baked
dependency instead of one per unit.

### Where mission settings fit

Mission-level toggles belong to *global policy*, not per-node wiring. There is
already one waiting: `MCF_Core_AutoWatcherRegistry` currently registers all
controllables, so AI counts as a watcher too — its own header calls this "a
known simplification, not a confirmed player-only filter". Player-only vs.
all-controllables is exactly a mission setting.

## The tension to resolve later

Groups 2 and 3 still need to be configurable by a mission maker inside Game
Master without writing script. Making them placeable is the crude way to get
that. Options worth weighing:

- Keep them placeable but in a clearly separate category, so the group 1
  building blocks stay clean.
- Expose them as components a mission maker adds to an existing entity,
  driven through Editor Attributes rather than the placement browser.
- One placeable "logic container" entity holding several group 3 nodes as
  child configuration, instead of one placeable per node.

Worth keeping in mind: the component is the real unit of reuse, and the
prefab is only one delivery wrapper around it. Group 2 in particular could
reasonably ship both ways — attachable as a component for the normal case,
and placeable for the static-sentry case — without duplicating logic.

No decision made. Revisit before the node count grows much further; the cost
of changing this rises with every prefab we add.
