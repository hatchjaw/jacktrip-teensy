#include <Audio.h>
#include <JackTripClient.h>
#include "SynchroTester/SynchroTester.h"

// Wait for a serial connection before proceeding with execution
#define WAIT_FOR_SERIAL
#undef WAIT_FOR_SERIAL

#ifndef NUM_JACKTRIP_CHANNELS
#define NUM_JACKTRIP_CHANNELS 2
#endif

// Define this to print packet stats.
#define SHOW_STATS
//#undef SHOW_STATS

// Shorthand to block and do nothing
#define WAIT_INFINITE() while (true) yield();

// Local udp port on which to receive packets.
const uint16_t kLocalUdpPort = 8888;
// Remote server IP address -- should match address in IPv4 settings.
IPAddress jackTripServerIP{192, 168, 10, 10};

// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

audio_block_t *inputQueue[NUM_JACKTRIP_CHANNELS];
JackTripClient jtc{NUM_JACKTRIP_CHANNELS, inputQueue, jackTripServerIP};
SynchroTester s;

// Send input from server back to server.
AudioConnection patchCord10(jtc, 0, jtc, 0);
// Send input from server to audio output.
AudioConnection patchCord20(jtc, 0, out, 0);
// Send input to synchronicity tester.
AudioConnection patchCord30(jtc, 0, s, 0);
// Combine with generated sawtooth.
AudioConnection patchCord40(s, 0, out, 1);
// Send synchronicity measure back to server.
AudioConnection patchCord50(s, 0, jtc, 1);

elapsedMillis performanceReport;
const uint32_t PERF_REPORT_INTERVAL = 5000;

//region Forward declarations
void startAudio();
//endregion

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (CrashReport) {  // Print any crash report
        Serial.println(CrashReport);
        CrashReport.clear();
    }

    Serial.printf("Sampling rate: %f\n", AUDIO_SAMPLE_RATE_EXACT);
    Serial.printf("Audio block samples: %d\n", AUDIO_BLOCK_SAMPLES);
    Serial.printf("Number of JackTrip channels: %d\n", NUM_JACKTRIP_CHANNELS);

#ifdef SHOW_STATS
    jtc.setShowStats(true, 5'000);
#endif

//    jtc.setOnConnected([] { s.setParamValue("reset", 1); });

    if (!jtc.begin(kLocalUdpPort)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

    AudioMemory(32);

    startAudio();
}

void loop() {
    if (!jtc.isConnected()) {
        jtc.connect(2500);
    } else {
        if (performanceReport > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            performanceReport = 0;
        }
    }
}


void startAudio() {
    audioShield.enable();
    // "...0.8 corresponds to the maximum undistorted output for a full scale
    // signal. Usually 0.5 is a comfortable listening level."
    // https://www.pjrc.com/teensy/gui/?info=AudioControlSGTL5000
    audioShield.volume(.8);

    audioShield.audioProcessorDisable();
    audioShield.autoVolumeDisable();
    audioShield.dacVolumeRampDisable();
}
