//! Debug overlay data source (ARCHITECTURE.md 7.7) -- aggregates counts
//! already exposed by other managers into one plain-text report. Not an
//! actual on-screen overlay widget (that is separate UI work); this only
//! builds the text something else can display.

class MCF_Core_DebugOverlay
{
	static string BuildReport()
	{
		string report = "=== MCF Debug Report ===\n";

		report += string.Format("Active events: %1\n", MCF_Core_EventManager.GetInstance().GetActiveEventCount());
		report += string.Format("Tagged entities: %1\n", MCF_Core_TagRegistry.GetInstance().GetRegisteredCount());
		report += string.Format("AAR log entries: %1\n", MCF_AAR_DebriefManager.GetInstance().GetLogCount());

		return report;
	}
}
