#include "ComputerCard.h"
#include <cmath>

/// Outputs sine wave at 440Hz

/// Uses an integer lookup table with linear interpolation, for speed.
/// At default clock rate of 125MHz, about 40 of these lookup-table
//  evaluations are possible in a 48kHz sample.

/// See (much simpler) sine_wave_float/ example for the same 440Hz sine
/// evaluated using floating-point arithmetic.

class SineWaveLookup : public ComputerCard
{
public:
	constexpr static uint32_t BASE_PHASE_INC_10HZ = 894785U; // 10 * 2^32 / 48000
	constexpr static int CONTROL_DIVISOR = 16;

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

	// Sine wave phase (0-2^32 gives 0-2pi phase range)
	uint32_t phase;
	uint32_t phase_increment;
	int control_counter;
	
	SineWaveLookup()
	{
		// Initialise phase of sine wave to 0
		phase = 0;
		phase_increment = BASE_PHASE_INC_10HZ << 5; // 320Hz default before first knob read
		control_counter = CONTROL_DIVISOR; // force control update on first sample
		
		for (unsigned i=0; i<tableSize; i++)
		{
			// just shy of 2^15 * sin
			sine[i] = int16_t(32000*sin(2*i*M_PI/double(tableSize)));
		}

	}

	inline void UpdatePhaseIncrementFromMainKnob()
	{
		int32_t knob = KnobVal(Knob::Main);
		if (knob < 0) knob = 0;
		if (knob > 4095) knob = 4095;

		// Map knob 0..4095 to 10 octaves: 10Hz..10240Hz.
		// 512 substeps per octave => 10*512 = 5120 total positions.
		uint32_t pos = (uint32_t(knob) * 5122U) >> 12; // shift-only approximation
		if (pos > 5120U) pos = 5120U;

		uint32_t octave_int = pos >> 9;   // 0..10
		uint32_t sub = pos & 0x1FF;       // 0..511

		uint32_t inc = BASE_PHASE_INC_10HZ << octave_int;

		// Use 2-bit interpolation between 1/128-octave table points to get 1/512-octave resolution.
		uint32_t idx = sub >> 2;        // 0..127
		uint32_t frac = sub & 0x3;      // 0..3
		uint32_t m1 = EXP2_Q16[idx];
		uint32_t m2 = EXP2_Q16[idx + 1];
		uint32_t mult = m1 + (((m2 - m1) * frac) >> 2); // Q16

		phase_increment = uint32_t((uint64_t(inc) * mult) >> 16);
	}
	
	virtual void ProcessSample()
	{
		if (++control_counter >= CONTROL_DIVISOR)
		{
			control_counter = 0;
			UpdatePhaseIncrementFromMainKnob();
		}

		uint32_t index = phase >> 23; // convert from 32-bit phase to 9-bit lookup table index
		int32_t r = (phase & 0x7FFFFF) >> 7; // fractional part is last 23 bits of phase, shifted to 16-bit 

		// Look up this index and next index in lookup table
		int32_t s1 = sine[index];
		int32_t s2 = sine[(index+1) & tableMask];

		// Linear interpolation of s1 and s2, using fractional part
		// Shift right by 20 bits
		// (16 bits of r, and 4 bits to reduce 16-bit signed sine table to 12-bit output)
		int32_t out = (s2 * r + s1 * (65536 - r)) >> 20;

		AudioOut1(out);
		AudioOut2(out);
		
		phase += phase_increment;
	}
};


int main()
{
	SineWaveLookup sw;
	sw.Run();
}

  
