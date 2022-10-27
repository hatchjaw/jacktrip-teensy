#include <NativeEthernet.h>
#include <Audio.h>
#include "faust/PassThrough/PassThrough.h"
#include <JackTripClient.h>
//#include <OSCBundle.h>

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

const uint16_t OSC_PORT = 5510;

//region Audio system objects
// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

JackTripClient jtc{clientIP, serverIP};

//EthernetUDP udp;

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

void receiveOSC();
//endregion

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (!jtc.begin(LOCAL_UDP_PORT)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

//    if (!udp.begin(OSC_PORT)) {
//        Serial.println("Failed to start listening for OSC messages.");
//        WAIT_INFINITE()
//    }

    AudioMemory(32);

    startAudio();
}

void loop() {
    if (!jtc.isConnected()) {
        jtc.connect(2500);
        if (jtc.isConnected()) {
            AudioProcessorUsageMaxReset();
            AudioMemoryUsageMaxReset();
        }
    } else {
        if (millis() - last_perf_report > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            last_perf_report = millis();
        }
    }

//    receiveOSC();
}

//void routeTest(OSCMessage &msg, int offset) {
//    Serial.println("!");
//}

//void receiveOSC() {
//    OSCMessage oscIn;
//    int size;
//    if ((size = udp.parsePacket())) {
//        Serial.printf("Packet size: %d\n", size);
//        while (size--) {
//            oscIn.fill(udp.read());
//        }
//
//        if (!oscIn.hasError()) {
//            Serial.printf("OSCMessage::dataCount: %d\n", oscIn.size());
//            oscIn.route("/test", [](OSCMessage &msg, int addrOffset) {
//                Serial.println("!");
//            });
//        } else {
//            Serial.printf("OSC reported an error with code %d\n", oscIn.getError());
//        }
//    }
//}

void startAudio() {
//    audioShield.enable(16000000, 4096.0l * AUDIO_SAMPLE_RATE_EXACT);
    audioShield.enable();
    // "...0.8 corresponds to the maximum undistorted output for a full scale
    // signal. Usually 0.5 is a comfortable listening level."
    // https://www.pjrc.com/teensy/gui/?info=AudioControlSGTL5000
    audioShield.volume(.8);

    audioShield.audioProcessorDisable();
    audioShield.autoVolumeDisable();
    audioShield.dacVolumeRampDisable();
}
