<p align="right"><a href="tuner.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Chromatic tuner

The device listens to one plucked string and shows the nearest equal-tempered
note and octave, measured frequency, and signed cents relative to that note.
The screen follows the local `assets/images/tuner/tuner-ui-concept-v2.png` design:
a large cream note, an arched coral/mint/amber gauge with a moving pointer,
and open-string markers for the selected instrument. The marker lights only when the detected note
matches that open string; other chromatic notes still show without a marker.
Negative cents means flat; positive means sharp. The green zone is within
5 cents. Automatic note selection always uses the nearest semitone, so a string
more than half a semitone away may show an adjacent note; verify the intended
note before adjusting. The menu offers Guitar 6 (E2 A2 D3 G3 B3 E4),
Bass 4 (E1 A1 D2 G2), and Violin 4 (G3 D4 A4 E5).

The supported detection range is 35..1200 Hz, with A4 initially 440 Hz. Hold OK
to open the instrument menu, use Up/Down to select, and press OK to confirm.
Outside the menu, Up/Down adjust the reference between 430 and 450 Hz;
OK pauses/resumes detection. Settings
are temporary. Pluck one string, mute the others, and place the microphone near
the instrument. Silence clears stale results after one second. The backlight
dims after 30 seconds without a detected pitch or button press and restores on
new input. Listening remains active while dimmed; this is not deep sleep.

Hold Up to switch between the tuner and metronome. The metronome has a matching
navy-and-cream screen with a swinging pointer and four beat lights. It starts
at 120 BPM in 4/4; the first beat has a higher click and coral light. In the
metronome, Up/Down change tempo by 1 BPM from 40 to 240. Hold OK to switch
the Up/Down controls between tempo and volume; the selected value is highlighted.
Volume changes in 5% steps from mute to 100%, starting at 65%. A short OK press
starts or pauses playback in either adjustment mode. Switching back to the
tuner stops the metronome. Tempo, volume, and playback state are temporary.
The two modes share the audio codec but do not record and play concurrently.
Click samples peak below half of full-scale PCM to leave headroom.

Audio is 16 kHz, 16-bit mono. Each 2048-sample frame covers 128 ms. Detection uses
an integer difference function, cumulative normalized threshold, and original
sample-rate refinement. Two consecutive frames with the same note are required
before showing a result. Harmonics, DC offset, noise, detuning, and adjustable
reference are covered by `tests/test_tuner_pitch.c`, run by `tools/validate.sh`.
Synthetic test accuracy does not establish microphone or sample-clock accuracy.
Very weak, clipped, noisy, chordal, or out-of-range signals may not yield a pitch.

Build with `./tools/validate.sh --firmware tuner`.
Sources live in `apps/tuner/`; the card is selected independently.
Device acceptance requires actual strings against a reference tuner,
pointer movement, silence recovery, pause, calibration and runtime stability.
