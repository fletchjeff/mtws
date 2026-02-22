

# README: RP2040 Centroid Additive Oscillator
## Overview
This code implements a high-performance, 16-partial additive oscillator specifically designed for the Raspberry Pi Pico (RP2040) and Euro-format hardware like the Music Thing Workshop ComputerCard.
Instead of traditional subtractive filtering, this oscillator uses a Centroid-Based Bandpass model. It generates 16 pure sine waves locked to the harmonic series and dynamically calculates their amplitudes based on distance from a moving "center" point.

## How the Controls Work
Y Knob (Centroid): Moves the "peak" of the frequency hump. At 0, the fundamental is the loudest. At full, the 16th partial is the loudest. This acts like a formant sweep.
X Knob (Slope / Q-Factor): Controls the width of the hump.
Low X: A broad plateau where many harmonics are audible (sounds like a rich sawtooth/square).
High X: A steep spike where only the harmonics immediately next to the centroid are audible (sounds like a ringing bandpass or vocal formant).

## Key Technical Features
RP2040 Hardware Interpolator: We offload the phase-accumulation math to the SIO interpolator (interp0). This does the 32-bit phase tracking and bit-shifting in a single hardware cycle, freeing up the CPU.

Anti-Aliasing: If you play a high note, harmonics that exceed the Nyquist limit (half your sample rate) are automatically muted. This prevents harsh digital "fold-over" distortion.

High-Precision Fixed-Point Math: The internal mixing is done at 24-bit and 32-bit precision before being truncated to 12-bit for the DAC. This entirely eliminates the "steep cliff" quantization noise on quiet partials.

Dynamic Normalization: As the width of the hump changes, the total energy in the system is calculated. The master volume is scaled inversely so the output never clips and never gets too quiet.

## The C++ Implementation (AdditiveOscillator.hpp)

Save this as a .hpp or .h file in your Pico project and include it in your main .cpp file.

```C++


#pragma once

#include "pico/stdlib.h"
#include "hardware/interp.h"
#include <math.h>

/**
 * @class AdditiveOscillator
 * @brief 16-partial harmonic oscillator with centroid-based morphing.
 */
class AdditiveOscillator {
private:
    static const int NUM_PARTIALS = 16;
    static const int SINE_LUT_SIZE = 2048;
    static const uint32_t SINE_LUT_MASK = SINE_LUT_SIZE - 1;

    // --- High-Precision Registers ---
    uint32_t active_weights[NUM_PARTIALS];
    uint32_t master_gain = 16777216; // 1.0 in 24-bit fixed point
    int16_t sine_lut[SINE_LUT_SIZE];

    // Hardware interpolator reference (interp0 or interp1)
    interp_hw_t* hw_interp; 

    // Audio Engine State
    uint32_t base_phase_increment;
    int safe_partial_count; // Tracks how many partials are below Nyquist
    float sample_rate;

    // Pre-calculated 1/n harmonic weights (24-bit precision)
    // 1.0 = 16777216. This provides the natural "Sawtooth" acoustic decay.
    const uint32_t BASE_WEIGHTS_24BIT[NUM_PARTIALS] = {
        16777216, 8388608, 5592405, 4194304, 3355443, 2796202, 2396745, 2097152,
        1864135, 1677721, 1525201, 1398101, 1290555, 1198372, 1118481, 1048576
    };

    /**
     * @brief Generates the Sine wave Look-Up Table
     */
    void init_lut() {
        for (int i = 0; i < SINE_LUT_SIZE; i++) {
            // Fill with 12-bit signed values (-2047 to 2047)
            sine_lut[i] = (int16_t)(sinf(i * 2.0f * M_PI / SINE_LUT_SIZE) * 2047.0f);
        }
    }

public:
    /**
     * @brief Constructor
     * @param interp_instance Pass interp0_hw or interp1_hw
     * @param sr The audio sample rate (e.g., 44100.0f)
     */
    AdditiveOscillator(interp_hw_t* interp_instance, float sr) {
        hw_interp = interp_instance;
        sample_rate = sr;
        safe_partial_count = NUM_PARTIALS;
        base_phase_increment = 0;
        
        init_lut();

        // Configure RP2040 Interpolator for Phase Accumulation
        // We use bits 21 to 31 (Top 11 bits) to index our 2048-size LUT
        interp_config cfg = interp_default_config();
        interp_config_set_mask(&cfg, 21, 31); 
        interp_set_config(hw_interp, 0, &cfg);
    }

    /**
     * @brief Update the fundamental pitch and calculate Anti-Aliasing limits.
     * Call this whenever the V/Oct pitch changes.
     */
    void set_frequency(float freq_hz) {
        // 1. Calculate the 32-bit phase step for the fundamental
        base_phase_increment = (uint32_t)((freq_hz / sample_rate) * 4294967296.0f);

        // 2. Anti-Aliasing (Nyquist limit)
        float nyquist = sample_rate / 2.0f;
        safe_partial_count = 0;

        for (int i = 0; i < NUM_PARTIALS; i++) {
            float partial_freq = freq_hz * (i + 1);
            if (partial_freq < nyquist) {
                safe_partial_count++;
            } else {
                break; // Stop counting once we exceed Nyquist
            }
        }
    }

    /**
     * @brief Update the harmonic spectrum hump.
     * Call this from your main loop when the X/Y ADC values change.
     * @param knob_x Slope/Q (0-4095). 0 = Flat, 4095 = Sharp Spike.
     * @param knob_y Centroid (0-4095). 0 = Fundamental, 4095 = 16th Partial.
     */
    void update_morph(uint16_t knob_x, uint16_t knob_y) {
        uint64_t total_energy = 0;

        // Map Y to a fixed-point index (0 to 15 << 12)
        uint32_t centroid_idx_fixed = (knob_y * (NUM_PARTIALS - 1));

        // Note: We only calculate energy for 'safe' partials! 
        // This ensures the volume normalizer ignores muted high frequencies.
        for (int i = 0; i < safe_partial_count; i++) {
            uint32_t current_idx_fixed = i << 12; 
            
            // 1. Calculate absolute distance from the centroid
            int32_t dist = (int32_t)current_idx_fixed - (int32_t)centroid_idx_fixed;
            if (dist < 0) dist = -dist;

            // 2. Apply 'X' to scale the distance (The Slope)
            uint32_t attenuation = (uint32_t)((dist * knob_x) >> 8); 

            // 3. Create a pseudo-bell curve by squaring the attenuation
            // This makes the hump feel much more natural and musical
            uint32_t norm_dist = attenuation >> 4; 
            uint32_t squared_drop = norm_dist * norm_dist;

            // 4. Calculate amplitude (Max 24-bit minus the drop)
            int32_t amplitude = 16777216 - squared_drop;
            if (amplitude < 0) amplitude = 0;

            // 5. Apply the fundamental 1/n physics rule
            active_weights[i] = (uint32_t)(((uint64_t)amplitude * BASE_WEIGHTS_24BIT[i]) >> 24);
            total_energy += active_weights[i];
        }

        // 6. Dynamic Normalization (Target 1.0 in 24-bit / Total Energy)
        if (total_energy > 0) {
            master_gain = (uint32_t)((16777216ULL * 16777216ULL) / total_energy);
        } else {
            master_gain = 0; // Prevent divide by zero if completely silenced
        }
    }

    /**
     * @brief Generate the next audio sample.
     * Call this inside your high-speed PWM/I2S audio interrupt.
     * @return A 12-bit unsigned integer (0-4095) ready for Pico PWM output.
     */
    inline uint16_t get_next_sample() {
        // 1. Advance the hardware phase accumulator
        interp_add_accumulator(hw_interp, 0, base_phase_increment);
        uint32_t base_phase = interp_get_full_value(hw_interp, 0);

        int64_t signal_sum = 0;

        // 2. Sum all safe partials
        for (int i = 0; i < safe_partial_count; i++) {
            // Multiply fundamental phase by harmonic index (1, 2, 3...)
            uint32_t phase = base_phase * (i + 1);
            
            // Extract the top 11 bits for the LUT index
            uint32_t idx = (phase >> 21) & SINE_LUT_MASK;

            // Multiply 12-bit Sine by 24-bit Active Weight
            signal_sum += (int64_t)sine_lut[idx] * active_weights[i];
        }

        // 3. Apply the Master Gain to normalize the volume
        int64_t normalized = (signal_sum * master_gain) >> 24;
        
        // 4. Truncate back to 12-bit and offset to center (0 to 4095 range)
        return (uint16_t)((normalized >> 24) + 2048);
    }
};
``` 

How to use it in your main.cpp:

```C++


// 1. Instantiate the oscillator using Interpolator 0 at 44.1kHz
AdditiveOscillator myOsc(interp0_hw, 44100.0f);

// 2. Set the pitch
myOsc.set_frequency(220.0f); // A3

// 3. Read your ADCs and update the shape (in your main while loop)
uint16_t knob_x = adc_read(); // X Knob
uint16_t knob_y = adc_read(); // Y Knob
myOsc.update_morph(knob_x, knob_y);

// 4. In your audio interrupt timer (running at 44.1kHz):
uint16_t audio_out = myOsc.get_next_sample();
pwm_set_chan_level(slice_num, chan_num, audio_out);

```

This setup fully resolves the sharp geometric drop-off, introduces proper mathematical scaling, guarantees you won't crash your DAC bounds, and pushes the heavy lifting to the silicon layer of the RP2040.
