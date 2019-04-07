#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "midi.h"
#include "sonic.h"
#include "unused.h"
#include <cmocka.h>
#include <sprite_eng.h>
#include <types.h>

#define FRAMES_PER_SEC 50

static int test_sonic_setup(UNUSED void** state)
{
    sonic_reset();
    return 0;
}

static void test_sonic_init(UNUSED void** state)
{
    Sprite dummySprite = {};
    Palette dummyPalette = {};
    const SpriteDefinition dummySpriteDef = { .palette = &dummyPalette };

    will_return(__wrap_SPR_addSprite, &dummySprite);

    sonic_init(&dummySpriteDef);
}

static void test_sonic_single_vsync_does_nothing(UNUSED void** state)
{
    Timing timing = {};

    will_return(__wrap_midi_timing, &timing);

    sonic_vsync();
}

static void test_sonic_goes_into_idle_mode(UNUSED void** state)
{
    const int ANIM_WAIT = 1;

    Timing timing = {};

    expect_any(__wrap_SPR_setAnimAndFrame, sprite);
    expect_value(__wrap_SPR_setAnimAndFrame, anim, ANIM_WAIT);
    expect_any(__wrap_SPR_setAnimAndFrame, frame);

    for (u16 i = 0; i < FRAMES_PER_SEC * 200; i++) {
        will_return(__wrap_midi_timing, &timing);

        sonic_vsync();
    }
}

static void test_sonic_runs(UNUSED void** state)
{
    const int ANIM_RUN = 3;

    Timing timing = {};

    expect_any(__wrap_SPR_setAnimAndFrame, sprite);
    expect_value(__wrap_SPR_setAnimAndFrame, anim, ANIM_RUN);
    expect_any(__wrap_SPR_setAnimAndFrame, frame);

    for (u16 i = 0; i < FRAMES_PER_SEC * 200; i++) {
        timing.clocks++;

        will_return(__wrap_midi_timing, &timing);

        sonic_vsync();
    }
}
