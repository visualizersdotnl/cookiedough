
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
#include "synth-ring-buffer.h"
#include "synth-math.h"
#include "synth-LUT.h"
#include "synth-DX-voice.h"

// Driver: Win32 MIDI input (M-AUDIO Oxygen 49 & Arturia BeatStep)
#include "Win-MIDI-in-Oxygen49.h"
#include "Win-MIDI-in-BeatStep.h"

// Driver: SDL2 audio output
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
	static RingBuffer s_ringBuf;

	/*
		State.

		May only be altered after acquiring lock.
	*/

	static std::mutex s_stateMutex;

	struct VoiceRequest
	{
		unsigned *pIndex;
		Waveform form;
		unsigned key; // [0..127] MIDI
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
	static unsigned s_releasing = 0;
	
	/*
		Voice API.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, unsigned key, float velocity)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		
		VoiceRequest request;
		request.pIndex = pIndex;
		request.form = form;
		request.key = key;
		request.velocity = velocity;
		s_voiceReq.push_back(request);

		// At this point no voice is allocated yet, but I might want to set it to a magic value so it won't retrigger (FIXME)
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

		s_voiceReleaseReq.push_back(request);
	}

	/*
		Voice logic.
	*/

	// Calculate operator frequency
	SFM_INLINE float CalcOpFreq(float fundamentalFreq, const FM_Patch::Operator &patchOp)
	{
		const unsigned coarse = patchOp.coarse;
		const float fine = patchOp.fine;

		float frequency;
		if (true == patchOp.fixed)
		{
			// Fixed ratio (taken from Volca FM third-party manual)
			const float coarseTab[4] = { 1, 10, 100, 1000 };
			frequency = coarseTab[coarse&3];
		}
		else
		{
			// Start at fundamental frequency
			frequency = fundamentalFreq;

			// Detune (taken from Hexter)
			const float detune = patchOp.detune*14.f;
			frequency += (detune-7.f)/32.f;

			// Coarse adjustment
			SFM_ASSERT(coarse < 32);
			if (0 == coarse)
				frequency *= 0.5f;
			else
				frequency *= coarse;
		}

		// Fine adjustment
		frequency *= 1.f+fine;

		return frequency;
	}

	// Calculate operator depth/amplitude
	SFM_INLINE float CalcOpIndex(bool isCarrier, unsigned key, float velocity, const FM_Patch::Operator &patchOp)
	{
		return patchOp.index;
	}

	static void InitializeDXVoice(const VoiceRequest &request, unsigned iVoice)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		DX_Voice &voice = s_DXvoices[iVoice];
		voice.ResetOperators();
//		voice.Reset();
		
		const unsigned key = request.key;
		const float fundamentalFreq = g_MIDIToFreqLUT[key];

		// It simply sounds better to add some curvature to velocity
		const float velocity    = request.velocity;
		const float invVelocity = 1.f-velocity;

		// Save copy
		voice.m_velocity = velocity;

		FM_Patch &patch = s_parameters.patch;

#if 1

		/*
			Test algorithm: single carrier & modulator
		*/

		// Operator #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			request.form, 
			CalcOpFreq(fundamentalFreq, patch.operators[0]), 
			CalcOpIndex(true, key, velocity, patch.operators[0]));

		// Operator #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].feedback = 1;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[1]), 
			CalcOpIndex(false, key, velocity, patch.operators[1]));

		/*
			End of Algorithm
		*/

#endif

		// Set per operator
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			const FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];
			DX_Voice::Operator &voiceOp = voice.m_operators[iOp];

			// Adj. velocity
			const float patchVel = patchOp.velSens*velocity;
		}
		
		// Enabled, up counter		
		voice.m_enabled = true;
		++s_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
		
	}

	SFM_INLINE void InstFrontVoice(unsigned iVoice)
	{
		const VoiceRequest &request = s_voiceReq.front();
		InitializeDXVoice(request, iVoice);
		s_voiceReq.pop_front();
	}

	// Update voices and handle trigger & release
	static void UpdateVoices()
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		// Process release requests
		for (auto &request : s_voiceReleaseReq)
		{
			const unsigned index = request.index;
			SFM_ASSERT(-1 != index);
			DX_Voice &voice = s_DXvoices[index];

			// Release immediately (FIXME)
			voice.m_enabled = false;
			--s_active;
			--s_releasing;

			Log("Voice freed: " + std::to_string(index));

			// Additional releasing voice
//			const float velocity = request.velocity;

//			++s_releasing;

//			Log("Voice released: " + std::to_string(request.index));
		}

/*
		// Update active voices
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			DX_Voice &voice = s_DXvoices[iVoice];
			const bool enabled = voice.m_enabled;

			if (true == enabled)
			{
				if (true)
				{
					voice.m_enabled = false;
					--s_active;
					--s_releasing;

					Log("Voice freed: " + std::to_string(iVoice));
				}
			}
		}
*/

		/*
			Voice allocation

			- First try to grab an existing free voice
			- No dice? Try releasing voice(s)

			A list of releasing voices (sortable) would be very useful to make this code a bit less amateurish
		*/

		// Spawn new voice(s) if any free voices available
		while (s_voiceReq.size() > 0 && s_active < kMaxVoices)
		{
			// Pick first free voice
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (false == voice.m_enabled)
				{
					InstFrontVoice(iVoice);

					Log("Voice triggered: " + std::to_string(iVoice));

					break;
				}
			}
		}

/*
		// Alternatively try to steal a releasing voice
		while (s_voiceReq.size() > 0 && s_releasing > 0)
		{
			float lowestOutput = 1.f;
			unsigned iRelease = -1;

			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (true == voice.m_ADSR.IsReleasing())
				{
					// Check output level
					const float output = voice.m_ADSR.m_ADSR.getOutput();
					if (lowestOutput >= output)
					{
						// Lowest so far (closer to end of cycle)
						iRelease = iVoice;
						lowestOutput = output;
					}
				}
			}

			// Got one to steal?
			if (-1 != iRelease)
			{
				DX_Voice &voice = s_DXvoices[iRelease];

				// Force free
				voice.m_enabled = false;
				--s_active;
				--s_releasing;

				InstFrontVoice(iRelease);

				Log("Voice triggered (stolen): " + std::to_string(iRelease) + " with output " + std::to_string(lowestOutput));
			}
			else
				break; // Nothing to steal at all, so bail
		}
*/

		// All release requests have been honoured; note triggers that can't be made are discarded
		s_voiceReleaseReq.clear();

		if (false == s_voiceReq.empty())
			Log("Number of voice requests ignored: " + std::to_string(s_voiceReq.size()));

		s_voiceReq.clear();
	}

	/*
		Parameter update for Oxygen 49 + BeatStep.
	*/

	static void UpdateState_Oxy49_BeatStep(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);

		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];

			if (true)
			{
				// Ratio
				patchOp.coarse = unsigned(WinMidi_GetOpCoarse(iOp)*31.f);
				patchOp.fine   = WinMidi_GetOpFine(iOp); // 1 octave (I believe the DX7 goes *close* to 1 octave)
			}
			else
			{
				// Fixed
				// FIXME
			}

			patchOp.detune = WinMidi_GetOpDetune(iOp);
		}
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	// Delay/Chorus (FIXME)
	SFM_INLINE void DelayToStereo(float mix) 
	{
		s_ringBuf.Write(mix);
		s_ringBuf.Write(mix);
	}

	// Returns loudest signal (linear amplitude)
	static void Render(float time)
	{
		// Lock ring buffer
		std::lock_guard<std::mutex> ringLock(s_ringBufMutex);

		// See if there's enough space in the ring buffer to justify rendering
		if (true == s_ringBuf.IsFull())
			return;

		const unsigned available = s_ringBuf.GetAvailable()>>1;
		if (available > kMinSamplesPerUpdate)
			return;

		// Amt. of samples (stereo)
		const unsigned numSamples = (kRingBufferSize>>1)-available;

		// Lock state
		std::lock_guard<std::mutex> stateLock(s_stateMutex);

		// Update voice logic
		UpdateVoices();

		const unsigned numVoices = s_active;

		if (0 == numVoices)
		{
			// Render silence (we still have to run the effects)
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				DelayToStereo(0.f);
			}
		}
		else
		{
			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				const float velocity = voice.m_velocity;
				
				if (true == voice.m_enabled)
				{
					float *buffer = s_voiceBuffers[curVoice];
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						float sample = voice.Sample(s_parameters);

						buffer[iSample] = sample;
					}

					++curVoice; // Do *not* use to index anything other than the temporary buffers
				}
			}

			// Mix & store voices
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const unsigned sampleCount = s_sampleCount+iSample;

				float mix = 0.f;
				for (unsigned iVoice = 0; iVoice < numVoices; ++iVoice)
				{
					const float sample = s_voiceBuffers[iVoice][iSample];
					SampleAssert(sample);

					mix = mix+sample;
				}

				// Apply Chorus/Delay
				DelayToStereo(mix);
			}
		}

		s_sampleCount += numSamples;
	}

}; // namespace SFM

using namespace SFM;

/*
	SDL2 stream callback.
*/

static void SDL2_Callback(void *pData, uint8_t *pStream, int length)
{
	const unsigned numSamplesReq = length/sizeof(float)/2;

	std::lock_guard<std::mutex> lock(s_ringBufMutex);
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable()>>1;
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		if (numSamplesAvail < numSamplesReq)
		{
			Log("Buffer underrun");
		}

		float *pWrite = reinterpret_cast<float*>(pStream);
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			// FIXME
			*pWrite++ = s_ringBuf.Read();
			*pWrite++ = s_ringBuf.Read();
		}

		s_sampleOutCount += numSamples;
	}
}

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	// Calc. all tables
	CalculateLUTs();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	// Reset parameter state
	s_parameters.SetDefaults();

	// Reset runtime state
	for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		s_DXvoices[iVoice].Reset();
	
	// Reset voice deques
	s_voiceReq.clear();
	s_voiceReleaseReq.clear();

	// Oxygen 49 driver + SDL2
	const bool midiIn = WinMidi_Oxygen49_Start() && WinMidi_BeatStep_Start();
	const bool audioOut = SDL2_CreateAudio(SDL2_Callback);

	return true == midiIn && audioOut;
}

void Syntherklaas_Destroy()
{
	SDL2_DestroyAudio();

	WinMidi_Oxygen49_Stop();
	WinMidi_BeatStep_Stop();
}

/*
	Frame update (called by Bevacqua).
*/

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	// Update state for M-AUDIO Oxygen 49 & Arturia BeatStep
	UpdateState_Oxy49_BeatStep(time);
	
	// Render
	Render(time);

	// Start audio stream on first call
	static bool first = false;
	if (false == first)
	{
		SDL2_StartAudio();
		first = true;

		Log("FM. BISON 0.3 is up & running!");
	}
}
