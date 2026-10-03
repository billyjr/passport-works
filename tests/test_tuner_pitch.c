#include "tuner_pitch.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int16_t pcm[TUNER_FRAME_SAMPLES];
static void tone(float hz, bool harmonic) {
    for (int i = 0; i < TUNER_FRAME_SAMPLES; ++i) {
        float phase = 6.28318530718f * hz * i / 16000;
        pcm[i] = (int16_t)(3500 * sinf(phase) + (harmonic ? 6500 * sinf(2 * phase) + 2000 * sinf(3 * phase) : 0) + 700);
    }
}
int main(void) {
    const float frequencies[] = {41.203f, 55, 65.406f, 73.416f, 82.407f, 98, 110, 146.832f, 196, 246.942f, 293.665f, 329.628f, 440, 659.255f, 880, 1100};
    for (unsigned i = 0; i < sizeof(frequencies)/sizeof(frequencies[0]); ++i) {
        for (int harmonic = 0; harmonic <= 1; ++harmonic) {
            tone(frequencies[i], harmonic);
            tuner_pitch_t p = tuner_detect(pcm, 440);
            assert(p.valid);
            assert(fabsf(1200 * log2f(p.frequency / frequencies[i])) < 6);
        }
    }
    tone(440 * powf(2, 23.0f / 1200), false);
    tuner_pitch_t p = tuner_detect(pcm, 440);
    assert(p.valid && p.midi == 69 && fabsf(p.cents - 23) < 3);
    tone(440 * powf(2, -31.0f / 1200), false);
    p = tuner_detect(pcm, 440);
    assert(p.valid && p.midi == 69 && fabsf(p.cents + 31) < 3);
    tone(442, false);
    p = tuner_detect(pcm, 442);
    assert(p.valid && fabsf(p.cents) < 3);
    memset(pcm, 0, sizeof(pcm));
    assert(!tuner_detect(pcm, 440).valid);
    for (int i = 0; i < TUNER_FRAME_SAMPLES; ++i) pcm[i] = 9000;
    assert(!tuner_detect(pcm, 440).valid);
    uint32_t noise = 7;
    for (int i = 0; i < TUNER_FRAME_SAMPLES; ++i) {
        noise = noise * 1664525u + 1013904223u;
        pcm[i] = (int16_t)((noise >> 16) - 32768);
    }
    assert(!tuner_detect(pcm, 440).valid);
    assert(strcmp(tuner_note_name(64), "E") == 0);
    puts("Tuner pitch: PASS (notes, harmonics, cents, calibration, silence, DC, noise)");
}
