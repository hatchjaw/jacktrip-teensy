#include <QNEthernet.h>
#include <Audio.h>
#include <OSCBundle.h>
#include <JackTripClient.h>
#include "WFS/WFS.h"

// Define this to wait for a serial connection before proceeding with execution
#define WAIT_FOR_SERIAL

// Define this to print packet stats.
#define SHOW_STATS

// Shorthand to block and do nothing
#define WAIT_INFINITE() while (true) yield();

// Local udp port on which to receive packets.
const uint16_t LOCAL_UDP_PORT = 8888;
// Remote server IP address -- should match address in IPv4 settings.
IPAddress jackTripServerIP{192, 168, 10, 10};
// Parameters for OSC over UDP multicast.
IPAddress oscMulticastIP{230, 0, 0, 20};
const uint16_t OSC_MULTICAST_PORT{41814};

//region Audio system objects
// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

JackTripClient jtc{jackTripServerIP};

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

namespace qn = qindesign::network;

qn::EthernetUDP udp;

//region Forward declarations
void startAudio();

void receiveOSC();

void parsePosition(OSCMessage &msg, int addrOffset);

void parseModule(OSCMessage &msg, int addrOffset);
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

#ifdef SHOW_STATS
    jtc.setShowStats(true, 5'000);
#endif

    if (!jtc.begin(LOCAL_UDP_PORT)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

    // NB. Ethernet::begin() already called by JackTripClient
    if (1 != udp.beginMulticast(oscMulticastIP, OSC_MULTICAST_PORT)) {
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
        receiveOSC();

        if (performanceReport > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            performanceReport = 0;
        }
    }
}

void parsePosition(OSCMessage &msg, int addrOffset) {
    // Get the source index and coordinate axis, e.g. "0/x"
    char path[20];
    msg.getAddress(path, addrOffset + 1);
    // Rough-and-ready check to prevent attempting to set an invalid source
    // position.
    auto sourceIdx{atoi(path)};
    Serial.println(sourceIdx);
    if (sourceIdx >= JackTripClient::getNumChannels()){
        Serial.printf("Invalid source index: %d\n", sourceIdx);
        return;
    }
    // Get the coordinate value (0-1).
    auto pos = msg.getFloat(0);
    Serial.printf("Setting \"%s\": %f\n", path, pos);
    // Set the parameter.
    wfs.setParamValue(path, pos);
}

void parseModule(OSCMessage &msg, int addrOffset){
    char ipString[15];
    IPAddress ip;
    msg.getString(0, ipString, 15);
    ip.fromString(ipString);
    if (ip == qn::Ethernet.localIP()){
        char id[2];
        msg.getAddress(id, addrOffset + 1);
        auto numericID = strtof(id, nullptr);
        Serial.printf("Setting module ID: %f\n", numericID);
        wfs.setParamValue("moduleID", numericID);
    }
}

/**
 * Expects messages of the form:
 *
 * set module ID
 * /module/0 "[IP address]"
 *
 * set source 0 co-ordinates
 * /source/0/x [0.0-1.0]
 * /source/0/y [0.0-1.0]
 */
void receiveOSC() {
    OSCBundle bundleIn;
    OSCMessage messageIn;
    int size;
    if ((size = udp.parsePacket()) > 0) {
//        Serial.printf("Packet size: %d\n", size);
        uint8_t buffer[size];
        udp.read(buffer, size);

        // Try to read as bundle
        bundleIn.fill(buffer, size);
        if (!bundleIn.hasError() && bundleIn.size() > 0) {
//            Serial.printf("OSCBundle::size: %d\n", bundleIn.size());

            bundleIn.route("/source", parsePosition);
            bundleIn.route("/module", parseModule);
        } else {
            // Try as message
            messageIn.fill(buffer, size);
            if (!messageIn.hasError() && messageIn.size() > 0) {
//                Serial.printf("OSCMessage::size: %d\n", messageIn.size());

                messageIn.route("/source", parsePosition);
                messageIn.route("/module", parseModule);
            }
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
