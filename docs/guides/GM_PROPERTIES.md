# Game Master property reference

**Generated from the source by `tools/generate_gm_reference.ps1`. Do not edit by hand** — re-run the script instead. Every row below is a real `[Attribute(...)]` declaration, so this file cannot describe a property that does not exist, and it goes stale the moment somebody adds one without re-running it.

Last generated: 2026-09-10. **186 properties across 47 classes.**

These are the rows you see in the **entity properties panel** when you select a placed object, and in **Game Master's attribute panel** for the handful of things MCF exposes there. They are set per placed entity, before or during a session.

They are not the whole story. MCF also has **four screens of its own**, opened by right-clicking an entity in Game Master, which edit content rather than settings — the intel editor, the device editor, the conversation editor and the mission data screen. Those are described on the wiki's [Game Master](https://github.com/olmopje/milsim-creator-framework/wiki/Game-Master) page, which is also where the operations board is explained.

## Contents

- [MCF (Core)](#mcf-core) — 23 properties
- [MCF_Ops](#mcf-ops) — 48 properties
- [MCF_Dialogue](#mcf-dialogue) — 31 properties
- [MCF_AI](#mcf-ai) — 41 properties
- [MCF_Objectives](#mcf-objectives) — 39 properties
- [MCF_Dev](#mcf-dev) — 4 properties

## MCF (Core)

### `MCF_AI_DispositionComponent`

<sub>MCF_AI_DispositionComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fTrust` | float | `50` | Slider | How far they trust the players to begin with. 50 is a stranger who has no reason to think either way. |
| `m_fFear` | float | `0` | Slider | How frightened they are to begin with, before anything the area adds. |
| `m_sAreaKey` | string | — | EditBox | Hostility area this person lives in. Hostility there raises their fear. Leave empty to ignore the area entirely. |
| `m_fAreaFearShare` | float | `0.5` | Slider | How much of the area's hostility becomes this person's fear. 0.5 means a village at 80 hostility adds 40 fear. |

### `MCF_Core_BudgetConfigComponent`

<sub>MCF_Core_BudgetManager.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_aLimits` | ref array<string> | — | — | Budget entries, formatted as \ |

### `MCF_Core_GameModeComponent`

<sub>MCF_Core_GameModeComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_bEnableAAR` | bool | `1` | CheckBox | Start the AAR/Debrief manager listening automatically. |
| `m_bEnableHostilityDecay` | bool | `0` | CheckBox | Enable automatic Hostility decay. |
| `m_fHostilityDecayRate` | float | `0.5` | EditBox | Hostility decay rate per second, if enabled above. |

### `MCF_Core_ObjectIdentityComponent`

<sub>MCF_Core_ObjectIdentityComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sTag` | string | — | EditBox | Unique tag used by other MCF nodes/modules to reference this entity instead of a hard entity reference. Leave empty if this entity does not need to be looked up by tag. |

### `MCF_Core_TickManagerComponent`

<sub>MCF_Core_TickManagerComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fCriticalInterval` | float | `0.5` | EditBox | Seconds between \ |
| `m_fCosmeticInterval` | float | `5` | EditBox | Seconds between \ |

### `MCF_Interact_HintComponent`

<sub>MCF_Interact_HintComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_aGenericLines` | ref array<string> | — | — | Tier 1 generic lines. One is picked at random when no pointer is shown. |
| `m_fPointerChance` | float | `0` | EditBox | Chance (0-1) of showing the pointer line instead of a generic one. 0 = Tier 1 only. |
| `m_sPointerTemplate` | string | — | EditBox | Pointer line template. %1 is replaced with the pointer target name. |
| `m_sPointerTargetName` | string | — | EditBox | Name/role/direction filled into the pointer template's %1. |
| `m_fRetryCooldownSeconds` | float | `60` | EditBox | Minimum seconds between pointer-chance attempts on this NPC. |

### `MCF_UI_LineDisplayComponent`

<sub>MCF_UI_LineDisplayComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fDisplayDuration` | float | `4` | EditBox | How long each line stays on screen, in seconds. |
| `m_sSubtitle` | string | — | EditBox | Optional smaller subtitle shown under every line. Leave empty for none. |

### `MCF_Voice_TextLineComponent`

<sub>MCF_Voice_TextLineComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sText` | string | — | EditBox | Text to display when this line plays. |
| `m_iPriority` | int | `0` | EditBox | Priority -- higher values jump ahead of lower-priority queued lines. |
| `m_sTriggerEvent` | string | — | EditBox | Event name that plays this line. Leave empty to play it only from script. |
| `m_eAudience` | MCF_EAudience | `0` | ComboBox | Who sees this line. GROUP is not implemented yet and falls back to everyone. |
| `m_sAudienceFactionKey` | string | — | EditBox | Faction key, used only when Audience is FACTION. Leave empty to show to everyone. |

## MCF_Ops

### `MCF_Device_App`

<sub>MCF_Device_Data.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_eKind` | MCF_EIntelApp | `1` | ComboBox | What kind of app this is. Decides the icon and what it says when empty. |
| `m_sLabel` | string | — | EditBox | Name under the tile. Leave empty to use the kind's own name. |
| `m_sEmptyText` | string | — | EditBox | What it says when there is nothing in it. Leave empty to use the kind's own line. |
| `m_aItems` | array<ref MCF_Device_Item> | — | — | What is in this app. |

### `MCF_Device_Item`

<sub>MCF_Device_Data.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sHeading` | string | — | EditBox | Short heading -- a sender, a subject line, a filename. |
| `m_sTimestamp` | string | — | EditBox | When this was written or received. Free text: '0412 hrs' or '14 MAR' as you please. |
| `m_sBody` | string | — | EditBox | The text itself. |
| `m_sImage` | ResourceName | — | ResourceNamePicker | Picture to show with this item, imported into an addon. Leave empty for none. |
| `m_sImageUrl` | string | — | EditBox | URL of a BASE64 TEXT copy of a picture. Not a .jpg -- see MCF_Device_ImageCache. Each player fetches it themselves. |

### `MCF_Device_Profile`

<sub>MCF_Device_Data.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sId` | string | — | EditBox | Id this profile is assigned by. Treat it like an event name: missions refer to it, so renaming breaks them. |
| `m_sDeviceName` | string | `Mobile phone` | EditBox | Name shown at the top and on the interaction prompt. |
| `m_eShell` | MCF_EDeviceShell | `0` | ComboBox | Which screen this was written for. The object in the world still decides what it physically is. |
| `m_aApps` | array<ref MCF_Device_App> | — | — | The apps on this device, in the order they appear. |

### `MCF_Device_ProfilesConfig`

<sub>MCF_Device_Data.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_aProfiles` | array<ref MCF_Device_Profile> | — | — | Every device profile MCF knows about. |

### `MCF_Devices_LockComponent`

<sub>MCF_Devices_LockComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_bSecured` | bool | `1` | CheckBox | Whether this device is secured at mission start. Turn off for a phone somebody left unlocked. |
| `m_iDifficulty` | int | `2` | Slider | How hard it is, for whichever of the three break-in games comes up: more steps, a wider tuning range, a longer port table, and less time on the clock. |
| `m_sBreakInVerb` | string | `Break into` | EditBox | Verb on the prompt while it is still locked. |
| `m_sOpenedEvent` | string | — | EditBox | Event published once this device is opened. Leave empty for none. |

### `MCF_Intel_CarrierComponent`

<sub>MCF_Intel_CarrierComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sDeviceName` | string | `Document` | EditBox | Name shown at the top of the viewer -- 'Nokia 3310', 'Handwritten letter', 'Field notebook'. |
| `m_eView` | MCF_EIntelView | `0` | ComboBox | How this reads. DOCUMENT is one page. DEVICE is a list of entries, like a phone. MAP is not built yet. |
| `m_sActionVerb` | string | `Read` | EditBox | Verb on the interaction prompt: Read, Examine, Search, Study. |
| `m_sProfileId` | string | — | EditBox | Id of the device profile this carries. Empty uses the entries below instead. |
| `m_aEntries` | ref array<ref MCF_Intel_Entry> | — | — | What this object contains. A letter has one entry; a phone has one per message. |
| `m_sPreviewPrefab` | ResourceName | — | ResourceNamePicker | Prefab whose model is rendered behind the screen. Empty draws a plain panel instead. |
| `m_vPreviewSize` | vector | `0.07 0.009 0.149` | — | Size of the preview model in metres: across the face, thickness, along its length. |

### `MCF_Intel_Entry`

<sub>MCF_Intel.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sHeading` | string | — | EditBox | Short heading -- a sender, a subject line, a filename. |
| `m_sTimestamp` | string | — | EditBox | When this was written or received. Free text, so a mission maker can write '0412 hrs' or '14 MAR' as they please. |
| `m_sBody` | string | — | EditBox | The text itself. |
| `m_eApp` | MCF_EIntelApp | `0` | ComboBox | Which section of the device this belongs to. Ignored by the plain viewer; the devices module uses it to decide which icon it sits behind. |

### `MCF_Intel_SourceComponent`

<sub>MCF_Intel_SourceComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sTriggerEvent` | string | — | EditBox | Event name that produces this intel. Leave empty and nothing will ever fire it. |
| `m_eMode` | MCF_EIntelSourceMode | `0` | ComboBox | DROP puts an object in the world that somebody must carry to the board. SIGNAL enters it on a faction's board directly. |
| `m_bOnce` | bool | `1` | CheckBox | Produce only the first time the event fires. Leave on unless you really want one piece of intel per trigger crossing. |
| `m_sPrefab` | ResourceName | — | ResourcePickerThumbnail | DROP: the object to spawn. Needs an MCF_Intel_CarrierComponent on it if the text below is to be used. |
| `m_vSpawnOffset` | vector | `0 0 0` | EditBox | DROP: where the object appears, relative to this entity. Use it to put the drop on a table rather than inside it. |
| `m_sFactionKey` | string | — | EditBox | SIGNAL: which faction learns this. Faction key, e.g. US or USSR. Required -- intel with no side goes to everyone. |
| `m_sSource` | string | `Document` | EditBox | What this came off: 'Handwritten letter', 'Radio intercept', 'Captured map'. Provenance is half of what makes intel judgeable. |
| `m_sHeading` | string | — | EditBox | Subject line. Leave the text fields empty in DROP mode to keep whatever the prefab already says. |
| `m_sTimestamp` | string | — | EditBox | When the source says it happened. Free text, as a person would write it. |
| `m_sBody` | string | — | EditBox | The content itself. |
| `m_sProducedEvent` | string | — | EditBox | Event published once this has produced its intel, so a chain can continue. Optional. |

### `MCF_Ops_GameModeComponent`

<sub>MCF_Ops_GameModeComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_bCreateSampleTask` | bool | `0` | CheckBox | Development aid: if the task store is empty at mission start, create a set of sample tasks that exercise every visibility rule. There is no way to author a task in game yet, so this exists to test the task system. Turn off for anything real. |
| `m_sSampleFactionKey` | string | `US` | EditBox | Faction key the faction-scoped sample task is addressed to. Only used when sample tasks are enabled. |

### `MCF_Squad_CohesionComponent`

<sub>MCF_Squad_CohesionComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sSquadKey` | string | — | EditBox | Squad identifier. Members of the same squad share this key. |
| `m_bEnablePositionSharing` | bool | `0` | CheckBox | Enable squad position sharing (members visible to each other only, never the whole army). |
| `m_bEnableMusterGate` | bool | `0` | CheckBox | Enable the muster gate -- IsSquadMustered() checks whether members are gathered within the muster radius. |
| `m_fMusterRadius` | float | `50` | EditBox | Muster radius in metres. All members must be within this distance of the muster point. |
| `m_bEnableRadioRespawnHint` | bool | `0` | CheckBox | Enable the radio respawn hint -- publishes an event a UI can show, making the existing but little-known mechanic visible. |

### `MCF_Task_BoardComponent`

<sub>MCF_Task_BoardComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fUseRange` | float | `6` | EditBox | How close a player must be, in metres, to enter intel at this board. |

## MCF_Dialogue

### `MCF_Dialogue_AssignComponent`

<sub>MCF_Dialogue_AssignComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sConversationId` | string | — | EditBox | Conversation id from Configs/Dialogue/MCF_Conversations.conf. |
| `m_fRadius` | float | `15` | Slider | Everybody within this many metres of the node is given the conversation. |
| `m_sTriggerEvent` | string | — | EditBox | MCF event that triggers the assignment. Leave empty to assign once when the mission starts. |
| `m_bOnce` | bool | `1` | CheckBox | Assign only the first time. Turn off to pick up people who arrive later, by firing the event again. |
| `m_fTrust` | float | `-1` | Slider | Trust to give these people, or -1 to leave whatever they already have. |
| `m_fFear` | float | `-1` | Slider | Fear to give these people, or -1 to leave whatever they already have. |

### `MCF_Dialogue_Choice`

<sub>MCF_Dialogue_Data.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sText` | string | — | EditBox | What the player says. |
| `m_fMinTrust` | float | `0` | Slider | Minimum trust this person must have in you before this can be said. |
| `m_fMaxFear` | float | `100` | Slider | Maximum fear they may be under. A terrified person will not tell you anything useful. |
| `m_sRequiresFlag` | string | — | EditBox | Flag that must already be set in this conversation. Use it for 'only after he has admitted it'. |
| `m_fTrustChange` | float | `0` | Slider | How much this changes their trust in you. |
| `m_fFearChange` | float | `0` | Slider | How much this changes their fear. |
| `m_sSetFlag` | string | — | EditBox | Flag set when this is said. Remembered for the rest of the mission. |
| `m_sPublishEvent` | string | — | EditBox | MCF event published when this is said. This is how a conversation causes anything: hang an Intel Source or a Recipe on it. |
| `m_sNextNodeId` | string | — | EditBox | Node this leads to. Leave empty to end the conversation. |
| `m_bHideWhenLocked` | bool | `0` | CheckBox | Hide this reply when its requirements are not met, instead of showing it greyed out. |
| `m_sLockedReason` | string | — | EditBox | Why it is greyed out, shown to the player. Leave empty for a generic line. |

### `MCF_Dialogue_Component`

<sub>MCF_Dialogue_Component.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sSpeakerName` | string | `Civilian` | EditBox | Name shown at the top of the conversation. 'Farmer', 'Wounded soldier', 'Ahmed'. |
| `m_sActionVerb` | string | `Talk` | EditBox | Verb on the interaction prompt: Talk, Question, Interrogate. |
| `m_sStartNodeId` | string | `start` | EditBox | Node the conversation opens on. |
| `m_fTalkRange` | float | `10` | Slider | How close a player must be to talk, in metres. Checked on the server so a client cannot claim to be here. |
| `m_aNodes` | ref array<ref MCF_Dialogue_Node> | — | — | The conversation itself. |

### `MCF_Dialogue_Conversation`

<sub>MCF_Dialogue_Library.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sId` | string | — | EditBox | Id this conversation is assigned by. Treat it like an event name: missions refer to it, so renaming breaks them. |
| `m_sSpeakerName` | string | `Civilian` | EditBox | Name shown at the top of the conversation. |
| `m_sActionVerb` | string | `Talk` | EditBox | Verb on the interaction prompt: Talk, Question, Interrogate. |
| `m_sStartNodeId` | string | `start` | EditBox | Node the conversation opens on. |
| `m_aNodes` | array<ref MCF_Dialogue_Node> | — | — | The conversation itself. |

### `MCF_Dialogue_LibraryConfig`

<sub>MCF_Dialogue_Library.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_aConversations` | array<ref MCF_Dialogue_Conversation> | — | — | Every conversation MCF knows about. |

### `MCF_Dialogue_Node`

<sub>MCF_Dialogue_Data.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sId` | string | — | EditBox | Name of this node, referred to by replies. 'start', 'admits_it', 'refuses'. |
| `m_sText` | string | — | EditBox | What they say here. |
| `m_aChoices` | array<ref MCF_Dialogue_Choice> | — | — | What the player may say back. A node with no replies ends the conversation once it has been read. |

## MCF_AI

### `MCF_AI_AmbientActorComponent`

<sub>MCF_AI_AmbientActorComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sArchetype` | string | — | EditBox | Archetype label, e.g. \ |
| `m_aPoiTags` | ref array<string> | — | — | Tags of MCF_Core_ObjectIdentityComponent-tagged Lifestyle POI entities this actor cycles between. |

### `MCF_AI_CivilianBehaviorHookComponent`

<sub>MCF_AI_CivilianBehaviorHookComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sAreaKey` | string | — | EditBox | Area key this civilian belongs to, matching the key used with MCF_Hostility_Manager.AddImpact(). |
| `m_fFearfulThreshold` | float | `30` | EditBox | Hostility value (0-100) above which this civilian's profile becomes \ |
| `m_fHostileThreshold` | float | `70` | EditBox | Hostility value (0-100) above which this civilian's profile becomes \ |

### `MCF_AI_CommandWatchdogComponent`

<sub>MCF_AI_CommandWatchdogComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fStuckThresholdSeconds` | float | `10` | EditBox | Seconds without movement before this entity is considered stuck. |
| `m_fMinMovementDistance` | float | `0.5` | EditBox | Minimum distance (metres) counted as \ |

### `MCF_AI_ComplianceComponent`

<sub>MCF_AI_ComplianceComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_bArmed` | bool | `1` | CheckBox | True if this NPC is armed (offers \ |
| `m_fBaseComplianceChance` | float | `0.5` | EditBox | Base chance (0-1) this NPC complies when commanded. |
| `m_sHostilityAreaKey` | string | — | EditBox | Hostility area key to penalize on unjustified use against this NPC (e.g. \ |
| `m_fUnjustifiedPenalty` | float | `10` | EditBox | Hostility penalty applied to m_sHostilityAreaKey on unjustified use. |
| `m_fMaxCommandDistance` | float | `5` | EditBox | Maximum distance (metres) a player can be to command this NPC. |
| `m_fMaxAimAngleDegrees` | float | `20` | EditBox | Maximum angle (degrees) between the player's aim direction and this NPC for the player to be considered \ |
| `m_fFearWeight` | float | `0.5` | Slider | How much of this person's fear counts towards giving up. A frightened man surrenders sooner. |
| `m_fArmedResistance` | float | `0.3` | Slider | How much less likely an ARMED person is to give up. Set high for soldiers, irrelevant for civilians. |
| `m_fWeaponRaisedWeight` | float | `0.25` | Slider | How much a raised weapon adds. This is the difference between shouting and threatening. |
| `m_fFearOnSurrender` | float | `25` | Slider | How much fear giving up puts into somebody. |
| `m_bPunishUnjustified` | bool | `1` | CheckBox | Whether shouting at this person without cause raises the area's hostility. Off for anybody who is fair game. |

### `MCF_AI_EventToWaypointComponent`

<sub>MCF_AI_EventToWaypointComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sTriggerEvent` | string | — | EditBox | Event name that triggers adding the waypoint to the group. |
| `m_sGroupTag` | string | — | EditBox | Tag (via MCF_Core_ObjectIdentityComponent) of the native SCR_AIGroup entity to command. |
| `m_sWaypointTag` | string | — | EditBox | Tag (via MCF_Core_ObjectIdentityComponent) of the native AIWaypoint entity to add (e.g. a placed SCR_AIAnimationWaypoint). |

### `MCF_AI_LifestylePOIComponent`

<sub>MCF_AI_LifestylePOIComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sRole` | string | — | EditBox | Role of this POI, e.g. \ |
| `m_iSlotCount` | int | `1` | EditBox | Maximum number of ambient actors that can occupy this POI at once. |

### `MCF_AI_ReleaseAction`

<sub>MCF_AI_SubdueActions.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fTrustOnRelease` | float | `20` | Slider | How much trust letting somebody go buys back. |

### `MCF_AI_RestrainAction`

<sub>MCF_AI_SubdueActions.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fFearOnRestrain` | float | `15` | Slider | How much more fear being restrained adds. |

### `MCF_AI_SimpleMoverComponent`

<sub>MCF_AI_SimpleMoverComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fMoveSpeed` | float | `2` | EditBox | Movement speed in metres per second. |
| `m_fArrivalDistance` | float | `0.3` | EditBox | Distance (metres) counted as \ |

### `MCF_AI_SubjectControlComponent`

<sub>MCF_AI_SubjectControlComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fFollowDistance` | float | `6` | Slider | How far back a follower trails, in metres. |
| `m_fLeadDistance` | float | `4` | Slider | How far ahead a led prisoner is marched, in metres. |
| `m_fStandOffDistance` | float | `8` | Slider | How far back somebody told to keep their distance is pushed. |
| `m_fRepointDistance` | float | `3` | Slider | How far the escort must move before the subject is re-pointed. Smaller is smoother and costs more. |
| `m_fEscapeChancePerSecond` | float | `0.04` | Slider | Chance per second that an unrestrained escortee tries to break away, at maximum nerve. |
| `m_sEscapeEvent` | string | `MCF_AI_SubjectEscaped` | EditBox | Event published when somebody breaks away. Hang a Recipe or an alarm on it. |
| `m_sPoseGraph` | ResourceName | — | EditBox | Animation graph (.agr) for the restrained pose. Empty means no pose at all -- which breaks nothing. |
| `m_sPoseGraphInstance` | ResourceName | — | EditBox | Animation instance (.asi) that goes with the graph above. |
| `m_sPoseCommand` | string | `CMD_Narrative` | EditBox | Command inside the graph that starts the pose. CMD_Narrative and CMD_Gesture_NPC are the two the narrative graph offers. |
| `m_iPoseCommandValue` | int | `0` | EditBox | Value passed with that command. |
| `m_sPoseVariable` | string | `TalkVariant` | EditBox | Variable that selects WHICH clip in the graph plays. TalkVariant is the narrative graph's selector, 1 to 100. |
| `m_iPoseVariant` | int | `1` | Slider | Which clip. Nobody knows which number is which without looking -- dial it in Game Master and watch. |

### `MCF_AI_WaypointAnimationComponent`

<sub>MCF_AI_WaypointAnimationComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sAnimationName` | string | — | EditBox | Animation name to request on arrival. Not wired to actually play it -- see file header. |
| `m_sEventName` | string | `MCF_AI_WaypointAnimationRequested` | EditBox | Event name published on arrival. |

## MCF_Objectives

### `MCF_Infra_NodeComponent`

<sub>MCF_Infra_NodeComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sOwnTag` | string | — | EditBox | This node's own tag, referenced by other nodes' Depends On list. |
| `m_aDependsOnTags` | ref array<string> | — | — | Tags of nodes this one depends on. If any is inactive, this node becomes inactive too. |

### `MCF_Obj_AlarmTriggerComponent`

<sub>MCF_Obj_AlarmTriggerComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sSourceEvent` | string | — | EditBox | Event name that triggers this alarm. |
| `m_sTriggeredEvent` | string | `MCF_Obj_AlarmRaised` | EditBox | Event name published when the alarm is raised. |

### `MCF_Obj_ConeDetectionTriggerComponent`

<sub>MCF_Obj_ConeDetectionTriggerComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fRadius` | float | `50` | EditBox | Detection radius in metres. |
| `m_fHalfAngleDegrees` | float | `45` | EditBox | Half-angle of the detection cone in degrees, measured from this entity's forward direction. |
| `m_sTriggeredEvent` | string | `MCF_Obj_ConeDetected` | EditBox | Event name published when a watched entity enters the cone. |
| `m_bTriggerOnce` | bool | `1` | CheckBox | If true, only fires once. |

### `MCF_Obj_LogicComponent`

<sub>MCF_Obj_LogicComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sMode` | string | `OR` | EditBox | Logic mode: \ |
| `m_aInputEvents` | ref array<string> | — | — | Event names this node listens to (OR/COUNTER modes). |
| `m_sOutputEvent` | string | `MCF_Obj_LogicFired` | EditBox | Event name published when this node's condition is met. |
| `m_iRequiredCount` | int | `1` | EditBox | Number of input occurrences required before firing (COUNTER mode only). |
| `m_sAndInput1` | string | — | EditBox | AND mode input slot 1. Leave empty if unused. |
| `m_sAndInput2` | string | — | EditBox | AND mode input slot 2. Leave empty if unused. |
| `m_sAndInput3` | string | — | EditBox | AND mode input slot 3. Leave empty if unused. |
| `m_sAndInput4` | string | — | EditBox | AND mode input slot 4. Leave empty if unused. |

### `MCF_Obj_ObjectiveComponent`

<sub>MCF_Obj_ObjectiveComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sTitle` | string | — | EditBox | Objective title shown to players. |
| `m_sDescription` | string | — | EditBox | Objective description shown to players. |
| `m_bVisibleOnMap` | bool | `1` | CheckBox | If true, this objective shows a marker on the map. Not implemented yet -- see the file header. |
| `m_sIntelGateEvent` | string | — | EditBox | Event name that unlocks this objective (intel gate). Leave empty for an objective that starts unlocked. |
| `m_sCompleteEvent` | string | — | EditBox | Event name that completes this objective. Leave empty to complete it only from script. |
| `m_sFailEvent` | string | — | EditBox | Event name that fails this objective. Leave empty to fail it only from script. |
| `m_bAnnounce` | bool | `1` | CheckBox | Announce unlock, completion and failure on screen. |
| `m_eAudience` | MCF_EAudience | `0` | ComboBox | Who sees this objective's announcements. GROUP is not implemented yet and falls back to everyone. |
| `m_sAudienceFactionKey` | string | — | EditBox | Faction key, used only when Audience is FACTION. Leave empty to announce to everyone. |

### `MCF_Obj_ObservationNode`

<sub>MCF_Obj_ObservationNode.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sReportEvent` | string | `MCF_Obj_ObservationReported` | EditBox | Event name published on the Event Bus when this POI reports activity. |
| `m_sTriggerEvent` | string | — | EditBox | Event name that makes this POI report. Leave empty to report only from script. |

### `MCF_Obj_ProximityTriggerComponent`

<sub>MCF_Obj_ProximityTriggerComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fRadius` | float | `50` | EditBox | Detection radius in metres. |
| `m_sTriggeredEvent` | string | `MCF_Obj_ProximityDetected` | EditBox | Event name published when a watched entity enters range. |
| `m_bTriggerOnce` | bool | `1` | CheckBox | If true, only fires once. If false, fires again each time an entity re-enters after leaving range. |

### `MCF_Obj_SpottedByPlayerComponent`

<sub>MCF_Obj_SpottedByPlayerComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_fMaxRange` | float | `100` | EditBox | Maximum distance in metres at which a watcher can spot this entity. |
| `m_fViewHalfAngleDegrees` | float | `45` | EditBox | Half-angle of the watcher's view cone in degrees. Wider than a weapon-aim cone -- this approximates natural field of view, not precise aim. |
| `m_sSpottedEvent` | string | `MCF_Obj_PlayerSpottedTarget` | EditBox | Event name published once a watcher spots this entity. |
| `m_bTriggerOnce` | bool | `1` | CheckBox | If true, only fires once. |

### `MCF_Obj_TriggerZoneComponent`

<sub>MCF_Obj_TriggerZoneComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sEventName` | string | `MCF_Obj_ZoneActivated` | EditBox | Event name published on the Event Bus when this zone activates. Follows the Module_Action naming contract (ARCHITECTURE.md 3.1). |
| `m_sTriggerEvent` | string | — | EditBox | Event name that activates this zone. Leave empty to activate it only from script. |
| `m_bTriggerOnce` | bool | `0` | CheckBox | If true, this zone only fires once -- repeated Activate() calls after the first do nothing. |

### `MCF_React_RecipeComponent`

<sub>MCF_React_RecipeComponent.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_sTriggerEvent` | string | — | EditBox | Event name that triggers this recipe. |
| `m_aSteps` | ref array<string> | — | — | Step values in order, formatted as \ |

## MCF_Dev

### `MCF_Dev_SelfTestComponent`

<sub>MCF_Dev_SelfTest.c</sub>

| Property | Type | Default | Shown as | What it does |
|---|---|---|---|---|
| `m_bEnabled` | bool | `0` | — | Run the multiplayer checks once enough players have joined. Leave off for a normal session. |
| `m_iExpectedPlayers` | int | `2` | — | How many players to wait for before starting. The checks need at least two, on two factions. |
| `m_iWaitSeconds` | int | `180` | — | Seconds to wait for them before giving up and saying so. |
| `m_sPictureUrl` | string | `https://raw.githubusercontent.com/olmopje/milsim-creator-framework/main/content/images/locomotive.txt` | — | The picture every client is asked to fetch. |

