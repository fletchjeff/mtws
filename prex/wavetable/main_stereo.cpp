#include "ComputerCard.h"
#include "wavetable_data.h"

/// 60-Waveform Wavetable Synthesizer - Stereo Reverse Edition
///
/// This implements classic wavetable synthesis with morphing between 60 waveforms.
/// AudioOut1 plays the waveform forward, AudioOut2 plays it in reverse for stereo effect.
///
/// Controls:
/// - Main Knob: Pitch control (±4 octaves around 440Hz)
/// - X Knob: Wavetable position (morphs between waveforms 0-59)
///
/// How it works:
/// 1. Main knob controls frequency (same as single-waveform version)
/// 2. X knob selects position in wavetable (0-59.99 with fractional interpolation)
/// 3. For each audio sample:
///    a. Calculate which waveform to read (e.g., 23.7 = between waveforms 23 and 24)
///    b. Look up sample from both waveforms at current phase (FORWARD)
///    c. Look up sample from both waveforms at inverted phase (REVERSE)
///    d. Interpolate between the two waveforms for both forward and reverse
///    e. Output forward to AudioOut1, reverse to AudioOut2
///
/// Memory usage: 72KB for wavetable (36,000 samples × 2 bytes)
/// RP2040 has 264KB RAM, so this is comfortable

class WavetableSynthStereo : public ComputerCard
{
public:
	// Table dimensions
	constexpr static unsigned numWaveforms = 60;
	constexpr static unsigned samplesPerWaveform = 600;

	// Phase accumulator (0 to 2^32 represents 0 to 2π)
	uint32_t phase;

	// Base phase increment for 440Hz (A4) at 48kHz sample rate
	// = 2^32 * 440 / 48000 = 39370534
	constexpr static uint32_t basePhaseInc = 39370534;

	WavetableSynthStereo()
	{
		phase = 0;
	}

	virtual void ProcessSample()
	{
		//
		// STEP 1: Determine wavetable position from X knob
		//

		// Read X knob (0-4095) and map to wavetable position (0.0 to 59.0)
		// We use 16-bit fixed point: multiply by 65536 for precision
		// xKnob * 59 / 4095 gives us 0-59 range
		// In fixed point: (xKnob * 59 * 65536) / 4095
		int32_t xKnob = KnobVal(Knob::X);

		// Calculate fractional waveform index in 16-bit fixed point
		// wavetablePos_fp16 ranges from 0 (wave 0) to 3866624 (wave 59.0 in 16-bit fixed)
		int32_t wavetablePos_fp16 = (xKnob * 59 * 65536) / 4095;

		// Split into integer and fractional parts
		int32_t waveIndex1 = wavetablePos_fp16 >> 16;       // Integer part: 0-59
		int32_t waveFrac = wavetablePos_fp16 & 0xFFFF;      // Fractional part: 0-65535

		// Get second waveform index for interpolation (wraps to 0 at end)
		int32_t waveIndex2 = (waveIndex1 + 1) % numWaveforms;

		//
		// STEP 2: Calculate sample position within waveform (phase)
		//

		// Calculate position in waveform using 64-bit intermediate
		uint64_t pos = uint64_t(phase) * samplesPerWaveform;

		uint32_t sampleIndex_fwd = pos >> 32;               // Integer part (0-599)
		uint32_t sampleFrac_fwd = (pos >> 16) & 0xFFFF;     // Fractional part (0-65535)

		// For reverse playback, invert both the sample index and the fractional part
		uint32_t sampleIndex_rev = (samplesPerWaveform - 1) - sampleIndex_fwd;
		uint32_t sampleFrac_rev = 65536 - sampleFrac_fwd;   // Invert fraction for proper interpolation

		//
		// STEP 3A: Look up FORWARD samples (AudioOut1)
		//

		// Get sample from first waveform at current phase (forward)
		int32_t wave1_s1_fwd = wavetable[waveIndex1][sampleIndex_fwd];
		int32_t wave1_s2_fwd = wavetable[waveIndex1][(sampleIndex_fwd + 1) % samplesPerWaveform];

		// Interpolate within first waveform (phase interpolation forward)
		int32_t wave1_sample_fwd = (wave1_s2_fwd * sampleFrac_fwd + wave1_s1_fwd * (65536 - sampleFrac_fwd)) >> 16;

		// Get sample from second waveform at current phase (forward)
		int32_t wave2_s1_fwd = wavetable[waveIndex2][sampleIndex_fwd];
		int32_t wave2_s2_fwd = wavetable[waveIndex2][(sampleIndex_fwd + 1) % samplesPerWaveform];

		// Interpolate within second waveform (phase interpolation forward)
		int32_t wave2_sample_fwd = (wave2_s2_fwd * sampleFrac_fwd + wave2_s1_fwd * (65536 - sampleFrac_fwd)) >> 16;

		// Interpolate between the two waveforms (wavetable interpolation forward)
		int32_t interpolated_fwd = (wave2_sample_fwd * waveFrac + wave1_sample_fwd * (65536 - waveFrac)) >> 16;

		//
		// STEP 3B: Look up REVERSE samples (AudioOut2)
		//

		// Get sample from first waveform at reverse phase
		// When reading backwards, we interpolate towards the previous sample
		int32_t wave1_s1_rev = wavetable[waveIndex1][sampleIndex_rev];
		uint32_t prevIndex_rev = (sampleIndex_rev == 0) ? (samplesPerWaveform - 1) : (sampleIndex_rev - 1);
		int32_t wave1_s2_rev = wavetable[waveIndex1][prevIndex_rev];

		// Interpolate within first waveform (phase interpolation reverse)
		int32_t wave1_sample_rev = (wave1_s2_rev * sampleFrac_rev + wave1_s1_rev * (65536 - sampleFrac_rev)) >> 16;

		// Get sample from second waveform at reverse phase
		int32_t wave2_s1_rev = wavetable[waveIndex2][sampleIndex_rev];
		int32_t wave2_s2_rev = wavetable[waveIndex2][prevIndex_rev];

		// Interpolate within second waveform (phase interpolation reverse)
		int32_t wave2_sample_rev = (wave2_s2_rev * sampleFrac_rev + wave2_s1_rev * (65536 - sampleFrac_rev)) >> 16;

		// Interpolate between the two waveforms (wavetable interpolation reverse)
		int32_t interpolated_rev = (wave2_sample_rev * waveFrac + wave1_sample_rev * (65536 - waveFrac)) >> 16;

		//
		// STEP 4: Output forward and reverse to separate channels
		//

		// Scale from 16-bit (±32767) to 12-bit (±2048) for DAC output
		int32_t out_fwd = interpolated_fwd >> 4;
		int32_t out_rev = interpolated_rev >> 4;

		AudioOut1(out_fwd);  // Forward playback
		AudioOut2(out_rev);  // Reverse playback

		//
		// STEP 5: Calculate and apply frequency control (Main knob)
		//

		// Read Main knob for frequency control (0-4095)
		// Map to ±4 octaves: knob center (~2048) = 440Hz, min = 27.5Hz, max = 7040Hz
		int32_t knob = KnobVal(Knob::Main);

		// Calculate frequency multiplier using octaves
		// Map knob 0-4095 to -4.0 to +4.0 octaves (in 16-bit fixed point)
		int32_t octaves_fp16 = (knob - 2048) * 128;  // -262144 to +261888 (±4.0 in 16-bit fixed)

		// Calculate 2^octaves using bit shifting for integer part and linear approximation
		// Split into integer octaves and fractional part
		int32_t octave_int = octaves_fp16 >> 16;    // Integer octaves (-4 to +3)
		int32_t octave_frac = octaves_fp16 & 0xFFFF; // Fractional part

		// Start with base phase increment
		uint32_t phaseInc = basePhaseInc;

		// Apply integer octave shifts (doubling or halving)
		if (octave_int >= 0) {
			phaseInc <<= octave_int;  // Shift left for positive octaves (multiply by 2^n)
		} else {
			phaseInc >>= (-octave_int);  // Shift right for negative octaves (divide by 2^n)
		}

		// Apply fractional octave using linear approximation: 2^x ≈ 1 + 0.693*x for small x
		// In fixed point: phaseInc + (phaseInc * frac * 45426) >> 32
		int64_t fracAdjust = (int64_t(phaseInc) * octave_frac * 45426) >> 32;
		phaseInc += fracAdjust;

		phase += phaseInc;
	}
};


int main()
{
	WavetableSynthStereo synth;
	synth.Run();
}
