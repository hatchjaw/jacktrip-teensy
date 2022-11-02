#include <NativeEthernet.h>
#include <Audio.h>
#include <OSCBundle.h>
#include <JackTripClient.h>
#include "WFS/WFS.h"

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
// Remote server clientIP address -- should match address in IPv4 settings.
IPAddress serverIP{192, 168, 10, 10};

const uint16_t OSC_PORT = 5510;

//region Audio system objects
// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

JackTripClient jtc{serverIP};

EthernetUDP udp;

WFS wfs;
AudioMixer4 mixerL;
AudioMixer4 mixerR;

// Audio system connections
AudioConnection patchCord00(jtc, 0, wfs, 0);
//AudioConnection patchCord10(jtc, 0, pt, 0);
//AudioConnection patchCord20(jtc, 0, out, 0);
AudioConnection patchCord30(jtc, 0, mixerL, 0);

AudioConnection patchCord35(jtc, 1, wfs, 1);
//AudioConnection patchCord40(jtc, 1, pt, 1);
//AudioConnection patchCord50(jtc, 1, out, 1);
AudioConnection patchCord60(jtc, 1, mixerR, 0);

//AudioConnection patchCord70(pt, 0, mixerL, 1);
//AudioConnection patchCord80(pt, 1, mixerR, 1);

// WFS outputs routed to Teensy outputs
AudioConnection patchCord85(wfs, 0, out, 0);
AudioConnection patchCord86(wfs, 1, out, 1);

// Mixer outputs routed to JackTripClient inputs -- to be sent to the JackTrip
// server over UDP.
AudioConnection patchCord90(mixerL, 0, jtc, 0);
AudioConnection patchCord91(mixerR, 0, jtc, 1);
//endregion

//region Warning params
elapsedMillis performanceReport;
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

    if (!udp.begin(OSC_PORT)) {
        Serial.println("Failed to start listening for OSC messages.");
        WAIT_INFINITE()
    }

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
        if (performanceReport > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            performanceReport = 0;
        }
    }

    receiveOSC();
}

/**
 * Expects messages of the form
 * `oscsend osc.udp://192.168.10.[n]:[OSC_PORT] /wfs/pos/[p] i [pos]
 * see `man oscsend`, or set up OSC control in Reaper and send to this Teensy's
 * IP and OSC UDP port.
 */
void receiveOSC() {
    OSCBundle bundleIn;
    OSCMessage messageIn;
    int size;
    if ((size = udp.parsePacket())) {
//        Serial.printf("Packet size: %d\n", size);
        byte buffer[size];
        udp.read(buffer, size);

        // Try to read as bundle
        bundleIn.fill(buffer, size);
        if (!bundleIn.hasError() && bundleIn.size() > 0) {
//            Serial.printf("OSCBundle::size: %d\n", bundleIn.size());

            bundleIn.route("/wfs/pos", [](OSCMessage &msg, int addrOffset) {
                // Get the input index (last character of the address)
                char address[10], path[20];
                msg.getAddress(address, addrOffset + 1);
                snprintf(path, sizeof path, "pos%s", address);
                auto pos = msg.getFloat(0);
                Serial.printf("Setting \"%s\": %f\n", path, pos);
                wfs.setParamValue(path, pos);
            });
        } else {
            // Try as message
            messageIn.fill(buffer, size);
            if (!messageIn.hasError() && messageIn.size() > 0) {
//                Serial.printf("OSCMessage::size: %d\n", messageIn.size());

                messageIn.route("/wfs/pos", [](OSCMessage &msg, int addrOffset) {
                    // Get the input index (last character of the address)
                    char address[10], path[20];
                    msg.getAddress(address, addrOffset + 1);
                    snprintf(path, sizeof path, "pos%s", address);
                    auto pos = msg.getFloat(0);
                    Serial.printf("Setting \"%s\": %f\n", path, pos);
                    wfs.setParamValue(path, pos);
                });
            }
        }

//        while (size--) {
//            oscIn.fill(udp.read());
//        }

//        if (!oscIn.hasError()) {
//            Serial.printf("OSCBundle::size: %d\n", oscIn.size());
//            oscIn.route("/track", [](OSCMessage &msg, int addrOffset) {
//                char address[64];
//                msg.getAddress(address);
//                Serial.println(address);
//            });
//
//            oscIn.route("/wfs/pos", [](OSCMessage &msg, int addrOffset) {
//                // Get the input index (last character of the address)
//                char address[10], path[20];
//                msg.getAddress(address, addrOffset+1);
//                snprintf(path, sizeof path, "pos%s", address);
//                auto pos = msg.getInt(0);
//                Serial.printf("Setting \"%s\": %d\n", path, pos);
//                wfs.setParamValue(path, pos);
//            });
//        } else {
//            Serial.printf("OSC reported an error with code %d\n", oscIn.getError());
//        }
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
