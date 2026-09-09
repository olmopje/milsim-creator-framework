//! Text-line node (ARCHITECTURE.md 4.4, simplified to text while audio is
//! parked). Enqueues its text into MCF_Voice_LineQueueManager when
//! triggered, which reaches players through MCF_UI_LineDisplayComponent.
//!
//! Set m_sTriggerEvent to have this line play from the Event Bus. Without
//! it, Play() is only reachable from script and nothing calls it, so the
//! node could be placed and configured but never say anything.
//!
//! Note the overlap with a Recipe step: a Recipe with a
//! "PLAY_TEXT_LINE:<text>" step does the same thing inline. This node is for
//! a line that several things should be able to trigger, or one a mission
//! maker wants to place and edit as its own object.

[ComponentEditorProps(category: "MCF/Voice", description: "Enqueues a text line when triggered.")]
class MCF_Voice_TextLineComponentClass : ScriptComponentClass
{
}

class MCF_Voice_TextLineComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Text to display when this line plays.")]
	protected string m_sText;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Priority -- higher values jump ahead of lower-priority queued lines.")]
	protected int m_iPriority;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Event name that plays this line. Leave empty to play it only from script.")]
	protected string m_sTriggerEvent;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(MCF_EAudience), desc: "Who sees this line. GROUP is not implemented yet and falls back to everyone.")]
	protected MCF_EAudience m_eAudience;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Faction key, used only when Audience is FACTION. Leave empty to show to everyone.")]
	protected string m_sAudienceFactionKey;

	protected ScriptInvoker m_TriggerInvoker;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		// Reaction and logic nodes are server-side, like the detection nodes
		// that feed them. On a client these would subscribe to events that
		// never fire there -- harmless today, but it leaves the client holding
		// framework state it should not have, and if anything ever did publish
		// locally the two machines would diverge.
		if (!Replication.IsServer())
			return;

		if (m_sTriggerEvent.IsEmpty())
		{
			MCF_Core_Log.Debug("TextLine init but no trigger event set -- inactive");
			return;
		}

		m_TriggerInvoker = MCF_Core_EventManager.GetInstance().GetInvoker(m_sTriggerEvent);
		m_TriggerInvoker.Insert(OnTriggerEvent);
		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer(m_sTriggerEvent, "MCF_Voice_TextLineComponent (play)");

		MCF_Core_Log.Debug("TextLine init, plays on '" + m_sTriggerEvent + "'");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_TriggerInvoker)
			m_TriggerInvoker.Remove(OnTriggerEvent);
	}

	protected void OnTriggerEvent(Managed payload)
	{
		Play();
	}

	void Play()
	{
		if (m_sText.IsEmpty())
			return;

		MCF_Core_Log.Debug("TextLine PLAYING: " + m_sText);
		MCF_Voice_LineQueueManager.GetInstance().Enqueue(m_sText, m_iPriority, m_eAudience, m_sAudienceFactionKey);
	}
}
