//! Interaction hint node (ARCHITECTURE.md 5.6). Covers Tier 1 (generic
//! text pool) and Tier 2 (chance-based pointer line) in one component:
//! leave m_fPointerChance at 0 for a Tier-1-only NPC.
//!
//! Enqueues its result into MCF_Voice_LineQueueManager (Phase 5's text
//! queue), reusing that instead of a separate display mechanism.
//!
//! Retry cooldown uses World.GetWorldTime() -- not yet confirmed against
//! the actual engine API, flagged for verification on first compile.

[ComponentEditorProps(category: "MCF/Interaction", description: "Tier 1/2 interaction hint -- generic lines, optionally with a chance-based pointer line.")]
class MCF_Interact_HintComponentClass : ScriptComponentClass
{
}

class MCF_Interact_HintComponent : ScriptComponent
{
	[Attribute(desc: "Tier 1 generic lines. One is picked at random when no pointer is shown.")]
	protected ref array<string> m_aGenericLines;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Chance (0-1) of showing the pointer line instead of a generic one. 0 = Tier 1 only.")]
	protected float m_fPointerChance;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Pointer line template. %1 is replaced with the pointer target name.")]
	protected string m_sPointerTemplate;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Name/role/direction filled into the pointer template's %1.")]
	protected string m_sPointerTargetName;

	[Attribute(defvalue: "60", uiwidget: UIWidgets.EditBox, desc: "Minimum seconds between pointer-chance attempts on this NPC.")]
	protected float m_fRetryCooldownSeconds;

	protected float m_fLastAttemptTime;

	//! Enqueues a line: a chance-based pointer line if the cooldown has
	//! elapsed and the roll succeeds, otherwise a random generic line.
	void Interact()
	{
		string line;

		if (m_fPointerChance > 0 && CanAttemptPointer())
		{
			m_fLastAttemptTime = GetGame().GetWorld().GetWorldTime();
			if (Math.RandomFloat01() < m_fPointerChance)
				line = string.Format(m_sPointerTemplate, m_sPointerTargetName);
		}

		if (line.IsEmpty())
			line = PickRandomGenericLine();

		MCF_Voice_LineQueueManager.GetInstance().Enqueue(line, 0);
	}

	protected bool CanAttemptPointer()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		return (now - m_fLastAttemptTime) >= m_fRetryCooldownSeconds;
	}

	protected string PickRandomGenericLine()
	{
		if (!m_aGenericLines || m_aGenericLines.IsEmpty())
			return "";

		int index = Math.RandomInt(0, m_aGenericLines.Count() - 1);
		return m_aGenericLines[index];
	}
}
