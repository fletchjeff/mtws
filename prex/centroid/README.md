# Additive Oscillator

16-partial additive synthesizer with variable harmonic spacing and spectral tilt control.

## Overview

This is an additive synthesis oscillator that generates sound by summing 16 individual sine wave partials. Unlike traditional additive synthesizers where the harmonics follow a fixed harmonic series, this implementation allows you to continuously vary the spacing between partials and control the spectral brightness.

## Controls

### Main Knob + CV1
- **Function**: Base pitch control with 1V/octave tracking
- Combines knob position with CV1 input for precise pitch control
- Full musical range from sub-audio to ~20kHz

### X Knob - Harmonic Spacing
- **Center (12 o'clock)**: Standard harmonic series (1f, 2f, 3f... 16f)
- **Counter-clockwise**: Compress harmonics - all partials move toward lower frequencies
- **Clockwise**: Stretch harmonics - all partials move toward higher frequencies

**Sound characteristics:**
- Center: Natural, harmonic sounds (like analog oscillators)
- Slight movement: Chorus/ensemble/detuning effects
- Moderate movement: Bell-like, inharmonic timbres
- Extreme settings: Metallic, alien, frequency-stacking effects

### Y Knob - Spectral Tilt (Brightness)
- **Full CCW**: Pure sine wave (only fundamental, no harmonics)
- **Center (12 o'clock)**: 1/n amplitude falloff (sawtooth-like spectrum)
- **Full CW**: All partials at equal amplitude (very bright, square-ish)

**Sound characteristics:**
- Full CCW: Pure, clean sine wave
- Slight CW from CCW: Warm, mellow tones
- Center: Rich harmonic content, classic analog sound
- Slight CW from center: Bright, full spectrum
- Full CW: Very bright, aggressive tone

The interpolation uses an exponential curve rather than linear, which makes the transition feel more natural and musical.

### CV2 - FM Modulation
- Fixed moderate amount of frequency modulation
- Apply audio-rate signals for classic FM timbres
- Applies equally to all partials

## Technical Implementation

### Architecture
- 16 independent sine wave oscillators
- 512-point sine lookup table with linear interpolation
- All processing done in fixed-point arithmetic (no floating-point in audio callback)
- Dynamic amplitude calculation per partial based on Y knob position

### Amplitude Interpolation
The Y knob implements a three-point interpolation:
1. **Y = 0**: Only fundamental audible (sine wave)
2. **Y = 2048**: 1/n amplitude falloff (sawtooth spectrum)
3. **Y = 4095**: All partials equal (bright)

Between these points, an exponential interpolation is used:
- **Left half (0-2048)**: Fade in higher harmonics from 0 to 1/n using squared interpolation
- **Right half (2048-4095)**: Boost higher harmonics from 1/n to equal using squared interpolation

This creates a smooth, natural-feeling brightness control.

### Harmonic Spacing
The X knob adjusts each partial's frequency by a fixed offset from its nominal harmonic position:
- Partial N normally plays at frequency N × fundamental
- X knob adds/subtracts up to ±1.0 from the harmonic number
- Example at full CCW: 2nd partial (normally 2f) plays at 1f (same as fundamental)
- Example at full CW: 2nd partial (normally 2f) plays at 3f

### Performance
- Estimated CPU usage: 1000-1200 cycles per sample
- Budget at 125MHz: 2500 cycles per sample
- Comfortable headroom remaining (~50%)
- Could potentially support 24+ partials if needed

## Building

From the `ComputerCard/build/` directory:

```bash
cmake ..
make additive
```

This generates `additive.uf2` which can be flashed to the Workshop Computer.

## Musical Applications

### Subtractive-style synthesis
Use Y knob to shape timbre like a filter, starting from bright and sweeping down to mellow.

### Inharmonic timbres
Use X knob away from center to create bell-like, metallic, or alien tones.

### Evolving pads
Slowly modulate both X and Y knobs for evolving, complex timbres.

### FM synthesis
Use CV2 with audio-rate modulation for classic FM timbres, combined with spectral shaping.

### Chord/cluster effects
At extreme X knob positions, all partials cluster together creating chorus-like effects.

## Possible Extensions

- Per-partial amplitude control via additional algorithm (e.g., odd/even harmonics)
- Individual partial detuning amount control
- Second oscillator for detuned unison
- Waveshaping/saturation on output
- LFO modulation of X or Y parameters
- More partials (24-32 should be achievable)
