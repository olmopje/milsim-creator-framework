//! What is on a device, as a mission maker authors it.
//!
//! A profile is a device's whole contents: the apps on it and what is in each.
//! It is authored once and assigned by id, so one "smuggler's phone" can be
//! given to any phone in any mission -- and two phones deliberately sharing a
//! profile becomes something you can ask for rather than an accident of editing
//! the prefab they both come from.
//!
//! WHY THIS IS DATA AND NOT ATTRIBUTES ON THE OBJECT. The same wall the
//! dialogue system hit, for the same reasons:
//!
//!   - An editor attribute carries twelve bytes and cannot hold text. A Game
//!     Master can move a slider; they cannot type a message into a prefab.
//!   - One prefab is behind every phone in the mission. Editing its entries
//!     edits all of them at once, which is never what anybody means.
//!   - Which apps exist was decided in code, so a mission where the phone has
//!     no mail client could not be expressed at all.
//!
//! MCF_Dialogue_Data.c is the worked example and this is deliberately the same
//! shape, down to the names: a library of authored things, each with an id,
//! assigned in the world, with runtime additions shadowing shipped ones.
//!
//! NOTHING HERE IS PHONE-SPECIFIC. A laptop profile is a profile whose shell is
//! LAPTOP and whose apps read as folders.

//! Which skin a profile was written for.
//!
//! The object in the world still wins if they disagree -- a phone is physically
//! a phone whatever profile is put on it -- but a profile that says what it was
//! meant for lets the editor warn instead of silently looking wrong.
enum MCF_EDeviceShell
{
	PHONE,
	LAPTOP
}

//! One thing you can open and read: a message, a call, a contact, a file.
[BaseContainerProps()]
class MCF_Device_Item
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Short heading -- a sender, a subject line, a filename.")]
	string m_sHeading;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "When this was written or received. Free text: '0412 hrs' or '14 MAR' as you please.")]
	string m_sTimestamp;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "The text itself.")]
	string m_sBody;

	//! A picture shipped with the mod: an imported texture, by resource name.
	//! Always available, on every machine, offline.
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, params: "edds", desc: "Picture to show with this item, imported into an addon. Leave empty for none.")]
	ResourceName m_sImage;

	//! A picture from outside the mod, fetched at runtime.
	//!
	//! IT MUST POINT AT BASE64 TEXT, not at a .jpg. The engine refuses an http
	//! address on a widget and its file download is inert; what works is
	//! fetching the image as text and rebuilding it locally. See
	//! MCF_Device_ImageCache for the whole chain and why it is the only one.
	//!
	//! EACH CLIENT FETCHES ITS OWN, because a texture has to exist on the
	//! machine drawing it. A player who cannot reach the address sees no
	//! picture, and that is normal rather than an error.
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "URL of a BASE64 TEXT copy of a picture. Not a .jpg -- see MCF_Device_ImageCache. Each player fetches it themselves.")]
	string m_sImageUrl;

	//! What the fetched copy is filed under. Two items naming the same picture
	//! should share one file rather than fetching it twice.
	string ImageKey()
	{
		return ImageUrl();
	}

	//! The address, without whatever whitespace came with it.
	//!
	//! TRIMMED HERE AS WELL AS ON THE WAY IN, because profiles written before
	//! Clean trimmed are already in the store, and a mission maker should not
	//! have to retype a url to fix a space they cannot see.
	string ImageUrl()
	{
		if (m_sImageUrl.IsEmpty())
			return "";

		return MCF_Device_Script.Trim(m_sImageUrl);
	}

	//! One line for a list.
	string DescribeShort()
	{
		string label = m_sHeading;
		if (label.IsEmpty())
			label = "(no subject)";

		if (m_sTimestamp.IsEmpty())
			return label;

		return m_sTimestamp + "   " + label;
	}

	//! Two lines for a device's own list: what it is, then when it was.
	//!
	//! Deliberately not DescribeShort, which the two authoring screens use in
	//! a numbered single-line list where a second line would only make the
	//! list harder to scan. This one is for the phone, where an inbox reads
	//! subject first and stamp underneath, the way every inbox does.
	//!
	//! The newline is safe because MCF_IntelRow wraps rather than clips; on a
	//! wide screen it simply breaks where it is told to.
	string DescribeRow()
	{
		string label = m_sHeading;
		if (label.IsEmpty())
			label = "(no subject)";

		if (m_sTimestamp.IsEmpty())
			return label;

		return label + "\n<color rgba=\"255,255,255,140\">" + m_sTimestamp + "</color>";
	}
}

//! One app, and what is filed under it.
[BaseContainerProps()]
class MCF_Device_App
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EIntelApp), desc: "What kind of app this is. Decides the icon and what it says when empty.")]
	MCF_EIntelApp m_eKind;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Name under the tile. Leave empty to use the kind's own name.")]
	string m_sLabel;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "What it says when there is nothing in it. Leave empty to use the kind's own line.")]
	string m_sEmptyText;

	[Attribute(desc: "What is in this app.")]
	ref array<ref MCF_Device_Item> m_aItems;

	//! WHY AN APP HAS BOTH A KIND AND A LABEL. The kind is what the software
	//! knows: which empty line to write, which icon to draw, whether to format
	//! it as a call log rather than a list. The label is what the mission maker
	//! wants on the screen, which might be another language, or a brand name on
	//! a phone where that matters. Fusing them would mean adding an enum value
	//! every time somebody wants a differently-named app.
	string ResolveLabel()
	{
		if (!m_sLabel.IsEmpty())
			return m_sLabel;

		return MCF_Device_Names.LabelFor(m_eKind);
	}

	string ResolveEmptyText()
	{
		if (!m_sEmptyText.IsEmpty())
			return m_sEmptyText;

		return MCF_Device_Names.EmptyTextFor(m_eKind);
	}
}

//! Everything on one device.
[BaseContainerProps()]
class MCF_Device_Profile
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Id this profile is assigned by. Treat it like an event name: missions refer to it, so renaming breaks them.")]
	string m_sId;

	[Attribute(defvalue: "Mobile phone", uiwidget: UIWidgets.EditBox, desc: "Name shown at the top and on the interaction prompt.")]
	string m_sDeviceName;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EDeviceShell), desc: "Which screen this was written for. The object in the world still decides what it physically is.")]
	MCF_EDeviceShell m_eShell;

	[Attribute(desc: "The apps on this device, in the order they appear.")]
	ref array<ref MCF_Device_App> m_aApps;
}

//! The config asset. One of these, listing every profile the mod ships.
[BaseContainerProps(configRoot: true)]
class MCF_Device_ProfilesConfig
{
	[Attribute(desc: "Every device profile MCF knows about.")]
	ref array<ref MCF_Device_Profile> m_aProfiles;
}

//! What each kind of app is called, and what it says when it is empty.
//!
//! Kept apart from the app class so that the shell can name a kind it has no
//! authored app for -- which is what happens for a device with no profile at
//! all, where the apps come from entries tagged on the object itself.
class MCF_Device_Names
{
	static string LabelFor(int kind)
	{
		switch (kind)
		{
			case MCF_EIntelApp.MESSAGES: return "MESSAGES";
			case MCF_EIntelApp.CALLS:    return "PHONE";
			case MCF_EIntelApp.CONTACTS: return "CONTACTS";
			case MCF_EIntelApp.EMAIL:    return "MAIL";
			case MCF_EIntelApp.NOTES:    return "NOTES";
			case MCF_EIntelApp.PHOTOS:   return "PHOTOS";
			case MCF_EIntelApp.FILES:    return "FILES";
			case MCF_EIntelApp.SETTINGS: return "SETTINGS";
		}

		return "INBOX";
	}

	//! Said in the app's own terms rather than one blank "nothing here",
	//! because which drawer is empty is itself worth knowing.
	static string EmptyTextFor(int kind)
	{
		switch (kind)
		{
			case MCF_EIntelApp.MESSAGES: return "No messages.";
			case MCF_EIntelApp.CALLS:    return "No calls in the log.";
			case MCF_EIntelApp.CONTACTS: return "No contacts saved.";
			case MCF_EIntelApp.EMAIL:    return "No mail.";
			case MCF_EIntelApp.NOTES:    return "No notes.";
			case MCF_EIntelApp.PHOTOS:   return "No photos.";
			case MCF_EIntelApp.FILES:    return "No files.";
		}

		return "Nothing here.";
	}


	//! A two- or three-letter mark for the tile on the home screen.
	//!
	//! WHY LETTERS AND NOT ICONS. An icon is an imported texture per app, and
	//! eight of them is eight resources to keep, translate and re-import. A
	//! short mark on a coloured tile reads as an app grid at phone size and
	//! costs nothing -- and it stays legible on the laptop, where the same
	//! tiles are drawn much larger.
	//! The sprite this app draws on its tile, from the base game's own icon
	//! atlas -- see MCF_Intel_ShellMenu.ICON_SET.
	//!
	//! WHY VANILLA SPRITES AND NOT OUR OWN ART. Every shipped image in an
	//! addon has to be imported by the Workbench and referenced by GUID, and
	//! this project cannot drive the Workbench headlessly -- eight icons is
	//! eight manual imports and eight chances of a metafile without a
	//! resource. The atlas is already installed on every machine that runs the
	//! game, its sprites are white masks that tint to any colour, and it costs
	//! one line each.
	//!
	//! An EMPTY string means "no sprite I am sure of". The tile then falls
	//! back to its letter mark, which is why MAIL and FILES still read "@" and
	//! "DIR": the atlas is binary to every tool here, so the only names usable
	//! are the ones the base game's own layouts are seen using, and neither a
	//! mail nor a folder glyph is among them. Two names away from complete --
	//! open the atlas in the Workbench's Resource Browser and fill them in.
	//! The icon this app draws on its tile: one of MCF's own textures, drawn
	//! white and tinted by nothing -- the tile underneath carries the colour.
	//!
	//! WHY OUR OWN ART AND NOT THE GAME'S ATLAS. The base game's icon atlas
	//! has a settings cog and a camera and not much else a phone wants; there
	//! is no envelope and no folder in any sprite name its own layouts use.
	//! Ours are eleven small PNGs under UI/images/MCF_Phone, imported the same
	//! way the models were -- write the file, write a .edds.meta beside it,
	//! give the Workbench focus and it builds the .edds. That recipe is in
	//! HANDOVER; it is what makes shipping our own UI art cheap.
	static string IconFor(int kind)
	{
		switch (kind)
		{
			case MCF_EIntelApp.MESSAGES: return "{6A1C4F0B39D35200}UI/images/MCF_Phone/icon_messages.edds";
			case MCF_EIntelApp.CALLS:    return "{6A1C4F0B39D35201}UI/images/MCF_Phone/icon_calls.edds";
			case MCF_EIntelApp.CONTACTS: return "{6A1C4F0B39D35202}UI/images/MCF_Phone/icon_contacts.edds";
			case MCF_EIntelApp.EMAIL:    return "{6A1C4F0B39D35203}UI/images/MCF_Phone/icon_mail.edds";
			case MCF_EIntelApp.NOTES:    return "{6A1C4F0B39D35204}UI/images/MCF_Phone/icon_notes.edds";
			case MCF_EIntelApp.PHOTOS:   return "{6A1C4F0B39D35205}UI/images/MCF_Phone/icon_photos.edds";
			case MCF_EIntelApp.FILES:    return "{6A1C4F0B39D35206}UI/images/MCF_Phone/icon_files.edds";
			case MCF_EIntelApp.SETTINGS: return "{6A1C4F0B39D35207}UI/images/MCF_Phone/icon_settings.edds";
		}

		return "";
	}

	static string GlyphFor(int kind)
	{
		switch (kind)
		{
			case MCF_EIntelApp.MESSAGES: return "SMS";
			case MCF_EIntelApp.CALLS:    return "TEL";
			case MCF_EIntelApp.CONTACTS: return "ABC";
			case MCF_EIntelApp.EMAIL:    return "@";
			case MCF_EIntelApp.NOTES:    return "TXT";
			case MCF_EIntelApp.PHOTOS:   return "IMG";
			case MCF_EIntelApp.FILES:    return "DIR";
			case MCF_EIntelApp.SETTINGS: return "SET";
		}

		return "APP";
	}

	//! The tile's colour, ARGB. Muted rather than saturated: this is a phone
	//! seen in a dim room through a game's colour grading, and full-strength
	//! primaries read as a toy.
	static int ColorFor(int kind)
	{
		switch (kind)
		{
			case MCF_EIntelApp.MESSAGES: return 0xFF1E7A3A;
			case MCF_EIntelApp.CALLS:    return 0xFF17569C;
			case MCF_EIntelApp.CONTACTS: return 0xFF5A4C8C;
			case MCF_EIntelApp.EMAIL:    return 0xFF9A3F1E;
			case MCF_EIntelApp.NOTES:    return 0xFF9A7A16;
			case MCF_EIntelApp.PHOTOS:   return 0xFF167878;
			case MCF_EIntelApp.FILES:    return 0xFF3E4A57;
			case MCF_EIntelApp.SETTINGS: return 0xFF32373D;
		}

		return 0xFF44494F;
	}
}
