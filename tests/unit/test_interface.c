#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "asserts.h"
#include "interface.h"
#include "midi.h"
#include "wraps.h"
#include <cmocka.h>
#include <types.h>

#define STATUS_CC 0xB0
#define STATUS_PITCH_BEND 0xE0

static void test_interface_tick_passes_note_on_to_midi_processor(void** state)
{
    const u8 expectedData = 60;
    const u8 expectedData2 = 127;

    for (int chan = 0; chan < MAX_MIDI_CHANS; chan++) {
        u8 expectedStatus = 0x90 + chan;

        stub_comm_read_returns_midi_event(
            expectedStatus, expectedData, expectedData2);

        expect_value(__wrap_midi_noteOn, chan, chan);
        expect_value(__wrap_midi_noteOn, pitch, expectedData);
        expect_value(__wrap_midi_noteOn, velocity, expectedData2);

        interface_tick();
    }
}

static void test_interface_tick_passes_note_off_to_midi_processor(void** state)
{
    u8 expectedStatus = 0x80;
    u8 expectedData = 60;
    u8 expectedData2 = 127;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedData, expectedData2);

    expect_value(__wrap_midi_noteOff, chan, 0);
    expect_value(__wrap_midi_noteOff, pitch, expectedData);

    interface_tick();
}

static void test_interface_does_nothing_for_control_change(void** state)
{
    u8 expectedStatus = 0xA0;
    u8 expectedData = 106;
    u8 expectedData2 = 127;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedData, expectedData2);

    interface_tick();
    interface_tick();
    interface_tick();
}

static void test_interface_sets_unknown_event_for_system_messages(void** state)
{
    u8 expectedStatus = 0xF0;

    will_return(__wrap_comm_read, expectedStatus);

    interface_tick();

    assert_int_equal(interface_lastUnknownStatus(), expectedStatus);
}

static void test_interface_sets_unknown_CC(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 0x9;
    u8 expectedValue = 0x50;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedController, expectedValue);

    interface_tick();

    ControlChange* cc = interface_lastUnknownCC();

    assert_int_equal(cc->controller, expectedController);
    assert_int_equal(cc->value, expectedValue);
}

static void test_interface_does_not_set_unknown_CC_for_known_CC(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 0x7;
    u8 expectedValue = 0x80;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedController, expectedValue);

    expect_value(__wrap_midi_channelVolume, chan, 0);
    expect_value(__wrap_midi_channelVolume, volume, expectedValue);

    interface_tick();

    ControlChange* cc = interface_lastUnknownCC();
    assert_int_not_equal(cc->controller, expectedController);
    assert_int_not_equal(cc->value, expectedValue);
}

static void test_interface_sets_channel_volume(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 0x7;
    u8 expectedValue = 0x50;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedController, expectedValue);

    expect_value(__wrap_midi_channelVolume, chan, 0);
    expect_value(__wrap_midi_channelVolume, volume, expectedValue);

    interface_tick();
}

static void test_interface_sets_pan(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 0x0A;
    u8 expectedValue = 0xFF;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedController, expectedValue);

    expect_value(__wrap_midi_pan, chan, 0);
    expect_value(__wrap_midi_pan, pan, expectedValue);

    interface_tick();
}

static void test_interface_initialises_synth(void** state)
{
    expect_function_call(__wrap_synth_init);
    interface_init();
}

static void test_interface_sets_fm_algorithm(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 0x0E;

    stub_comm_read_returns_midi_event(expectedStatus, expectedController, 20);

    expect_value(__wrap_synth_algorithm, channel, 0);
    expect_value(__wrap_synth_algorithm, algorithm, 1);

    interface_tick();
}

static void test_interface_sets_fm_feedback(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 0x0F;
    u8 midiValue = 33;
    u8 expectedFeedback = 2;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedController, midiValue);

    expect_value(__wrap_synth_feedback, channel, 0);
    expect_value(__wrap_synth_feedback, feedback, expectedFeedback);

    interface_tick();
}

static void test_interface_sets_channel_AMS(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 76;

    stub_comm_read_returns_midi_event(expectedStatus, expectedController, 32);

    expect_value(__wrap_synth_ams, channel, 0);
    expect_value(__wrap_synth_ams, ams, 1);

    interface_tick();
}

static void test_interface_sets_channel_FMS(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 75;

    stub_comm_read_returns_midi_event(expectedStatus, expectedController, 32);

    expect_value(__wrap_synth_fms, channel, 0);
    expect_value(__wrap_synth_fms, fms, 2);

    interface_tick();
}

static void test_interface_sets_all_notes_off(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedController = 123;
    u8 expectedValue = 0;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedController, expectedValue);

    expect_value(__wrap_midi_noteOff, chan, 0);
    expect_value(__wrap_midi_noteOff, pitch, 0);

    interface_tick();
}

static void test_interface_sets_operator_total_level(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 50;

    for (u8 cc = 16; cc <= 19; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, expectedValue);

        u8 expectedOp = cc - 16;
        expect_value(__wrap_synth_operatorTotalLevel, channel, 0);
        expect_value(__wrap_synth_operatorTotalLevel, op, expectedOp);
        expect_value(
            __wrap_synth_operatorTotalLevel, totalLevel, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_multiple(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 4;

    for (u8 cc = 20; cc <= 23; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 32);

        u8 expectedOp = cc - 20;
        expect_value(__wrap_synth_operatorMultiple, channel, 0);
        expect_value(__wrap_synth_operatorMultiple, op, expectedOp);
        expect_value(__wrap_synth_operatorMultiple, multiple, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_detune(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 2;

    for (u8 cc = 24; cc <= 27; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 32);

        u8 expectedOp = cc - 24;
        expect_value(__wrap_synth_operatorDetune, channel, 0);
        expect_value(__wrap_synth_operatorDetune, op, expectedOp);
        expect_value(__wrap_synth_operatorDetune, detune, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_rate_scaling(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 2;

    for (u8 cc = 39; cc <= 42; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 64);

        u8 expectedOp = cc - 39;
        expect_value(__wrap_synth_operatorRateScaling, channel, 0);
        expect_value(__wrap_synth_operatorRateScaling, op, expectedOp);
        expect_value(
            __wrap_synth_operatorRateScaling, rateScaling, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_attack_rate(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 2;

    for (u8 cc = 43; cc <= 46; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 8);

        u8 expectedOp = cc - 43;
        expect_value(__wrap_synth_operatorAttackRate, channel, 0);
        expect_value(__wrap_synth_operatorAttackRate, op, expectedOp);
        expect_value(
            __wrap_synth_operatorAttackRate, attackRate, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_first_decay_rate(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 2;

    for (u8 cc = 47; cc <= 50; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 8);

        u8 expectedOp = cc - 47;
        expect_value(__wrap_synth_operatorFirstDecayRate, channel, 0);
        expect_value(__wrap_synth_operatorFirstDecayRate, op, expectedOp);
        expect_value(
            __wrap_synth_operatorFirstDecayRate, firstDecayRate, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_second_decay_rate(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 1;

    for (u8 cc = 51; cc <= 54; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 8);

        u8 expectedOp = cc - 51;
        expect_value(__wrap_synth_operatorSecondDecayRate, channel, 0);
        expect_value(__wrap_synth_operatorSecondDecayRate, op, expectedOp);
        expect_value(__wrap_synth_operatorSecondDecayRate, secondDecayRate,
            expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_secondary_amplitude(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 1;

    for (u8 cc = 55; cc <= 58; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 8);

        u8 expectedOp = cc - 55;
        expect_value(__wrap_synth_operatorSecondaryAmplitude, channel, 0);
        expect_value(__wrap_synth_operatorSecondaryAmplitude, op, expectedOp);
        expect_value(__wrap_synth_operatorSecondaryAmplitude,
            secondaryAmplitude, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_amplitude_modulation(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 1;

    for (u8 cc = 70; cc <= 73; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 96);

        u8 expectedOp = cc - 70;
        expect_value(__wrap_synth_operatorAmplitudeModulation, channel, 0);
        expect_value(__wrap_synth_operatorAmplitudeModulation, op, expectedOp);
        expect_value(__wrap_synth_operatorAmplitudeModulation,
            amplitudeModulation, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_operator_release_rate(void** state)
{
    u8 expectedStatus = STATUS_CC;
    u8 expectedValue = 1;

    for (u8 cc = 59; cc <= 62; cc++) {
        stub_comm_read_returns_midi_event(expectedStatus, cc, 8);

        u8 expectedOp = cc - 59;
        expect_value(__wrap_synth_operatorReleaseRate, channel, 0);
        expect_value(__wrap_synth_operatorReleaseRate, op, expectedOp);
        expect_value(
            __wrap_synth_operatorReleaseRate, releaseRate, expectedValue);

        interface_tick();
    }
}

static void test_interface_sets_global_LFO_enable(void** state)
{
    const u8 cc = 74;
    stub_comm_read_returns_midi_event(STATUS_CC, cc, 64);
    expect_value(__wrap_synth_enableLfo, enable, 1);

    interface_tick();
}

static void test_interface_sets_global_LFO_frequency(void** state)
{
    const u8 cc = 1;
    stub_comm_read_returns_midi_event(STATUS_CC, cc, 16);
    expect_value(__wrap_synth_globalLfoFrequency, freq, 1);

    interface_tick();
}

static void test_interface_sets_polyphonic_mode(void** state)
{
    const u8 cc = 80;
    stub_comm_read_returns_midi_event(STATUS_CC, cc, 64);

    expect_value(__wrap_midi_setPolyphonic, state, true);
    will_return(__wrap_midi_getPolyphonic, true);

    interface_tick();

    assert_true(interface_polyphonic());
}

static void test_interface_unsets_polyphonic_mode(void** state)
{
    const u8 cc = 80;
    stub_comm_read_returns_midi_event(STATUS_CC, cc, 0);

    expect_value(__wrap_midi_setPolyphonic, state, false);
    will_return(__wrap_midi_getPolyphonic, false);

    interface_tick();

    assert_false(interface_polyphonic());
}

static void test_interface_sets_pitch_bend(void** state)
{
    u8 expectedStatus = STATUS_PITCH_BEND;
    u16 expectedValue = 12000;
    u8 expectedValueLower = expectedValue & 0x007F;
    u8 expectedValueUpper = expectedValue >> 7;

    stub_comm_read_returns_midi_event(
        expectedStatus, expectedValueLower, expectedValueUpper);

    expect_value(__wrap_midi_pitchBend, chan, 0);
    expect_value(__wrap_midi_pitchBend, bend, expectedValue);

    interface_tick();
}
