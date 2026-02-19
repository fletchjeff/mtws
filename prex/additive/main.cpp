#include "ComputerCard.h"

class AdditiveBaseline : public ComputerCard
{
public:
	static constexpr uint32_t BASE_PHASE_INC_20HZ = 1789569U; // 20 * 2^32 / 48000
	static constexpr uint32_t CUTOFF_PHASE_INC_90PCT_NYQUIST = 1932735283U; // 21600 * 2^32 / 48000
	static constexpr unsigned TABLE_SIZE = 256;
	static constexpr uint32_t TABLE_MASK = TABLE_SIZE - 1;
	static constexpr int PARTIALS = 12;
	static constexpr int CONTROL_DIVISOR = 16; // control-rate decimation
	static constexpr int32_t UNITY_Q12 = 4096;
	static constexpr int32_t X_CENTER = 2048;
	static constexpr int32_t X_CENTER_DEADBAND = 80; // center "detent" width
	static constexpr int32_t X_EDGE_DEADBAND = 80;   // edge lock width for exact 1x/3x
	static constexpr int32_t FULL_SHIFT_MORPH_GAIN_NUM = 5; // push edges to full collapse sooner
	static constexpr int32_t FULL_SHIFT_MORPH_GAIN_DEN = 4;
	static constexpr uint32_t CUTOFF_PHASE_INC_FADE_START = 1811939327U; // 15/16 of cutoff
	static constexpr int32_t Y_LOW_TAPER_RANGE = 256; // smooth fade of upper partials near CCW
	static constexpr uint32_t EXP2_Q16[129] = {
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

	static constexpr int16_t sine_table[TABLE_SIZE] = {
	        0,    50,   100,   151,   201,   251,   300,   350,   399,   449,   497,   546,   594,   642,   690,   737,
	      783,   830,   875,   920,   965,  1009,  1052,  1095,  1137,  1179,  1219,  1259,  1299,  1337,  1375,  1411,
	     1447,  1483,  1517,  1550,  1582,  1614,  1644,  1674,  1702,  1729,  1756,  1781,  1805,  1828,  1850,  1871,
	     1891,  1910,  1927,  1944,  1959,  1973,  1986,  1997,  2008,  2017,  2025,  2032,  2037,  2041,  2045,  2046,
	     2047,  2046,  2045,  2041,  2037,  2032,  2025,  2017,  2008,  1997,  1986,  1973,  1959,  1944,  1927,  1910,
	     1891,  1871,  1850,  1828,  1805,  1781,  1756,  1729,  1702,  1674,  1644,  1614,  1582,  1550,  1517,  1483,
	     1447,  1411,  1375,  1337,  1299,  1259,  1219,  1179,  1137,  1095,  1052,  1009,   965,   920,   875,   830,
	      783,   737,   690,   642,   594,   546,   497,   449,   399,   350,   300,   251,   201,   151,   100,    50,
	        0,   -50,  -100,  -151,  -201,  -251,  -300,  -350,  -399,  -449,  -497,  -546,  -594,  -642,  -690,  -737,
	     -783,  -830,  -875,  -920,  -965, -1009, -1052, -1095, -1137, -1179, -1219, -1259, -1299, -1337, -1375, -1411,
	    -1447, -1483, -1517, -1550, -1582, -1614, -1644, -1674, -1702, -1729, -1756, -1781, -1805, -1828, -1850, -1871,
	    -1891, -1910, -1927, -1944, -1959, -1973, -1986, -1997, -2008, -2017, -2025, -2032, -2037, -2041, -2045, -2046,
	    -2047, -2046, -2045, -2041, -2037, -2032, -2025, -2017, -2008, -1997, -1986, -1973, -1959, -1944, -1927, -1910,
	    -1891, -1871, -1850, -1828, -1805, -1781, -1756, -1729, -1702, -1674, -1644, -1614, -1582, -1550, -1517, -1483,
	    -1447, -1411, -1375, -1337, -1299, -1259, -1219, -1179, -1137, -1095, -1052, -1009,  -965,  -920,  -875,  -830,
	     -783,  -737,  -690,  -642,  -594,  -546,  -497,  -449,  -399,  -350,  -300,  -251,  -201,  -151,  -100,   -50,
	};
	static constexpr int16_t PARTIAL_GAIN_Q12[PARTIALS] = {
		4096, // 1/1
		2048, // 1/2
		1365, // 1/3
		1024, // 1/4
		 819, // 1/5
		 683, // 1/6
		 585, // 1/7
		 512, // 1/8
		 455, // 1/9
		 410, // 1/10
		 372, // 1/11
		 341, // 1/12
	};

	uint32_t phases[PARTIALS] = {};
	int32_t pitch_smoothed = 2048 << 4; // 12.4 fixed-point smoothed pitch control
	int control_counter = CONTROL_DIVISOR;
	uint32_t cached_partial_inc[PARTIALS] = {};
	int32_t cached_gain_q12[PARTIALS] = {};
	uint8_t cached_alias_w_q7[PARTIALS] = {};
	uint32_t cached_inv_gain_q16 = 0;

	inline int16_t __not_in_flash_func(LookupSine)(uint32_t p)
	{
		uint32_t index = (p >> 24) & TABLE_MASK;
		uint32_t frac = (p >> 16) & 0xFF; // 8-bit fractional interpolation

		int32_t s1 = sine_table[index];
		int32_t s2 = sine_table[(index + 1) & TABLE_MASK];
		return int16_t(s1 + (((s2 - s1) * int32_t(frac)) >> 8));
	}

	inline uint32_t __not_in_flash_func(PitchToPhaseInc)(int32_t pitch_control)
	{
		// Map 0..4095 to 20Hz..5120Hz (exactly 8 octaves).
		// octaves_fp16 runs from 0.0 to 8.0 in Q16.
		uint32_t octaves_fp16 = (uint32_t(pitch_control) * (8U << 16)) / 4095U;
		uint32_t octave_int = octaves_fp16 >> 16;   // 0..8
		uint32_t octave_frac = octaves_fp16 & 0xFFFF;

		uint32_t phase_inc = BASE_PHASE_INC_20HZ;
		if (octave_int > 8) octave_int = 8;
		phase_inc <<= octave_int;

		// Fractional octave multiplier from lookup table with linear interpolation.
		uint32_t idx = octave_frac >> 9;      // 0..127
		uint32_t frac = octave_frac & 0x1FF;  // 0..511
		uint32_t m1 = EXP2_Q16[idx];
		uint32_t m2 = EXP2_Q16[idx + 1];
		uint32_t mult = m1 + (((m2 - m1) * frac) >> 9); // Q16

		return uint32_t((uint64_t(phase_inc) * mult) >> 16);
	}

	inline int32_t __not_in_flash_func(PartialGainQ12)(int i, int32_t y)
	{
		if (i == 0) return UNITY_Q12; // fundamental is always full-scale
		int32_t base = PARTIAL_GAIN_Q12[i];
		if (y <= 2048) {
			// CCW -> center: fade from 0 to 1/n
			return (base * y) >> 11;
		}
		// Center -> CW: fade from 1/n to 1
		int32_t mix = y - 2048; // 0..2047
		return base + (((UNITY_Q12 - base) * mix) >> 11);
	}

	inline int32_t __not_in_flash_func(MorphQ16FromX)(int32_t x)
	{
		// Morph control with detents:
		// center -> 0, full CCW -> -1, full CW -> +1.
		if (x <= X_EDGE_DEADBAND) return -(1 << 16);
		if (x >= (4095 - X_EDGE_DEADBAND)) return (1 << 16);
		if (x >= (X_CENTER - X_CENTER_DEADBAND) && x <= (X_CENTER + X_CENTER_DEADBAND)) {
			return 0;
		}

		if (x < (X_CENTER - X_CENTER_DEADBAND)) {
			// Map [edge..center-left] -> [-1..0]
			int32_t in_lo = X_EDGE_DEADBAND;
			int32_t in_hi = X_CENTER - X_CENTER_DEADBAND;
			int32_t num = x - in_lo;
			int32_t den = in_hi - in_lo;
			return -(1 << 16) + int32_t((int64_t(num) * (1 << 16)) / den);
		}

		// Map [center-right..edge] -> [0..+1]
		int32_t in_lo = X_CENTER + X_CENTER_DEADBAND;
		int32_t in_hi = 4095 - X_EDGE_DEADBAND;
		int32_t num = x - in_lo;
		int32_t den = in_hi - in_lo;
		return int32_t((int64_t(num) * (1 << 16)) / den);
	}

	inline int32_t __not_in_flash_func(HarmonicQ16FromMorph)(int i, int32_t morph_q16)
	{
		// i=0 is fundamental, always 1x.
		if (i == 0) return (1 << 16);

		int32_t base = (i + 1) << 16; // harmonic series at center
		if (morph_q16 < 0) {
			// CCW: collapse upper partials toward 1x.
			int32_t u = -morph_q16; // 0..65536
			return base - int32_t((int64_t(base - (1 << 16)) * u) >> 16);
		}

		// CW: collapse upper partials toward 3x.
		int32_t u = morph_q16; // 0..65536
		return base + int32_t((int64_t((3 << 16) - base) * u) >> 16);
	}

	inline int32_t __not_in_flash_func(HarmonicQ16SingleShift)(int i, int32_t morph_q16)
	{
		// Single-step shift mode:
		// center -> n, full CCW -> n-1, full CW -> n+1
		if (i == 0) return (1 << 16);
		int32_t base = (i + 1) << 16;
		int32_t h = base + morph_q16;
		if (h < (1 << 16)) h = (1 << 16);
		return h;
	}

	inline uint8_t __not_in_flash_func(CleanAliasWeightQ7)(uint32_t partial_inc)
	{
		if (partial_inc <= CUTOFF_PHASE_INC_FADE_START) return 128;
		if (partial_inc >= CUTOFF_PHASE_INC_90PCT_NYQUIST) return 0;
		uint32_t rem = CUTOFF_PHASE_INC_90PCT_NYQUIST - partial_inc;
		uint32_t span = CUTOFF_PHASE_INC_90PCT_NYQUIST - CUTOFF_PHASE_INC_FADE_START;
		return uint8_t((uint64_t(rem) * 128U) / span);
	}

	void __not_in_flash_func(UpdateControlRate)()
	{
		int32_t x = KnobVal(Knob::X);
		if (x < 0) x = 0;
		if (x > 4095) x = 4095;
		int32_t y = KnobVal(Knob::Y);
		if (y < 0) y = 0;
		if (y > 4095) y = 4095;

		uint32_t phase_inc = PitchToPhaseInc(pitch_smoothed >> 4);
		Switch mode = SwitchVal();
		bool full_shift_mode = (mode != Middle); // Up + Down = full_shift, Middle = single_shift
		int32_t morph_q16 = MorphQ16FromX(x);
		int32_t morph_full_q16 = morph_q16;
		if (full_shift_mode) {
			morph_full_q16 = (morph_q16 * FULL_SHIFT_MORPH_GAIN_NUM) / FULL_SHIFT_MORPH_GAIN_DEN;
			if (morph_full_q16 > (1 << 16)) morph_full_q16 = (1 << 16);
			if (morph_full_q16 < -(1 << 16)) morph_full_q16 = -(1 << 16);
		}

		int32_t gain_sum = 0;
		for (int i = 0; i < PARTIALS; ++i) {
			int32_t harmonic_q16 = full_shift_mode
				? HarmonicQ16FromMorph(i, morph_full_q16)
				: HarmonicQ16SingleShift(i, morph_q16);
			uint32_t partial_inc = uint32_t((uint64_t(phase_inc) * uint32_t(harmonic_q16)) >> 16);

			int32_t gain_q12 = PartialGainQ12(i, y);
			// Smoothly taper upper partials to zero near full CCW without a hard jump.
			if (i > 0 && y < Y_LOW_TAPER_RANGE) {
				gain_q12 = (gain_q12 * y) / Y_LOW_TAPER_RANGE;
			}
			uint8_t alias_w_q7 = CleanAliasWeightQ7(partial_inc);

			cached_partial_inc[i] = partial_inc;
			cached_gain_q12[i] = gain_q12;
			cached_alias_w_q7[i] = alias_w_q7;
			gain_sum += (gain_q12 * alias_w_q7) >> 7;
		}

		if (gain_sum > 0) {
			cached_inv_gain_q16 = uint32_t((uint64_t(UNITY_Q12) << 16) / uint32_t(gain_sum));
		} else {
			cached_inv_gain_q16 = 0;
		}
	}

	void ProcessSample() override __not_in_flash_func()
	{
		// Main knob + attenuated CV1, then smooth to reduce audible stepping.
		int32_t pitch = KnobVal(Knob::Main) + (CVIn1() >> 1);
		if (pitch < 0) pitch = 0;
		if (pitch > 4095) pitch = 4095;

		pitch_smoothed = (31 * pitch_smoothed + (pitch << 4)) >> 5;
		if (++control_counter >= CONTROL_DIVISOR || SwitchChanged()) {
			control_counter = 0;
			UpdateControlRate();
		}

		int32_t sum = 0;
		for (int i = 0; i < PARTIALS; ++i) {
			phases[i] += cached_partial_inc[i];
			int32_t s = LookupSine(phases[i]);
			int32_t contrib = (s * cached_gain_q12[i]) >> 12;
			sum += (contrib * cached_alias_w_q7[i]) >> 7;
		}

		int32_t out = 0;
		if (cached_inv_gain_q16 > 0) {
			out = int32_t((int64_t(sum) * cached_inv_gain_q16) >> 16);
		}

		if (out > 2047) out = 2047;
		if (out < -2048) out = -2048;

		AudioOut1(out);
		AudioOut2(out);
	}
};

int main()
{
	AdditiveBaseline synth;
	synth.Run();
}
