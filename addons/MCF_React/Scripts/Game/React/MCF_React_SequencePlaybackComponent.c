//! Sequence Playback (ARCHITECTURE.md 5.13) -- plays back samples and
//! cues recorded by MCF_React_SequenceRecorderComponent. Advances itself
//! every frame via EOnFrame while playing, and now actually relocates its
//! owner along the recorded path (via SetOrigin each frame) -- this is
//! straight-line interpolation between samples, not real character
//! movement/animation. Cue firing reuses MCF_React_StepRunner (the same
//! execution used by Recipe steps).

[ComponentEditorProps(category: "MCF/React", description: "Plays back a recorded sequence -- moves the owner along the path and fires cues.")]
class MCF_React_SequencePlaybackComponentClass : ScriptComponentClass
{
}

class MCF_React_SequencePlaybackComponent : ScriptComponent
{
	protected ref array<ref MCF_React_SequenceSample> m_aSamples;
	protected ref array<ref MCF_React_SequenceCue> m_aCues;
	protected int m_iNextCueIndex;
	protected float m_fPlaybackTime;
	protected bool m_bPlaying;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bPlaying)
			Advance(timeSlice, owner);
	}

	//! Loads a recorded sequence and starts playback from time 0.
	void LoadSequence(array<ref MCF_React_SequenceSample> samples, array<ref MCF_React_SequenceCue> cues)
	{
		m_aSamples = samples;
		m_aCues = cues;
		m_iNextCueIndex = 0;
		m_fPlaybackTime = 0;
		m_bPlaying = true;
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}

	//! Advances playback time, moves owner to the interpolated position,
	//! and fires any cues whose timestamp has been reached. Called
	//! automatically every frame while playing (see EOnFrame), but can
	//! also be called directly if needed.
	void Advance(float deltaTime, IEntity owner)
	{
		if (!m_bPlaying || !m_aCues)
			return;

		m_fPlaybackTime += deltaTime;

		if (owner)
			owner.SetOrigin(GetCurrentPosition());

		while (m_iNextCueIndex < m_aCues.Count() && m_aCues[m_iNextCueIndex].m_fTimestamp <= m_fPlaybackTime)
		{
			MCF_React_StepRunner.RunStep(m_aCues[m_iNextCueIndex].m_sValue);
			m_iNextCueIndex++;
		}

		if (m_iNextCueIndex >= m_aCues.Count() && IsPastLastSample())
			m_bPlaying = false;
	}

	protected bool IsPastLastSample()
	{
		if (!m_aSamples || m_aSamples.IsEmpty())
			return true;

		return m_fPlaybackTime >= m_aSamples[m_aSamples.Count() - 1].m_fTimestamp;
	}

	//! Returns the recorded position at or nearest to the current
	//! playback time. Returns "0 0 0" if no samples are loaded.
	vector GetCurrentPosition()
	{
		if (!m_aSamples || m_aSamples.IsEmpty())
			return "0 0 0";

		vector result = m_aSamples[0].m_vPosition;
		foreach (MCF_React_SequenceSample sample : m_aSamples)
		{
			if (sample.m_fTimestamp > m_fPlaybackTime)
				break;
			result = sample.m_vPosition;
		}

		return result;
	}

	bool IsPlaying()
	{
		return m_bPlaying;
	}
}
