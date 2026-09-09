//! The operations board -- the command centre a player opens from the
//! physical board in the world.
//!
//! Left: the taskings this player is entitled to see. Right: the selected one
//! as a five-paragraph operation order. Bottom: what they may do about it.
//!
//! READ AND AMEND ARE SEPARATE MODES, and that is the main lesson from the
//! first version. Showing the rendered order and the six input fields at once
//! meant the same text twice on one screen, the order pushed off the top, and
//! a board that looked like a form instead of a plan. Most people who open
//! this are reading; only a commander occasionally writes. So it opens in read
//! mode and AMEND swaps the pane.
//!
//! ON THE WORDS. The enum names in the code are engineering words and they do
//! not belong on a board a unit reads. PUBLISHED in particular is wrong to a
//! soldier: what it means is that an order is posted and nobody has picked it
//! up, which is OPEN. Orders carry a reference (OPORD-003) because units refer
//! to orders by number on the net, not by their subject line. The paragraph
//! headings are the real five-paragraph order, numbered as a unit numbers
//! them. See MCF_Task.StateLabel and MCF_Task.GetReference.
//!
//! ON THE MENU REGISTRATION CHAIN. All of it is verified in a running game,
//! which is worth recording because most of it had no vanilla precedent:
//!
//!   - `modded enum ChimeraMenuPreset` compiles and the engine links the new
//!     member to a config entry BY NAME, as menuManagerDoc.c says it does.
//!   - Configs/System/chimeraMenus.conf is overridden by giving MCF's copy the
//!     vanilla GUID, and `MenuPresets +{ ... }` APPENDS: all 58 vanilla
//!     presets survive and MCF's is added, so MCF does not go stale when
//!     Bohemia adds a menu.
//!   - Vanilla's own documented route (override Game.GetMenuPreset) is NOT
//!     open to a mod -- ChimeraGame is generated and Reforger already spent
//!     that hook on ChimeraMenuPreset.
//!
//! ON THE CURSOR AND MOVEMENT: nothing here turns the cursor on or stops the
//! player moving. `ActionContext "MenuContext"` on the preset does both --
//! priority 50 with no Overlay flag against character contexts at 10, so the
//! character receives no input while the board is open.
//!
//! ON CONTROLLER SUPPORT, designed in rather than bolted on:
//!
//!   - Every interactive element is an SCR_ButtonTextComponent or an inherited
//!     WLib_EditBox, not a bare widget with a click handler. The WidgetLibrary
//!     components carry the focus and navigation handling a stick needs; a
//!     hand-rolled one is mouse-only and unreachable on a pad.
//!   - Both scroll panes carry SCR_GamepadScrollComponent.
//!   - A row is focused explicitly on selection, so a pad has a start point.
//!   - Buttons are disabled rather than hidden, so their positions never move
//!     under a player navigating by direction.
//!
//! ON READING THE LIST: the board draws from GetTasksVisibleTo, never from
//! GetAllTasks or Count. On a hosted server the client mirror and the
//! authoritative store are the same singleton, so a host reading the raw store
//! sees every order in the mission including other people's.

//! Adds MCF's screens to the game's menu preset list.
//!
//! WARNING: a value appended by a modded enum takes the next free ordinal,
//! which depends on what else is loaded. Never persist this number and never
//! send it over the wire -- refer to it only by name.
class MCF_PlanningBoardMenu : ChimeraMenuBase
{
	protected static const ResourceName TASK_ENTRY_LAYOUT = "{6A1C4F0B39D2B000}UI/layouts/MCF/MCF_PlanningBoard_TaskEntry.layout";

	protected static const string W_STATUS = "Status";
	protected static const string W_TASK_LIST = "TaskList";
	protected static const string W_DETAIL_BODY = "DetailBody";
	protected static const string W_EDIT_PANE = "EditPane";

	protected static const string W_BUTTON_NEW = "ButtonNew";
	protected static const string W_BUTTON_EDIT = "ButtonEdit";
	protected static const string W_BUTTON_SAVE = "ButtonSave";
	protected static const string W_BUTTON_ACCEPT = "ButtonAccept";
	protected static const string W_BUTTON_PUBLISH = "ButtonPublish";
	protected static const string W_BUTTON_COMPLETE = "ButtonComplete";
	protected static const string W_BUTTON_CANCEL = "ButtonCancel";
	protected static const string W_BUTTON_DELETE = "ButtonDelete";
	protected static const string W_BUTTON_MODE_TASKS = "ButtonModeTasks";
	protected static const string W_BUTTON_MODE_INTEL = "ButtonModeIntel";
	protected static const string W_BUTTON_DRAFT_FROM_INTEL = "ButtonDraftFromIntel";
	protected static const string W_BUTTON_CLOSE = "ButtonClose";

	protected static const string W_EDIT_TITLE     = "EditTitle";
	protected static const string W_EDIT_SITUATION = "EditSituation";
	protected static const string W_EDIT_MISSION   = "EditMission";
	protected static const string W_EDIT_EXECUTION = "EditExecution";
	protected static const string W_EDIT_ADMIN     = "EditAdmin";
	protected static const string W_EDIT_SIGNAL    = "EditSignal";

	protected TextWidget m_wStatus;
	protected VerticalLayoutWidget m_wTaskList;
	protected RichTextWidget m_wDetailBody;
	protected Widget m_wEditPane;

	protected SCR_ButtonTextComponent m_ButtonNew;
	protected SCR_ButtonTextComponent m_ButtonEdit;
	protected SCR_ButtonTextComponent m_ButtonSave;
	protected SCR_ButtonTextComponent m_ButtonAccept;
	protected SCR_ButtonTextComponent m_ButtonPublish;
	protected SCR_ButtonTextComponent m_ButtonComplete;
	protected SCR_ButtonTextComponent m_ButtonCancel;
	protected SCR_ButtonTextComponent m_ButtonDelete;
	protected SCR_ButtonTextComponent m_ButtonModeTasks;
	protected SCR_ButtonTextComponent m_ButtonModeIntel;
	protected SCR_ButtonTextComponent m_ButtonDraftFromIntel;
	protected SCR_ButtonTextComponent m_ButtonClose;

	protected SCR_EditBoxComponent m_EditTitle;
	protected SCR_EditBoxComponent m_EditSituation;
	protected SCR_EditBoxComponent m_EditMission;
	protected SCR_EditBoxComponent m_EditExecution;
	protected SCR_EditBoxComponent m_EditAdmin;
	protected SCR_EditBoxComponent m_EditSignal;

	protected ref array<string> m_aRowTaskIds = {};
	protected ref array<SCR_ButtonTextComponent> m_aRowButtons = {};

	//! Kept alongside the components because SCR_WLibComponentBase holds its
	//! root widget in a protected field with no public getter, and focusing a
	//! row needs the widget itself.
	protected ref array<Widget> m_aRowWidgets = {};

	protected string m_sSelectedTaskId;
	protected string m_sSelectedIntelId;

	//! The board shows one list at a time. Taskings and intel share the list
	//! and detail widgets rather than each getting their own pane, because two
	//! half-height lists side by side is how a board becomes unreadable.
	protected bool m_bIntelMode;
	protected bool m_bAmending;

	//! Opens the board. Client side -- a screen is not game state.
	static MCF_PlanningBoardMenu Open()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return null;

		return MCF_PlanningBoardMenu.Cast(menuManager.OpenMenu(ChimeraMenuPreset.MCF_PlanningBoard));
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		Widget root = GetRootWidget();
		if (!root)
		{
			MCF_Core_Log.Warn("operations board opened with no root widget -- check the Layout path in chimeraMenus.conf");
			return;
		}

		m_wStatus = TextWidget.Cast(root.FindAnyWidget(W_STATUS));
		m_wTaskList = VerticalLayoutWidget.Cast(root.FindAnyWidget(W_TASK_LIST));
		m_wDetailBody = RichTextWidget.Cast(root.FindAnyWidget(W_DETAIL_BODY));
		m_wEditPane = root.FindAnyWidget(W_EDIT_PANE);

		// Bound one by one rather than through a helper taking a function
		// pointer: a bare unc parameter is not something this codebase has
		// proven compiles, and a button that silently fails to bind is exactly
		// the class of defect that has cost this project whole test rounds.
		m_ButtonNew = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_NEW, root);
		if (m_ButtonNew)
			m_ButtonNew.m_OnClicked.Insert(OnNewClicked);

		m_ButtonEdit = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_EDIT, root);
		if (m_ButtonEdit)
			m_ButtonEdit.m_OnClicked.Insert(OnAmendClicked);

		m_ButtonSave = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_SAVE, root);
		if (m_ButtonSave)
			m_ButtonSave.m_OnClicked.Insert(OnSaveClicked);

		m_ButtonAccept = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_ACCEPT, root);
		if (m_ButtonAccept)
			m_ButtonAccept.m_OnClicked.Insert(OnAcceptClicked);

		m_ButtonPublish = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_PUBLISH, root);
		if (m_ButtonPublish)
			m_ButtonPublish.m_OnClicked.Insert(OnPublishClicked);

		m_ButtonComplete = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_COMPLETE, root);
		if (m_ButtonComplete)
			m_ButtonComplete.m_OnClicked.Insert(OnCompleteClicked);

		m_ButtonCancel = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CANCEL, root);
		if (m_ButtonCancel)
			m_ButtonCancel.m_OnClicked.Insert(OnCancelClicked);

		m_ButtonDelete = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_DELETE, root);
		if (m_ButtonDelete)
			m_ButtonDelete.m_OnClicked.Insert(OnDeleteClicked);

		m_ButtonModeTasks = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_MODE_TASKS, root);
		if (m_ButtonModeTasks)
			m_ButtonModeTasks.m_OnClicked.Insert(OnModeTasksClicked);

		m_ButtonModeIntel = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_MODE_INTEL, root);
		if (m_ButtonModeIntel)
			m_ButtonModeIntel.m_OnClicked.Insert(OnModeIntelClicked);

		m_ButtonDraftFromIntel = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_DRAFT_FROM_INTEL, root);
		if (m_ButtonDraftFromIntel)
			m_ButtonDraftFromIntel.m_OnClicked.Insert(OnDraftFromIntelClicked);

		// A visible way out, because Escape is not always available: in the
		// Workbench it stops the play session instead of closing the screen.
		m_ButtonClose = SCR_ButtonTextComponent.GetButtonText(W_BUTTON_CLOSE, root);
		if (m_ButtonClose)
			m_ButtonClose.m_OnClicked.Insert(OnCloseClicked);

		m_EditTitle     = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_TITLE, root);
		m_EditSituation = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_SITUATION, root);
		m_EditMission   = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_MISSION, root);
		m_EditExecution = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_EXECUTION, root);
		m_EditAdmin     = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_ADMIN, root);
		m_EditSignal    = SCR_EditBoxComponent.GetEditBoxComponent(W_EDIT_SIGNAL, root);

		SetAmending(false);
		Refresh();
		MCF_Core_Log.Debug("operations board opened");
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

	override void OnMenuClose()
	{
		super.OnMenuClose();
		MCF_Core_Log.Debug("operations board closed");
	}

	//! Back steps out of amend mode before it closes the board, so a commander
	//! who hit AMEND by mistake does not have to reopen the whole screen.
	protected void OnBack()
	{
		if (m_bAmending)
		{
			SetAmending(false);
			ShowDetail(MCF_Task_Store.GetInstance().GetTask(m_sSelectedTaskId));
			return;
		}

		Close();
	}

	// ---------------------------------------------------------------- list

	//! Rebuilds the list from what this client currently knows. Called on open
	//! and again whenever the server pushes a change, because a board showing
	//! a stale plan is worse than one showing none.
	void Refresh()
	{
		if (!m_wTaskList)
			return;

		SyncModeButtons();

		if (m_bIntelMode)
		{
			RefreshIntel();
			return;
		}

		ClearList();

		array<MCF_Task> visible = {};
		MCF_Task_Store.GetInstance().GetTasksVisibleTo(GetLocalPlayerId(), GetLocalFactionKey(), visible);

		foreach (MCF_Task task : visible)
			AddRow(task);

		if (m_wStatus)
		{
			if (visible.IsEmpty())
				m_wStatus.SetText("No taskings held." + ViewerTag());
			else
				m_wStatus.SetText(visible.Count().ToString() + " tasking(s) held" + ViewerTag());
		}

		if (!m_sSelectedTaskId.IsEmpty() && m_aRowTaskIds.Contains(m_sSelectedTaskId))
			Select(m_sSelectedTaskId);
		else if (!m_aRowTaskIds.IsEmpty())
			Select(m_aRowTaskIds[0]);
		else
			ShowDetail(null);
	}

	protected void ClearList()
	{
		while (m_wTaskList.GetChildren())
		{
			m_wTaskList.GetChildren().RemoveFromHierarchy();
		}

		m_aRowTaskIds.Clear();
		m_aRowButtons.Clear();
		m_aRowWidgets.Clear();
	}

	protected void AddRow(notnull MCF_Task task)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets(TASK_ENTRY_LAYOUT, m_wTaskList);
		if (!row)
		{
			MCF_Core_Log.Warn("operations board could not create a tasking row -- check the entry layout path");
			return;
		}

		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!button)
		{
			MCF_Core_Log.Warn("tasking row has no SCR_ButtonTextComponent");
			return;
		}

		// Reference and state on the row itself: a board is meant to be
		// scanned, and a list of bare subject lines cannot tell you which
		// orders are still open at a glance.
		button.SetText(task.GetReference() + "  " + MCF_Task.StateLabel(task.m_eState) + "  " + task.m_sTitle);
		button.m_OnClicked.Insert(OnRowClicked);

		m_aRowTaskIds.Insert(task.m_sId);
		m_aRowButtons.Insert(button);
		m_aRowWidgets.Insert(row);
	}

	protected void OnRowClicked(SCR_ButtonTextComponent button)
	{
		int index = m_aRowButtons.Find(button);
		if (index < 0)
			return;

		// Changing which order you are looking at drops you out of amend
		// mode. Carrying half-typed text from one order onto another is the
		// kind of thing that quietly rewrites the wrong plan.
		if (m_bAmending)
			SetAmending(false);

		if (m_bIntelMode)
		{
			SelectIntel(m_aRowTaskIds[index]);
			return;
		}

		Select(m_aRowTaskIds[index]);
	}

	protected void Select(string taskId)
	{
		m_sSelectedTaskId = taskId;

		int index = m_aRowTaskIds.Find(taskId);
		foreach (int i, SCR_ButtonTextComponent button : m_aRowButtons)
		{
			// (toggled, animate, invokeChange): no animation, and no change
			// event, because this is us reflecting a selection rather than the
			// player toggling anything.
			button.SetToggled(i == index, false, false);
		}

		if (index >= 0)
			GetGame().GetWorkspace().SetFocusedWidget(m_aRowWidgets[index]);

		ShowDetail(MCF_Task_Store.GetInstance().GetTask(taskId));
	}

	// -------------------------------------------------------------- detail

	//! Renders one order the way a unit writes one.
	//!
	//! Empty paragraphs are omitted rather than shown as a numbered heading
	//! with nothing under it: a squad leader firing off a two-line retasking
	//! mid-contact should not have it padded out with four empty sections.
	protected void ShowDetail(MCF_Task task)
	{
		if (!m_wDetailBody)
			return;

		if (!task)
		{
			m_wDetailBody.SetText("No tasking selected.");
			FillEditors(null);
			UpdateButtons(null);
			return;
		}

		string body = task.GetReference() + "   " + MCF_Task.StateLabel(task.m_eState);
		body = body + "\n" + task.m_sTitle;

		body = body + Paragraph("1. SITUATION", task.m_sSituation);
		body = body + Paragraph("2. MISSION", task.m_sMission);
		body = body + Paragraph("3. EXECUTION", task.m_sExecution);
		body = body + Paragraph("4. SERVICE SUPPORT", task.m_sAdminLogistics);
		body = body + Paragraph("5. COMMAND & SIGNAL", task.m_sCommandSignal);
		body = body + Paragraph("BACK BRIEF", task.m_sBackBrief);

		m_wDetailBody.SetText(body);
		FillEditors(task);
		UpdateButtons(task);
	}

	protected string Paragraph(string heading, string text)
	{
		if (text.IsEmpty())
			return string.Empty;

		return "\n\n" + heading + "\n" + text;
	}

	// ------------------------------------------------------------- amending

	//! Swaps the right-hand pane between reading an order and writing one.
	protected void SetAmending(bool amending)
	{
		m_bAmending = amending;

		if (m_wEditPane)
			m_wEditPane.SetVisible(amending);

		if (m_wDetailBody)
			m_wDetailBody.SetVisible(!amending);

		// Enforce Script has no ternary operator -- if-then-else is not an
		// expression here.
		if (m_ButtonEdit)
		{
			if (amending)
				m_ButtonEdit.SetText("DISCARD");
			else
				m_ButtonEdit.SetText("AMEND");
		}
	}

	protected void OnAmendClicked(SCR_ButtonTextComponent button)
	{
		if (m_bAmending)
		{
			// Leaving amend mode without recording throws the typing away and
			// redraws from the authoritative copy.
			SetAmending(false);
			ShowDetail(MCF_Task_Store.GetInstance().GetTask(m_sSelectedTaskId));
			return;
		}

		SetAmending(true);
		UpdateButtons(MCF_Task_Store.GetInstance().GetTask(m_sSelectedTaskId));
	}

	//! Loads the selected order into the input fields.
	//!
	//! Filled from the task rather than left as the player last typed, because
	//! a refresh can arrive at any moment -- somebody else accepting an order
	//! re-sends everyone's list -- and keeping stale typing over freshly
	//! authoritative text is how you overwrite a change you never saw.
	protected void FillEditors(MCF_Task task)
	{
		if (!task)
		{
			SetEditorValue(m_EditTitle, string.Empty);
			SetEditorValue(m_EditSituation, string.Empty);
			SetEditorValue(m_EditMission, string.Empty);
			SetEditorValue(m_EditExecution, string.Empty);
			SetEditorValue(m_EditAdmin, string.Empty);
			SetEditorValue(m_EditSignal, string.Empty);
			return;
		}

		SetEditorValue(m_EditTitle, task.m_sTitle);
		SetEditorValue(m_EditSituation, task.m_sSituation);
		SetEditorValue(m_EditMission, task.m_sMission);
		SetEditorValue(m_EditExecution, task.m_sExecution);
		SetEditorValue(m_EditAdmin, task.m_sAdminLogistics);
		SetEditorValue(m_EditSignal, task.m_sCommandSignal);
	}

	protected void SetEditorValue(SCR_EditBoxComponent editor, string value)
	{
		if (editor)
			editor.SetValue(value);
	}

	protected string GetEditorValue(SCR_EditBoxComponent editor)
	{
		if (!editor)
			return string.Empty;

		return editor.GetValue();
	}

	// ------------------------------------------------------------- buttons

	//! Buttons reflect what the order allows and what this player may do.
	//! The permission check here is a courtesy -- the server checks again, and
	//! that is the check that counts.
	protected void UpdateButtons(MCF_Task task)
	{
		int playerId = GetLocalPlayerId();
		MCF_Task_Permissions permissions = MCF_Task_Permissions.GetInstance();

		bool isAuthor = task && task.m_iAuthorPlayerId == playerId;
		bool isAssignee = task
			&& task.m_eAssigneeType == MCF_ETaskAssignee.PLAYER
			&& task.m_sAssigneeId == playerId.ToString();

		bool mayEditThis = task && (isAuthor || permissions.Can(playerId, MCF_ETaskAction.EDIT));

		// An order is running once it is on the board and until it is closed.
		bool isRunning = task
			&& (task.m_eState == MCF_ETaskState.PUBLISHED
				|| task.m_eState == MCF_ETaskState.ASSIGNED
				|| task.m_eState == MCF_ETaskState.ACKNOWLEDGED);

		SetButtonEnabled(m_ButtonNew, permissions.Can(playerId, MCF_ETaskAction.CREATE) && !m_bAmending);
		SetButtonEnabled(m_ButtonEdit, mayEditThis);
		SetButtonEnabled(m_ButtonSave, mayEditThis && m_bAmending);

		SetButtonEnabled(m_ButtonAccept, task
			&& task.m_eState == MCF_ETaskState.PUBLISHED
			&& permissions.Can(playerId, MCF_ETaskAction.ACCEPT)
			&& !m_bAmending);

		SetButtonEnabled(m_ButtonPublish, task
			&& task.m_eState == MCF_ETaskState.DRAFT
			&& isAuthor
			&& permissions.Can(playerId, MCF_ETaskAction.PUBLISH)
			&& !m_bAmending);

		// Closing out is for whoever owns the order or is carrying it, plus
		// anyone with authority over other people's orders.
		bool mayClose = isRunning && (isAuthor || isAssignee || permissions.Can(playerId, MCF_ETaskAction.EDIT)) && !m_bAmending;

		SetButtonEnabled(m_ButtonComplete, mayClose);
		SetButtonEnabled(m_ButtonCancel, mayClose);

		// Removing is only offered for an order that is finished with, or a
		// draft nobody has ever seen. A running order has to be closed out
		// first -- see the reasoning in MCF_RpcAsk_DeleteTask. The server
		// enforces this too; greying the button here only avoids offering a
		// press that would be refused.
		bool isClosed = task
			&& (task.m_eState == MCF_ETaskState.COMPLETE
				|| task.m_eState == MCF_ETaskState.CANCELLED
				|| task.m_eState == MCF_ETaskState.FAILED);

		bool isOwnDraft = task && task.m_eState == MCF_ETaskState.DRAFT && isAuthor;

		SetButtonEnabled(m_ButtonDelete, (isClosed || isOwnDraft) && mayEditThis && !m_bAmending);
	}

	protected void SetButtonEnabled(SCR_ButtonTextComponent button, bool enabled)
	{
		if (button)
			button.SetEnabled(enabled);
	}

	// ------------------------------------------------------------- actions

	protected void OnNewClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller)
			return;

		controller.MCF_RequestCreateTask("New order");
	}

	//! Sends everything currently typed for the selected order to the server.
	//!
	//! Typing is not saved as you go, on purpose. Saving per keystroke would
	//! mean an RPC per character and a half-written order visible to everyone
	//! the moment a commander pauses to think.
	protected void OnSaveClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedTaskId.IsEmpty())
			return;

		MCF_Task edited = new MCF_Task();
		edited.m_sId = m_sSelectedTaskId;
		edited.m_sTitle = GetEditorValue(m_EditTitle);
		edited.m_sSituation = GetEditorValue(m_EditSituation);
		edited.m_sMission = GetEditorValue(m_EditMission);
		edited.m_sExecution = GetEditorValue(m_EditExecution);
		edited.m_sAdminLogistics = GetEditorValue(m_EditAdmin);
		edited.m_sCommandSignal = GetEditorValue(m_EditSignal);

		controller.MCF_RequestSaveTask(edited);
		SetAmending(false);
	}

	protected void OnAcceptClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedTaskId.IsEmpty())
			return;

		controller.MCF_RequestAcceptTask(m_sSelectedTaskId);
	}

	protected void OnPublishClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedTaskId.IsEmpty())
			return;

		controller.MCF_RequestPublishTask(m_sSelectedTaskId);
	}

	protected void OnCompleteClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedTaskId.IsEmpty())
			return;

		controller.MCF_RequestCloseTask(m_sSelectedTaskId, MCF_ETaskState.COMPLETE);
	}

	protected void OnCancelClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedTaskId.IsEmpty())
			return;

		controller.MCF_RequestCloseTask(m_sSelectedTaskId, MCF_ETaskState.CANCELLED);
	}

	//! Removes an order from the board for good.
	//!
	//! No confirmation prompt yet. It is deliberately limited to orders that
	//! are already closed, or the player's own draft that nobody has seen, so
	//! the worst a stray press can do is lose your own words -- never delete a
	//! plan a squad is still executing.
	protected void OnDeleteClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedTaskId.IsEmpty())
			return;

		// Drop the selection before the refresh comes back, otherwise the
		// board tries to reselect an order that no longer exists.
		string removed = m_sSelectedTaskId;
		m_sSelectedTaskId = string.Empty;

		controller.MCF_RequestDeleteTask(removed);
	}


	// ------------------------------------------------------------- helpers

	protected SCR_PlayerController GetLocalController()
	{
		return SCR_PlayerController.Cast(GetGame().GetPlayerController());
	}

	protected int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}

	//! What MCF thinks the player reading this board is.
	//!
	//! Shown rather than hidden on purpose: roles are resolved from vanilla's
	//! own command state, and the fastest way to catch that reading wrong is
	//! for the player who is a section commander to see themselves labelled a
	//! soldier. Display only -- the server resolves the role again before it
	//! lets anything happen.
	protected string ViewerTag()
	{
		MCF_ETaskRole role = MCF_Task_Permissions.GetInstance().ResolveRole(GetLocalPlayerId());
		return "      YOU: " + MCF_Task_Permissions.RoleLabel(role);
	}

	protected string GetLocalFactionKey()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return string.Empty;

		Faction faction = factionManager.GetLocalPlayerFaction();
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	//! Lets the transport layer tell an open board that something changed,
	//! without the transport needing to know whether a board is open.
	static void RefreshIfOpen()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
			return;

		MCF_PlanningBoardMenu board = MCF_PlanningBoardMenu.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.MCF_PlanningBoard));
		if (board)
			board.Refresh();
	}

	// --------------------------------------------------------------- intel

	protected void OnModeTasksClicked(SCR_ButtonTextComponent button)
	{
		SetIntelMode(false);
	}

	protected void OnModeIntelClicked(SCR_ButtonTextComponent button)
	{
		SetIntelMode(true);
	}

	protected void SetIntelMode(bool intel)
	{
		if (m_bIntelMode == intel)
			return;

		m_bIntelMode = intel;

		// Leaving the taskings side half-way through writing an order would
		// strand the typing with no visible way back to it.
		if (m_bAmending)
			SetAmending(false);

		Refresh();
	}

	//! The two tabs reflect which list is showing. Set without firing change
	//! events, because this is the board describing itself rather than the
	//! player pressing anything.
	protected void SyncModeButtons()
	{
		if (m_ButtonModeTasks)
			m_ButtonModeTasks.SetToggled(!m_bIntelMode, false, false);

		if (m_ButtonModeIntel)
			m_ButtonModeIntel.SetToggled(m_bIntelMode, false, false);
	}

	//! Fills the shared list with intel reports instead of taskings.
	protected void RefreshIntel()
	{
		ClearList();

		array<MCF_Intel_Record> visible = {};
		MCF_Intel_Store.GetInstance().GetVisibleTo(GetLocalPlayerId(), GetLocalFactionKey(), visible);

		foreach (MCF_Intel_Record record : visible)
			AddIntelRow(record);

		if (m_wStatus)
		{
			if (visible.IsEmpty())
				m_wStatus.SetText("No intel entered. Reports appear here once somebody logs what they found." + ViewerTag());
			else
				m_wStatus.SetText(visible.Count().ToString() + " intel report(s) held" + ViewerTag());
		}

		if (!m_sSelectedIntelId.IsEmpty() && m_aRowTaskIds.Contains(m_sSelectedIntelId))
			SelectIntel(m_sSelectedIntelId);
		else if (!m_aRowTaskIds.IsEmpty())
			SelectIntel(m_aRowTaskIds[0]);
		else
			ShowIntelDetail(null);
	}

	protected void AddIntelRow(notnull MCF_Intel_Record record)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets(TASK_ENTRY_LAYOUT, m_wTaskList);
		if (!row)
			return;

		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.FindButtonTextComponent(row);
		if (!button)
			return;

		button.SetText(record.GetReference() + "  " + record.m_sHeading);
		button.m_OnClicked.Insert(OnRowClicked);

		// Reuses the taskings arrays deliberately: one list widget, one set of
		// rows, one selection mechanism. The id held in it is an intel id
		// while the board is in intel mode, which is why every read of it goes
		// through a mode check.
		m_aRowTaskIds.Insert(record.m_sId);
		m_aRowButtons.Insert(button);
		m_aRowWidgets.Insert(row);
	}

	protected void SelectIntel(string recordId)
	{
		m_sSelectedIntelId = recordId;

		int index = m_aRowTaskIds.Find(recordId);
		foreach (int i, SCR_ButtonTextComponent button : m_aRowButtons)
		{
			button.SetToggled(i == index, false, false);
		}

		if (index >= 0)
			GetGame().GetWorkspace().SetFocusedWidget(m_aRowWidgets[index]);

		ShowIntelDetail(MCF_Intel_Store.GetInstance().GetRecord(recordId));
	}

	protected void ShowIntelDetail(MCF_Intel_Record record)
	{
		if (!m_wDetailBody)
			return;

		if (!record)
		{
			m_wDetailBody.SetText("No report selected.");
			UpdateIntelButtons(null);
			return;
		}

		string body = record.GetReference();

		if (!record.m_sSource.IsEmpty())
			body = body + "   SOURCE: " + record.m_sSource;

		if (!record.m_sTimestamp.IsEmpty())
			body = body + "   " + record.m_sTimestamp;

		body = body + "\n" + record.m_sHeading;

		if (!record.m_sBody.IsEmpty())
			body = body + "\n\n" + record.m_sBody;

		m_wDetailBody.SetText(body);
		UpdateIntelButtons(record);
	}

	//! In intel mode the tasking buttons mean nothing, so they go dark rather
	//! than disappear -- positions stay put for anyone navigating by direction.
	protected void UpdateIntelButtons(MCF_Intel_Record record)
	{
		int playerId = GetLocalPlayerId();

		SetButtonEnabled(m_ButtonNew, false);
		SetButtonEnabled(m_ButtonEdit, false);
		SetButtonEnabled(m_ButtonSave, false);
		SetButtonEnabled(m_ButtonAccept, false);
		SetButtonEnabled(m_ButtonPublish, false);
		SetButtonEnabled(m_ButtonComplete, false);
		SetButtonEnabled(m_ButtonCancel, false);
		SetButtonEnabled(m_ButtonDelete, false);

		SetButtonEnabled(m_ButtonDraftFromIntel, record
			&& MCF_Task_Permissions.GetInstance().Can(playerId, MCF_ETaskAction.CREATE));
	}

	//! The bridge between the two layers: turn a report into an order.
	//!
	//! The new order starts as a draft with the report already written into
	//! its SITUATION paragraph, because that is what SITUATION is for -- what
	//! is known, friendly and enemy. The commander writes the mission
	//! themselves. Seeding a whole order from intel would pretend the plan
	//! writes itself, which is precisely the decision a commander is there to
	//! make.
	protected void OnDraftFromIntelClicked(SCR_ButtonTextComponent button)
	{
		SCR_PlayerController controller = GetLocalController();
		if (!controller || m_sSelectedIntelId.IsEmpty())
			return;

		controller.MCF_RequestCreateTaskFromIntel(m_sSelectedIntelId);

		// Drop the player back on the taskings side, where the new draft is.
		SetIntelMode(false);
	}

	//! Closes the board. Always enabled -- a way out must never be
	//! conditional on anything.
	protected void OnCloseClicked(SCR_ButtonTextComponent button)
	{
		Close();
	}
}
