# Research: Editor Attributes / "Scenario properties" panel

**Status:** PoC implemented, engine-level verified, AND confirmed working in-game via screenshots
from a live Game Master session. Radius-visualization follow-up (debug sphere) implemented and
compiled clean. Remaining: user's own functional test of "Trigger Once" behavior, and rollout to
the remaining prefabs.
**Date:** 2026-09-09 (follow-up session to the Game Master visibility fix, see `HANDOVER.md`)

## Summary

The "Edit" panel in Game Master (where vanilla content shows configurable fields like "Set
faction") is a **completely separate system** from the Placeable Registry we fixed yesterday. Our
prefabs show "No properties" simply because no Editor Attribute object exists that recognizes our
`MCF_*Component` classes — nothing is broken. The plain `[Attribute(...)]` tags on our script
components (visible in Workbench's Object Properties when editing the `.et` file) are a
**different mechanism**: that's prefab-authoring-time serialization, never read by this in-game
panel.

## How the system works

### 1. One flat, global attribute list per editor mode

`SCR_AttributesManagerEditorComponentClass` (component on `EditorModeEdit.et`, our override) has:

```
[Attribute(category: "Attributes")]
protected ref array<ref SCR_EditorAttributeList> m_AttributeLists;
```

Vanilla's `EditorModeEdit.et` has exactly one entry here:

```
SCR_AttributesManagerEditorComponent "{54C8DACDCD4AE14E}" {
 m_AttributeLists {
  SCR_EditorAttributeList "{5606E7C45FE427EF}" : "{F3D6C6D25642352C}Configs/Editor/AttributeLists/Edit.conf" {
  }
 }
 m_MenuPreset EditorAttributesDialog
}
```

`Configs/Editor/AttributeLists/Edit.conf` holds **85 attribute instances** in a flat array
(`SCR_EditorAttributeList.m_aAttributes`) — no per-prefab configuration at all. Each attribute is
its own class inheriting from `SCR_BaseEditorAttribute`.

### 2. Filtering happens per attribute, not per prefab

When a player selects a placed item and chooses "Edit",
`SCR_AttributesManagerEditorComponent.GetVariables()` loops through **every** attribute in the
list and calls:

```
SCR_BaseEditorAttributeVar var = attribute.ReadVariable(item, this);
```

where `item` is the `SCR_EditableEntityComponent` of the selected item (or the game mode instance
itself, for global/scenario attributes). If `ReadVariable` returns `null`, that attribute simply
isn't shown for this item. If it returns a value, the attribute appears in the panel. **This is
exactly why our items show nothing: no attribute in `Edit.conf` recognizes an `MCF_*Component`.**

Confirming evidence from `SCR_BaseFactionEditableAttribute.ReadVariable`:
```c
SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
if (!editableEntity) return null;
if (!ValidEntity(editableEntity.GetOwner())) return null;   // per-item filter
...
```

### 3. The contract: `SCR_BaseEditorAttribute`

Each attribute is its own class inheriting from `SCR_BaseEditorAttribute` (file:
`scripts/Game/Editor/Containers/Attributes/SCR_BaseEditorAttribute.c`), implementing at minimum:

```c
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class MCF_XxxEditorAttribute : SCR_BaseEditorAttribute
{
    override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
    {
        // cast item, validate, return null if not applicable
        // otherwise: return SCR_BaseEditorAttributeVar.CreateFloat/CreateBool/CreateInt(...)
    }

    override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
    {
        // cast item, write the value back onto the component
    }
}
```

Base class fields (from `SCR_BaseEditorAttribute`):
- `m_UIInfo` — `SCR_EditorAttributeUIInfo` (Name, Description, optional Icon)
- `m_bIsServer` (default `"1"` = true) — whether this attribute is handled server-authoritatively
  (RPC round-trip via `StartEditingServer`/`ConfirmEditingServer`) or locally on the editor
  owner's machine (`m_bIsServer 0`). See "Server vs. local" below.
- `m_CategoryConfig` — path to an `SCR_EditorAttributeCategory` config (groups attributes
  visually in the panel, e.g. `Configs/Editor/AttributeCategories/Entity.conf`)
- `m_Layout` — UI widget layout (see "Layouts" below)

`SCR_BaseEditorAttributeVar` (file `AttributeVariables/SCR_BaseEditorAttributeVar.c`) is a simple
value wrapper (everything internally stored as a `vector`) with static factory methods:
`CreateInt(int)`, `CreateFloat(float)`, `CreateBool(bool)`, `CreateVector(vector)`, and matching
`GetInt()/GetFloat()/GetBool()/GetVector()`.

### 4. The proven pattern for entity-scoped, component-based attributes

Vanilla's explosive-charge attributes are the perfect analog for our situation (a custom script
component on a placed item, not a built-in game system):

```c
// scripts/Game/Editor/Containers/Attributes/SCR_ExplosiveFuzeArmingAttribute.c
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_ExplosiveFuzeArmingAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		SCR_ExplosiveChargeComponent explosiveComp = SCR_ExplosiveChargeComponent.Cast(owner.FindComponent(SCR_ExplosiveChargeComponent));
		if (!explosiveComp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(explosiveComp.GetUsedFuzeType() != SCR_EFuzeType.NONE);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		IEntity owner = editableEntity.GetOwner();
		if (!owner) return;
		SCR_ExplosiveChargeComponent explosiveComp = SCR_ExplosiveChargeComponent.Cast(owner.FindComponent(SCR_ExplosiveChargeComponent));
		if (!explosiveComp) return;

		if (var.GetBool())
			explosiveComp.ArmWithTimedFuze(true); // etc.
		else
			explosiveComp.DisarmChargeSilent();
	}
}
```

The float variant (`SCR_ExplosiveFuzeTimerAttribute`) is identical in shape but uses
`CreateFloat`/`GetFloat()` and inherits from `SCR_BaseValueListEditorAttribute` (for the slider
config, see below) instead of `SCR_BaseEditorAttribute` directly.

**Translated to MCF:** for `MCF_Obj_ProximityTriggerComponent.m_fRadius` this becomes:

```c
SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
if (!editableEntity) return null;
IEntity owner = editableEntity.GetOwner();
if (!owner) return null;
MCF_Obj_ProximityTriggerComponent comp = MCF_Obj_ProximityTriggerComponent.Cast(owner.FindComponent(MCF_Obj_ProximityTriggerComponent));
if (!comp) return null;
return SCR_BaseEditorAttributeVar.CreateFloat(comp.GetRadius());
```

**Implementation detail:** our MCF components had their `[Attribute(...)]` fields as `protected`
with no public getter/setter (e.g. `protected float m_fRadius;`). Every field made editable
through the Edit panel needs a public `GetXxx()`/`SetXxx()` on the component itself — just like
vanilla's `explosiveComp.SetFuzeTime(...)`. Small, low-risk addition per component (done for
`m_fRadius`/`m_bTriggerOnce` on `MCF_Obj_ProximityTriggerComponent` as part of this PoC).

### 5. Layouts (UI widgets)

Available under `UI/layouts/Editor/Attributes/AttributePrefabs/`:

| Layout | Use | Attribute base class |
|---|---|---|
| `AttributePrefab_Checkbox.layout` | bool | `SCR_BaseEditorAttribute` directly, `CreateBool` |
| `AttributePrefab_Slider.layout` | float with min/max/step | `SCR_BaseValueListEditorAttribute` (has `m_baseValues SCR_EditorAttributeBaseValues` with `m_fMin/m_fMax/m_fStep/m_iDecimals/m_sSliderValueFormating`) |
| `AttributePrefab_Spinbox.layout` | numeric stepper / fixed value list | often `SCR_BaseFloatValueHolderEditorAttribute` (`m_aValues` array of `SCR_EditorAttributeFloatStringValueHolder`, each with Name + float value) |
| `AttributePrefab_Dropdown.layout` / `AttributePrefab_DropdownWithParam.layout` | choice list | same idea, dropdown variant |
| `AttributePrefab_ButtonBox_Selection.layout` / `_MultiSelection.layout` | button row (e.g. faction picker) | `SCR_BasePresetsEditorAttribute` / `SCR_BaseMultiSelectPresetsEditorAttribute` |
| `AttributePrefab_SliderVector.layout` | vector slider | — |

For MCF's fields, **Checkbox** (bools like `m_bTriggerOnce`) and **Slider** (floats like
`m_fRadius`) cover most cases. There's no ready-made "free text" layout in this list — vanilla
solves text fields like task names with dedicated custom layouts under
`AttributePrefabs/Custom/` per specific text field. For `m_sTriggeredEvent` (event name as a
string) further research is needed to reuse an existing custom text layout or build our own —
this is the one place where the "copy the vanilla pattern" recipe doesn't have a direct 1:1
example.

### 6. Categories

`m_CategoryConfig` points to an `SCR_EditorAttributeCategory` config
(`Configs/Editor/AttributeCategories/*.conf`, 22 of them, each simply an `SCR_UIInfo` with Name +
Icon). We can reuse a generic existing category (`Entity.conf` is the neutral choice, used by
e.g. `SCR_EntityFactionEditorAttribute`, `SetBuildingProgressEditorAttribute`,
`SetPropBaseBudgetEditorAttribute`) — or, as a nicer option, create our own
`Configs/Editor/AttributeCategories/MCF.conf` so our attributes group under their own "MCF"
heading in the panel instead of blending into vanilla entries.

### 7. Server vs. local (`m_bIsServer`)

- Vanilla's explosive-fuze attributes leave `m_bIsServer` at its default `1` (not explicitly
  overridden in Edit.conf) → server-authoritative: opening the panel first sends an RPC to the
  server (`StartEditingServer`) which fetches the current value and sends it back; confirming
  sends the new value via `ConfirmEditingServer` to the server, which runs `WriteVariable` there.
- With `m_bIsServer 0`, `ReadVariable`/`WriteVariable` run entirely locally on the editor owner's
  machine (`ConfirmEditing()`'s "apply locally" branch) — no RPC plumbing needed, but the change
  then only lands on that machine, not automatically on the server/other clients, unless the
  underlying component logic already replicates.

**Recommendation for MCF:** use `m_bIsServer 0` for the first implementation — no RPC plumbing
needed, quick to verify, and Game Master is in practice usually operated by the host/admin. For
fields that affect live, authoritative gameplay behavior in a dedicated-server setup with
multiple GMs (such as `m_fRadius`, read every tick in `OnTickCritical`), `m_bIsServer 1` is the
safer choice long-term, provided the value is re-read correctly after being changed (already the
case for `m_fRadius`, since the component reads the field directly instead of caching it).

### 8. Registration: own `.conf` + `.meta`, same recipe as the Placeable Registry

Exactly the same approach as yesterday's `MCF_PlaceableEntities.conf`:

**`Configs/Editor/MCF_EditorAttributes.conf`** (root type `SCR_EditorAttributeList`):
```
SCR_EditorAttributeList {
 m_aAttributes {
  MCF_ProximityRadiusEditorAttribute "{<GUID>}" {
   m_UIInfo SCR_EditorAttributeUIInfo "{<GUID>}" {
    Name "Detection Radius"
    Description "Detection radius in metres."
   }
   m_bIsServer 0
   m_CategoryConfig "{<GUID>}Configs/Editor/AttributeCategories/Entity.conf"
   m_Layout "{680E4985E42137FB}UI/layouts/Editor/Attributes/AttributePrefabs/AttributePrefab_Slider.layout"
   m_baseValues SCR_EditorAttributeBaseValues "{<GUID>}" {
    m_fMin 5
    m_fMax 500
    m_fStep 5
    m_iDecimals 0
   }
  }
 }
}
```

**`Configs/Editor/MCF_EditorAttributes.conf.meta`** — identical template to
`MCF_PlaceableEntities.conf.meta`, GUID must match the `Name` reference:
```
MetaFileClass {
 Name "{<SAME GUID AS ABOVE>}Configs/Editor/MCF_EditorAttributes.conf"
 Configurations {
  CONFResourceClass PC {
  }
  CONFResourceClass XBOX_ONE : PC {
  }
  CONFResourceClass XBOX_SERIES : PC {
  }
  CONFResourceClass PS4 : PC {
  }
  CONFResourceClass PS5 : PC {
  }
  CONFResourceClass HEADLESS : PC {
  }
 }
}
```

**Wiring in our `Prefabs/Editor/Modes/EditorModeEdit.et` override:** add a second entry to
`SCR_AttributesManagerEditorComponent.m_AttributeLists`, next to the existing vanilla
`Edit.conf` reference:
```
SCR_AttributesManagerEditorComponent "{54C8DACDCD4AE14E}" {
 m_AttributeLists {
  SCR_EditorAttributeList "{5606E7C45FE427EF}" : "{F3D6C6D25642352C}Configs/Editor/AttributeLists/Edit.conf" {
  }
  SCR_EditorAttributeList "{<new GUID>}" : "{<GUID from MCF_EditorAttributes.conf>}Configs/Editor/MCF_EditorAttributes.conf" {
  }
 }
 m_MenuPreset EditorAttributesDialog
}
```
(Exactly like yesterday's own `SCR_PlaceableEntitiesRegistry` entry sitting next to the vanilla
registries in `m_Registries`.)

### 9. Game-Master-only radius visualization (debug sphere)

Added to `MCF_Obj_ProximityTriggerComponent` in response to the user request: while Game Master
is open, draw a wireframe sphere at the trigger's own radius every frame, so mission makers can
actually see the area they're configuring instead of only reading a number in the panel.

Implementation:
- `SetEventMask(owner, EntityEvent.FRAME);` added in `EOnInit`.
- New `override void EOnFrame(IEntity owner, float timeSlice)`:
  ```c
  override void EOnFrame(IEntity owner, float timeSlice)
  {
      if (!SCR_EditorManagerEntity.IsOpenedInstance())
          return;

      Shape.CreateSphere(DEBUG_SPHERE_COLOR, ShapeFlags.WIREFRAME | ShapeFlags.ONCE, owner.GetOrigin(), m_fRadius);
  }
  ```
- Gated on `SCR_EditorManagerEntity.IsOpenedInstance(bool includeLimited = true)` (static, file
  `scripts/Game/Editor/Entities/SCR_EditorManagerEntity.c`), the simple/correct check for "is the
  local Game Master editor currently open." Never draws outside Game Master.
- `Shape.CreateSphere` (`scripts/Core/generated/Debug/Shape.c`) renders **locally only** — never
  networked — and `ShapeFlags.ONCE` means each call auto-destroys after being drawn, so there's no
  shape handle to track or clean up.
- Because the component reads `m_fRadius` directly (not a cached copy) both here and in
  `OnTickCritical`, moving the Editor Attribute slider changes the sphere's size on the next
  frame automatically — no extra wiring needed between the attribute and the visual.

## Open uncertainties for later

1. **Free-text attributes** (e.g. `m_sTriggeredEvent`, event names): no direct
   `AttributePrefab_*.layout` for a plain string input box in the standard list; vanilla solves
   this with custom layouts under `AttributePrefabs/Custom/` per specific task text field.
   Further research needed, or expose bool/float fields first (covers a large part of MCF's
   components already: `m_fRadius`, `m_bTriggerOnce`, etc.) and text fields in a later
   iteration.
2. **Getter/setter additions:** every MCF component that gets an editable field needs a public
   accessor (usually didn't exist — fields were `protected` with no getter/setter). Small,
   low-risk change per component, but a concrete action item for each of the remaining
   components.
3. **Duplicate-attribute check:** `GetIsAttributeDuplicate()` in
   `SCR_AttributesManagerEditorComponent` disallows two active attributes of the same type
   unless they inherit `SCR_BaseDuplicatableEditorAttribute`. For MCF this means: **one
   attribute class per field, per component type** (so one class for
   `MCF_Obj_ProximityTriggerComponent.m_fRadius`, a separate one for
   `MCF_Obj_AlarmTriggerComponent`'s own radius field, etc.) — no generic "FloatFieldAttribute"
   that works across multiple component types via reflection; that pattern doesn't exist here,
   every attribute is hard-linked to one component type via casting in `ReadVariable`.
4. **Test method:** exact same proven workflow as the Placeable Registry fix: take a working
   vanilla example (`SCR_ExplosiveFuzeArmingAttribute`/`...TimerAttribute`, applied to
   `E_ExplosiveCharge` prefabs), duplicate with `game_duplicate`, inspect with `prefab`
   (`include_raw: true`) to see exactly how that prefab configures its
   `SCR_ExplosiveChargeComponent`, and compare 1:1.

## Implementation progress (this session)

1. ✅ Added `GetRadius()`/`SetRadius()`/`GetTriggerOnce()`/`SetTriggerOnce()` to
   `MCF_Obj_ProximityTriggerComponent`.
2. ✅ `Scripts/Game/Editor/MCF_ProximityRadiusEditorAttribute.c` (float/slider, mirrors
   `SCR_ExplosiveFuzeTimerAttribute`) and `Scripts/Game/Editor/MCF_ProximityTriggerOnceEditorAttribute.c`
   (bool/checkbox, mirrors `SCR_ExplosiveFuzeArmingAttribute`) — both structurally validated with
   `script_analyze` (clean: 1 class, 3 methods each, correct overrides).
3. ✅ `Configs/Editor/MCF_EditorAttributes.conf` + `.meta` created (GUID `{D1297B1F0C2377E2}`),
   containing both attributes. `wb_resources getInfo` against the live Workbench session confirms
   the file parses correctly and both vanilla references (`m_CategoryConfig` → `Entity.conf`,
   `m_Layout` → the Slider/Checkbox layouts) resolve fine — the earlier `find_broken_refs` hits on
   these same GUIDs were a false positive from the MCP's own lightweight resource index (which
   only tracks 19 resources total, not the ~85k vanilla game files); the live Workbench resolves
   them correctly. Confirmed this is a pre-existing index limitation, not new: even yesterday's
   already-proven-working `MCF_PlaceableEntities.conf` GUID doesn't resolve via `resolve_guid`
   either.
4. ✅ Wired into `EditorModeEdit.et`'s `m_AttributeLists`, next to vanilla's `Edit.conf` entry.
5. ✅ **Full Workbench restart done** (user closed the app, session re-launched via
   `wb_launch` with `addon.gproj` + `MCFTestworld.ent`). The fresh session's script compile log
   shows zero errors for either new attribute class (no more "Unknown class"). `wb_resources
   getInfo` on `MCF_EditorAttributes.conf` now returns both attributes correctly parsed. Most
   importantly, `wb_resources getInfo` on `EditorModeEdit.et` (which returns Workbench's fully
   merged/resolved view of the prefab, the same view the running game actually uses) confirms
   `SCR_AttributesManagerEditorComponent.m_AttributeLists` now holds **two** lists: vanilla's 85
   attributes from `Edit.conf`, followed immediately by our `MCF_ProximityRadiusEditorAttribute`
   and `MCF_ProximityTriggerOnceEditorAttribute`, both with correctly resolved `m_UIInfo`,
   `m_CategoryConfig`, `m_Layout`, and (for the radius one) `m_baseValues`. This is engine-level
   confirmation, not just our own file content — Workbench itself parsed and merged it exactly as
   the vanilla registry fix was confirmed yesterday.
6. ✅ **Interactive in-game check: confirmed by the user.** Screenshots from a live Game Master
   session show the "Editing: Scenario properties" panel correctly rendering both "Detection
   Radius" (50m, slider) and "Trigger Once" (Yes, toggle) with the configured description text.
   The Editor Attributes PoC is confirmed end-to-end, not just at the config-parsing level.
7. ✅ **Radius visualization (Game-Master-only debug sphere), implemented and compile-verified.**
   See section 9 above for the implementation. Verification done this session:
   - `script_analyze`: 2 classes, 11 methods, 9 fields — clean structural parse.
   - `script_lint`: 0 errors, 0 warnings (9 pre-existing style infos, consistent with the rest of
     the file).
   - Live compile confirmation: after the code change, a `wb_resources rebuild` on the addon
     triggered Workbench's own internal `Game destroyed` → full script recompile → world reload
     cycle (console.log, session `logs_2026-09-09_04-32-32`). The recompile log shows **"Module:
     Game; loaded 5702x files; 11171x classes"** with **zero (E) script errors** — only the
     project's existing, unrelated "obsolete API" warnings (`PlayerData`, `GetDSSession`,
     `GetPlayerPlatformKind`, etc., none touching MCF files). `error.log` and `script.log` for
     this session contain **no** error lines referencing `MCF_Obj_ProximityTriggerComponent`,
     `Shape`, `EOnFrame`, or `SCR_EditorManagerEntity`. This confirms the new code compiles
     cleanly.
   - Not yet confirmed: the actual **visual** result (does the sphere render, at the right size,
     only while Game Master is open) — that needs an in-game look, same as the base panel did.
   - Side note on tooling: during this `wb_resources rebuild` call, Workbench's own process hit an
     internal engine assertion ("Assertion failed ... Resources are leaking! Check log!",
     `crash.log` in the same session) around the same timestamp the MCP call timed out
     client-side and briefly reported "Workbench: disconnected". The process recovered on its own
     (confirmed via `wb_state` immediately after: normal edit mode, 173232 entities) and the
     recompile/reload described above completed successfully afterwards. Noting this as a known
     side effect of `wb_resources rebuild` on this addon — consistent with HANDOVER.md's existing
     note that this call can time out client-side while the underlying operation keeps running;
     new detail is that it can also trip a resource-leak assertion in Workbench itself, which the
     app appears to auto-recover from.
8. ℹ️ **Pre-existing WORLD-loader parse errors (PLACEABLE/HAS_AREA) — new data point.** The known
   issue (`Cannot parse integer value` / `Unknown keyword/data 'PLACEABLE'` / `'HAS_AREA'` when
   `MCFTestworld.ent` loads an already-placed `MCF_Obj_ProximityTrigger` instance) recurred again
   during this session's earlier load passes (04:32 and 04:35), plus a wider cluster of similar
   `WORLD (E)` parse errors on other data (`m_sWorldFile`, `m_bIsModded`, `SlidingTrackMaterial`,
   `Parent`, `m_UIInfo`, `RplComponent`, `Hierarchy`, `Unexpected end of scope`) — none of these
   reference any MCF class or file by name, so they read as the same class of "raw instance data
   parsed by an older/different parser than the Resource Database uses" issue, just triggered by
   more content this session (the user placed two new `MCF_Obj_ProximityTrigger` instances while
   testing, at 04:35:20 and 04:37:34).
   **New finding:** after the `wb_resources rebuild`-triggered full reload at ~04:40 (fresh
   `Game destroyed` → recreated → `Entities load '$MCF:worlds/arland/MCFTestworld.ent'` →
   `CreateEntities`), **none of these WORLD parse errors recurred** — that reload's own
   `CreateEntities` pass only logged the separate, unrelated, already-known `SCR_MapEntity`
   duplicate warning ("Multiple map entities present!", an Arland map quirk with nothing to do
   with MCF). This suggests the parse errors are not a permanent corruption of the saved world
   data, but something tied to specific mid-session states (possibly stale in-memory instance
   data from placing/duplicating entities live, cleared by a full reload) — still not conclusively
   diagnosed, and still not caused by anything edited this session (the `.et` prefab file itself
   was never touched). Worth a dedicated, separate investigation later; not blocking.
9. ⏳ **Outstanding, explicitly the user's own task:** functional test of "Trigger Once" — does
   the trigger actually fire once and then stay silent (vs. re-firing on repeated entry) in a
   live test. This needs an entity to actually walk in and out of the radius, which only the user
   can drive interactively.
10. ⏳ Roll out to remaining prefabs/fields after the PoC is fully confirmed working in-game.
