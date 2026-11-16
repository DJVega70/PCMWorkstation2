# PCM Workstation (VST3 instrument)

A 16-voice, 8-part multitimbral PCM-style workstation synth, in the spirit of late-80s digital
workstations. Original design and code: no third-party samples, names or artwork.

![preview](preview.png)

## What it is
* **Sound engine:** every sound is an *attack transient* (one-shot, pitch-shifted across the keyboard)
  plus a *looped waveform*, filtered and enveloped. All 28 loops and 14 transients are generated
  mathematically at start-up (about 0.06 s), so there is no sample ROM and nothing sampled from hardware.
* **100 programs** (25 sound families x 4 voicings: normal / Br(ight) / So(ft) / Wd (wide)): pianos,
  electric piano, organs, strings, pads, brass, sitar, harpsichord, mallets, bells, choir, voice,
  flute, reed, basses, leads, digital pads and more.
* **100 combinations:** layers, keyboard splits, and one 8-part multi setup.
* **Three modes:** PROGRAM (part 1 plays everything), COMBI (all active parts play, honouring each
  part's key range, transpose, level and pan), MULTI (each part answers its own MIDI channel).
* **16 voices**, oldest/quietest voice stealing; sustain pedal, pitch bend, mod wheel, on-screen joystick.
* **Two effect units** in series: reverb, delay, chorus, flanger, distortion, rotary speaker.
* **Tone macros:** brightness, resonance, attack, release, LFO depth, bend range.
* **Not included:** a built-in sequencer. Use your DAW's piano roll, which is far more capable.

## Measured with `tests/pcm_test.cpp` (48 kHz)
| Check | Result |
|---|---|
| Pitch accuracy | within 3 cents at notes 36, 69, 96 (measured: exact to 0.01 Hz) |
| Polyphony | 20 notes in -> exactly 16 voices; 16-note chord peaks at 0.28 |
| Release | all voices decay to true silence |
| Routing | MULTI: channel 2 triggers only part 2; split and layer combinations trigger the right parts |
| All 100 programs | 300 renders (3 notes each): none non-finite, none silent, none clipping |
| Loudness | peak spread across programs at C4: 11 dB (pads and bright voicings are the loudest) |
| Aliasing | saw at note 100: non-harmonic energy 154 dB below the signal |
| Controls | velocity scales level; joystick bend = 199.9 cents for a 2-semitone range; pedal holds then releases |
| Effects | all six finite, tails decay to silence, mix 0 is bit-exact dry |
| CPU | 16 voices + 2 effects: about 1.3% of one core |

These sounds are synthesized approximations of instrument families, not recordings.
They are not meant to reproduce any specific hardware's samples.

## Build (Windows, via GitHub Actions)
Push this repo to GitHub, open the Actions tab, open the latest run and download the
`PCMWorkstation-VST3-win64` artifact. Unzip it and copy the `PCM Workstation.vst3` folder to
`C:\Program Files\Common Files\VST3\`, then rescan plugins in your DAW. In FL Studio add it as an
instrument (it appears in the plugin list as "PCM Workstation") and play it from a MIDI keyboard or
the piano roll.

## Run the tests
`g++ -O2 -std=c++17 -o pcm_test tests/pcm_test.cpp && ./pcm_test`

`tools/gen_levels.cpp` regenerates `Source/Levels.h` (loudness matching of the 100 programs).
