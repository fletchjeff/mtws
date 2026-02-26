#include "ComputerCard.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include <cmath>

/// Three-voice additive sine oscillator scaffold (centroid_3).
///
/// Current implementation:
/// - Voice 1: fundamental (1x), gain 1.0
/// - Voice 2: second harmonic (2x), gain 0.5 (-6dB)
/// - Voice 3: third harmonic (3x), gain 1/3 (~-9.54dB)
/// - Main knob controls base pitch over 10 octaves: 10Hz..10240Hz
/// - Pitch mapping uses integer/fixed-point math with EXP2_Q16 lookup
/// - Control path runs at 1/16 audio rate (CONTROL_DIVISOR) for efficiency
/// - Voices are summed to mono and normalized (6/11) to preserve headroom
/// - Output is sent to both AudioOut1 and AudioOut2
///
/// This structure is intended to scale to higher voice counts (e.g. 16 voices)
/// with minimal code changes.

class SineWaveLookup : public ComputerCard
{
	public:
	constexpr static uint32_t BASE_PHASE_INC_10HZ = 894785U; // 10 * 2^32 / 48000
	constexpr static int CONTROL_DIVISOR = 16;
	constexpr static int VOICES = 3;
	constexpr static uint32_t VOICE_RATIO_Q16[VOICES] = { 65536U, 131072U, 196608U }; // 1x, 2x, 3x
	constexpr static int32_t VOICE_GAIN_Q12[VOICES] = { 4096, 2048, 1365 };            // 1.0, 0.5, ~0.333
	constexpr static int32_t MIX_NORM_Q12 = 2234; // round((6/11) * 4096)
	constexpr static uint32_t CORE1_CONTROL_PERIOD_US = (CONTROL_DIVISOR * 1000000U) / 48000U;
	struct ControlFrame {
		uint32_t voice_phase_increment[VOICES];
		int32_t voice_gain_q12[VOICES];
		int32_t mix_norm_q12;
	};
	struct ControlInputSnapshot {
		uint32_t main_knob;
		uint32_t knob_x;
		uint32_t knob_y;
		uint32_t switch_pos;
	};

	// Q16 lookup for 2^(x) over x=0..1 in 1/128 steps (129 endpoints).
	constexpr static uint32_t EXP2_Q16[129] = {
	     65536,  65892,  66250,  66609,  66971,  67335,  67700,  68068,  68438,  68809,  69183,  69558,  69936,  70316,  70698,  71082,
	     71468,  71856,  72246,  72638,  73032,  73429,  73828,  74229,  74632,  75037,  75444,  75854,  76266,  76680,  77096,  77515,
	     77936,  78359,  78785,  79212,  79642,  80075,  80510,  80947,  81386,  81828,  82273,  82719,  83169,  83620,  84074,  84531,
	     84990,  85451,  85915,  86382,  86851,  87322,  87796,  88273,  88752,  89234,  89719,  90206,  90696,  91188,  91684,  92181,
	     92682,  93185,  93691,  94200,  94711,  95226,  95743,  96263,  96785,  97311,  97839,  98370,  98905,  99442,  99982, 100524,
	    101070, 101619, 102171, 102726, 103283, 103844, 104408, 104975, 105545, 106118, 106694, 107274, 107856, 108442, 109031, 109623,
	    110218, 110816, 111418, 112023, 112631, 113243, 113858, 114476, 115098, 115723, 116351, 116983, 117618, 118257, 118899, 119544,
	    120194, 120846, 121502, 122162, 122825, 123492, 124163, 124837, 125515, 126197, 126882, 127571, 128263, 128960, 129660, 130364,
	    131072,
	};

	// 512-point (9-bit) lookup table
	// If memory was a concern we could reduce this to ~1/4 of the size,
	// by exploiting symmetry of sine wave, but this only uses 2KB of ~250KB on the RP2040
	constexpr static unsigned tableSize = 512;
	int16_t sine[tableSize];

	// Bitwise AND of index integer with tableMask will wrap it to table size
	constexpr static uint32_t tableMask = tableSize - 1;

	// Per-voice phase accumulators.
	uint32_t phases[VOICES];
	ControlFrame control_frames[2];
	// Core0 writes the latest controls; core1 reads snapshot to build the next frame.
	volatile ControlInputSnapshot control_snapshot;
		// Core1 publishes the active frame index after writing the inactive frame.
		volatile uint32_t published_frame_index;
		int control_counter;

		// Converts a 0..4095 main knob snapshot to base phase increment (10Hz..10240Hz).
		inline uint32_t BasePhaseIncrementFromMainKnob(uint32_t knob)
		{
			if (knob > 4095U) knob = 4095U;

		// Map knob 0..4095 to 10 octaves: 10Hz..10240Hz.
		// 512 substeps per octave => 10*512 = 5120 total positions.
		uint32_t pos = (knob * 5122U) >> 12; // shift-only approximation
		if (pos > 5120U) pos = 5120U;

		uint32_t octave_int = pos >> 9; // 0..10
		uint32_t sub = pos & 0x1FF;     // 0..511

		uint32_t inc = BASE_PHASE_INC_10HZ << octave_int;

		// Use 2-bit interpolation between 1/128-octave table points to get 1/512-octave resolution.
		uint32_t idx = sub >> 2;    // 0..127
		uint32_t frac = sub & 0x3;  // 0..3
		uint32_t m1 = EXP2_Q16[idx];
		uint32_t m2 = EXP2_Q16[idx + 1];
		uint32_t mult = m1 + (((m2 - m1) * frac) >> 2); // Q16

			return uint32_t((uint64_t(inc) * mult) >> 16);
		}

		// Builds a complete control frame in the inactive buffer, then atomically publishes it.
		void PublishControlFrameFromCore1(uint32_t base_inc)
		{
			uint32_t read_index = published_frame_index;
		uint32_t write_index = read_index ^ 1U;
		ControlFrame& write_frame = control_frames[write_index];

		// Write a complete frame into the inactive buffer.
		for (int i = 0; i < VOICES; ++i) {
			write_frame.voice_phase_increment[i] = uint32_t((uint64_t(base_inc) * VOICE_RATIO_Q16[i]) >> 16);
			write_frame.voice_gain_q12[i] = VOICE_GAIN_Q12[i];
		}
		write_frame.mix_norm_q12 = MIX_NORM_Q12;

		// Ensure frame writes complete before making the new frame visible.
		__dmb();
			published_frame_index = write_index;
		}

		// Static trampoline required by pico_multicore to call the instance loop on core 1.
		static void core1_entry()
		{
			((SineWaveLookup *)ThisPtr())->ControlCoreLoop();
		}

		// Core 1 control-rate worker: reads snapshots, computes frame, and publishes periodically.
		void ControlCoreLoop()
		{
			while (1)
		{
			// Step 6 split: core1 owns control math and frame publication cadence.
			__dmb();
			uint32_t main_knob = control_snapshot.main_knob;
			uint32_t base_inc = BasePhaseIncrementFromMainKnob(main_knob);
			PublishControlFrameFromCore1(base_inc);
				busy_wait_us_32(CORE1_CONTROL_PERIOD_US);
			}
		}
		
		// Initializes lookup table/state, primes control buffers, and launches core 1.
		SineWaveLookup()
		{
			// Initialise phase of sine wave to 0
		for (int i = 0; i < VOICES; ++i) {
			phases[i] = 0;
		}
		uint32_t initial_base_phase_increment = BASE_PHASE_INC_10HZ << 5; // 320Hz default before first knob read
		control_snapshot.main_knob = 2048U;
		control_snapshot.knob_x = 0U;
		control_snapshot.knob_y = 0U;
		control_snapshot.switch_pos = uint32_t(Middle);
		for (int i = 0; i < VOICES; ++i) {
			control_frames[0].voice_phase_increment[i] = uint32_t((uint64_t(initial_base_phase_increment) * VOICE_RATIO_Q16[i]) >> 16);
			control_frames[0].voice_gain_q12[i] = VOICE_GAIN_Q12[i];
			control_frames[1].voice_phase_increment[i] = control_frames[0].voice_phase_increment[i];
			control_frames[1].voice_gain_q12[i] = control_frames[0].voice_gain_q12[i];
		}
		control_frames[0].mix_norm_q12 = MIX_NORM_Q12;
		control_frames[1].mix_norm_q12 = MIX_NORM_Q12;
		published_frame_index = 0;
		control_counter = CONTROL_DIVISOR; // force control update on first sample
		
		for (unsigned i=0; i<tableSize; i++)
		{
			// just shy of 2^15 * sin
			sine[i] = int16_t(32000*sin(2*i*M_PI/double(tableSize)));
		}

			multicore_launch_core1(core1_entry);
		}

		// Core 0 control snapshot capture: reads UI and stores values for core 1 consumption.
		inline void ControlUpdates()
		{
			// Step 6 split: core0 captures controls into a shared snapshot.
		int32_t main = KnobVal(Knob::Main);
		if (main < 0) main = 0;
		if (main > 4095) main = 4095;
		int32_t x = KnobVal(Knob::X);
		if (x < 0) x = 0;
		if (x > 4095) x = 4095;
		int32_t y = KnobVal(Knob::Y);
		if (y < 0) y = 0;
		if (y > 4095) y = 4095;

		control_snapshot.main_knob = uint32_t(main);
		control_snapshot.knob_x = uint32_t(x);
		control_snapshot.knob_y = uint32_t(y);
		control_snapshot.switch_pos = uint32_t(SwitchVal());
			__dmb();
		}

		// 32-bit phase to sine conversion with linear interpolation from 512-point table.
		inline int32_t LookupSineLinear(uint32_t p)
		{
			uint32_t index = p >> 23; // convert from 32-bit phase to 9-bit lookup table index
		int32_t r = (p & 0x7FFFFF) >> 7; // fractional part is last 23 bits of phase, shifted to 16-bit

		int32_t s1 = sine[index];
		int32_t s2 = sine[(index + 1) & tableMask];
			return (s2 * r + s1 * (65536 - r)) >> 20;
		}
		
		// Audio-rate loop: runs control snapshot cadence, consumes published frame, and outputs mono sum.
		virtual void ProcessSample()
		{
			// This creates a slower control update path. If only runs once every 16 (CONTROL_DIVISOR) 
		// samples, at about 3 kHz.
		if (++control_counter >= CONTROL_DIVISOR)
		{
			control_counter = 0;
			ControlUpdates();
		}

		// Read published index then barrier before consuming that frame data.
		uint32_t read_index = published_frame_index;
		__dmb();
		const ControlFrame& frame = control_frames[read_index];

		int32_t sum = 0;
		for (int i = 0; i < VOICES; ++i) {
			phases[i] += frame.voice_phase_increment[i];
			int32_t s = LookupSineLinear(phases[i]);
			sum += (s * frame.voice_gain_q12[i]) >> 12;
		}

		// Fixed linear normalization keeps headroom and preserves harmonic ratio.
		int32_t out = int32_t((int64_t(sum) * frame.mix_norm_q12) >> 12);
		if (out > 2047) out = 2047;
		if (out < -2048) out = -2048;

		AudioOut1(out);
		AudioOut2(out);
	}
};


// Program entry: constructs the card app and starts the blocking audio run loop.
int main()
{
	SineWaveLookup sw;
	sw.Run();
}

  
