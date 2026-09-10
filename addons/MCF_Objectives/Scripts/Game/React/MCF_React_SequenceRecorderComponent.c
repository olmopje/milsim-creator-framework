//! Sequence Recorder (ARCHITECTURE.md 5.13) -- records position over time
//! plus timestamped cues, for MCF_React_SequencePlaybackComponent to
//! replay later. Not motion capture: it records where an entity went and
//! when existing actions were triggered, not new skeletal animation.
//!
//! RecordSample() must be called periodically while recording -- there is
//! no Tick Manager yet, so nothing calls it automatically.

class MCF_React_SequenceSample
{
	vector m_vPosition;
	float m_fTimestamp;

	void MCF_React_SequenceSample(vector position, float timestamp)
	{
		m_vPosition = position;
		m_fTimestamp = timestamp;
	}
}

class MCF_React_SequenceCue
{
	string m_sValue;
	float m_fTimestamp;

	void MCF_React_SequenceCue(string value, float timestamp)
	{
		m_sValue = value;
		m_fTimestamp = timestamp;
	}
}

[ComponentEditorProps(category: "MCF/React", description: "Records position samples and cues over time for later playback.")]
class MCF_React_SequenceRecorderComponentClass : ScriptComponentClass
{
}

class MCF_React_SequenceRecorderComponent : ScriptComponent
{
	protected ref array<ref MCF_React_SequenceSample> m_aSamples;
	protected ref array<ref MCF_React_SequenceCue> m_aCues;
	protected bool m_bRecording;
	protected float m_fRecordStartTime;

	//! Clears any previous recording and starts a new one.
	void StartRecording()
	{
		m_aSamples = new array<ref MCF_React_SequenceSample>();
		m_aCues = new array<ref MCF_React_SequenceCue>();
		m_bRecording = true;
		m_fRecordStartTime = GetGame().GetWorld().GetWorldTime();
	}

	//! Call periodically with the entity's current position while
	//! recording. Does nothing if not currently recording.
	void RecordSample(vector position)
	{
		if (!m_bRecording)
			return;

		float elapsed = GetGame().GetWorld().GetWorldTime() - m_fRecordStartTime;
		m_aSamples.Insert(new MCF_React_SequenceSample(position, elapsed));
	}

	//! Records a cue at the current moment, using the same "TYPE:value"
	//! encoding as MCF_React_RecipeComponent steps.
	void RecordCue(string cueValue)
	{
		if (!m_bRecording)
			return;

		float elapsed = GetGame().GetWorld().GetWorldTime() - m_fRecordStartTime;
		m_aCues.Insert(new MCF_React_SequenceCue(cueValue, elapsed));
	}

	void StopRecording()
	{
		m_bRecording = false;
	}

	bool IsRecording()
	{
		return m_bRecording;
	}

	array<ref MCF_React_SequenceSample> GetSamples()
	{
		return m_aSamples;
	}

	array<ref MCF_React_SequenceCue> GetCues()
	{
		return m_aCues;
	}
}
