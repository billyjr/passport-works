#include "metronome.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    metronome_t m = {.bpm = 120, .volume = 65};
    int beat = -1;
    assert(!metronome_tick(&m, 0, &beat));
    metronome_start(&m, 1000000);
    assert(metronome_tick(&m, 1000000, &beat) && beat == 0);
    assert(!metronome_tick(&m, 1499999, &beat));
    assert(metronome_tick(&m, 1500000, &beat) && beat == 1);
    assert(metronome_tick(&m, 3000000, &beat) && beat == 0);
    metronome_set_bpm(&m, 240, 3000000);
    assert(!metronome_tick(&m, 3249999, &beat));
    assert(metronome_tick(&m, 3250000, &beat) && beat == 1);
    metronome_set_bpm(&m, 10, 3250000);
    assert(m.bpm == 40);
    metronome_set_bpm(&m, 999, 3250000);
    assert(m.bpm == 240);
    metronome_set_volume(&m, 70);
    assert(m.volume == 70);
    metronome_set_volume(&m, -5);
    assert(m.volume == 0);
    metronome_set_volume(&m, 105);
    assert(m.volume == 100);
    metronome_stop(&m);
    assert(!metronome_tick(&m, 4000000, &beat));
    puts("Metronome timing: PASS");
}
