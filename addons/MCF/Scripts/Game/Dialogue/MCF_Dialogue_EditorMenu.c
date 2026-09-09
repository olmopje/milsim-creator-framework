//! Where a Game Master writes what somebody says and what you can say back.
//!
//! Opened from the right-click menu on a person, never from the world -- this
//! is authoring, not play.
//!
//! WHAT IT EDITS IS A LIBRARY CONVERSATION, not this one person's script. That
//! was a deliberate choice: writing on the person is quicker to build and
//! quicker to regret, because the same words then have to be typed again for
//! the next ten civilians and are lost when the first one dies. A library
//! entry has a name, can be given to a hundred people with the Conversation
//! slider, outlives everybody, and is saved with the mission. The person you
//! opened it on is simply who it is assigned to when you save.
//!
//! THREE LEVELS, TWO LISTS. A conversation is nodes; a node is one thing they
//! say plus the replies to it. The left list is the nodes, the right list is
//! the selected node's replies, and the fields below each belong to whatever
//! is selected above them.
//!
//! EVERYTHING IS HELD LOCALLY UNTIL SAVE. A Game Master rewriting four nodes
//! commits them as one change rather than sending an RPC per keystroke, and
//! backing out costs nothing.

class MCF_Dialogue_EditorMenu : ChimeraMenuBase
{
	protected static const ResourceName ENTRY_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string W_STATUS = "Status";
	protected static const string W_LIBRARY_LIST = "LibraryList";
	protected static const string W_NODE_LIST = "NodeList";
	protected static const string W_CHOICE_LIST = "ChoiceList";

	protected static const string W_EDIT_ID = "EditId";
	protected static const string W_EDIT_SPEAKER = "EditSpeaker";
	protected static const string W_EDIT_VERB = "EditVerb";
	protected static const string W_EDIT_NODE_ID = "EditNodeId";
	protected static const string W_EDIT_NODE_TEXT = "EditNodeText";
	protected static const string W_EDIT_CHOICE_TEXT = "EditChoiceText";
	protected static const string W_EDIT_CHOICE_NEXT = "EditChoiceNext";
	protected static const string W_EDIT_CHOICE_EVENT = "EditChoiceEvent";
	protected static const string W_EDIT_CHOICE_TRUST = "EditChoiceTrust";
	protected static const string W_EDIT_CHOICE_FEAR = "EditChoiceFear";
	protected static const string W_EDIT_CHOICE_MIN_TRUST = "EditChoiceMinTrust";
	protected static const string W_EDIT_CHOICE_MAX_FEAR = "EditChoiceMaxFear";
	protected static const string W_EDIT_CHOICE_NEEDS_FLAG = "EditChoiceNeedsFlag";
	protected static const string W_EDIT_CHOICE_SETS_FLAG = "EditChoiceSetsFlag";
	protected static const string W_EDIT_CHOICE_LOCKED = "EditChoiceLocked";

	protected static const string W_BUTTON_ADD_NODE = "ButtonAddNode";
	protected static const string W_BUTTON_REMOVE_NODE = "ButtonRemoveNode";
	protected static const string W_BUTTON_ADD_CHOICE = "ButtonAddChoice";
	protected static const string W_BUTTON_REMOVE_CHOICE = "ButtonRemoveChoice";
	protected static const string W_BUTTON_NEW = "ButtonNew";
	protected static const string W_BUTTON_DELETE = "ButtonDelete";
	protected static const string W_BUTTON_SAVE = "ButtonSave";
	protected static const string W_BUTTON_SAVE_NEW = "ButtonSaveNew";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	protected static MCF_Dialogue_EditorMenu s_Instance;
	protected static MCF_Dialogue_Component s_PendingDialogue;
	protected static SCR_EditableEntityComponent s_PendingEditable;

	protected MCF_Dialogue_Component m_Dialogue;

	//! The wire identity of the person. It is the EDITABLE COMPONENT, not the
	//! entity: Replication.FindItemId resolves replicated items, and an
	//! IEntity is not one.
	protected SCR_EditableEntityComponent m_Editable;

	protected TextWidget m_wStatus;
	protected VerticalLayoutWidget m_wLibraryList;
	protected VerticalLayoutWidget m_wNodeList;
	protected VerticalLayoutWidget m_wChoiceList;

	protected SCR_EditBoxComponent m_EditId;
	protected SCR_EditBoxComponent m_EditSpeaker;
	protected SCR_EditBoxComponent m_EditVerb;
	protected SCR_EditBoxComponent m_EditNodeId;
	protected SCR_EditBoxComponent m_EditNodeText;
	protected SCR_EditBoxComponent m_EditChoiceText;
	protected SCR_EditBoxComponent m_EditChoiceNext;
	protected SCR_EditBoxComponent m_EditChoiceEvent;
	protected SCR_EditBoxComponent m_EditChoiceTrust;
	protected SCR_EditBoxComponent m_EditChoiceFear;
	protected SCR_EditBoxComponent m_EditChoiceMinTrust;
	protected SCR_EditBoxComponent m_EditChoiceMaxFear;
	protected SCR_EditBoxComponent m_EditChoiceNeedsFlag;
	protected SCR_EditBoxComponent m_EditChoiceSetsFlag;
	protected SCR_EditBoxComponent m_EditChoiceLocked;

	protected SCR_ButtonTextComponent m_ButtonSave;
	protected SCR_ButtonTextComponent m_ButtonSaveNew;

	protected ref array<SCR_ButtonTextComponent> m_aLibraryButtons = {};
	protected ref array<string> m_aLibraryIds = {};
	protected ref array<bool> m_aLibraryShipped = {};

	protected ref array<SCR_ButtonTextComponent> m_aNodeButtons = {};
	protected ref array<Widget> m_aNodeWidgets = {};
	protected ref array<SCR_ButtonTextComponent> m_aChoiceButtons = {};
	protected ref array<Widget> m_aChoiceWidgets = {};

	//! The working copy.
	protected ref MCF_Dialogue_Conversation m_Draft;
	protected int m_iNode = -1;
	protected int m_iChoice = -1;

	static MCF_Dialogue_EditorMenu OpenFor(notnull MCF_Dialogue_Component dialogue, SCR_EditableEntityComponent editable)
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return null;

		s_PendingDialogue = dialogue;
		s_PendingEditable = editable;

		// OpenDialog, not OpenMenu: it sits inside the Game Master editor the
		// way the attributes window does, instead of taking the whole screen
		// and hiding the person being written for.
		MCF_Dialogue_EditorMenu menu = MCF_Dialogue_EditorMenu.Cast(menuManager.OpenDialog(ChimeraMenuPreset.MCF_DialogueEditor, DialogPriority.INFORMATIVE, 0, true));
		if (!menu)
		{
			s_PendingDialogue = null;
			s_PendingEditable = null;
		}

		return menu;
	}

	//! Called when the server sends back the library index.
	static void ReceiveList(string packed)
	{
		if (s_Instance)
			s_Instance.LoadLibrary(packed);
	}

	//! Called when the server sends back the conversation being edited.
	//! Static because the RPC handler is on the player controller.
	static void ReceiveConversation(string data)
	{
		if (s_Instance)
			s_Instance.LoadDraftFrom(data);
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		s_Instance = this;
		m_Dialogue = s_PendingDialogue;
		m_Editable = s_PendingEditable;
		s_PendingDialogue = null;
		s_PendingEditable = null;

		Widget root = GetRootWidget();
		if (!root || !m_Dialogue)
		{
			MCF_Core_Log.Warn("conversation editor opened with nobody to write for");
			return;
		}

		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wLibraryList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_LIBRARY_LIST));
		m_wNodeList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_NODE_LIST));
		m_wChoiceList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_CHOICE_LIST));

		m_EditId = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_ID, root);
		m_EditSpeaker = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_SPEAKER, root);
		m_EditVerb = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_VERB, root);
		m_EditNodeId = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_NODE_ID, root);
		m_EditNodeText = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_NODE_TEXT, root);
		m_EditChoiceText = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_TEXT, root);
		m_EditChoiceNext = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_NEXT, root);
		m_EditChoiceEvent = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_EVENT, root);
		m_EditChoiceTrust = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_TRUST, root);
		m_EditChoiceFear = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_FEAR, root);
		m_EditChoiceMinTrust = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_MIN_TRUST, root);
		m_EditChoiceMaxFear = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_MAX_FEAR, root);
		m_EditChoiceNeedsFlag = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_NEEDS_FLAG, root);
		m_EditChoiceSetsFlag = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_SETS_FLAG, root);
		m_EditChoiceLocked = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_CHOICE_LOCKED, root);

		// Wired one by one rather than through a helper that takes the handler
		// as a parameter. Passing a method as an argument is not something
		// this project has proven in Enforce, and a button that silently does
		// nothing is a bad thing to discover in a live session.
		SCR_ButtonTextComponent buttonAddNode = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_ADD_NODE, root);
		if (buttonAddNode)
			buttonAddNode.m_OnClicked.Insert(OnAddNode);

		SCR_ButtonTextComponent buttonRemoveNode = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_REMOVE_NODE, root);
		if (buttonRemoveNode)
			buttonRemoveNode.m_OnClicked.Insert(OnRemoveNode);

		SCR_ButtonTextComponent buttonAddChoice = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_ADD_CHOICE, root);
		if (buttonAddChoice)
			buttonAddChoice.m_OnClicked.Insert(OnAddChoice);

		SCR_ButtonTextComponent buttonRemoveChoice = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_REMOVE_CHOICE, root);
		if (buttonRemoveChoice)
			buttonRemoveChoice.m_OnClicked.Insert(OnRemoveChoice);


		m_ButtonSave = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_SAVE, root);
		if (m_ButtonSave)
			m_ButtonSave.m_OnClicked.Insert(OnSave);

		m_ButtonSaveNew = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_SAVE_NEW, root);
		if (m_ButtonSaveNew)
			m_ButtonSaveNew.m_OnClicked.Insert(OnSaveAsNew);

		SCR_ButtonTextComponent buttonNew = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_NEW, root);
		if (buttonNew)
			buttonNew.m_OnClicked.Insert(OnNewConversation);

		SCR_ButtonTextComponent buttonDelete = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_DELETE, root);
		if (buttonDelete)
			buttonDelete.m_OnClicked.Insert(OnDeleteConversation);

		SCR_ButtonTextComponent buttonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (buttonClose)
			buttonClose.m_OnClicked.Insert(OnCloseClicked);


		RequestOrStartDraft();
		RequestLibrary();
	}


	override void OnMenuClose()
	{
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

	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}

	// ---------------------------------------------------------------- draft

	//! Asks the server for the conversation this person is running, or starts
	//! a blank one.
	//!
	//! ASKED, NOT READ LOCALLY. A Game Master's own machine has the shipped
	//! config but not conversations written this session on a dedicated
	//! server, so reading the local library would quietly open an empty editor
	//! over somebody's existing words and save over them.
	protected void RequestOrStartDraft()
	{
		string current = m_Dialogue.GetConversationId();

		if (current.IsEmpty())
		{
			StartBlankDraft();
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
		{
			StartBlankDraft();
			return;
		}

		SetStatus("Fetching '" + current + "'...");
		controller.MCF_RequestConversationText(current);
	}

	// -------------------------------------------------------------- library

	protected void RequestLibrary()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.MCF_RequestConversationList();
	}

	//! Draws the library index. A leading '*' marks a conversation that ships
	//! with the mod, which can be opened and copied but not deleted.
	void LoadLibrary(string packed)
	{
		Clear(m_wLibraryList);
		m_aLibraryButtons.Clear();
		m_aLibraryIds.Clear();
		m_aLibraryShipped.Clear();

		array<string> entries = {};
		packed.Split(",", entries, true);

		foreach (string entry : entries)
		{
			if (entry.IsEmpty())
				continue;

			bool shipped = entry.StartsWith("*");

			string id = entry;
			if (shipped)
				id = entry.Substring(1, entry.Length() - 1);

			Widget row = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wLibraryList);
			if (!row)
				continue;

			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!button)
				continue;

			string label = id;
			if (shipped)
				label = id + "   [mod]";

			button.SetText(label);
			button.m_OnClicked.Insert(OnLibraryClicked);

			m_aLibraryButtons.Insert(button);
			m_aLibraryIds.Insert(id);
			m_aLibraryShipped.Insert(shipped);
		}
	}

	protected void OnLibraryClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aLibraryButtons.Find(button);
		if (index < 0)
			return;

		foreach (int i, SCR_ButtonTextComponent row : m_aLibraryButtons)
		{
			row.SetToggled(i == index, false, false);
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		// Fetched rather than opened from a local copy, for the same reason
		// the first load is: this machine may not have what the server has.
		SetStatus("Fetching '" + m_aLibraryIds[index] + "'...");
		controller.MCF_RequestConversationText(m_aLibraryIds[index]);
	}

	protected void OnNewConversation(SCR_ButtonTextComponent button)
	{
		StartBlankDraft();
		SetStatus("New conversation. Give it a name, then SAVE.");
	}

	protected void OnDeleteConversation(SCR_ButtonTextComponent button)
	{
		if (!m_Draft)
			return;

		int index = m_aLibraryIds.Find(m_Draft.m_sId);

		if (index >= 0 && m_aLibraryShipped[index])
		{
			SetStatus("'" + m_Draft.m_sId + "' ships with the mod and cannot be deleted.");
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		controller.MCF_RequestDeleteConversation(m_Draft.m_sId);
		SetStatus("Deleting '" + m_Draft.m_sId + "'...");
	}

	protected void StartBlankDraft()
	{
		m_Draft = new MCF_Dialogue_Conversation();
		m_Draft.m_sId = "conversation";
		m_Draft.m_sSpeakerName = m_Dialogue.GetSpeakerName();
		m_Draft.m_sActionVerb = m_Dialogue.GetActionVerb();
		m_Draft.m_sStartNodeId = "start";
		m_Draft.m_aNodes = {};

		MCF_Dialogue_Node node = new MCF_Dialogue_Node();
		node.m_sId = "start";
		node.m_sText = "";
		node.m_aChoices = {};
		m_Draft.m_aNodes.Insert(node);

		ShowHeader();
		m_iNode = 0;
		RebuildNodes();
		SetStatus("New conversation. Nothing is saved until you press SAVE.");
	}

	void LoadDraftFrom(string data)
	{
		MCF_Dialogue_Conversation loaded = MCF_Dialogue_Script.Deserialize(data);

		if (!loaded)
		{
			StartBlankDraft();
			return;
		}

		m_Draft = loaded;
		if (!m_Draft.m_aNodes)
			m_Draft.m_aNodes = {};

		ShowHeader();
		m_iNode = 0;
		RebuildNodes();
		SetStatus("Editing '" + m_Draft.m_sId + "'. Nothing is saved until you press SAVE.");
	}

	protected void ShowHeader()
	{
		SetValue(m_EditId, m_Draft.m_sId);
		SetValue(m_EditSpeaker, m_Draft.m_sSpeakerName);
		SetValue(m_EditVerb, m_Draft.m_sActionVerb);
	}

	//! Pulls what is typed back into the working copy. Called before anything
	//! that changes which node or reply is shown, so switching does not
	//! quietly discard what was just typed.
	protected void Capture()
	{
		if (!m_Draft)
			return;

		if (m_EditId)
			m_Draft.m_sId = m_EditId.GetValue();

		if (m_EditSpeaker)
			m_Draft.m_sSpeakerName = m_EditSpeaker.GetValue();

		if (m_EditVerb)
			m_Draft.m_sActionVerb = m_EditVerb.GetValue();

		MCF_Dialogue_Node node = CurrentNode();
		if (node)
		{
			if (m_EditNodeId)
				node.m_sId = m_EditNodeId.GetValue();

			if (m_EditNodeText)
				node.m_sText = m_EditNodeText.GetValue();
		}

		MCF_Dialogue_Choice choice = CurrentChoice();
		if (!choice)
			return;

		if (m_EditChoiceText)
			choice.m_sText = m_EditChoiceText.GetValue();

		if (m_EditChoiceNext)
			choice.m_sNextNodeId = m_EditChoiceNext.GetValue();

		if (m_EditChoiceEvent)
			choice.m_sPublishEvent = m_EditChoiceEvent.GetValue();

		if (m_EditChoiceTrust)
			choice.m_fTrustChange = m_EditChoiceTrust.GetValue().ToFloat();

		if (m_EditChoiceFear)
			choice.m_fFearChange = m_EditChoiceFear.GetValue().ToFloat();

		if (m_EditChoiceMinTrust)
			choice.m_fMinTrust = m_EditChoiceMinTrust.GetValue().ToFloat();

		if (m_EditChoiceMaxFear)
			choice.m_fMaxFear = m_EditChoiceMaxFear.GetValue().ToFloat();

		if (m_EditChoiceNeedsFlag)
			choice.m_sRequiresFlag = m_EditChoiceNeedsFlag.GetValue();

		if (m_EditChoiceSetsFlag)
			choice.m_sSetFlag = m_EditChoiceSetsFlag.GetValue();

		if (m_EditChoiceLocked)
			choice.m_sLockedReason = m_EditChoiceLocked.GetValue();
	}

	protected MCF_Dialogue_Node CurrentNode()
	{
		if (!m_Draft || !m_Draft.m_aNodes || m_iNode < 0 || m_iNode >= m_Draft.m_aNodes.Count())
			return null;

		return m_Draft.m_aNodes[m_iNode];
	}

	protected MCF_Dialogue_Choice CurrentChoice()
	{
		MCF_Dialogue_Node node = CurrentNode();
		if (!node || !node.m_aChoices || m_iChoice < 0 || m_iChoice >= node.m_aChoices.Count())
			return null;

		return node.m_aChoices[m_iChoice];
	}

	// ---------------------------------------------------------------- lists

	protected void RebuildNodes()
	{
		Clear(m_wNodeList);
		m_aNodeButtons.Clear();
		m_aNodeWidgets.Clear();

		if (!m_Draft || !m_Draft.m_aNodes)
			return;

		foreach (int i, MCF_Dialogue_Node node : m_Draft.m_aNodes)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wNodeList);
			if (!row)
				continue;

			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!button)
				continue;

			button.SetText(NodeLabel(i, node));
			button.m_OnClicked.Insert(OnNodeClicked);

			m_aNodeButtons.Insert(button);
			m_aNodeWidgets.Insert(row);
		}

		if (m_iNode < 0 || m_iNode >= m_aNodeButtons.Count())
			m_iNode = 0;

		SelectNode(m_iNode);
	}

	protected string NodeLabel(int index, MCF_Dialogue_Node node)
	{
		string id = node.m_sId;
		if (id.IsEmpty())
			id = "(no id)";

		// The start node is marked, because a conversation whose start id does
		// not match any node opens on nothing and the author has no other way
		// to see it.
		if (m_Draft && id == m_Draft.m_sStartNodeId)
			id = id + "   [START]";

		return id;
	}

	protected void OnNodeClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aNodeButtons.Find(button);
		if (index < 0)
			return;

		Capture();
		m_iChoice = -1;
		SelectNode(index);
		RefreshNodeLabels();
	}

	protected void SelectNode(int index)
	{
		m_iNode = index;

		foreach (int i, SCR_ButtonTextComponent button : m_aNodeButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		MCF_Dialogue_Node node = CurrentNode();

		if (node)
		{
			SetValue(m_EditNodeId, node.m_sId);
			SetValue(m_EditNodeText, node.m_sText);
		}
		else
		{
			SetValue(m_EditNodeId, "");
			SetValue(m_EditNodeText, "");
		}

		RebuildChoices();
	}

	protected void RefreshNodeLabels()
	{
		if (!m_Draft || !m_Draft.m_aNodes)
			return;

		foreach (int i, SCR_ButtonTextComponent button : m_aNodeButtons)
		{
			if (i < m_Draft.m_aNodes.Count())
				button.SetText(NodeLabel(i, m_Draft.m_aNodes[i]));
		}
	}

	protected void RebuildChoices()
	{
		Clear(m_wChoiceList);
		m_aChoiceButtons.Clear();
		m_aChoiceWidgets.Clear();

		MCF_Dialogue_Node node = CurrentNode();
		if (!node || !node.m_aChoices)
		{
			ShowChoiceFields(null);
			return;
		}

		foreach (int i, MCF_Dialogue_Choice choice : node.m_aChoices)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wChoiceList);
			if (!row)
				continue;

			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
			if (!button)
				continue;

			button.SetText(ChoiceLabel(i, choice));
			button.m_OnClicked.Insert(OnChoiceClicked);

			m_aChoiceButtons.Insert(button);
			m_aChoiceWidgets.Insert(row);
		}

		if (m_iChoice < 0 || m_iChoice >= m_aChoiceButtons.Count())
			m_iChoice = 0;

		SelectChoice(m_iChoice);
	}

	protected string ChoiceLabel(int index, MCF_Dialogue_Choice choice)
	{
		string text = choice.m_sText;
		if (text.IsEmpty())
			text = "(empty reply)";

		return (index + 1).ToString() + ".  " + text;
	}

	protected void OnChoiceClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aChoiceButtons.Find(button);
		if (index < 0)
			return;

		Capture();
		SelectChoice(index);
		RefreshChoiceLabels();
	}

	protected void SelectChoice(int index)
	{
		m_iChoice = index;

		foreach (int i, SCR_ButtonTextComponent button : m_aChoiceButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		ShowChoiceFields(CurrentChoice());
	}

	protected void RefreshChoiceLabels()
	{
		MCF_Dialogue_Node node = CurrentNode();
		if (!node || !node.m_aChoices)
			return;

		foreach (int i, SCR_ButtonTextComponent button : m_aChoiceButtons)
		{
			if (i < node.m_aChoices.Count())
				button.SetText(ChoiceLabel(i, node.m_aChoices[i]));
		}
	}

	protected void ShowChoiceFields(MCF_Dialogue_Choice choice)
	{
		if (!choice)
		{
			SetValue(m_EditChoiceText, "");
			SetValue(m_EditChoiceNext, "");
			SetValue(m_EditChoiceEvent, "");
			SetValue(m_EditChoiceTrust, "0");
			SetValue(m_EditChoiceFear, "0");
			SetValue(m_EditChoiceMinTrust, "0");
			SetValue(m_EditChoiceMaxFear, "100");
			SetValue(m_EditChoiceNeedsFlag, "");
			SetValue(m_EditChoiceSetsFlag, "");
			SetValue(m_EditChoiceLocked, "");
			return;
		}

		SetValue(m_EditChoiceText, choice.m_sText);
		SetValue(m_EditChoiceNext, choice.m_sNextNodeId);
		SetValue(m_EditChoiceEvent, choice.m_sPublishEvent);
		SetValue(m_EditChoiceTrust, choice.m_fTrustChange.ToString());
		SetValue(m_EditChoiceFear, choice.m_fFearChange.ToString());
		SetValue(m_EditChoiceMinTrust, choice.m_fMinTrust.ToString());
		SetValue(m_EditChoiceMaxFear, choice.m_fMaxFear.ToString());
		SetValue(m_EditChoiceNeedsFlag, choice.m_sRequiresFlag);
		SetValue(m_EditChoiceSetsFlag, choice.m_sSetFlag);
		SetValue(m_EditChoiceLocked, choice.m_sLockedReason);
	}

	protected void Clear(VerticalLayoutWidget list)
	{
		if (!list)
			return;

		while (list.GetChildren())
		{
			list.GetChildren().RemoveFromHierarchy();
		}
	}

	protected void SetValue(SCR_EditBoxComponent editor, string value)
	{
		if (editor)
			editor.SetValue(value);
	}

	protected void SetStatus(string text)
	{
		if (m_wStatus)
			m_wStatus.SetText(text);
	}

	// -------------------------------------------------------------- editing

	protected void OnAddNode(SCR_ButtonTextComponent button)
	{
		Capture();

		if (!m_Draft)
			return;

		MCF_Dialogue_Node node = new MCF_Dialogue_Node();
		node.m_sId = "node_" + (m_Draft.m_aNodes.Count() + 1).ToString();
		node.m_aChoices = {};
		m_Draft.m_aNodes.Insert(node);

		m_iNode = m_Draft.m_aNodes.Count() - 1;
		m_iChoice = -1;
		RebuildNodes();
	}

	protected void OnRemoveNode(SCR_ButtonTextComponent button)
	{
		if (!m_Draft || !m_Draft.m_aNodes || m_iNode < 0 || m_iNode >= m_Draft.m_aNodes.Count())
			return;

		// The last node is never removed. A conversation with no nodes cannot
		// be opened, and an author who saved one would have to start over.
		if (m_Draft.m_aNodes.Count() <= 1)
		{
			SetStatus("A conversation needs at least one thing to say.");
			return;
		}

		m_Draft.m_aNodes.Remove(m_iNode);

		if (m_iNode >= m_Draft.m_aNodes.Count())
			m_iNode = m_Draft.m_aNodes.Count() - 1;

		m_iChoice = -1;
		RebuildNodes();
	}

	protected void OnAddChoice(SCR_ButtonTextComponent button)
	{
		Capture();

		MCF_Dialogue_Node node = CurrentNode();
		if (!node)
			return;

		if (!node.m_aChoices)
			node.m_aChoices = {};

		MCF_Dialogue_Choice choice = new MCF_Dialogue_Choice();
		choice.m_sText = "New reply";
		choice.m_fMaxFear = 100;
		node.m_aChoices.Insert(choice);

		m_iChoice = node.m_aChoices.Count() - 1;
		RebuildChoices();
	}

	protected void OnRemoveChoice(SCR_ButtonTextComponent button)
	{
		MCF_Dialogue_Node node = CurrentNode();
		if (!node || !node.m_aChoices || m_iChoice < 0 || m_iChoice >= node.m_aChoices.Count())
			return;

		node.m_aChoices.Remove(m_iChoice);

		if (m_iChoice >= node.m_aChoices.Count())
			m_iChoice = node.m_aChoices.Count() - 1;

		RebuildChoices();
	}

	// --------------------------------------------------------------- saving

	protected void OnSave(SCR_ButtonTextComponent button)
	{
		Send(false);
	}

	//! Saves under a fresh id, so the shipped conversation this was opened
	//! from stays as it was.
	protected void OnSaveAsNew(SCR_ButtonTextComponent button)
	{
		Send(true);
	}

	protected void Send(bool asNew)
	{
		Capture();

		if (!m_Draft)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		if (m_Draft.m_sId.IsEmpty())
		{
			SetStatus("Give the conversation a name first.");
			return;
		}

		// The start node has to exist or the conversation opens on nothing.
		// Caught here rather than at the person's feet ten minutes later.
		if (!HasNode(m_Draft.m_sStartNodeId))
		{
			MCF_Dialogue_Node first = m_Draft.m_aNodes[0];
			m_Draft.m_sStartNodeId = first.m_sId;
			SetStatus("Start set to '" + first.m_sId + "', the first node.");
			RefreshNodeLabels();
		}

		if (asNew)
			m_Draft.m_sId = m_Draft.m_sId + "_copy";

		RplId assignTo = RplId.Invalid();
		if (m_Editable)
			assignTo = Replication.FindItemId(m_Editable);

		controller.MCF_RequestSaveConversation(MCF_Dialogue_Script.Serialize(m_Draft), assignTo);

		SetValue(m_EditId, m_Draft.m_sId);
		SetStatus("Sent '" + m_Draft.m_sId + "' to the server.");

		// The index may have gained an entry, so redraw it.
		RequestLibrary();
	}

	protected bool HasNode(string nodeId)
	{
		if (nodeId.IsEmpty() || !m_Draft || !m_Draft.m_aNodes || m_Draft.m_aNodes.IsEmpty())
			return false;

		foreach (MCF_Dialogue_Node node : m_Draft.m_aNodes)
		{
			if (node && node.m_sId == nodeId)
				return true;
		}

		return false;
	}
}
