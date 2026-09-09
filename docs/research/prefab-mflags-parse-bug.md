# Prefab `m_Flags` parse bug (root cause of missing Game Master entries)

Status: **RESOLVED** — all 12 MCF prefabs fixed and verified.

## Symptom

Only 4 of 12 MCF prefabs appeared in the Game Master placement browser.
Searching for "mcf" in the in-game search box returned nothing, although the
"System" category tree did list MCF entries.

`wb_resources getInfo` on the 8 affected prefabs returned:

- `SCR_EditableEntityComponent.m_UIInfo.Name` as an empty string, even though
  the raw `.et` file clearly contained the correct `Name "..."` value.
- Only 2 components in the entity, with `RplComponent` and `Hierarchy`
  missing entirely — both from the resource database *and* from live
  entities instantiated from those prefabs.

## Root cause

The prefabs authored `m_Flags` as a braced list of named flags:

```
m_Flags {
 PLACEABLE
 VIRTUAL
 HAS_AREA
}
```

The engine's entity-template parser expects `m_Flags` to be an **integer
bitmask**, not a braced block of names. A fresh parse produces this exact
cascade (from `console.log` on a clean Workbench boot):

```
WORLD (E): Cannot parse integer value at offset 286(0x11e)      <- m_Flags {
WORLD (E): Unknown keyword/data 'PLACEABLE' at offset 301(0x12d)
WORLD (E): Unknown class 'm_UIInfo' at offset 375(0x177)
WORLD (E): Unknown keyword/data 'RplComponent' at offset 477(0x1dd)
WORLD (E): Unknown keyword/data 'Hierarchy' at offset 516(0x204)
WORLD (E): Unexpected end of scope at offset 546(0x222)
```

The parser fails on `m_Flags`, loses brace synchronisation, and silently
discards everything after it — the UI name, `RplComponent` and `Hierarchy`.
Because Game Master's placement browser and its search box both match on
`m_UIInfo.Name`, entities with a blank name never surfaced.

Note that `status` stays `0` and `message` stays empty in the `getInfo`
response, so the failure is completely invisible unless you read the log.

## Fix

Remove the `m_Flags { ... }` block entirely. The flags are already provided
by the shared base template
`{996046FE206C699A}Prefabs/Editor/Components/Default_SCR_EditableEntityComponent.ct`,
which resolves to `m_Flags: 769`. After removal, `getInfo` reports the
correct `Name`, all four components, and `m_Flags: 769`.

Because the braced block never parsed in the first place, those flags were
never actually applied to any MCF prefab — including the four that appeared
to work. Removing the block therefore changes no effective behaviour; it
only stops the parse from breaking. `HAS_AREA` was likewise never in effect.

## Why 4 prefabs appeared to work

Their resource-database entries were cached in a good state from an earlier
point. They were not genuinely parsing correctly — a fresh parse of those
files produced the same failure. This is why the problem looked inconsistent
and resisted diagnosis for so long.

## Operational findings (important for future debugging)

- **`wb_resources rebuild` does not re-read `.et` entity data.** Proven with
  a marker test: the file on disk was changed to a different component class,
  and after `rebuild` the tool still reported the *old* class. Only a full
  Workbench restart re-parses entity templates. The `.meta` file *is* re-read
  (so `resourceName` updates), which makes `rebuild` look like it worked.
- **`wb_resources register` and `wb_prefabs createTemplate` hang the NET API**
  (10s timeout, "Workbench: disconnected") when creating a new resource entry.
  They appear to trigger a native modal dialog that cannot be dismissed
  headlessly. Avoid both; they require killing and relaunching Workbench.
- **An addon-wide `wb_resources rebuild` takes longer than the 10s MCP call
  timeout.** The timeout is not a crash — the rebuild continues in the
  background and Workbench responds again after roughly a minute.
- **Never call `wb_resources rebuild` while `wb_state` reports `Mode: game`.**
  Doing so breaks `SCR_PlacingEditorComponent` and the placement browser.
  Confirm `Mode: edit` first.
- Log-driven diagnosis beats inspection here: `getInfo` reports success on a
  broken parse, and the direct file-parsing tools (`prefab action=inspect`)
  read the raw text and therefore always look correct. Only
  `logs_filter` on the console channel with a pattern like
  `Unknown class|Unknown keyword|Cannot parse|Unexpected end` exposes the
  real failure.

## Verification

Clean Workbench boot after the fix: zero matches for
`Unknown class|Unknown keyword|Cannot parse|Unexpected end` in the console
log, and all 12 prefabs report their correct `Name`, four components, and
`m_Flags: 769`.

The same parse errors previously attributed to a "pre-existing world loader
bug" in `MCFTestworld.ent` were the same root cause and are now also gone.
