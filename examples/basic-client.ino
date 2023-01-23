#include <Audio.h>
#include <JackTripClient.h>

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

// UDP in to I2C out.
AudioConnection patchCord1(jtc, 0, out, 0);
AudioConnection patchCord2(jtc, 1, out, 0);
// Patch JTC back to itself for UDP out.
AudioConnection patchCord3(jtc, 0, jtc, 0);
AudioConnection patchCord4(jtc, 1, jtc, 1);

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

#ifdef SHOW_STATS
    jtc.setShowStats(true, 5'000);
#endif

    if (!jtc.begin(kLocalUdpPort)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

    AudioMemory(32);
    audioShield.enable();
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