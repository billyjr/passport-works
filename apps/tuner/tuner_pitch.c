#include "tuner_pitch.h"
#include <math.h>
#include <stddef.h>

const char *tuner_note_name(int midi) {
    static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return names[(midi % 12 + 12) % 12];
}

tuner_pitch_t tuner_detect(const int16_t *pcm, float reference_hz) {
    tuner_pitch_t result = {0};
    if (!pcm || reference_hz < 400 || reference_hz > 480) return result;
    int16_t samples[TUNER_FRAME_SAMPLES / 2];
    float difference[230] = {0};
    float normalized[230] = {0};
    int64_t sum = 0;
    uint64_t energy = 0;
    unsigned clipped = 0;
    for (int i = 0; i < TUNER_FRAME_SAMPLES; ++i) {
        sum += pcm[i];
        energy += (int64_t)pcm[i] * pcm[i];
        if (pcm[i] > 32000 || pcm[i] < -32000) ++clipped;
    }
    float mean = (float)sum / TUNER_FRAME_SAMPLES;
    result.rms = sqrtf(fmaxf(0, (float)energy / TUNER_FRAME_SAMPLES - mean * mean));
    if (result.rms < 100 || clipped > TUNER_FRAME_SAMPLES / 20) return result;
    // Average adjacent samples before downsampling; integer differences keep
    // the hot loop inexpensive on the ESP32-C3 without an FPU.
    for (int i = 0; i < TUNER_FRAME_SAMPLES / 2; ++i) {
        samples[i] = ((int32_t)pcm[2 * i] + pcm[2 * i + 1]) / 16;
    }
    float running = 0;
    normalized[0] = 1;
    for (int lag = 1; lag <= 229; ++lag) {
        uint64_t d = 0;
        for (int i = 0; i < 512; ++i) {
            int32_t delta = (int32_t)samples[i] - samples[i + lag];
            d += (uint32_t)(delta * delta);
        }
        difference[lag] = (float)d;
        running += difference[lag];
        normalized[lag] = running > 0 ? difference[lag] * lag / running : 1;
    }
    int best = 0;
    for (int lag = 7; lag <= 228; ++lag) {
        if (normalized[lag] < 0.15f) {
            while (lag < 228 && normalized[lag + 1] < normalized[lag]) ++lag;
            best = lag;
            break;
        }
    }
    if (!best) return result;
    // Refine against original-rate samples: high strings need finer than
    // an 8 kHz lag grid, especially when their harmonics dominate.
    int center_lag = best * 2;
    float refined[7];
    int minimum = 3;
    for (int k = 0; k < 7; ++k) {
        int lag = center_lag + k - 3;
        uint64_t d = 0;
        for (int i = 0; i < 1024; ++i) {
            int32_t delta = ((int32_t)pcm[i] - pcm[i + lag]) / 8;
            d += (uint32_t)(delta * delta);
        }
        refined[k] = (float)d;
    }
    minimum = 0;
    for (int k = 1; k < 7; ++k) if (refined[k] < refined[minimum]) minimum = k;
    if (minimum == 0 || minimum == 6) return result;
    float period = center_lag + minimum - 3;
    float left = refined[minimum - 1], center = refined[minimum], right = refined[minimum + 1];
    float denominator = left - 2 * center + right;
    if (denominator > 0) {
        float offset = 0.5f * (left - right) / denominator;
        if (fabsf(offset) <= 1) period += offset;
    }
    float hz = 16000 / period;
    if (hz < 35 || hz > 1200) return result;
    float note = 69 + 12 * log2f(hz / reference_hz);
    result.midi = (int)lroundf(note);
    result.cents = 100 * (note - result.midi);
    result.frequency = hz;
    result.valid = true;
    return result;
}
