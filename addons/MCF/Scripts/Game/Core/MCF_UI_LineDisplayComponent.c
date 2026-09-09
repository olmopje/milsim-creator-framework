//! Puts MCF text lines on players' screens.
//!
//! This is the seam between MCF's server-side logic and what a player
//! actually sees, and it follows the vanilla pattern (SCR_EditorTask):
//! **decide on the server, broadcast, filter locally on each client.**
//!
//!   server : hears MCF_Voice_LineDisplayed on the event bus, broadcasts an
//!            RPC to every machine, and releases the line queue
//!   client : receives the RPC, decides whether the line is for it, and
//!            renders through the vanilla popup widget
//!
//! Filtering happens on the client rather than the server choosing recipients,
//! because that is how vanilla does it and because it keeps the server from
//! having to track who is where. The cost is that a client receives lines it
//! will not show; for text that is fine.
//!
//! Place on the GameMode entity, which carries an RplComponent and exists on
//! every machine.
//!
//! ON QUEUEING: MCF used to hold each line on screen for m_fDisplayDuration
//! before letting the next through. That cost a measured eight seconds between
//! an event and its line appearing, and could deadlock. It was also redundant:
//! SCR_PopUpNotification maintains its own priority queue. The queue is now
//! released immediately after broadcasting; m_fDisplayDuration is purely how
//! long the popup stays visible.

[ComponentEditorProps(category: "MCF/Core", description: "Broadcasts MCF text lines to players and shows them via the vanilla popup widget.")]
class MCF_UI_LineDisplayComponentClass : ScriptComponentClass
{
}

class MCF_UI_LineDisplayComponent : ScriptComponent
{
	[Attribute(defvalue: "4", uiwidget: UIWidgets.EditBox, desc: "How long each line stays on screen, in seconds.")]
	protected float m_fDisplayDuration;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Optional smaller subtitle shown under every line. Leave empty for none.")]
	protected string m_sSubtitle;

	protected ScriptInvoker m_LineInvoker;

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer())
		{
			MCF_Core_Log.Debug("LineDisplay init on client -- will render lines received by RPC");
			return;
		}

		m_LineInvoker = MCF_Core_EventManager.GetInstance().GetInvoker("MCF_Voice_LineDisplayed");
		m_LineInvoker.Insert(OnLineDisplayed);

		MCF_Core_ValidationRegistry.GetInstance().RegisterConsumer("MCF_Voice_LineDisplayed", "MCF_UI_LineDisplayComponent (screen output)");

		MCF_Core_Log.Debug("LineDisplay init on server -- listening for MCF_Voice_LineDisplayed");
	}

	override void OnDelete(IEntity owner)
	{
		if (m_LineInvoker)
			m_LineInvoker.Remove(OnLineDisplayed);
	}

	//! Server side.
	protected void OnLineDisplayed(Managed payload)
	{
		MCF_Voice_LinePayload linePayload = MCF_Voice_LinePayload.Cast(payload);
		if (!linePayload || linePayload.m_sText.IsEmpty())
		{
			MCF_Core_Log.Debug("LineDisplay got an empty payload -- releasing queue");
			MCF_Voice_LineQueueManager.GetInstance().MarkLineFinished();
			return;
		}

		Broadcast(linePayload.m_sText, linePayload.m_eAudience, linePayload.m_sFactionKey, linePayload.m_iPlayerId);
	}

	//! Server side. Sends a line to every machine, then lets the queue move on.
	//! Also callable directly by anything that wants to say something without
	//! going through the line queue.
	void Broadcast(string text, MCF_EAudience audience = MCF_EAudience.EVERYONE, string factionKey = "", int playerId = 0)
	{
		if (!Replication.IsServer())
			return;

		MCF_Core_Log.Debug("LineDisplay broadcasting (" + typename.EnumToString(MCF_EAudience, audience) + "): " + text);

		Rpc(RpcDo_ShowLine, text, audience, factionKey, playerId);

		// Run it locally too: on a hosted server this machine is also a
		// player, and on a dedicated server it simply finds no widget.
		RpcDo_ShowLine(text, audience, factionKey, playerId);

		MCF_Voice_LineQueueManager.GetInstance().MarkLineFinished();
	}

	//! Runs on every machine. Each one decides for itself whether to show it.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowLine(string text, MCF_EAudience audience, string factionKey, int playerId)
	{
		if (!IsForLocalPlayer(audience, factionKey, playerId))
			return;

		Show(text);
	}

	//! The local audience check.
	protected bool IsForLocalPlayer(MCF_EAudience audience, string factionKey, int playerId)
	{
		if (audience == MCF_EAudience.EVERYONE)
			return true;

		if (audience == MCF_EAudience.PLAYER)
			return GetLocalPlayerId() == playerId;

		if (audience == MCF_EAudience.FACTION)
		{
			if (factionKey.IsEmpty())
				return true;

			return GetLocalFactionKey() == factionKey;
		}

		// GROUP needs the command hierarchy that comes with the task system.
		// Show it rather than swallow it: a half-built filter that silently
		// hides messages is worse than one that is too generous and says so.
		MCF_Core_Log.Warn("audience GROUP is not implemented yet -- showing this line to everyone");
		return true;
	}

	//! \return The local player's id, or 0 if there is no local player
	//! (a dedicated server).
	protected int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}

	//! \return The local player's faction key, or an empty string if it
	//! cannot be resolved.
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

	//! Renders locally. Never touches the line queue -- that is server state.
	protected void Show(string text)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
		{
			// No widget here. Normal on a dedicated server and in the World
			// Editor; both have no screen to draw on.
			MCF_Core_Log.Debug("LineDisplay has no popup widget on this machine -- not rendering: " + text);
			return;
		}

		MCF_Core_Log.Debug("LineDisplay showing: " + text);
		popup.PopupMsg(text, m_fDisplayDuration, m_sSubtitle);
	}
}
