# What is on a device, and who decides

Status: **design**. Nothing here is built. Written 2026-09-10, after the phone
shell became something worth putting real content into.

---

## 1. The problem this solves

A phone's content lives in `m_aEntries` on `MCF_Intel_CarrierComponent`, as
attributes on the prefab. That was right when a phone had three messages. It
stops working the moment a Game Master wants to change it:

- **Editor attributes carry twelve bytes and cannot hold text.** The same wall
  the dialogue system hit. A Game Master can move a slider; they cannot type a
  message into a prefab attribute at runtime.
- **One prefab is behind every phone.** Editing the prefab's entries edits every
  phone in the mission at once, which is never what anybody means.
- **Which apps exist is decided in code.** `CollectApps` lists a fixed set. A
  mission where the phone has no mail client, or has a banking app, cannot be
  expressed at all.

The dialogue system solved exactly this problem for conversations, and the
solution transfers unchanged.

## 2. The shape, copied from conversations

| Conversations | Devices | What it is |
|---|---|---|
| `MCF_Dialogue_Conversation` | `MCF_Device_Profile` | One authored thing, with an id |
| `MCF_Dialogue_Node` | `MCF_Device_App` | A part of it |
| `MCF_Dialogue_Choice` | `MCF_Device_Item` | A leaf |
| `MCF_Conversations.conf` | `MCF_DeviceProfiles.conf` | What ships with the mod |
| `MCF_Dialogue_Library` | `MCF_Device_Library` | Config + runtime, persisted |
| `MCF_Dialogue_Script` | `MCF_Device_Script` | Text serialisation |
| `MCF_Dialogue_AssignComponent` | `m_sProfileId` on the carrier | Which one this object uses |
| `MCF_Dialogue_EditorMenu` | `MCF_Device_EditorMenu` | Where a Game Master writes it |

**A profile is assigned by id, not embedded.** One "smuggler's phone" profile
can be given to any phone, in any mission, by anybody -- and two phones sharing
a profile is a deliberate thing a mission maker can now do, rather than an
accident of prefab editing.

**Runtime shadows shipped.** A profile a Game Master writes this session with
the same id as a shipped one replaces it for this mission and leaves the mod's
copy alone. Same rule, same reason.

## 3. The data

```
MCF_Device_Profile
  m_sId              "smuggler_phone"
  m_sDeviceName      "Mobile phone"        shown at the top and in the prompt
  m_eShell           PHONE | LAPTOP        which skin draws it
  m_aApps            the apps on it, in the order they appear

MCF_Device_App
  m_sId              "messages"            unique within the profile
  m_sLabel           "MESSAGES"            what is under the tile
  m_eKind            MESSAGES | CALLS | ...  what it is, for icon and empty text
  m_sEmptyText       "No messages."        optional; the kind has a default
  m_aItems           what is in it

MCF_Device_Item
  m_sHeading         "M. - 02:14"
  m_sTimestamp       "14 MAR"
  m_sBody            the text
```

**Why an app has both a kind and a label.** The kind is what the software knows
-- which empty line to write, which icon to use once there are icons, whether
the shell should format it as a call log rather than a list. The label is what
the mission maker wants on the screen, which might be in another language, or
might be "WHATSAPP" on a phone where that matters. Fusing them would mean
adding an enum value to add a differently-named app.

**Why `m_eShell` is on the profile and not only on the carrier.** A laptop
profile put on a phone is a mistake worth catching, and it also means one
profile carries everything needed to draw the device. `MCF_EIntelView` on the
carrier still wins if they disagree -- the object in the world knows what it
physically is -- but the profile can say what it was written for.

## 4. What happens to `m_aEntries`

It stays, and it is the fallback.

A carrier with a profile id uses the profile. A carrier without one uses its own
entries, exactly as today, through a synthesised single-app profile. That means:

- every existing prefab keeps working with no edit,
- the shell has ONE code path -- it always reads a profile,
- and the migration is a mission maker's choice rather than a flag day.

`m_eApp` on `MCF_Intel_Entry` becomes what it always was underneath: the way an
entry says which app it belongs to when there is no profile to say it.

## 5. What the shell has to stop doing

`MCF_Intel_ShellMenu` is 900 lines and does five jobs. Before the profile system
lands, it gets split, because adding a sixth job to it is how it becomes
unmaintainable:

| Piece | What it owns |
|---|---|
| `MCF_Intel_ShellMenu` | The menu: opening, closing, the three screens, input |
| `MCF_Device_Layout` | Geometry -- the preview box, the glass, the text width caps |
| `MCF_Device_Presenter` | Turning a profile into what the screens show |
| `MCF_Intel_ShellMenu` (page mode) | Paper and notepad, which have no apps |

The geometry piece is the one that matters most: it is the part that took a full
day and five silent failures to get right, and it has nothing to do with intel.
A laptop shell will want it verbatim.

## 6. How the laptop reuses this

Nothing in the list above says "phone". The laptop needs:

- a profile with `m_eShell = LAPTOP` and apps that read as folders,
- its own layout (a wide screen, a sidebar instead of a tile grid),
- the same `MCF_Device_Layout` with a different model and a different aspect,
- nothing else.

The one thing to get right now, so the laptop is cheap later: **the presenter
must not know which layout it is feeding.** It answers "what apps are there",
"what is in this app", "what does this item say" -- and a wide layout asks the
same questions a narrow one does.

## 7. The Game Master's screen

Copy `MCF_Dialogue_EditorMenu`, which already solves the hard parts: a list of
profiles, an editor for one, save-as-new with a unique id, delete refused on
shipped content, and everything routed through `SCR_PlayerController` so the
server owns the data.

Reached the same way too -- a right-click context action on the device, beside
the intel editor that exists now.

Two things it must do that the dialogue editor does not:

- **Add and remove apps**, not just their content. That is the request.
- **Reorder them**, because the order is the home screen.

## 8. Persistence

`MCF_Core_PersistentStore`, the same keys pattern:

```
deviceProfiles          "smuggler_phone,mill_laptop"
deviceProfile.smuggler_phone   <serialised>
```

Which means a phone written during a mission is still written after a server
restart -- the thing the whole persistent-server design exists for, and which
conversations already prove works.

## 9. Open

- Whether `MCF_EIntelApp` survives at all, or whether `m_eKind` on the app
  replaces it. Leaning towards replacing it, but the enum is in the wire format
  and the store, so it cannot simply be deleted.
- Icons per app kind. Needs one small texture and an import; nothing else is
  blocking it.
- Whether a Game Master can write a profile onto a device that has none, or
  only pick from existing profiles. The first is more useful and more work.
