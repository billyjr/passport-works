#pragma once
#include <stdbool.h>
#include <stdint.h>
#define TUNER_FRAME_SAMPLES 2048

typedef struct {
    bool valid;
    float frequency;
    float cents;
    float rms;
    int midi;
} tuner_pitch_t;

// 16 kHz mono input, 50..1200 Hz; caller owns its frame.
tuner_pitch_t tuner_detect(const int16_t *pcm, float reference_hz);
const char *tuner_note_name(int midi);
