
/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl
*/

// Shut it, MSVC.
#define _CRT_SECURE_NO_WARNINGS

// C++11
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <deque>

#include "FM_BISON.h"

#include "synth-midi.h"
#include "synth-parameters.h"
#include "synth-ringbuffer.h"
#include "synth-math.h"
#include "synth-LUT.h"
#include "synth-DX-voice.h"

// Win32 MIDI input (M-AUDIO Oxygen 49 & Arturia BeatStep)
#include "Win-MIDI-in-Oxygen49.h"

// SDL2 audio output
#include "SDL2-audio-out.h"

namespace SFM
{
	/*
		Global sample counts.
	*/

 	static unsigned s_sampleCount = 0;
	static unsigned s_sampleOutCount = 0;

	/*
		Ring buffer.
	*/

	static std::mutex s_ringBufMutex;
	static FIFO s_ringBuf;

	/*
		State.

		May only be altered after acquiring lock.
	*/

	static std::mutex s_stateMutex;

	struct VoiceRequest
	{
		unsigned *pIndex;
		Waveform form;
		float frequency;
		float velocity;
	};

	struct VoiceReleaseRequest
	{
		unsigned index;
		float velocity;
	};

	// Voice req.
	static std::deque<VoiceRequest> s_voiceReq;
	static std::deque<VoiceReleaseRequest> s_voiceReleaseReq;

	// Parameters
	static Parameters s_parameters;

	// Voices
	static DX_Voice s_DXvoices[kMaxVoices];
	static unsigned s_active = 0;

	
	/*
		Voice API.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, float frequency, float velocity)
	{
		SFM_ASSERT(true == InAudibleSpectrum(frequency));

		std::lock_guard<std::mutex> lock(s_stateMutex);
		
		VoiceRequest request;
		request.pIndex = pIndex;
		request.form = form;
		request.frequency = frequency;
		request.velocity = velocity;
		s_voiceReq.push_front(request);

		// At this point there is no voice yet
		if (nullptr != pIndex)
			*pIndex = -1;
	}

	void ReleaseVoice(unsigned index, float velocity)
	{
		SFM_ASSERT(-1 != index);

		std::lock_guard<std::mutex> lock(s_stateMutex);

		VoiceReleaseRequest request; 
		request.index = index;
		request.velocity = velocity;

		s_voiceReleaseReq.push_front(request);
	}

	/*
		Voice logic.
	*/

	SFM_INLINE float CalcOpFreq(float frequency, Patch::Operator &patchOp)
	{
//		frequency *= g_modRatioLUT[patchOp.coarse];

		frequency *= g_CM[patchOp.coarse][1];
		frequency *= patchOp.detune;
		frequency *= patchOp.fine;

		return frequency;
	}

	static void InitializeDXVoice(const VoiceRequest &request, unsigned iVoice)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		DX_Voice &voice = s_DXvoices[iVoice];
		voice.Reset();
		
		const float frequency = request.frequency;

		// Each has a distinct effect (linear, exponential, inverse exponential)
		const float velocity       = request.velocity;
		const float velocityExp    = velocity*velocity;
		const float velocityInvExp = Clamp(invsqrf(velocity));
		
		// Master/global
		const float masterAmp = velocity*kMaxVoiceAmp;
		const float masterFreq = frequency;
		const float modDepth = s_parameters.modDepth;

		/*
			Test algorithm: Volca FM algorithm #5
		*/

		Patch &patch = s_parameters.patch;

		// Carrier #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulator = 3;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(request.form, CalcOpFreq(masterFreq, patch.operators[0]), masterAmp*patch.operators[0].amplitude);

		// Carrier #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].modulator = 4;
		voice.m_operators[1].isCarrier = true;
		voice.m_operators[1].oscillator.Initialize(request.form, CalcOpFreq(masterFreq, patch.operators[1]), masterAmp*patch.operators[1].amplitude);

		// Carrier #3
		voice.m_operators[2].enabled = true;
		voice.m_operators[2].modulator = 5;
		voice.m_operators[2].isCarrier = true;
		voice.m_operators[2].oscillator.Initialize(request.form, CalcOpFreq(masterFreq, patch.operators[2]), masterAmp*patch.operators[2].amplitude);

		// Modulator #1
		voice.m_operators[3].enabled = true;
		voice.m_operators[3].feedback = -1;
		voice.m_operators[3].oscillator.Initialize(kSine, CalcOpFreq(masterFreq, patch.operators[3]), modDepth*patch.operators[3].amplitude);

		// Modulator #1
		voice.m_operators[4].enabled = true;
		voice.m_operators[4].feedback = 0-1;
		voice.m_operators[4].oscillator.Initialize(kSine, CalcOpFreq(masterFreq, patch.operators[4]), modDepth*patch.operators[4].amplitude);

		// Modulator #1
		voice.m_operators[5].enabled = true;
		voice.m_operators[5].feedback = 5;
		voice.m_operators[5].oscillator.Initialize(kSine, CalcOpFreq(masterFreq, patch.operators[5]), modDepth*patch.operators[5].amplitude);

		/*
			End of Algorithm
		*/

		// Start master ADSR
		voice.m_ADSR.Start(s_parameters.m_envParams, velocity);

		// Enabled, up counter		
		voice.m_enabled = true;
		++s_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
		
	}

	// Update voices and handle trigger & release
	static void UpdateVoices()
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		// Process release requests
		for (auto &request : s_voiceReleaseReq)
		{
			SFM_ASSERT(-1 != request.index);

			DX_Voice &voice = s_DXvoices[request.index];
			voice.m_ADSR.Stop(request.velocity);

			Log("Voice released: " + std::to_string(request.index));
		}

		// Update active voices
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			DX_Voice &voice = s_DXvoices[iVoice];
			const bool enabled = voice.m_enabled;

			if (true == enabled)
			{
				if (true == voice.m_ADSR.IsIdle())
				{
					voice.m_enabled = false;
					--s_active;
				}
			}
		}

		// Spawn new voice(s)
		while (s_voiceReq.size() > 0 && s_active < kMaxVoices-1)
		{
			// Pick first free voice (FIXME: need proper heuristic in case we're out of voices)
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (false == voice.m_enabled)
				{
					const VoiceRequest request = s_voiceReq.front();
					InitializeDXVoice(request, iVoice);
					s_voiceReq.pop_front();

					Log("Voice triggered: " + std::to_string(iVoice));

					break;
				}
			}
		}

		// All release requests have been honoured; as for triggers, they may have to wait
		// if there's no slot available at this time.
		s_voiceReleaseReq.clear();
	}

	/*
		Parameter update.

		Currently there's only one of these for the Oxygen 49 (+ Arturia BeatStep) driver.

		Ideally all inputs interact on sample level, but this is impractical and only done for a few parameters.

		FIXME:
			- Update parameters every N samples (nuggets).
			- Remove all rogue parameter probes.
			  + WinMidi_GetPitchBend()
			  + WinMidi_GetMasterDrive()

		Most tweaking done here on the normalized input parameters should be adapted for the first VST attempt.
	*/

	static void UpdateState_Oxy49_BeatStep(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);

		// Drive
		s_parameters.drive = dBToAmplitude(kDriveHidB)*WinMidi_GetMasterDrive();

		// Master ADSR
		s_parameters.m_envParams.attack  = WinMidi_GetAttack();
		s_parameters.m_envParams.decay   = WinMidi_GetDecay();
		s_parameters.m_envParams.release = WinMidi_GetRelease();
		s_parameters.m_envParams.sustain = WinMidi_GetSustain();

		// Modulation depth
		const float alpha = 1.f/dBToAmplitude(-12.f);
		s_parameters.modDepth = WinMidi_GetModulation()*alpha;

		/*
			Uppdate current operator
		*/

		const unsigned iOp = WinMidi_GetOperator();
		if (-1 != iOp)
		{
			SFM_ASSERT(iOp >= 0 && iOp < kNumOperators);

			Patch::Operator &patchOp = s_parameters.patch.operators[iOp];
	
//			const unsigned index = unsigned(WinMidi_GetOperatorCoarse()*(g_numModRatios-1));
//			SFM_ASSERT(index < g_numModRatios);
//			patchOp.coarse = index;

			const unsigned index = unsigned(WinMidi_GetOperatorCoarse()*(g_CM_size-1));
			SFM_ASSERT(index < g_CM_size);
			patchOp.coarse = index;

			// Fine tuning (1 octave, ain't that much?)
			const float fine = WinMidi_GetOperatorFinetune()*0.99f;
			patchOp.fine = fine; // powf(2.f, fine);

			// Go up 7 or down 7 semitones
			patchOp.detune = powf(2.f, ( -7.f + 14.f*WinMidi_GetOperatorDetune() )/12.f);

			patchOp.amplitude = WinMidi_GetOperatorAmplitude();
		}
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];


	// Returns loudest signal (linear amplitude)
	static float Render(float time)
	{
		float loudest = 0.f;

		// Lock ring buffer
		std::lock_guard<std::mutex> ringLock(s_ringBufMutex);

		// See if there's enough space in the ring buffer to justify rendering
		if (true == s_ringBuf.IsFull())
			return loudest;

		const unsigned available = s_ringBuf.GetAvailable();
		if (available > kMinSamplesPerUpdate)
			return loudest;

		const unsigned numSamples = kRingBufferSize-available;

		// Lock state
		std::lock_guard<std::mutex> stateLock(s_stateMutex);

		// Handle voice logic
		UpdateVoices();

		const unsigned numVoices = s_active;

		if (0 == numVoices)
		{
			loudest = 0.f;

			// Render silence (we still have to run the effects)
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
				s_ringBuf.Write(0.f);
		}
		else
		{
			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];

				if (true == voice.m_enabled)
				{
					// This should keep as close to the sample as possible (FIXME)
					const float bend = WinMidi_GetPitchBend()*kPitchBendRange;
					voice.SetPitchBend(bend);
	
					float *buffer = s_voiceBuffers[curVoice];
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						/* const */ float sample = voice.Sample(s_parameters);
						buffer[iSample] = sample;
					}

					++curVoice; // Do *not* use to index anything other than the temporary buffers
				}
			}

			loudest = 0.f;

			// Mix & store voices
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const unsigned sampleCount = s_sampleCount+iSample;

				float mix = 0.f;
				for (unsigned iVoice = 0; iVoice < numVoices; ++iVoice)
				{
					const float sample = s_voiceBuffers[iVoice][iSample];
					SampleAssert(sample);

					mix = SoftClamp(mix+sample);
				}

				// Drive
				const float drive = WinMidi_GetMasterDrive();
				mix = ultra_tanhf(mix*drive);

				// Write
				SampleAssert(mix);
				s_ringBuf.Write(mix);

				loudest = std::max<float>(loudest, fabsf(mix));
			}
		}

		s_sampleCount += numSamples;

		return loudest;
	}

}; // namespace SFM

using namespace SFM;

/*
	SDL2 stream callback.
*/

static void SDL2_Callback(void *pData, uint8_t *pStream, int length)
{
	const unsigned numSamplesReq = length/sizeof(float);

	std::lock_guard<std::mutex> lock(s_ringBufMutex);
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable();
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		if (numSamplesAvail < numSamplesReq)
		{
//			SFM_ASSERT(false);
			Log("Buffer underrun");
		}

		float *pWrite = reinterpret_cast<float*>(pStream);
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			*pWrite++ = s_ringBuf.Read();

		s_sampleOutCount += numSamples;
	}
}

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	// Calc. LUTs
	CalculateLUTs();
	CalculateMidiToFrequencyLUT();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	// Reset parameter state
	s_parameters.SetDefaults();

	// Reset runtime state
	for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
	{
		s_DXvoices[iVoice].m_enabled = false;
	}

	// Reset voice deques
	s_voiceReq.clear();
	s_voiceReleaseReq.clear();

	// Oxygen 49 driver + SDL2
	const bool midiIn = WinMidi_Oxygen49_Start(); /*  && WinMidi_BeatStep_Start(); */
	const bool audioOut = SDL2_CreateAudio(SDL2_Callback);

	return true == midiIn && audioOut;
}

void Syntherklaas_Destroy()
{
	SDL2_DestroyAudio();
	WinMidi_Oxygen49_Stop();
//	WinMidi_BeatStep_Stop();
}

/*
	Frame update (called by Bevacqua).
*/

float Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	// Update state for M-AUDIO Oxygen 49 & Arturia BeatStep
	UpdateState_Oxy49_BeatStep(time);
	
	// Render
	float loudest = Render(time);

	// Start audio stream on first call
	static bool first = false;
	if (false == first)
	{
		SDL2_StartAudio();
		first = true;

		Log("FM. BISON is up & running!");
	}

	return loudest;
}
