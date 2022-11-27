//
// Created by tar on 27/11/22.
//
#include <Audio.h>
#include <JackTripClient.h>
#include "Gain/Gain.h"

// Define this to wait for a serial connection before proceeding with execution
#define WAIT_FOR_SERIAL

#ifndef NUM_JACKTRIP_CHANNELS
#define NUM_JACKTRIP_CHANNELS 1
#endif

// Define this to print packet stats.
#define SHOW_STATS

// Shorthand to block and do nothing
#define WAIT_INFINITE() while (true) yield();

// Local udp port on which to receive packets.
const uint16_t kLocalUdpPort = 8888;
// Remote server IP address -- should match address in IPv4 settings.
IPAddress jackTripServerIP{192, 168, 10, 10};
// Parameters for OSC over UDP multicast.
IPAddress oscMulticastIP{230, 0, 0, 20};
const uint16_t kOscMulticastPort{41814};

// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

audio_block_t *inputQueue[NUM_JACKTRIP_CHANNELS];
JackTripClient jtc{NUM_JACKTRIP_CHANNELS, inputQueue, jackTripServerIP};

Gain g;

// Server -> UDP (1 channel) -> JTC
// JTC out 0 -> audio -> Gain in 0
// Gain out 0 -> audio -> I2S in 0
// Gain out 0 -> audio -> JTC in 0
// JTC -> UDP (1 channel) -> Server
AudioConnection patchCord10(jtc, 0, g, 0);
AudioConnection patchCord20(g, 0, out, 0);
AudioConnection patchCord30(g, 0, jtc, 0);

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

#ifdef SHOW_STATS
    jtc.setShowStats(true, 5'000);
#endif

    if (!jtc.begin(kLocalUdpPort)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

    AudioMemory(32);

    startAudio();

    g.setParamValue("gain", .9);
}

void loop() {
    if (!jtc.isConnected()) {
        jtc.connect(2500);
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
