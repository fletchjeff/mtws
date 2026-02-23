#include "ComputerCard.h"
#include "hardware/interp.h"
#include "pitch_phase_lut.h"

#include <array>
#include <cstdint>

/*
 * Minimal Additive Oscillator (MVP)
 * ---------------------------------
 * - 16 harmonic partials
 * - Centroid on X knob, using approach #2 (blend adjacent integer-centroid profiles)
 * - Y controls slope width:
 *   - Y full CCW: narrow peak around centroid
 *   - Y full CW : full 1/n slope from centroid to edges
 * - Nyquist-safe partial count (anti-aliasing)
 * - Normalization via LUT + interpolation (no runtime division)
 * - Main knob controls fundamental pitch
 * - Integer/fixed-point DSP path + RP2040 interpolator phase accumulator
 */
class MinimalAdditiveOscillator
{
public:
	static constexpr int kPartials = 16;
	static constexpr uint32_t kNyquistPhaseInc = 0x80000000u;
	static constexpr uint32_t kOneQ24 = 16777216u;
	static constexpr uint32_t kQ12One = 4096u;

	MinimalAdditiveOscillator(interp_hw_t *interp) : interp_(interp)
	{
		InitInterpolator();
		SetBasePhaseIncrement(0);
		SetCentroidKnob(0);
		SetSlopeKnob(4095);
		UpdateWeightsAndNormalization();
	}

	bool SetBasePhaseIncrement(uint32_t phase_inc)
	{
		base_phase_inc_ = phase_inc;

		// Anti-aliasing: keep only harmonics below Nyquist by phase increment.
		int new_safe_partial_count = 0;
		for (int i = 0; i < kPartials; ++i) {
			uint64_t partial_inc = static_cast<uint64_t>(base_phase_inc_) * static_cast<uint64_t>(i + 1);
			if (partial_inc < kNyquistPhaseInc) {
				++new_safe_partial_count;
			} else {
				break;
			}
		}

		bool changed = (new_safe_partial_count != safe_partial_count_);
		safe_partial_count_ = new_safe_partial_count;
		return changed;
	}

	void SetCentroidKnob(uint16_t centroid_knob)
	{
		centroid_knob_ = centroid_knob;
	}

	void SetSlopeKnob(uint16_t slope_knob)
	{
		slope_knob_ = slope_knob;
	}

	void UpdateWeightsAndNormalization()
	{
		// X knob mapped to centroid position in Q12 over [0, 15].
		// Endpoint lock ensures full CW lands exactly on partial 15.
		uint32_t centroid_q12 = (centroid_knob_ >= 4095u)
			? (15u << 12)
			: (static_cast<uint32_t>(centroid_knob_) * 15u);
		uint32_t slope_scale_q12 = SlopeScaleQ12();

		uint32_t center0 = centroid_q12 >> 12;
		if (center0 > 15u) center0 = 15u;
		uint32_t center1 = (center0 < 15u) ? (center0 + 1u) : 15u;
		uint32_t frac_q12 = centroid_q12 & 0x0FFFu;

		uint64_t sum_q24 = 0;
		for (int i = 0; i < safe_partial_count_; ++i) {
			uint32_t w0 = ProfileWeightQ24(static_cast<uint32_t>(i), center0, slope_scale_q12);
			uint32_t w1 = ProfileWeightQ24(static_cast<uint32_t>(i), center1, slope_scale_q12);
			int64_t dw = static_cast<int64_t>(w1) - static_cast<int64_t>(w0);
			uint32_t w = static_cast<uint32_t>(static_cast<int64_t>(w0) + ((dw * frac_q12) >> 12));
			active_weights_q24_[i] = w;
			sum_q24 += w;
		}

		for (int i = safe_partial_count_; i < kPartials; ++i) {
			active_weights_q24_[i] = 0;
		}

		master_gain_q24_ = GainFromSumQ24(sum_q24);
	}

	int16_t __not_in_flash_func(NextSample)()
	{
		interp_add_accumulator(interp_, 0, base_phase_inc_);
		uint32_t base_phase = interp_get_accumulator(interp_, 0);

		int64_t signal_sum = 0;
		for (int i = 0; i < safe_partial_count_; ++i) {
			uint32_t phase = base_phase * static_cast<uint32_t>(i + 1);
			int16_t s = LookupSineLinear(phase);
			signal_sum += static_cast<int64_t>(s) * static_cast<int64_t>(active_weights_q24_[i]);
		}

		// signal_sum is Q24-scaled; apply Q24 master gain and bring back to signed 12-bit.
		int64_t normalized_q24 = (signal_sum * static_cast<int64_t>(master_gain_q24_)) >> 24;
		int32_t out = static_cast<int32_t>(normalized_q24 >> 24);

		if (out > 2047) out = 2047;
		if (out < -2048) out = -2048;
		return static_cast<int16_t>(out);
	}

private:
	// 256-point signed sine LUT in 12-bit amplitude domain.
	static constexpr int kSineTableSize = 256;
	static constexpr uint32_t kSineMask = kSineTableSize - 1;

	// Distance-domain 1/n distribution in Q24 (1.0 = 16777216), plus zero endpoint.
	static constexpr uint32_t kDistanceWeightQ24[kPartials + 1] = {
		16777216, 8388608, 5592405, 4194304,
		3355443, 2796202, 2396745, 2097152,
		1864135, 1677721, 1525201, 1398101,
		1290555, 1198372, 1118481, 1048576, 0,
	};

	// Y->slope scale in Q12. Full CW = 1.0x (pure 1/n). Full CCW = 8.0x (narrow).
	static constexpr uint32_t kSlopeScaleMinQ12 = kQ12One;
	static constexpr uint32_t kSlopeScaleMaxQ12 = 32768u;

	// Normalization LUT parameters:
	// master_gain_q24 ~= (1<<48) / sum_q24, but read from LUT + interpolation.
	static constexpr uint64_t kNormNumeratorQ48 = 1ull << 48;
	static constexpr uint32_t kNormLutShift = 17;                  // LUT step = 2^17 in Q24
	static constexpr uint32_t kNormLutSize = 1024;                 // 1025 entries with endpoint
	static constexpr uint32_t kNormLutFracMask = (1u << kNormLutShift) - 1u;
	static constexpr uint32_t kNormSumMaxQ24 = 6u * kOneQ24;       // conservative upper clamp

	static constexpr int16_t kSineTable[kSineTableSize] = {
		    0,    50,   100,   151,   201,   251,   300,   350,
		  399,   449,   497,   546,   594,   642,   690,   737,
		  783,   830,   875,   920,   965,  1009,  1052,  1095,
		 1137,  1179,  1219,  1259,  1299,  1337,  1375,  1411,
		 1447,  1483,  1517,  1550,  1582,  1614,  1644,  1674,
		 1702,  1729,  1756,  1781,  1805,  1828,  1850,  1871,
		 1891,  1910,  1927,  1944,  1959,  1973,  1986,  1997,
		 2008,  2017,  2025,  2032,  2037,  2041,  2045,  2046,
		 2047,  2046,  2045,  2041,  2037,  2032,  2025,  2017,
		 2008,  1997,  1986,  1973,  1959,  1944,  1927,  1910,
		 1891,  1871,  1850,  1828,  1805,  1781,  1756,  1729,
		 1702,  1674,  1644,  1614,  1582,  1550,  1517,  1483,
		 1447,  1411,  1375,  1337,  1299,  1259,  1219,  1179,
		 1137,  1095,  1052,  1009,   965,   920,   875,   830,
		  783,   737,   690,   642,   594,   546,   497,   449,
		  399,   350,   300,   251,   201,   151,   100,    50,
		    0,   -50,  -100,  -151,  -201,  -251,  -300,  -350,
		 -399,  -449,  -497,  -546,  -594,  -642,  -690,  -737,
		 -783,  -830,  -875,  -920,  -965, -1009, -1052, -1095,
		-1137, -1179, -1219, -1259, -1299, -1337, -1375, -1411,
		-1447, -1483, -1517, -1550, -1582, -1614, -1644, -1674,
		-1702, -1729, -1756, -1781, -1805, -1828, -1850, -1871,
		-1891, -1910, -1927, -1944, -1959, -1973, -1986, -1997,
		-2008, -2017, -2025, -2032, -2037, -2041, -2045, -2046,
		-2047, -2046, -2045, -2041, -2037, -2032, -2025, -2017,
		-2008, -1997, -1986, -1973, -1959, -1944, -1927, -1910,
		-1891, -1871, -1850, -1828, -1805, -1781, -1756, -1729,
		-1702, -1674, -1644, -1614, -1582, -1550, -1517, -1483,
		-1447, -1411, -1375, -1337, -1299, -1259, -1219, -1179,
		-1137, -1095, -1052, -1009,  -965,  -920,  -875,  -830,
		 -783,  -737,  -690,  -642,  -594,  -546,  -497,  -449,
		 -399,  -350,  -300,  -251,  -201,  -151,  -100,   -50,
	};

	interp_hw_t *interp_;
	uint32_t base_phase_inc_ = 0;
	uint32_t master_gain_q24_ = 0;
	uint32_t active_weights_q24_[kPartials] = {};
	int safe_partial_count_ = 0;
	uint16_t centroid_knob_ = 0;
	uint16_t slope_knob_ = 4095;

	using NormLut = std::array<uint32_t, kNormLutSize + 1>;

	static constexpr NormLut kNormGainLutQ24 = []() constexpr {
		NormLut lut = {};
		for (uint32_t i = 0; i <= kNormLutSize; ++i) {
			uint32_t sum_q24 = i << kNormLutShift;
			if (sum_q24 < kOneQ24) sum_q24 = kOneQ24;
			if (sum_q24 > kNormSumMaxQ24) sum_q24 = kNormSumMaxQ24;
			lut[i] = static_cast<uint32_t>(kNormNumeratorQ48 / static_cast<uint64_t>(sum_q24));
		}
		return lut;
	}();

	static inline uint32_t DistanceWeightInterpolatedQ24(uint32_t dist_q12)
	{
		static constexpr uint32_t kMaxDistQ12 = static_cast<uint32_t>(kPartials) << 12;
		if (dist_q12 >= kMaxDistQ12) {
			return 0;
		}

		uint32_t idx = dist_q12 >> 12;      // 0..15
		uint32_t frac = dist_q12 & 0x0FFFu; // 0..4095
		uint32_t w0 = kDistanceWeightQ24[idx];
		uint32_t w1 = kDistanceWeightQ24[idx + 1u];
		int64_t dw = static_cast<int64_t>(w1) - static_cast<int64_t>(w0);
		return static_cast<uint32_t>(static_cast<int64_t>(w0) + ((dw * frac) >> 12));
	}

	inline uint32_t SlopeScaleQ12() const
	{
		// Invert knob sense: CCW narrows, CW widens to pure 1/n.
		uint32_t inv = 4095u - static_cast<uint32_t>(slope_knob_);
		uint32_t delta = kSlopeScaleMaxQ12 - kSlopeScaleMinQ12;
		uint32_t add = (delta * inv + 2048u) >> 12; // divide by 4096 with rounding
		if (slope_knob_ == 0u) {
			add = delta; // exact CCW endpoint
		}
		return kSlopeScaleMinQ12 + add;
	}

	static inline uint32_t ProfileWeightQ24(uint32_t partial_idx, uint32_t center_idx, uint32_t slope_scale_q12)
	{
		uint32_t dist = (partial_idx >= center_idx) ? (partial_idx - center_idx) : (center_idx - partial_idx);
		uint32_t dist_q12 = dist << 12;
		uint32_t scaled_dist_q12 = static_cast<uint32_t>((static_cast<uint64_t>(dist_q12) * slope_scale_q12) >> 12);
		return DistanceWeightInterpolatedQ24(scaled_dist_q12);
	}

	static inline uint32_t GainFromSumQ24(uint64_t sum_q24)
	{
		if (sum_q24 == 0) {
			return 0;
		}

		uint32_t sum = static_cast<uint32_t>((sum_q24 > kNormSumMaxQ24) ? kNormSumMaxQ24 : sum_q24);
		uint32_t idx = sum >> kNormLutShift;
		if (idx >= kNormLutSize) {
			return kNormGainLutQ24[kNormLutSize];
		}

		uint32_t frac = sum & kNormLutFracMask;
		int64_t g0 = static_cast<int64_t>(kNormGainLutQ24[idx]);
		int64_t g1 = static_cast<int64_t>(kNormGainLutQ24[idx + 1u]);
		int64_t dg = g1 - g0;
		return static_cast<uint32_t>(g0 + ((dg * frac) >> kNormLutShift));
	}

	void InitInterpolator()
	{
		interp_config cfg = interp_default_config();
		interp_config_set_mask(&cfg, 0, 31);
		interp_set_config(interp_, 0, &cfg);
		interp_set_accumulator(interp_, 0, 0);
	}

	inline int16_t __not_in_flash_func(LookupSineLinear)(uint32_t phase) const
	{
		uint32_t idx = (phase >> 24) & kSineMask;
		uint32_t frac = (phase >> 16) & 0xFFu;
		int32_t s1 = kSineTable[idx];
		int32_t s2 = kSineTable[(idx + 1u) & kSineMask];
		return static_cast<int16_t>(s1 + (((s2 - s1) * static_cast<int32_t>(frac)) >> 8));
	}
};

class MinimalAdditiveCard : public ComputerCard
{
public:
	static constexpr int kControlDivisor = 16;

	MinimalAdditiveCard() : osc_(interp0_hw) {}

	void ProcessSample() override __not_in_flash_func()
	{
		if (++control_counter_ >= kControlDivisor) {
			control_counter_ = 0;
			UpdateControlRate();
		}

		int16_t out = osc_.NextSample();
		AudioOut1(out);
		AudioOut2(out);
	}

private:
	MinimalAdditiveOscillator osc_;
	int control_counter_ = kControlDivisor;
	uint16_t last_centroid_ = 0xFFFFu;
	uint16_t last_slope_ = 0xFFFFu;

	inline int32_t __not_in_flash_func(Clamp12Bit)(int32_t v) const
	{
		if (v < 0) return 0;
		if (v > 4095) return 4095;
		return v;
	}

	void __not_in_flash_func(UpdateControlRate)()
	{
		// Main controls pitch, X controls centroid, Y controls slope width.
		int32_t main_knob = Clamp12Bit(KnobVal(Knob::Main));
		int32_t centroid = Clamp12Bit(KnobVal(Knob::X));
		int32_t slope = Clamp12Bit(KnobVal(Knob::Y));

		uint32_t phase_inc = kMainPitchPhaseIncQ32[static_cast<uint32_t>(main_knob)];
		bool spectrum_dirty = osc_.SetBasePhaseIncrement(phase_inc);

		uint16_t centroid_u12 = static_cast<uint16_t>(centroid);
		if (centroid_u12 != last_centroid_) {
			last_centroid_ = centroid_u12;
			osc_.SetCentroidKnob(centroid_u12);
			spectrum_dirty = true;
		}

		uint16_t slope_u12 = static_cast<uint16_t>(slope);
		if (slope_u12 != last_slope_) {
			last_slope_ = slope_u12;
			osc_.SetSlopeKnob(slope_u12);
			spectrum_dirty = true;
		}

		if (spectrum_dirty) {
			osc_.UpdateWeightsAndNormalization();
		}
	}
};

int main()
{
	MinimalAdditiveCard synth;
	synth.Run();
}
