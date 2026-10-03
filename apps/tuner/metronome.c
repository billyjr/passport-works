#include "metronome.h"

static int64_t period_us(int bpm) { return 60000000LL / bpm; }

void metronome_start(metronome_t *m, int64_t now_us) {
    m->beat = 0;
    m->next_us = now_us;
    m->running = true;
}

void metronome_stop(metronome_t *m) { m->running = false; }

void metronome_set_bpm(metronome_t *m, int bpm, int64_t now_us) {
    if (bpm < 40) bpm = 40;
    if (bpm > 240) bpm = 240;
    m->bpm = bpm;
    if (m->running) m->next_us = now_us + period_us(bpm);
}

void metronome_set_volume(metronome_t *m, int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    m->volume = volume;
}

bool metronome_tick(metronome_t *m, int64_t now_us, int *beat) {
    if (!m->running || now_us < m->next_us) return false;
    int64_t period = period_us(m->bpm);
    int64_t skipped = (now_us - m->next_us) / period;
    m->beat = (m->beat + (int)(skipped % 4)) % 4;
    *beat = m->beat;
    m->beat = (m->beat + 1) % 4;
    m->next_us += (skipped + 1) * period;
    return true;
}
