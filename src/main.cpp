#include <NativeEthernet.h>
#include <Audio.h>
#include "faust/PassThrough/PassThrough.h"
#include "JackTripClient.h"

// Define this to wait for a serial connection before proceeding with execution
//#define WAIT_FOR_SERIAL

// Define this to try to set clientIP via dhcp (otherwise configure manually)
//#define CONF_DHCP

//#ifndef CLIENT_IP_LAST_BYTE
//#define CLIENT_IP_LAST_BYTE 0x10
//#endif

// Shorthand to block and do nothing
#define WAIT_INFINITE() while (true) yield();

// Local udp port on which to receive packets.
const uint16_t LOCAL_UDP_PORT = 8888;
// For manual clientIP address configuration -- IP to assign to the ethernet shield.
IPAddress clientIP{192, 168, 10, 100};
// Remote server clientIP address -- should match address in IPv4 settings.
IPAddress serverIP{192, 168, 10, 10};

//region Audio system objects
// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

JackTripClient jtc{clientIP, serverIP};

PassThrough pt;
AudioMixer4 mixerL;
AudioMixer4 mixerR;

// Audio system connections
AudioConnection patchCord1(jtc, 0, pt, 0);
AudioConnection patchCord2(jtc, 0, out, 0);
AudioConnection patchCord3(jtc, 0, mixerL, 0);
AudioConnection patchCord4(jtc, 1, pt, 1);
AudioConnection patchCord5(jtc, 1, out, 1);
AudioConnection patchCord6(jtc, 1, mixerR, 0);
AudioConnection patchCord7(pt, 0, mixerL, 1);
AudioConnection patchCord8(pt, 1, mixerR, 1);
AudioConnection patchCord9(mixerL, 0, jtc, 0);
AudioConnection patchCord10(mixerR, 0, jtc, 1);
//endregion

//region Warning params
uint32_t last_perf_report = millis();
const uint32_t PERF_REPORT_INTERVAL = 5000;
//endregion

//region Forward declarations
void startAudio();
//endregion

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (!jtc.begin(LOCAL_UDP_PORT)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

    AudioMemory(32);

    startAudio();
}

void loop() {
    if (jtc.isConnected()) {
        if (millis() - last_perf_report > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            last_perf_report = millis();
        }
    } else {
        jtc.connect(2500);
        if (jtc.isConnected()) {
            startAudio();
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

    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
}
