#include "ComputerCard.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include <cmath>

/// Three-voice additive sine oscillator scaffold (centroid_3).
///
/// Current implementation:
/// - Voice ratios are harmonic: 1x, 2x, 3x
/// - Main knob controls base pitch over 10 octaves: 10Hz..10240Hz
/// - X knob controls centroid position of a symmetric gain bump
/// - Baseline gain curve is g(d) = 1 / (1 + d), where d is partial-index distance
/// - Y knob controls bump width:
///   - CCW: narrow (at most ~2 active partials)
///   - Mid: baseline 1/(1+d)
///   - CW: flat (all partials equal)
/// - In Switch::Up mode, Y also warps frequencies around midpoint:
///   - Y<50%: collapse ratios toward centroid ratio
///   - Y=50%: harmonic ratios (calm point)
///   - Y>50%: repel ratios away from centroid ratio
/// - Step 10 adds smoothing of per-voice ratio/gain targets on core 1
///   and clamps on published frame values for stability.
/// - Control-rate computation is offloaded to core 1
/// - Voices are summed to mono, normalized per control frame, and sent to both outputs
///
/// This structure is intended to scale to higher voice counts (e.g. 16 voices)
/// with minimal code changes.

class SineWaveLookup : public ComputerCard
{
public:
	constexpr static uint32_t BASE_PHASE_INC_10HZ = 894785U; // 10 * 2^32 / 48000
	constexpr static int CONTROL_DIVISOR = 16;
	constexpr static int VOICES = 16;
	constexpr static uint32_t UNITY_Q12 = 4096U;
	constexpr static int32_t MIN_RATIO_Q16 = 6554;     // 0.1x lower bound
	constexpr static int32_t MAX_RATIO_Q16 = 1048576;  // 16x hard upper bound
	constexpr static uint32_t NYQUIST_PHASE_INC = 0x7FFFFFFFU;
	constexpr static int RATIO_SMOOTH_SHIFT = 3;       // 1/8 smoothing per control tick
	constexpr static int GAIN_SMOOTH_SHIFT = 3;        // 1/8 smoothing per control tick
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

	// Core 0 writes these snapshot values; core 1 reads them to compute the next frame.
	volatile ControlInputSnapshot control_snapshot;

	// Core 1 publishes this active frame index after fully writing the inactive frame.
	volatile uint32_t published_frame_index;

	int control_counter;
	int32_t smoothed_ratio_q16[VOICES];
	int32_t smoothed_gain_q12[VOICES];

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
		uint32_t idx = sub >> 2;   // 0..127
		uint32_t frac = sub & 0x3; // 0..3
		uint32_t m1 = EXP2_Q16[idx];
		uint32_t m2 = EXP2_Q16[idx + 1];
		uint32_t mult = m1 + (((m2 - m1) * frac) >> 2); // Q16

		return uint32_t((uint64_t(inc) * mult) >> 16);
	}

	// Step 8a gain model: distance from centroid in Q12 partial-index units.
	// Returns Q12 gain g(d) = 1 / (1 + d), where d >= 0.
	inline int32_t CentroidBaseGainQ12(int partial_index, uint32_t centroid_q12)
	{
		uint32_t pos_q12 = uint32_t(partial_index) << 12;
		uint32_t d_q12 = (pos_q12 >= centroid_q12) ? (pos_q12 - centroid_q12) : (centroid_q12 - pos_q12);
		uint32_t denom_q12 = UNITY_Q12 + d_q12;
		uint64_t numer = uint64_t(UNITY_Q12) * UNITY_Q12; // 1.0 in Q12 divided by Q12 denominator
		return int32_t((numer + (denom_q12 >> 1)) / denom_q12); // rounded divide on control core
	}

	// Step 8b narrow endpoint curve: g_narrow(d) = max(0, 1 - d), in Q12.
	inline int32_t CentroidNarrowGainQ12(int partial_index, uint32_t centroid_q12)
	{
		uint32_t pos_q12 = uint32_t(partial_index) << 12;
		uint32_t d_q12 = (pos_q12 >= centroid_q12) ? (pos_q12 - centroid_q12) : (centroid_q12 - pos_q12);
		if (d_q12 >= UNITY_Q12) return 0;
		return int32_t(UNITY_Q12 - d_q12);
	}

	// Linear interpolation in Q12 between a and b using t in 0..4096.
	inline int32_t LerpQ12(int32_t a, int32_t b, uint32_t t_q12)
	{
		if (t_q12 > UNITY_Q12) t_q12 = UNITY_Q12;
		return int32_t((int64_t(a) * int32_t(UNITY_Q12 - t_q12) + int64_t(b) * int32_t(t_q12)) >> 12);
	}

	// Integer one-pole smoothing step with guaranteed progress for small deltas.
	inline int32_t SmoothToward(int32_t current, int32_t target, int shift)
	{
		if (shift <= 0) return target;
		int32_t delta = target - current;
		int32_t step = delta >> shift;
		if (step == 0 && delta != 0) {
			step = (delta > 0) ? 1 : -1;
		}
		return current + step;
	}

	// Harmonic ratio in Q16 for voice index i (0->1x, 1->2x, ...).
	inline int32_t HarmonicRatioQ16(int i)
	{
		return int32_t((i + 1) << 16);
	}

	// Step 8b piecewise Y shaping:
	// - Y low half: blend narrow -> base
	// - Y high half: blend base -> flat
	inline int32_t ShapedCentroidGainQ12(int partial_index, uint32_t centroid_q12, uint32_t knob_y)
	{
		if (knob_y > 4095U) knob_y = 4095U;

		int32_t g_base = CentroidBaseGainQ12(partial_index, centroid_q12);
		int32_t g_narrow = CentroidNarrowGainQ12(partial_index, centroid_q12);

		if (knob_y <= 2048U) {
			uint32_t t_q12 = knob_y << 1; // 0..4096 across lower half
			return LerpQ12(g_narrow, g_base, t_q12);
		}

		uint32_t t_q12 = (knob_y - 2048U) << 1; // 0..4094 across upper half
		if (knob_y >= 4095U) t_q12 = UNITY_Q12; // force exact flat endpoint
		return LerpQ12(g_base, int32_t(UNITY_Q12), t_q12);
	}

	// Builds a complete control frame in the inactive buffer, then atomically publishes it.
	void PublishControlFrameFromCore1(uint32_t base_inc, uint32_t knob_x, uint32_t knob_y, uint32_t switch_pos)
	{
		uint32_t read_index = published_frame_index;
		uint32_t write_index = read_index ^ 1U;
		ControlFrame& write_frame = control_frames[write_index];

		if (knob_x > 4095U) knob_x = 4095U;
		uint32_t centroid_max_q12 = uint32_t(VOICES - 1) << 12;
		uint32_t centroid_q12 = uint32_t((uint64_t(knob_x) * centroid_max_q12 + 2048U) >> 12);
		int32_t centroid_ratio_q16 = int32_t((1U << 16) + (centroid_q12 << 4));
		bool alt_mode = (switch_pos == uint32_t(Up));
		int32_t max_ratio_q16 = MAX_RATIO_Q16;
		if (base_inc > 0U) {
			uint32_t max_ratio_from_nyquist_q16 = uint32_t((uint64_t(NYQUIST_PHASE_INC) << 16) / base_inc);
			if (max_ratio_from_nyquist_q16 < uint32_t(max_ratio_q16)) {
				max_ratio_q16 = int32_t(max_ratio_from_nyquist_q16);
			}
		}
		if (max_ratio_q16 < MIN_RATIO_Q16) max_ratio_q16 = MIN_RATIO_Q16;

		int32_t gain_sum_q12 = 0;
		for (int i = 0; i < VOICES; ++i) {
			int32_t target_ratio_q16 = HarmonicRatioQ16(i);
			if (alt_mode) {
				if (knob_y < 2048U) {
					uint32_t warp_q12 = (2048U - knob_y) << 1; // 0..4096
					target_ratio_q16 = LerpQ12(target_ratio_q16, centroid_ratio_q16, warp_q12);
				} else if (knob_y > 2048U) {
					uint32_t warp_q12 = (knob_y - 2048U) << 1; // 0..4094
					if (knob_y >= 4095U) warp_q12 = UNITY_Q12;
					int32_t repel_ratio_q16 = (target_ratio_q16 << 1) - centroid_ratio_q16;
					target_ratio_q16 = LerpQ12(target_ratio_q16, repel_ratio_q16, warp_q12);
				}
				if (target_ratio_q16 < MIN_RATIO_Q16) target_ratio_q16 = MIN_RATIO_Q16;
				if (target_ratio_q16 > max_ratio_q16) target_ratio_q16 = max_ratio_q16;
			}

			smoothed_ratio_q16[i] = SmoothToward(smoothed_ratio_q16[i], target_ratio_q16, RATIO_SMOOTH_SHIFT);
			if (smoothed_ratio_q16[i] < MIN_RATIO_Q16) smoothed_ratio_q16[i] = MIN_RATIO_Q16;
			if (smoothed_ratio_q16[i] > max_ratio_q16) smoothed_ratio_q16[i] = max_ratio_q16;

			uint32_t voice_inc = uint32_t((uint64_t(base_inc) * uint32_t(smoothed_ratio_q16[i])) >> 16);
			if (voice_inc > NYQUIST_PHASE_INC) voice_inc = NYQUIST_PHASE_INC;
			write_frame.voice_phase_increment[i] = voice_inc;

			int32_t target_gain_q12 = ShapedCentroidGainQ12(i, centroid_q12, knob_y);
			if (target_gain_q12 < 0) target_gain_q12 = 0;
			if (target_gain_q12 > int32_t(UNITY_Q12)) target_gain_q12 = int32_t(UNITY_Q12);
			smoothed_gain_q12[i] = SmoothToward(smoothed_gain_q12[i], target_gain_q12, GAIN_SMOOTH_SHIFT);
			if (smoothed_gain_q12[i] < 0) smoothed_gain_q12[i] = 0;
			if (smoothed_gain_q12[i] > int32_t(UNITY_Q12)) smoothed_gain_q12[i] = int32_t(UNITY_Q12);
			write_frame.voice_gain_q12[i] = smoothed_gain_q12[i];
			gain_sum_q12 += smoothed_gain_q12[i];
		}

		if (gain_sum_q12 > 0) {
			uint64_t numer = uint64_t(UNITY_Q12) * UNITY_Q12;
			write_frame.mix_norm_q12 = int32_t((numer + (uint32_t(gain_sum_q12) >> 1)) / uint32_t(gain_sum_q12));
		} else {
			write_frame.mix_norm_q12 = int32_t(UNITY_Q12);
		}
		if (write_frame.mix_norm_q12 < 0) write_frame.mix_norm_q12 = 0;
		if (write_frame.mix_norm_q12 > int32_t(UNITY_Q12)) write_frame.mix_norm_q12 = int32_t(UNITY_Q12);

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
			__dmb();
			uint32_t main_knob = control_snapshot.main_knob;
			uint32_t knob_x = control_snapshot.knob_x;
			uint32_t knob_y = control_snapshot.knob_y;
			uint32_t switch_pos = control_snapshot.switch_pos;
			PublishControlFrameFromCore1(BasePhaseIncrementFromMainKnob(main_knob), knob_x, knob_y, switch_pos);
			busy_wait_us_32(CORE1_CONTROL_PERIOD_US);
		}
	}

	// Initializes lookup table/state, primes control buffers, and launches core 1.
	SineWaveLookup()
	{
		for (int i = 0; i < VOICES; ++i) {
			phases[i] = 0;
		}

		control_snapshot.main_knob = 2048U;
		control_snapshot.knob_x = 0U;
		control_snapshot.knob_y = 0U;
		control_snapshot.switch_pos = uint32_t(Middle);

		uint32_t initial_base_inc = BASE_PHASE_INC_10HZ << 5; // 320Hz default before first knob read
		uint32_t initial_centroid_q12 = 0U; // X=0 -> centroid at partial 1 (index 0)
		int32_t initial_gain_sum_q12 = 0;

		for (int i = 0; i < VOICES; ++i) {
			control_frames[0].voice_phase_increment[i] = uint32_t((uint64_t(initial_base_inc) * uint32_t(HarmonicRatioQ16(i))) >> 16);
			control_frames[0].voice_gain_q12[i] = ShapedCentroidGainQ12(i, initial_centroid_q12, control_snapshot.knob_y);
			smoothed_ratio_q16[i] = HarmonicRatioQ16(i);
			smoothed_gain_q12[i] = control_frames[0].voice_gain_q12[i];
			initial_gain_sum_q12 += control_frames[0].voice_gain_q12[i];
		}

		if (initial_gain_sum_q12 > 0) {
			uint64_t numer = uint64_t(UNITY_Q12) * UNITY_Q12;
			control_frames[0].mix_norm_q12 = int32_t((numer + (uint32_t(initial_gain_sum_q12) >> 1)) / uint32_t(initial_gain_sum_q12));
		} else {
			control_frames[0].mix_norm_q12 = int32_t(UNITY_Q12);
		}

		for (int i = 0; i < VOICES; ++i) {
			control_frames[1].voice_phase_increment[i] = control_frames[0].voice_phase_increment[i];
			control_frames[1].voice_gain_q12[i] = control_frames[0].voice_gain_q12[i];
		}
		control_frames[1].mix_norm_q12 = control_frames[0].mix_norm_q12;

		published_frame_index = 0;
		control_counter = CONTROL_DIVISOR; // force control update on first sample

		for (unsigned i = 0; i < tableSize; i++)
		{
			// just shy of 2^15 * sin
			sine[i] = int16_t(32000 * sin(2 * i * M_PI / double(tableSize)));
		}

		multicore_launch_core1(core1_entry);
	}

	// Core 0 control snapshot capture: reads UI and stores values for core 1 consumption.
	inline void ControlUpdates()
	{
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
		control_snapshot.knob_y = uint32_t(y); // Step 8b width + Step 9 alt-mode frequency warp.
		control_snapshot.switch_pos = uint32_t(SwitchVal());
		__dmb();
	}

	// 32-bit phase to sine conversion with linear interpolation from 512-point table.
	inline int32_t LookupSineLinear(uint32_t p)
	{
		uint32_t index = p >> 23;           // convert from 32-bit phase to 9-bit table index
		int32_t r = (p & 0x7FFFFF) >> 7;    // fractional part in Q16
		int32_t s1 = sine[index];
		int32_t s2 = sine[(index + 1) & tableMask];
		return (s2 * r + s1 * (65536 - r)) >> 20;
	}

	// Audio-rate loop: runs control snapshot cadence, consumes published frame, and outputs mono sum.
	virtual void ProcessSample()
	{
		// This creates a slower control update path. If only runs once every 16 samples,
		// the control-rate path runs at about 3kHz.
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
