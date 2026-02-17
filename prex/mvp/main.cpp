#include "ComputerCard.h"
#include <cmath>

class MVPSine300Hz : public ComputerCard
{
public:
	static constexpr uint32_t PHASE_INC_300HZ = 26843546U; // 300 * 2^32 / 48000
	static constexpr unsigned TABLE_SIZE = 512;
	static constexpr uint32_t TABLE_MASK = TABLE_SIZE - 1;

	int16_t sine_table[TABLE_SIZE];
	uint32_t phase = 0;

	MVPSine300Hz()
	{
		for (unsigned i = 0; i < TABLE_SIZE; ++i) {
			sine_table[i] = int16_t(2047.0 * sin(2.0 * M_PI * double(i) / double(TABLE_SIZE)));
		}
	}

	inline int16_t __not_in_flash_func(LookupSine)(uint32_t p)
	{
		uint32_t index = (p >> 23) & TABLE_MASK;
		uint32_t frac = (p >> 7) & 0xFFFF;

		int32_t s1 = sine_table[index];
		int32_t s2 = sine_table[(index + 1) & TABLE_MASK];
		return int16_t((s2 * frac + s1 * (65536 - frac)) >> 16);
	}

	void ProcessSample() override __not_in_flash_func()
	{
		phase += PHASE_INC_300HZ;
		int16_t out = LookupSine(phase);
		AudioOut1(out);
		AudioOut2(out);
	}
};

int main()
{
	MVPSine300Hz mvp;
	mvp.Run();
}
