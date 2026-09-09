//! Shared step-execution logic, used by both MCF_React_RecipeComponent
//! (manually assembled recipes) and MCF_React_SequencePlaybackComponent
//! (recorded sequences) -- both encode steps the same way
//! ("TYPE:value"), so both run them the same way.

class MCF_React_StepRunner
{
	static void RunStep(string rawStep)
	{
		array<string> parts = new array<string>();
		rawStep.Split(":", parts, false);
		if (parts.Count() < 2)
			return;

		string typeStr = parts[0];
		string value = parts[1];

		if (typeStr == "PUBLISH_EVENT")
			MCF_Core_EventManager.GetInstance().Publish(value, null);
		else if (typeStr == "PLAY_TEXT_LINE")
			MCF_Voice_LineQueueManager.GetInstance().Enqueue(value, 0);
		else if (typeStr == "REQUEST_ANIMATION")
			MCF_Core_EventManager.GetInstance().Publish("MCF_AI_WaypointAnimationRequested", null);
	}
}
