//! The conversation screen.
//!
//! A RUNNING CONVERSATION, NOT A PANEL. The screen is a band across the lower
//! half with the world still visible above it, so you are talking to a person
//! you can see rather than reading a dialog box that happens to be about them.
//! What has been said stays on screen and scrolls up as the conversation goes
//! on, and your own replies are written into it -- so a player can look back
//! at what they said two questions ago, which is exactly when it matters.
//!
//! Only the last few exchanges are kept. Not for memory: it is so the text
//! always fits without anybody having to scroll, which is a thing players do
//! not do mid-conversation. The transcript is a record of the last minute, not
//! an archive.
//!
//! DUMB BY DESIGN. It draws what the server sent and sends back which button
//! was pressed -- it holds no conversation graph, no trust, no fear and no
//! idea where a reply leads. Everything it knows arrived in one
//! MCF_Dialogue_View and is replaced wholesale by the next one. That is what
//! makes it safe to let a client render a conversation at all. The transcript
//! is the one exception, and it is only a copy of what has already been shown.
//!
//! LOCKED REPLIES ARE SHOWN, NOT HIDDEN. A greyed line with a reason after it
//! is the only feedback a player ever gets that trust and fear are real. Hide
//! them and the system becomes invisible: the same civilian simply says less,
//! and nobody learns that waving a rifle around caused it. An author who wants
//! a reply genuinely secret ticks m_bHideWhenLocked and the server never sends
//! it at all.

class MCF_Dialogue_Menu : ChimeraMenuBase
{
	protected static const ResourceName ENTRY_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string W_SPEAKER = "Speaker";
	protected static const string W_BODY = "BodyText";
	protected static const string W_CHOICE_LIST = "ChoiceList";
	protected static const string W_HINT = "Hint";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	//! How many lines of conversation stay on screen. Four, because the panel
	//! is deliberately shallow -- the transcript is a record of the last
	//! half-minute, not an archive, and anything that does not fit without
	//! scrolling may as well not be there.
	protected static const int TRANSCRIPT_LINES = 4;

	//! The open conversation, so an RPC arriving from the server can find the
	//! screen it belongs to. One at a time: you cannot talk to two people at
	//! once, and a second conversation replaces the first.
	protected static MCF_Dialogue_Menu s_Instance;

	//! Who we are talking to, kept so every reply can name them again. Set
	//! before the menu opens because the menu manager gives no way to pass an
	//! argument in -- the same trick the intel viewer uses.
	protected static RplId s_PendingSpeakerId;

	protected RplId m_SpeakerId;
	protected string m_sNodeId;
	protected string m_sSpeakerName;

	protected Widget m_wRoot;
	protected TextWidget m_wSpeaker;
	protected RichTextWidget m_wBody;
	protected VerticalLayoutWidget m_wChoiceList;
	protected TextWidget m_wHint;
	protected SCR_ButtonTextComponent m_ButtonClose;

	protected ref array<string> m_aTranscript = {};
	protected ref array<SCR_ButtonTextComponent> m_aChoiceButtons = {};
	protected ref array<Widget> m_aChoiceWidgets = {};
	protected ref array<int> m_aChoiceIndices = {};
	protected ref array<string> m_aChoiceTexts = {};

	//! Tracked here rather than asked of the button. SCR_WLibComponentBase
	//! exposes SetEnabled but no getter that this project has verified, and a
	//! guess would fail at the one moment it matters -- picking where the
	//! gamepad cursor starts.
	protected ref array<bool> m_aChoiceEnabled = {};

	//! Opens the screen and asks the server to start the conversation.
	//!
	//! The screen appears before the server has answered, showing the speaker
	//! and nothing else. Waiting for the round trip instead would mean a press
	//! that appears to do nothing for a moment, which players read as a broken
	//! prompt and press again.
	static MCF_Dialogue_Menu OpenFor(RplId speakerId, string speakerName)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return null;

		s_PendingSpeakerId = speakerId;

		MCF_Dialogue_Menu menu = MCF_Dialogue_Menu.Cast(menuManager.OpenMenu(ChimeraMenuPreset.MCF_Dialogue));
		if (!menu)
			return null;

		menu.ShowWaiting(speakerName);
		menu.StartLook();

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestDialogueBegin(speakerId);

		return menu;
	}

	//! Called from the server's reply. Static because the RPC handler lives on
	//! the player controller and has no reference to the screen.
	static void ShowView(notnull MCF_Dialogue_View view)
	{
		if (!s_Instance)
			return;

		s_Instance.Render(view);
	}

	static void CloseIfOpen()
	{
		if (s_Instance)
			s_Instance.Close();
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		s_Instance = this;
		m_SpeakerId = s_PendingSpeakerId;
		m_aTranscript.Clear();

		m_wRoot = GetRootWidget();
		if (!m_wRoot)
		{
			MCF_Core_Log.Warn("dialogue menu opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wSpeaker = TextWidget.Cast(m_wRoot.FindAnyWidget(W_SPEAKER));
		m_wBody = RichTextWidget.Cast(m_wRoot.FindAnyWidget(W_BODY));
		m_wChoiceList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget(W_CHOICE_LIST));
		m_wHint = TextWidget.Cast(m_wRoot.FindAnyWidget(W_HINT));

		// A visible way out, because Escape is not always available: in the
		// Workbench it stops the play session instead of closing the screen.
		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, m_wRoot);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);
	}

	override void OnMenuClose()
	{
		// Tell the server we have gone, so it forgets where we were rather
		// than leaving the conversation open on a player who walked off.
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestDialogueEnd(m_SpeakerId);

		StopLook();

		if (s_Instance == this)
			s_Instance = null;

		super.OnMenuClose();
	}

	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	override void OnMenuFocusLost()
	{
		super.OnMenuFocusLost();
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	protected void OnBack()
	{
		Close();
	}

	// ------------------------------------------------------------ transcript

	void ShowWaiting(string speakerName)
	{
		m_sSpeakerName = speakerName;

		if (m_wSpeaker)
			m_wSpeaker.SetText(speakerName);

		if (m_wBody)
			m_wBody.SetText("...");

		if (m_wHint)
			m_wHint.SetText("");
	}

	//! Adds a line to the running conversation and drops the oldest once it
	//! no longer fits on screen.
	protected void AddToTranscript(string who, string text)
	{
		if (text.IsEmpty())
			return;

		// NEWEST FIRST. The panel sits at the bottom of the screen and does
		// not scroll itself, so appending would push the line you actually
		// need out of sight below the fold. Reading downwards into older
		// lines costs a moment; reading a reply that is not on screen costs
		// the conversation.
		m_aTranscript.InsertAt(who + "\n" + text, 0);

		while (m_aTranscript.Count() > TRANSCRIPT_LINES)
			m_aTranscript.Remove(m_aTranscript.Count() - 1);

		if (!m_wBody)
			return;

		string page = "";
		foreach (int i, string line : m_aTranscript)
		{
			if (i > 0)
				page = page + "\n\n";

			page = page + line;
		}

		m_wBody.SetText(page);
	}

	protected void Render(notnull MCF_Dialogue_View view)
	{
		m_sNodeId = view.m_sNodeId;

		if (!view.m_sSpeaker.IsEmpty())
			m_sSpeakerName = view.m_sSpeaker;

		if (m_wSpeaker)
			m_wSpeaker.SetText(m_sSpeakerName);

		// The very first line replaces the placeholder rather than being
		// appended under it.
		if (m_aTranscript.IsEmpty() && m_wBody)
			m_wBody.SetText("");

		// ToUpper mutates in place and returns a length, the same trap as
		// Replace. Assigning its result is a compile error, and calling it
		// inline silently passes an int.
		string speakerLabel = m_sSpeakerName;
		speakerLabel.ToUpper();

		AddToTranscript(speakerLabel, view.m_sText);

		ClearChoices();

		foreach (MCF_Dialogue_ChoiceView choice : view.m_aChoices)
			AddChoice(choice);

		if (m_wHint)
		{
			if (view.m_bEnded)
				m_wHint.SetText("There is nothing more to say.");
			else
				m_wHint.SetText("");
		}

		if (m_ButtonClose)
		{
			if (view.m_bEnded)
				m_ButtonClose.SetText("LEAVE");
			else
				m_ButtonClose.SetText("WALK AWAY");
		}

		// Put the cursor on the first thing that can actually be pressed, so
		// a gamepad has somewhere to start without hunting.
		FocusFirstEnabled();
	}

	// --------------------------------------------------------------- choices

	protected void ClearChoices()
	{
		if (m_wChoiceList)
		{
			while (m_wChoiceList.GetChildren())
			{
				m_wChoiceList.GetChildren().RemoveFromHierarchy();
			}
		}

		m_aChoiceWidgets.Clear();
		m_aChoiceButtons.Clear();
		m_aChoiceIndices.Clear();
		m_aChoiceTexts.Clear();
		m_aChoiceEnabled.Clear();
	}

	protected void AddChoice(notnull MCF_Dialogue_ChoiceView choice)
	{
		if (!m_wChoiceList)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wChoiceList);
		if (!row)
			return;

		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!button)
			return;

		// Numbered the way a conversation is read down, not by the index the
		// server uses -- those are the same today and need not stay so.
		string label = (m_aChoiceButtons.Count() + 1).ToString() + ".  " + choice.m_sText;

		// The reason rides on the line itself rather than in a tooltip. A
		// tooltip is a thing you have to discover; this is a thing you read.
		if (!choice.m_bEnabled && !choice.m_sLockedReason.IsEmpty())
			label = label + "      [ " + choice.m_sLockedReason + " ]";

		button.SetText(label);
		button.SetEnabled(choice.m_bEnabled, false);
		button.m_OnClicked.Insert(OnChoiceClicked);

		m_aChoiceWidgets.Insert(row);
		m_aChoiceButtons.Insert(button);
		m_aChoiceIndices.Insert(choice.m_iIndex);
		m_aChoiceTexts.Insert(choice.m_sText);
		m_aChoiceEnabled.Insert(choice.m_bEnabled);
	}

	protected void FocusFirstEnabled()
	{
		foreach (int i, bool enabled : m_aChoiceEnabled)
		{
			if (enabled)
			{
				GetGame().GetWorkspace().SetFocusedWidget(m_aChoiceWidgets[i]);
				return;
			}
		}
	}

	protected void OnChoiceClicked(SCR_ButtonTextComponent button)
	{
		int row = m_aChoiceButtons.Find(button);
		if (row < 0)
			return;

		if (!m_aChoiceEnabled[row])
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		// Written into the transcript here rather than when the server
		// answers, because the player said it the moment they pressed it and
		// a reply that appears half a second late reads as lag.
		AddToTranscript("YOU", m_aChoiceTexts[row]);

		// The choices are cleared but the screen is not otherwise advanced.
		// The server answers with the next one, and letting the client guess
		// would mean two versions of the conversation whenever the server
		// refuses a reply.
		int index = m_aChoiceIndices[row];
		ClearChoices();

		controller.MCF_RequestDialogueChoose(m_SpeakerId, m_sNodeId, index);
	}

	// ------------------------------------------------------------ head look

	//! Asks the person to watch us while we are talking to them.
	//!
	//! Driven from the client and not the server, because this is a visual and
	//! the only person it has to be right for is the one standing in front of
	//! them. KNOWN LIMIT: a third player watching the conversation from a
	//! distance does not see the head turn. The body turn they do see, because
	//! that goes through the server and the entity's own replication.
	protected void StartLook()
	{
		MCF_Dialogue_Component speaker = ResolveSpeaker();
		if (speaker)
			speaker.StartLookingAtLocalPlayer();
	}

	protected void StopLook()
	{
		MCF_Dialogue_Component speaker = ResolveSpeaker();
		if (speaker)
			speaker.StopLookingAtLocalPlayer();
	}

	//! HARD-WON, and the same on both sides of the wire: Replication.FindItem
	//! takes and returns the replicated COMPONENT, and for MCF that component
	//! is SCR_EditableEntityComponent -- RplComponent exposes no GetOwner() to
	//! script, so there is no way back from it to the person.
	protected MCF_Dialogue_Component ResolveSpeaker()
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(m_SpeakerId));
		if (!editable)
			return null;

		IEntity speaker = editable.GetOwner();
		if (!speaker)
			return null;

		return MCF_Dialogue_Component.Cast(speaker.FindComponent(MCF_Dialogue_Component));
	}

	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}
}
