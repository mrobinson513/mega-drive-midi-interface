#include "comm_demo.h"
#include <joy.h>
#include <stdbool.h>
#include <sys.h>
#include "midi_fm.h"
#include "log.h"

#define programChange 0xC0
#define noteOnStatus 0x90
#define noteOffStatus 0x80
#define noteKey 48
#define noteVelocity 127

#define PROGRAM_INDEX 1
#define NOTE_KEY_1_INDEX 3
#define NOTE_ON_END_INDEX 4
#define NOTE_KEY_2_INDEX 6
#define NOTE_OFF_END_INDEX 7

#define NOTE_ON_WAIT 10000
#define NOTE_OFF_WAIT 500

static u8 cursor;
static u16 wait;
static bool enabled;
static u8 pitch;
static u8 program;

u8 track[] = {
    programChange,
    0,
    noteOnStatus,
    noteKey,
    noteVelocity,
    noteOffStatus,
    noteKey,
    noteVelocity,
};

void comm_demo_init(void)
{
    JOY_init();
    cursor = 0;
    wait = 0;
    enabled = false;
    pitch = noteKey;
    program = 0;
}

u8 comm_demo_read_ready(void)
{
    if (!enabled) {
        if (JOY_readJoypad(JOY_1) & BUTTON_A) {
            enabled = true;
        }
    }
    if (!enabled) {
        return false;
    }

    if (wait == 0) {
        return true;
    } else {
        wait--;
        return false;
    }
}

u8 comm_demo_read(void)
{
    if (cursor == 0) {
        track[PROGRAM_INDEX] = program;
        track[NOTE_KEY_1_INDEX] = pitch;
        track[NOTE_KEY_2_INDEX] = pitch;
        log_info("Demo: Pitch=%d Prg=%d", pitch, program);
    }
    u8 data = track[cursor];
    cursor++;
    if (cursor == NOTE_ON_END_INDEX + 1) {
        wait = NOTE_ON_WAIT;
    }
    if (cursor == NOTE_OFF_END_INDEX + 1) {
        wait = NOTE_OFF_WAIT;
        cursor = 0;
    }
    return data;
}

void comm_demo_vsync(void)
{
    JOY_update();

    u16 curState = JOY_readJoypad(JOY_1);
    if (curState & BUTTON_UP) {
        pitch++;
        if (pitch > MAX_MIDI_PITCH) {
            pitch = MAX_MIDI_PITCH;
        }
    }
    if (curState & BUTTON_DOWN) {
        pitch--;
        if (pitch < MIN_MIDI_PITCH) {
            pitch = MIN_MIDI_PITCH;
        }
    }
    if (curState & BUTTON_RIGHT) {
        program++;
        if (program > 0x7F) {
            program = 0x7F;
        }
    }
    if (curState & BUTTON_LEFT) {
        program--;
        if (program == 0xFF) {
            program = 0;
        }
    }
}

u8 comm_demo_write_ready(void)
{
    return false;
}

void comm_demo_write(u8 data)
{
    (void)data;
}
