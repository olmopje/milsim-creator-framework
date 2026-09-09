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
	MAP
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
