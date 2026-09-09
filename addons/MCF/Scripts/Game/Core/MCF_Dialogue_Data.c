//! What a conversation is made of, as a mission maker authors it.
//!
//! A conversation is a set of nodes. A node is one thing the other person
//! says, plus the things you may say back. Each reply can require something,
//! change something, and lead somewhere -- and that is the whole grammar.
//! Deliberately small: a branching tree with conditions and effects covers
//! nearly every scene a mission needs, and every feature past that point is
//! one a Game Master has to learn before they can use any of it.
//!
//! WHY REQUIREMENTS AND EFFECTS ARE PLAIN NUMBERS. A civilian who has watched
//! you point a rifle at his neighbours should be harder to talk to, and that
//! has to be legible to whoever builds the mission. Trust and fear are two
//! numbers on the person you are talking to; a reply asks for a minimum trust
//! or a maximum fear, and moves them. No scripting, no expressions -- if a
//! scene needs logic beyond that, the reply publishes an event and the
//! existing node graph handles it, which is machinery that already exists and
//! is already tested.
//!
//! WHY EFFECTS ARE EVENTS, NOT ACTIONS. A reply cannot spawn intel, fail an
//! objective or raise an alarm by itself. It publishes an event, and the
//! trigger layer does the rest. That keeps one way of causing things to
//! happen in MCF instead of two, and it means the day something new can be
//! caused, conversations can cause it without being touched.

//! One thing the player may say, and what it costs and buys.
[BaseContainerProps()]
class MCF_Dialogue_Choice
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "What the player says.")]
	string m_sText;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, params: "0 100 1", desc: "Minimum trust this person must have in you before this can be said.")]
	float m_fMinTrust;

	[Attribute(defvalue: "100", uiwidget: UIWidgets.Slider, params: "0 100 1", desc: "Maximum fear they may be under. A terrified person will not tell you anything useful.")]
	float m_fMaxFear;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Flag that must already be set in this conversation. Use it for 'only after he has admitted it'.")]
	string m_sRequiresFlag;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, params: "-100 100 1", desc: "How much this changes their trust in you.")]
	float m_fTrustChange;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, params: "-100 100 1", desc: "How much this changes their fear.")]
	float m_fFearChange;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Flag set when this is said. Remembered for the rest of the mission.")]
	string m_sSetFlag;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "MCF event published when this is said. This is how a conversation causes anything: hang an Intel Source or a Recipe on it.")]
	string m_sPublishEvent;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Node this leads to. Leave empty to end the conversation.")]
	string m_sNextNodeId;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Hide this reply when its requirements are not met, instead of showing it greyed out.")]
	bool m_bHideWhenLocked;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Why it is greyed out, shown to the player. Leave empty for a generic line.")]
	string m_sLockedReason;
}

//! One thing the other person says, and what may be said back.
[BaseContainerProps()]
class MCF_Dialogue_Node
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Name of this node, referred to by replies. 'start', 'admits_it', 'refuses'.")]
	string m_sId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "What they say here.")]
	string m_sText;

	[Attribute(desc: "What the player may say back. A node with no replies ends the conversation once it has been read.")]
	ref array<ref MCF_Dialogue_Choice> m_aChoices;
}
