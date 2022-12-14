//
// Created by tar on 27/11/22.
//
#include <Audio.h>
#include <JackTripClient.h>
#include "Gain/Gain.h"

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

JackTripClient jtc{NUM_JACKTRIP_CHANNELS, jackTripServerIP};

Gain g1, g2;

// Server -> UDP (2 channels) -> JTC
// JTC out 0 -> audio -> Gain1 in 0
// JTC out 1 -> audio -> Gain2 in 0
// Gain1 out 0 -> audio -> I2S in 0
// Gain2 out 0 -> audio -> I2S in 1
// Gain1 out 0 -> audio -> JTC in 0
// Gain2 out 0 -> audio -> JTC in 1
// JTC -> UDP (2 channels) -> Server
AudioConnection patchCord10(jtc, 0, g1, 0);
AudioConnection patchCord15(jtc, 1, g2, 0);
AudioConnection patchCord20(g1, 0, out, 0);
AudioConnection patchCord25(g2, 0, out, 1);
AudioConnection patchCord30(g1, 0, jtc, 0);
AudioConnection patchCord35(g2, 0, jtc, 1);

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

#ifdef SHOW_STATS
    jtc.setShowStats(true, 5'000);
#endif

    if (!jtc.begin(kLocalUdpPort)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

    AudioMemory(32);

    startAudio();

    g1.setParamValue("gain", .9);
    g2.setParamValue("gain", .9);
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
