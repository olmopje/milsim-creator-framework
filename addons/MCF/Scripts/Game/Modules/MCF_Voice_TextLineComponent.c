//! Text-line node (ARCHITECTURE.md 4.4, simplified to text while audio is
//! parked). Enqueues its text into MCF_Voice_LineQueueManager when
//! triggered.

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

	void Play()
	{
		MCF_Voice_LineQueueManager.GetInstance().Enqueue(m_sText, m_iPriority);
	}
}
