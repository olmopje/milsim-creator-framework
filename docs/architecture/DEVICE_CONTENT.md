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

---

## 10. Authoring inside the phone, and what "unread" means

Proposed 2026-09-10, **built 2026-09-11**. What follows is what the code does,
not what was hoped for; where the plan and the build differ, the build is
described and the reason is given.

### 10.1 The Game Master edits the device on the device

There is no separate authoring screen. **Edit device** opens *the phone* — the
same shell a player sees — in author mode: same tiles, same lists, same
navigation, plus author rows on each screen. `MCF_Intel_ShellMenu.OpenForAuthor`
is the entry point.

Two differences from a player's phone, both deliberate:

- **No lock screen.** A Game Master is not breaking into anything.
- **Author rows come first.** "+ New item" sits at the top of a list, not at
  the bottom of a long inbox where nobody finds it.

An item's fields are rows rather than a form with six boxes: six labelled boxes
need a screen twice this wide, and this glass is about 250 pixels across.
Tapping a row opens a full-screen editor for the one field it holds; a yes/no
row (such as *unread*) flips where it stands.

**Object or profile — the decision that shapes everything.** A carrier holds
either its own entries or the id of a shared profile (`smuggler_phone`). The
server upserts **any profile arriving with an id** into `MCF_Device_Library`,
which means a draft that keeps its id rewrites the shared profile and changes
every handset in the mission carrying it. That is why `OpenForAuthor` copies the
source profile and **clears `m_sId` on the draft**:

```c
m_Draft = MCF_Device_Script.Deserialize(MCF_Device_Script.Serialize(source));
m_Draft.m_sId = "";   // or the server rewrites the shared library profile
```

Editing the shared profile on purpose is still wanted and is still not built.
It needs its own row and its own warning; until then, editing is per object.

### 10.2 Read and unread

Two different facts, in two different places:

1. **Authored-new** — the mission maker says an item starts unread. Content:
   `m_bNew` on the item, a fifth field appended to the wire format (appended,
   never inserted — the format is in saved missions), the same for everybody.
2. **Read by me** — whether *this player* has opened it. Not content, and not
   replicated: two players who both pick up the same phone have genuinely
   different answers. `MCF_Device_ReadState` keeps it per client in
   `MCF_Core_PersistentStore` under the `dev.read.` prefix.

A badge shows where **authored-new AND not read by me**.

**The key is the object's RplId, not the profile id.** Keying on the profile id
made three handsets carrying `smuggler_phone` share one read log: opening a
message on one marked it read on all of them. `Replication.FindItemId(carrier)`
is unique per object. The trade is deliberate and documented: an RplId is
session-scoped, so read state does not survive a server restart. Being right
within a session beats being wrong across them.

Opening an item is what reads it — not hovering, not scrolling past.

### 10.3 Where the badges go

- **App tile**: a count, top right, `PaintBadge`. Hidden at zero, "9+" above
  nine.
- **List row**: a dot at the trailing edge in the accent (`0.76 0.39 0.08`). No
  number; the row is one item.
- **Lock screen**: up to three notification cards — sender and time, never the
  body. A locked phone that shows the message has given away what breaking in
  was for.

Counting is the presenter's job (`UnreadCount`, `UnreadItems`), not the shell's.

---

## 11. One app, one screen

Built 2026-09-11. Until this, every app on the phone opened the same
heading-date-body reader, and the device read as a menu with eight entry points
rather than as a phone. Four apps now have a screen of their own. Which reader
an item opens into is decided in `ShowEntry` by `m_OpenApp.m_eKind`.

### 11.1 The heading separator

Mission makers were already writing `"M. - 02:14"` and `"Outgoing - 0412"` into
headings before anything read them apart, because that is how a person writes a
line like that. The shell now reads it: `" - "` splits a heading into **who**
and **when-or-what-about** (`HeadPart` / `TailPart`).

A heading with no separator is all left half, so **nothing authored before this
changes how it looks**.

| App | `m_sHeading` | `m_sTimestamp` | `m_sBody` |
|---|---|---|---|
| MESSAGES | `sender - time` | date | the conversation (see 11.2) |
| EMAIL | `sender - subject` | date | the letter |
| CALLS | `direction - time` | date | `who - how long` |
| CONTACTS | name | **the number** | a note about them |
| NOTES / FILES / SETTINGS | title | date | the text |

### 11.2 MESSAGES — a conversation

A sender across the top with their disc beside it, then the body as bubbles.

**The authoring convention is one character.** Every line of the body is a
bubble; a line starting with `>` is from whoever owns the phone and sits on the
right, everything else came from the other end and sits on the left. A mission
maker who writes a conversation gets a conversation; one who writes a paragraph
gets a paragraph in a single bubble, which is also correct.

`MCF_PhoneBubble.layout` carries **both sides and hides one**. Alignment cannot
be changed at runtime without slot calls the rest of this device does not use,
so the row has a Left and a Right and the shell hides the one it does not want.
Four widgets, and it cannot be got wrong.

Each side **stretches** with a wide margin on the far side rather than hugging
its text: a bubble that sizes itself to its content cannot wrap, and a message
that does not wrap runs off the edge of a handset.

### 11.3 EMAIL — a letter

From, date and subject in a card, the body in a reading column under it. The
difference between this and the chat screen is the whole point of having two: a
message is a conversation and a mail is a document, and a device where both look
the same is a device the player cannot read at a glance.

### 11.4 CONTACTS — a list and a card

The list is names against numbers — the number is the second line, and it is the
number that makes a row read as a contact rather than as a heading with a circle
next to it. Nothing sits on the right, where a message list puts a time.

The card is the disc at eight times the size, the name, the number, one CALL
button and the note. **CALL is hidden when no number is saved**: a control that
cannot do anything is a phone lying about what it knows, which is the one thing
a piece of evidence must not do. CALL does not ring anybody — it puts the number
on the dialler, which is the screen the player would have reached typing it
themselves.

### 11.5 CALLS — a dialler, not a list

Every other app is a list of things somebody wrote down; the phone app is a
machine you operate. Twelve keys, the last three calls above them, a number
display that says live whether the number is in the phone book, and a CALL
button.

The keys are the dialler's own, **not the passcode pad's**: the pad is a door in
front of the phone and the dialler is a screen inside it, and sharing twelve
buttons between them meant one was always sitting in the other's geometry.

CALL answers the only question a prop phone can answer: does this handset know
whose number that is. There is no voice on the other end and there is not going
to be.

The recents row flips the authored order — the **name** (from the body) is what
a player scans for, the **direction and time** (from the heading) is what they
check afterwards.

### 11.6 PHOTOS — a grid

A photograph's thumbnail is its own title. A row reading "Truck at the mill"
with a coloured disc beside it tells the player strictly less than the picture
does, and photos are the only app where that is true.

Twelve tiles. A device carrying more says so at the bottom rather than dropping
them silently. A tile with nothing on it yet shows its heading and is the one
worth opening, because opening an item is what asks the cache for the picture.

### 11.7 Which apps did not get one

NOTES, FILES, SETTINGS and GENERAL keep the plain reader, and should. They are
documents and nothing else; giving them a bespoke screen would be decoration.

### 11.8 The list row itself

64 units tall, not 58. The title sits against the top of the row and the preview
against the bottom, so the gap between them is whatever the row has left over —
which is how every SMS client on a real handset does it, and why two lines
crammed into 58 units read as one clump of text.

The list and reader panes used to be pinned inside a `SizeLayoutWidget` with
`WidthOverride 200` while the glass is more than twice that wide, so rows used
less than half the screen. **There is no runtime setter for that override** —
the fix is `AllowWidthOverride 0` plus `HorizontalAlign 3` on the size layout,
which lets it stretch to the scroll's viewport. Every scroll pane on this device
now has that shape.

### 11.9 The clock

The status bar, the home screen and the lock screen all read
`ChimeraWorld.GetTimeAndWeatherManager()` — the mission's own time, not the
player's. All three are re-read on the one-second status tick. The lock screen's
clock used to be set once when the screen opened and drifted away from the world
the longer the phone stayed up.
