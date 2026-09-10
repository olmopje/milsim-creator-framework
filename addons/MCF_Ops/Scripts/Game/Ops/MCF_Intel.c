//! What an intel object contains, and how it presents itself.
//!
//! This is layer 1's output made durable. A trigger firing puts a line on a
//! screen and the information evaporates; an intel object sits in the world,
//! has to be found, and has to be carried to somebody who can act on it. See
//! docs/research/command-center-design.md.
//!
//! DELIBERATELY NOT ONE CLASS PER KIND. A letter, a phone, a photo and a map
//! are the same data wearing different framing: a heading, some text, and
//! optionally a place. Splitting them into separate systems would triple the
//! work and produce three inconsistent viewers. The kind only decides how it
//! is drawn.

//! How an intel object presents itself when read.
enum MCF_EIntelView
{
	//! One body of text, full width. A letter, an order, a note, a page torn
	//! out of a logbook.
	DOCUMENT,
	//! A list of entries with the selected one shown beside it. A phone's
	//! messages, a notebook's pages, a radio log.
	DEVICE,
	//! A place on the map, with a radius and a deliberate error. Not built
	//! yet -- listed here because the data model has to hold it before the
	//! map work starts, not after.
	MAP,

	// APPENDED, NEVER REORDERED. These values are written into the wire
	// format by MCF_Intel_CarrierComponent.SerializeContent and survive into
	// the persistent store, so inserting one above would silently turn every
	// saved letter into something else.

	//! A single sheet held in the hands. One entry per page, turned rather
	//! than listed.
	PAPER,
	//! A bound pad, leafed through the same way. Same behaviour as PAPER, a
	//! different object in the hands.
	NOTEPAD,
	//! A phone: a home screen of apps, a list inside each, one message at a
	//! time. Entries are grouped by MCF_EIntelApp.
	PHONE,
	//! The same, framed as a laptop.
	LAPTOP
}

//! Which part of a device an entry belongs to.
//!
//! WHY THIS LIVES IN OPS AND NOT IN THE DEVICES MODULE. Grouping is a property
//! of the intel, not of the thing displaying it: a notebook has sections and a
//! filing cabinet has drawers for the same reason a phone has apps. Ops' own
//! viewer ignores this and shows one flat list, which is right for a letter.
//! The devices module reads it to decide which icon an entry sits behind.
//!
//! GENERAL means "no particular section", and is what every entry authored
//! before this existed reads as -- see ApplySerializedContent, where a missing
//! fourth field falls back to it.
enum MCF_EIntelApp
{
	GENERAL,
	MESSAGES,
	EMAIL,
	NOTES,
	PHOTOS,
	FILES
}

//! One readable item inside an intel object.
//!
//! A letter has exactly one. A phone has one per message. Keeping both in the
//! same shape is what lets one viewer draw both.
[BaseContainerProps()]
class MCF_Intel_Entry
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Short heading -- a sender, a subject line, a filename.")]
	string m_sHeading;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "When this was written or received. Free text, so a mission maker can write '0412 hrs' or '14 MAR' as they please.")]
	string m_sTimestamp;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "The text itself.")]
	string m_sBody;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EIntelApp), desc: "Which section of the device this belongs to. Ignored by the plain viewer; the devices module uses it to decide which icon it sits behind.")]
	MCF_EIntelApp m_eApp;

	//! One line for the list on the left.
	string DescribeShort()
	{
		string label = m_sHeading;
		if (label.IsEmpty())
			label = "(no subject)";

		if (m_sTimestamp.IsEmpty())
			return label;

		return m_sTimestamp + "   " + label;
	}
}
