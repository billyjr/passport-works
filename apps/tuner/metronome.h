#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int bpm;
    int volume;
    int beat;
    int64_t next_us;
    bool running;
} metronome_t;

void metronome_start(metronome_t *m, int64_t now_us);
void metronome_stop(metronome_t *m);
void metronome_set_bpm(metronome_t *m, int bpm, int64_t now_us);
void metronome_set_volume(metronome_t *m, int volume);
// Returns true once for a due beat, skipping missed beats after a stall.
bool metronome_tick(metronome_t *m, int64_t now_us, int *beat);
