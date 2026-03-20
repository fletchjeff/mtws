# Knots
Knots is a 6 oscillator firmware for the Music Thing Workshop System.

_**Note:** This is the write up about bulding this thing. If you just want the [user guide](USER_GUIDE.md) <- click that._

This all started because I wanted an additive oscillator that would let me change the frequencies of the partials in ways most of the ones I have access to don't. Specifically I wanted something like the Centroid dissonance effect from Native Instrument's Razor. It's really cool! I was looking at the Xoac Odessa when I saw something about the Music Thing Workshop System, I think probably the CDM article. It's been out for a while and I was a bit suprised I had not heard of it. It seemed like a cool device, the kit form was cheaper than than the Odessa (and easier to justify the spend to my bank account) and I enjoy doing the DIY builds. Plus I figured could can still user Razor to make those "centroidy" sounds. 

I bought it from Thonk and set aside a Saturday to do the build.

> Aside: I had 2 dry joints that I had to fix. One on the top filter's resonance pot and one on the lower Osciallator's FM pot. Resoldered those and it all worked.

After I started playing I realised that, in theory, it would be possible to make the additive oscialltor I wanted, so I took a look at the [repo](link.to.repo) and saw that python was an option. My python is reasonable and I could probably make what I wanted. However, if I wanted this to work with lot of modulation input and midi, it would have to be C++. I have not written any C++ in ages, and even when I did it was minimal. Writing DSP code optimised for a board without floating point math requires years C++ experience that I don't have, and honestly something I'm not willing to put the time in to learn. 

But...

I do have access to AI coding tools and the current generation are very good at writing code. 

> Aside: I know generative AI is a devisive topic. Some of the companies and the people who run them seem to not care about any of concequences of the job losses or environmental impact they cause. I'm not going to tackle the ethics of using AI in this write up.  

## Getting Started

I cloned the repo, pried the main knob off, put in a blank card, plugged the USB cable and jumped in to see how well this would work.

After getting the build chain running and testing that I could build and test a new firm I was ready. My first attempt was to try [one shot](one.shot.link) a version of the Plaits additive oscillator. Just write up what I want: to replicate what existed first it an modify it later. This totally failed to work. But after a few iterations of prompt, build, test I got something that kinda worked but it wasn't great. However, this was C++ code that was bunch of sine waves at different frequencies and there was usable sound coming out of the outputs. I didn't have to know how to write C++ to use C++.

The next step was to stand on the sholders of giants and see what is possible and understand what could and could not be done. 

## Limitations and Creativity

While I'm not proficient at C++ I am "good with computers" and have a reasonable grasp of DSP and audio fundamentals. This means I have a good idea about what can be done and specific about what I want. However the Computer board has limited compute power, certainly less than my Macbook and not optimsed for audio like the Plaits. It can do a lot, but there limits. And limitations can be great for creativity. 

There key limitation being floating point arithmetic. This is a great [video essay](https://www.youtube.com/watch?v=y-NOz94ZEOA) on the topic. Modern DSP algorithms rely on compute heavy calcuations and floating point arithmetic is essential. But the Music Thing Workshop System is based on the [RP2040](https://en.wikipedia.org/wiki/RP2040) which does not have hardware floating-point unit. As an example, doing a division as part of an algorithm adds extra load on the processor and can be prone to errors. The RP2040 does fixed point arithmetic, which is much faster and with right approach, it's good enough.

I learned about these limitations by going through the examples, other releases and the [Utility-Pair repo](https://github.com/chrisgjohnson/Utility-Pair/). There is a lot you can learn by reading through the Utility-Pair source code. 

_**Note:** Chris also wrote the excellent ComputerCard impelemtation for the Workshop System._

It's such a useful resource that I made the my coding agent look in that source code for implementation ideas first. There is lot of optimisations that work well on the RP2040. 

For example:

```c
uint32_t __not_in_flash_func(rnd12)()
{
	static uint32_t lcg_seed = 1;
	lcg_seed = 1664525 * lcg_seed + 1013904223;
	return lcg_seed >> 20;
}
```

That is how you generate [random enough numbers](https://en.wikipedia.org/wiki/Linear_congruential_generator) in C++. I did not any of this when I started!

It's worth going through the code in the [Workshop System ComputerCard](https://github.com/TomWhitwell/Workshop_Computer/tree/main/Demonstrations%2BHelloWorlds/PicoSDK/ComputerCard) [Utility-Pair](https://github.com/chrisgjohnson/Utility-Pair/). 

I've added a lot of this information to the AGENTS files in this repo to guide the coding agents to use the optimisation. Well, I say I added them, but I told Codex to do it! More on that later.

Even without a floating point unit, the RP2040 is still very capable and has a few tricks to get more performance:

* **Dual Core**
- The RP2040 has 2 cores so you can run different processes on each core. The knots runs audio generation on one core and the control processes on the other. Doing this split helped a lot with performance.

* **Overclocking** 
- The default CPU clock speed for the Workshop System is 133MHz, but it's [certified to run at 200MHz](https://researchsnipers.com/boost-your-raspberry-pi-pico-2040-to-200mhz-with-the-latest-sdk-update/). Close to double the perfomance.

Once I got to grips with the limitations and where to find optimised C++ code and examples, I tried building my additive oscillator idea again. No one shotting, but progressively adding capabilties until I had something that worked well and I was happy with. 

Part of this learning meant spending time with the excellent [Twists](https://github.com/TomWhitwell/Workshop_Computer/commits/main/releases/10_twists), a port of Mutable Instruments Braids and I figured if I could make the additive oscillator, I could probably make other others. So I borrowed the switchable oscillator idea from Twists and decided to give a bigger impelemtation a go. 6 interesting oscillators with lots of control.

## Design Criteria
Once I had a general plan, I set some criteria for the overall build and each oscillator (which I call engines because that's what they're called in the [Plaits](https://github.com/pichenettes/eurorack/tree/master/plaits/dsp/engine) repo). There is a lot that's borrowed from other modules and reworked to fit within the performance constraints of the RP2040 and the layout of the Computer modules I/O.

#### Useable Sounds
I want to actually use this. I'm working on a specific project and want to use the Workshop System as the main sound design source so I need these to be useable. The engines should have interesting timbres and have a useable range for the control parameters. I  settled on a mix of ideas from various other devices I've use with some adjustments I wished they could like, like centroid for the additive oscillator.

#### Integration
This has to be built to work with rest of the modules on the Workshop System. It's not an isollated oscillator, it's part of set of tools that can interact with each other to provide a much bigger sonic palette. This applies to the inputs and outputs, the design should lend it self to patching with the filters, oscillators and slopes. This should apply to Eurorack in general too, make it a useful part of a sound design workflow.

#### Stereo
The Computer module has 2 audio outputs, the mixer has 2 pannable inputs and I have 2 ears. This seems almost pre-ordained. All 6 engines must have dual ouputs that will operate as 2 indepent, mono sound sources or as an interesting stereo source when panned.

#### Alternative Mode
On of the nice features of the Plaits is the Aux out. It's a differentiated output version of the current engine and I really like the idea. But if I'm using both outputs (more on this later), I don't have a spare "Aux" out. By using the Z switch, I can give the module an "Alternative" mode which should implement the same osciallator, but like, different somehow. This gives a broader sonic pallette.

#### MIDI
This idea I borrowed directly from Twists and goes back to my original design criteria, I need to use this. Making the USB port a class compliant MIDI interface means I can connect it to Ableton on my Mac or the Ableton move and sequence notes and control stuff. 

## Inputs, Outputs and Control
With the design criteria in mind I started building. Well, telling Codex what to do. The [ComputerCard.h](https://github.com/TomWhitwell/Workshop_Computer/blob/main/Demonstrations%2BHelloWorlds/PicoSDK/ComputerCard/ComputerCard.h) file _"is a header-only C++ library, providing a class that manages the hardware aspects of the Music Thing Modular Workshop System Computer."_ It wires up the ins and outs of the Computer module to make things easier to use from the start without having to worry about the complexity of doing this. It's extremely helpful and something coding agents and use as it and focus on sound generation and control and on the detail of managing I/O, which has it's own quirks and gotchas. 

The ComputerCard uses a specific layout for the input and output jacks. 2 x Audio I/O, 2 x CV I/O and 2 x Pulse I/O. You don't have to use it this way and I repurposed the 2 audio in's as CV in, but I kept the rest as I think it's a great layout.

#### Main, X and Y
The main knob is for tuning the main oscillator of othe engine. It's a 10 octave span from 10Hz to 10KHz, clamped at either end so CV or MIDI input won't push it past those min and max values. I tried different values for this range and this felt the most usable. It's get's a but noisy at the very high for some of the engines, which I like. Plus I'm old, so while my dog doesn't appreciate the aliasing, I don't notice it :)

The X and Y trimmers adjust 2 parameters that are exposed from the engines. These differ per engine and in some cases are different in the Alt mode. I tried to make the paramters musical to encourge using them. 

Below the X and Y timmers are the two audio inputs. To me that layout lends itself to using those audio inputs as CV inputs for the X and Y parameters. The ComputerCard has a function `Connected(Input::Audio1)` that will allow for different code to run if there a cable connected to the input jack. If there is a cable connected the X and Y trimmers become attenuators for the CV signal on the input cable. These Audio inputs now run at control rate, not at the full audio rate. See the the Coding Agents section for detail.

Both the Audio and CV inputs can accept values from -6v to 6v and the slopes running in Loop mode output 0v to 12v, so there is a mismatch that would normally be solved using the input trimmers as [attenuverters](https://www.modularbias.com/glossary/attenuverter/), not attenuators. But the built in SineSquare Oscillators map to the ±6v range and sound great as modulators, and there is an easy work around the 0-12v issue I'll talk about the CV I/O section. 

So, Audio In 1 + 2 were repurposed as control inputs for the X and Y parameters and the trimmers act attention control for the input source.

The X trimmer has an additional function used in conjuction with the Z switch.

#### The Z Switch
The Z switch does 4 things:
1. In the middle position, the module is in the Normal mode, with normal outputs on Audio Out 1 + 2
2. In the up position, the module is in the Alt mode, with alternative outputs on Audio Out 1 + 2
3. Pushing the momentary switch down and returing to the middle will move the engine to the next slot, and change the LED. Note: the change happens when you release the Z switch, not when you push it down, to allow for the clock stuff.
4. Holding the switch down and moving the X trimmer will adjust the speed of the clock output from Pulse Out 2.


> Tip: The term to stop the switch from being jittery or just continuously changing while it's being held down is [debouncing](https://en.wikipedia.org/wiki/Switch#Contact_bounce). If you want to implement something similar, tell the coding agent to use "debouncing" for the momentary switch.

#### Audio Out
Audio Out 1 and 2 are the two outputs for the module and will either output the Normal mode or Alt mode, depending on the Z switch position.

#### CV I/O
The repurposed Audio In 1 + 2 for the X/Y CV is explained above. The remaining CV I/O is

* **CV In 1**
- 1v per Octave CV in connected to the main tuning. It works relative to the Main knob position but doesn't exceed the 10Hz - 10KHz range limit. 

> Tip: This is fun to connect to the SineSquare Oscillators, but as there is no attenuator for it, you can pass the osciallator output through the Stompbox section and use the Blend knob to attentuate the signal. CV In 1 runs at control rate - approx 1kHz - so you it can get FM-ish sounds this way but it's not full on FM.

* **CV In 2**
- CV In 2 is a VCA, an idea I borrowed took from the Twists but implemented differently. It acts like standard VCA for both outputs, but you need to provide the gate/envelope. The input responds to 0-5v range signals. Zero and below mutes the both outputs, 5 and above (up to 6 really) is unity gain for the outputs. It's also unity gain if there is nothing plugged in. There is configureable slew in the code that stops the VCA from being clicky when getting gate inputs. I like where it is but you can change and recompile. Look for `kVCAGainSmoothShift`, or tell your coding agent to do it. 

_**Note:** the Ring Mod can also be used as a VCA, but then you don't get stereo and you no longer have a ring modulator. This frees it up for additional weirdness!_

> Tip: Patching the Pulse Out 1 to CV In 2 with USB Midi will gate the outputs to note on/off messages. And if you patch Pulse Out 1 into a slope input and then into CV In 2, you can create variable envelopes to shape the start and end of the notes. 

* **CV Out 1**
- This passes the Midi note out in 1v per Octave format. This is independent of the Main knob or CV In 1 so it's a direct Midi to CV mapping, using the `CVOut1MIDINote()` function from the ComputerCard which is the calibrated output. Handy for sending Midi sequences to other oscillators.

* **CV Out 2**
- CV Out 2 is a secret weapon for me. The main connection is to Midi CC 74, so you can send control messages from your DAW/Midi sequencer can control whatever accepts -5 to 5v input. Midi CC is 128 steps, so it might be a bit jagged for precision requirements. However when is starts up or the Midi CC 74 value is 0, it is outputting -5v. Which can be very useful on the workshop system.

> Tip: The SineSquare Oscillators can become LFOs if you patch the -5v coming out of CV Out 2 into the Pitch In. They just run a lot slower!

> Tip: The Slopes are type of variable [slew rate limiter](https://en.wikipedia.org/wiki/Slew_rate) and you can change them from the default 0-12v output to -5v to 7v range by patching the -5v coming out of CV Out 2 into the Slope input, not the Slope's CV In, which changes the rate. This is hand for getting another LFO that works well with other CV inputs on the Computer module.

#### Pulse I/O
The bottom row of jacks is the Pulse section. These in's are simple.

* **Pulse In 1**
- When plugged in it overides the Z up/middle position setting and goes between Normal mode with a low input (0v) and Alt mode with a high input (5v).

* **Pulse In 2**
- When plugged in it overides the Z momentray position setting and advances to the next enging on each new rising edge (0 to 5v).

> Tip: Connecting the SineSquare Oscillators square outputs to these Pulse inputs gets very interesting!

* **Pulse Out 1**
- This follows the Midi gate on/off, going high (5v) while the note is held.

* **Pulse Out 2**
- The is other secret weapon. Pulse Out 2 is clock output that can drive sequencers, delays or just make interesting rythmic patterns. The speed of the clock can be adjusted by holding the Z switch down and turning the X trimmer. Once you've turned the X knob the engine slot won't advance when you release Z. The clock rate goes from 20 - 999 BPM. If there a Midi clock present at the USB Midi interface, the ouput will follow the Midi clock and the X knob changes the divisions applied.

| Division | Rate |
| --- | --- |
| 24 | 1/4 note |
| 12 | 1/8 note |
| 8 | 1/8 note triplet |
| 6 | 1/16 note |
| 4 | 1/16 note triplet |
| 3 | 1/32 note |
| 2 | 1/32 triplet |
| 1 | raw 24 PPQN clock |

- The output is a ±5ms, 5v pulse for each clock tick. I tested with with my Moog DFAM and drives the sequencer as expected!

> Tip: Plugging Pulse Out 2 output into a Slope input lets you keep the tempo of the Slope output but adjust the slopes angles for interesting rythmic timbre changes.

#### USB
The USB input is purely a class compliant Midi interface. It only does Midi note value, on/off and Midi CC 74. The note value is a "last note wins" implentation. The CV Out 1 and main tuning keep the last value too. It's hardcoded to Midi channel 1, but you can change this and recompile the firmware if you want something else, ask your coding agent! I may make this configureable with a web interface at some point, but not today.

#### LEDs
This shows the currently select engine and flashes briefly when recieving Midi notes.

## The 6 Engines
_**Note:** I came up with these names are a few beers and some fine Dutch herbs. Not my best work but I'm standing by them._

All the engines use one or more [phase acculumators](https://en.wikipedia.org/wiki/Numerically_controlled_oscillator#Phase_accumulator) as the main source oscillator driving the outputs and then use the phase position to select the output value from a look up table with some interpolation create the out waves used by the engines.

#### Sawsome
It's a [supersaw](https://en.wikipedia.org/wiki/Roland_JP-8000#Supersaw) implementaion with 7 waves spread across the stereo field.

*Normal Mode*  
This is the saw wave mode, with 7 saw waves made using a wrapping phase accumulator with bandlimiting to deal with aliasing. I learned about [PolyBlep](https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/) as a the way to keep Saw waves from aliasing at higher frequencies. The 7 saw waves are spread out as follows.

| Voice | Tune | Pan | Gain |
|---|---:|---:|---:|
| 1 | -106.8c | -0.8999 | 0.5000 |
| 2 | -63.8c | -0.6001 | 0.7000 |
| 3 | -24.2c | -0.3000 | 0.8501 |
| 4 | 0.0c | 0.0000 | 1.0000 |
| 5 | +27.3c | +0.3000 | 0.8501 |
| 6 | +67.6c | +0.6001 | 0.7000 |
| 7 | +91.3c | +0.8999 | 0.5000 |

**X** controls the stereo spread between the two outputs and **Y** controls the amount of detune. Both going from 0 at full CCW to the values in the above table at full CW.

*Alt Mode*  
This is a super triangle mode, pretty much the same thing but with triangles waves. The triangle waves are made using the saw wave generation process and a per-voice leaky integrator. A what? See the Agent Coding section for some details on this. **X** and **Y** do the same thing.

#### Bender
A combination wavefolder and bitcrusher.

*Normal Mode*  
2 sine waves using the `SineLUT` function are generated with one going through a wavefolder to Audio Out 1 and the other going through a bitcrusher to Audio Out 2. The code of both of them was borrowed from Utility-Pair. 

**X** is the amount of folding and **Y** is the amount of crushing.

*Alt Mode*  
In Alt Mode the wavefolder is routed through an additional bitcrusher first before going to output and vice versa. 

**X** is the amount of folding for both folders and **Y** is the amount of crushing for both bitcrushers.

#### Floatble
A wavetable oscillator with 2 wavetables based on 16 x 256 sample [AKWF](https://www.adventurekid.se/akrt/waveforms/adventure-kid-waveforms/) single cycle waves. One wavetable per mode, i.e. Normal and Alt use a different wavetable. The 2 audio outputs are different interpolated morphing positions over the 16 waves with **X** and **Y** used to adjust the position. You sweep through the same wavetable but the outputs and just different postions in the wavetable. 

I originally wanted to have 2 different wavetables per mode, so 4 in total, but the reads from flash via XIP were too much and caused it to glitch. Something I learned about during this whole process. 

The actual wavetables used were createed using a tool I got the coding agent to make that lets me browse through, filter, audition and assemble my own "curated" wavetable. See the [floatable wavetable creator](mtws/tools/floatable_wavetable_creator) folder. 

- [ ] TO-DO, picture of the tool goes of the here.

This took an evening to make while dual screening a show and is surpsingly fun to use. If you don't like the wavetables being used, use the tool to make your own or tell your coding agent to use some wave files you like, convert to the required header format and rebuild the firmware. 

#### Cumulus
This is the original additive osciallator I was going for. I borrowed most of the fundamental design ideas from the Plait Harmonic Oscillator which is a bump and slope implementation with 16 partials. The Plaits uses 24 which is more than the RP2040 can handle. 

- [ ] TO-DO, picture of the bump and slope goes here.

It's 16 x phase increment oscillators whose frequencies follow the standandard [harmonic series](https://en.wikipedia.org/wiki/Harmonic_series_(music)) from the fundamental, each generating a sine wave using the `SinLUT()` function. 

The was some complexity managing the overall output gain to keep a constant percieved volume as the gain of the individual partials changed. There was a lot of back and forth with the coding agent to get this right. 

_Normal Mode_  
**X** moves the _bump_ from the first partial to the last (16). This is interpolate between partials but it can sound like it's stepping with a steep slop. **Y** sets the _slope_ for very steep at full CCW, where only the 2 closest partials to the _bump_ postion are active to flat at full CW, where all partials have the same gain. Well approximately at either end, making exactly flat or only the 2 partials would require extra caculations for an alreay busy osciallator. At the mid point of **Y** the partials follow a $1/N$  distrubution, also a [harmonic series](https://en.wikipedia.org/wiki/Harmonic_series_(mathematics)) but like, a maths one. 

There is a different code path used to calculate the gain for each partial depending on whether **Y** is before of after the mid point to make the scaling sound natural. Ask your coding agent about this: _"How does the Y slope calculation path change over at the mid point?"_

_Alt Mode_  
The only change with Alt mode is that it implements a "centroid" feature that can change the frequencies of the partials. Using the _bump_, i.e. the **X** position as the central point, **Y** will additionally modify the partial frequencies to "move in" toward the _bump_ positional going CCW and "move out" to the edges (partial 1 and 16) goin CW. Try it, it sounds very cool!

- [ ] TO-DO, picture of the bump and slope plus harmonic shift goes here.

#### Losenge
I have a very old and somewhat road worn Nord Modular Micro that was my introduction to the world of modular synthesis. 

- [ ] TO-DO, picture of the nord

It has both a vocal oscillator and vowel filter that I really liked and I wanted to make something similar. I tried making this work by putting 2 bandpass filters at the vowel [formant frequencies](https://en.wikipedia.org/wiki/Formant) after a saw wave but it didn't have the depth of sound I remember from the Nord. I want back to Twists/Braids and the `VOWL` was much closer to what I wanted, so I got the coding agent to implement that.

The concept is 3 there are "formant" osciallators running at "fixed" frequencies. 2 x sine waves using `SinLUT()` and a square wave using `SquareQ12()`. They are running at the 3 formant frequencies for the vowel sound we want to make. As for *A* its a sin wave at 609Hz, another 1000Hz and the square running at 2450Hz. These are then summed/mixed together at gain values of 1, 0.5 and 0.251 respectively. The frequeny and gain values vary per vowel and there are tables with all these values in the code. 

There is also the primary phase increment based ramp oscillator, with it's frequeny attached to the Main knob and other frequency modifiers, known as the "[glottal](https://en.wikipedia.org/wiki/Glottis) envelope" that does 2 things:

1. It applies amplitude modulation to the 3 summed formant oscillators.
2. It resests the the phase of all 3 of the formant oscillators every time it wraps, like oscillator sync. 

This gives the impression of an overall fundamnetal frequency being applied to the 3 formant oscillators that can be shifted around while keep the underlying vowel consistent. By changing the frequency and gain values of the formant osciallatros independently from the main glottal envelope, you get the singing voice timbre of the losenge engine.

_Normal Mode_
This runs the 3 formant oscillators at F1, F2 and F3 for the associated vowel. _**Note:** This is the mapping for a male voice_

| Vowel | F1 (Hz) | F2 (Hz) | F3 (Hz) |
| --- | ---: | ---: | ---: |
| A | 609 | 1000 | 2450 |
| E | 400 | 1700 | 2300 |
| IY | 238 | 1741 | 2450 |
| O | 325 | 700 | 2550 |
| U | 415 | 1400 | 2200 |

**X** morphs the frequency and gain values between the vowels, moving from A > E > IY > O > U over the rand of the know.

**Y** changes the F1 value but keeps the ratios between the same so it moves from sounding  darker/lower at at full CCW to brighter/higher at full CW.

## Alt Mode
This is the brighter upper-formant table. It replaces the base `F1/F2/F3` set with an upper `F2/F3/F4` values.

| Vowel | F2-like (Hz) | F3-like (Hz) | F4-like (Hz) |
| --- | ---: | ---: | ---: |
| A | 1000 | 2450 | 3300 |
| E | 1700 | 2300 | 3500 |
| IY | 1741 | 2450 | 3300 |
| O | 700 | 2550 | 3400 |
| U | 1400 | 2200 | 3300 |

I originally used the mapping for a female voice, but it didn't sound all the different when keeping the fundamental gottal envelope frequency the same.


#### Din Sum

## Working With a Coding Agent

* gemini vs claude vs codex

* how to get past the AI being wrong. sound artifacts rather than errors
* looking at some code.

* workflow - single engine, bring back into main path.

* AGENTS file - utility pair, examples, braids
  * dual core stuff
  * control rate vs audio rate
  * tell me the plan before
  * skills, for future me and you
  * comment everywhere and write docs.
  * Look at other examples first

* trade offs
  * control vs pure
  * anti aliasing

* tools - CLI scope, web front end.

* Ask Questions.

> How is triangle in the alt mode for sawsome generated?

In `sawsome`, the alt-mode triangle is generated by taking the same per-voice PolyBLEP saw and running it through a simple per-voice leaky integrator. The mode switch happens in sawsome_engine.cpp (line 170): normal mode calls PolyBlepSawQ12(...), while alt mode calls PolyBlepTriangleQ12(...).

The triangle function itself is in sawsome_engine.cpp (line 154):

it first computes the band-limited saw with PolyBlepSawQ12(...)
then accumulates that into tri_state_[voice_index]
then applies a small leak with tri_state_[voice_index] -= tri_state_[voice_index] >> 11
then scales it back down with >> 5 and clamps to 12-bit
So the core shape is basically:

saw = PolyBlepSawQ12(...)
tri_state += saw
tri_state -= tri_state >> 11
triangle = tri_state >> 5
A couple of useful details:

It is not using a LUT for the triangle.
It is per voice, using tri_state_[kNumSawsomeVoices] in sawsome_engine.h (line 23).
The leak term keeps the integrator from drifting or blowing up over time.
The detune, pan, and gain setup stay the same; alt mode only changes the waveform source.

Explain to me how this `tri_state_[voice_index] -= tri_state_[voice_index] >> 11` creates a triangle

That line does not create the triangle by itself. The triangle-ish waveform comes from the whole block in sawsome_engine.cpp (line 154):

int32_t saw = PolyBlepSawQ12(...);
tri_state_[voice_index] += saw;
tri_state_[voice_index] -= tri_state_[voice_index] >> 11;
return Clamp12(tri_state_[voice_index] >> 5);
The key step is tri_state += saw. That is a discrete-time integrator: each sample, it adds the current saw value into a running state. Then tri_state -= tri_state >> 11 subtracts about 1/2048 of that state each sample, since >> 11 is roughly divide-by-2048. So the full update is approximately:

y[n] = 0.9995 * y[n-1] + x[n]
That is a leaky integrator, or a very slow one-pole low-pass.

Why that sounds triangle-like: integrating a bright waveform knocks down its upper harmonics much harder, so the saw becomes much softer and more “triangle-ish.” The leak keeps the state from drifting or blowing up from tiny DC/rounding errors, and it stabilizes the amplitude. At 48 kHz, that leak constant is gentle enough that over most of one oscillator cycle it behaves almost like a plain integrator.

One important nuance: this is not a mathematically perfect triangle. A perfect triangle is more naturally made by integrating a square wave or folding phase directly. Here it’s really “PolyBLEP saw through a leaky integrator,” which is a cheap, stable approximation that gets close enough sonically.

What is XIP?

`XIP` means `execute in place`.

On the RP2040, your program and `const` data usually live in external QSPI flash, but the chip memory-maps that flash so code can run from it and data can be read from it without first copying everything into SRAM.

In practice for this repo:
- functions not marked to run from RAM often execute from flash via XIP
- large `static const` tables like the `floatable` wavetable banks are typically read directly from flash via that same mapping
- this saves scarce SRAM, which is why it’s attractive for big lookup tables

The tradeoff is speed and predictability:
- SRAM access is faster and more deterministic
- XIP flash access goes through the flash interface and cache, so it can be slower or less predictable under heavy access
- that’s why hot code paths sometimes use `__not_in_flash_func(...)` to place the function in RAM instead of executing from flash

So when I said the `floatable` tables are being read through XIP, I meant:
- the wavetable arrays are stored in flash
- `RenderSample()` dereferences them directly each audio tick
- they are not copied into a separate RAM buffer first

A simple mental model:
- SRAM: fast workbench
- flash/XIP: big storage shelf you can reach directly, but not as quickly as the workbench

If you want, I can explain how `__not_in_flash_func` and the RP2040 linker/script usually decide what ends up in RAM vs flash in this project.

