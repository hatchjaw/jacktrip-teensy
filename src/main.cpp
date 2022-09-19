#include <NativeEthernet.h>
#include "PacketHeader.h"
#include <Audio.h>
#include <OSCBundle.h>
#include "faust/SpringGrain/SpringGrain.h"
#include "JackTripClient.h"

//region Network parameters
// This MAC address is arbitrarily assigned to the ethernet shield.
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE0};
// For manual ip address configuration -- IP to assign to the ethernet shield.
IPAddress ip{192, 168, 1, 20};
// Remote server ip address -- should match address in IPv4 settings.
IPAddress peerAddress{192, 168, 1, 66};
// A flag to indicate whether Teensy is connected to the jacktrip server.
bool connected{false};
// Remote server tcp port
const uint16_t REMOTE_TCP_PORT = 4464;
// Local udp port to receive packets on
const uint16_t LOCAL_UDP_PORT = 8888;

// Set this to use the ip address from the dhcp
// #define CONF_DHCP
// Set this to manually configure the ip address
#define CONF_MANUAL

// Set this to wait for a serial connection before proceeding with the execution
#define WAIT_FOR_SERIAL

//endregion

//region Audio parameters
const uint8_t NUM_CHANNELS = 2;
// Can't use 2 channels and 512 samples due to a bug in the udp implementation (can't send packets bigger than 2048)
const uint16_t NUM_SAMPLES = 256;
// Number of buffers per channel
const uint8_t NUM_BUFFERS = NUM_SAMPLES / AUDIO_BLOCK_SAMPLES;
//endregion

//region Faust parameter names
const std::string GRAIN_SIZE = "Grain length (s)";
const std::string GRAIN_DENSITY = "Grain density";
const std::string GRAIN_SPEED = "Grain speed";
const std::string GRAIN_REGULARITY = "Rhythm";
const std::string FREEZE = "Freeze";
//endregion

// The UDP socket we use
EthernetUDP Udp;

// UDP remote port to send audio packets to
uint16_t remote_udp_port;

//region Audio system objects
// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

JackTripClient jtc{ip, peerAddress};

// Audio circular buffers
AudioPlayQueue pql;
AudioPlayQueue pqr;
AudioRecordQueue rql;
AudioRecordQueue rqr;

SpringGrain sg;
AudioMixer4 mixerL;
AudioMixer4 mixerR;

// Audio system connections
//// Play queue L (jacktrip in) routed to mixer L, granulator in L, and teensy out L.
//AudioConnection patchCord1(pql, 0, mixerL, 0);
//AudioConnection patchCord2(pql, 0, sg, 0);
//AudioConnection patchCord3(pql, 0, out, 0);
//// Play queue R (jacktrip in) routed to mixer R, granulator in R, and teensy out R.
//AudioConnection patchCord4(pqr, 0, mixerR, 0);
//AudioConnection patchCord5(pqr, 0, sg, 1);
//AudioConnection patchCord6(pqr, 0, out, 1);
//// Granulator outs routed to mixers
//AudioConnection patchCord7(sg, 0, mixerL, 1);
//AudioConnection patchCord8(sg, 1, mixerR, 1);
//// Mixer outs routed to record queues (jacktrip out).
//AudioConnection patchCord9(mixerL, rql);
//AudioConnection patchCord10(mixerR, rqr);

// JackTripClient output 1 (jacktrip in) routed to mixer L, granulator in L, and teensy out L.
AudioConnection patchCord11(jtc, 0, mixerL, 0);
AudioConnection patchCord12(jtc, 0, sg, 0);
AudioConnection patchCord13(jtc, 0, out, 0);
// JackTripClient output 2 (jacktrip in) routed to mixer R, granulator in R, and teensy out R.
AudioConnection patchCord14(jtc, 1, mixerR, 0);
AudioConnection patchCord15(jtc, 1, sg, 1);
AudioConnection patchCord16(jtc, 1, out, 1);
// Granulator outs routed to mixers
AudioConnection patchCord17(sg, 0, mixerL, 1);
AudioConnection patchCord18(sg, 1, mixerR, 1);
// Mixer outs routed to JackTripClient inputs (jacktrip out).
AudioConnection patchCord19(mixerL, 0, jtc, 0);
AudioConnection patchCord20(mixerR, 0, jtc, 1);
//endregion

// Shorthand to block and do nothing
#define WAIT_INFINITE() while (true) yield();

// Size of the jacktrip packets
const uint32_t BUFFER_SIZE = PACKET_HEADER_SIZE + NUM_CHANNELS * NUM_SAMPLES * 2;
// jacktrip exit packet size
const uint32_t EXIT_PACKET_SIZE = 63;
// Packet buffer (in/out)
uint8_t buffer[BUFFER_SIZE];

uint16_t seq = 0;
JacktripPacketHeader HEADER = JacktripPacketHeader{
        0,
        0,
        NUM_SAMPLES,
        samplingRateT::SR44,
        16,
        NUM_CHANNELS,
        NUM_CHANNELS};

//region Warning params
uint32_t last_receive = millis();
uint32_t last_perf_report = millis();
const uint32_t PERF_REPORT_DELAY = 3000;
//endregion

//region Forward declarations
bool containsOnly(const uint8_t *bufferToCheck, uint8_t value, size_t length);

void attemptJacktripConnection(uint16_t timeout = 1000);

void closeJacktripConnection();

void stopAudio();

EthernetLinkStatus startEthernet();

void startAudio();
//endregion

void setup() {
    // Open serial communications
    Serial.begin(0);

#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (!jtc.begin(LOCAL_UDP_PORT)) {
        Serial.println("Failed to initialise jacktrip client.");
        WAIT_INFINITE()
    }

//    auto status = startEthernet();
//    if (status != LinkON) {
//        WAIT_INFINITE()
//    }
//
//    Serial.print("My ip is ");
//    Ethernet.localIP().printTo(Serial);
//    Serial.println();
//
//    Serial.printf("Packet size is %d bytes\n", BUFFER_SIZE);

    // Init ntp client
    // audio packets timestamps don't seem to be necessary
    /*
    ntp::begin();
    setSyncProvider(ntp::getTime);

    if (timeStatus() == timeNotSet)
        Serial.println("Can't sync time");

    uint64_t usec = now();
    Serial.printf("%lu secs\n", usec);
     */

    // + 10 for the audio input
    AudioMemory(NUM_BUFFERS * NUM_CHANNELS + 2 + 20);
    pql.setMaxBuffers(NUM_BUFFERS);
    pqr.setMaxBuffers(NUM_BUFFERS);
    Serial.printf("Allocated %d buffers\n", NUM_BUFFERS * NUM_CHANNELS + 2 + 20);

    // Granular engine parameters
    sg.setParamValue(GRAIN_SPEED, -.85);
    sg.setParamValue(GRAIN_DENSITY, 4.5);
    sg.setParamValue(GRAIN_SIZE, .045);

    startAudio();
}

void loop() {
    // TODO read osc parameters
    // TODO filter audio before transmitting

    if (jtc.isConnected()) {
        if (millis() - last_perf_report > PERF_REPORT_DELAY) {
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


    if (false) {
        // Send audio when there are enough samples to fill a packet
        while (rql.available() >= NUM_BUFFERS &&
               (NUM_CHANNELS == 1 || (NUM_CHANNELS > 1 && rqr.available() >= NUM_BUFFERS))) {
            HEADER.TimeStamp = seq;
            HEADER.SeqNumber = seq;
            memcpy(buffer, &HEADER, PACKET_HEADER_SIZE);
            uint8_t *pos = buffer + PACKET_HEADER_SIZE;

            for (int i = 0; i < NUM_BUFFERS; i++) {
                memcpy(pos, rql.readBuffer(),
                       AUDIO_BLOCK_SAMPLES * 2);
                rql.freeBuffer();
                pos += AUDIO_BLOCK_SAMPLES * 2;
            }

            if (NUM_CHANNELS > 1) {
                for (int i = 0; i < NUM_BUFFERS; i++) {
                    memcpy(pos, rqr.readBuffer(), AUDIO_BLOCK_SAMPLES * 2);
                    rqr.freeBuffer();
                    pos += AUDIO_BLOCK_SAMPLES * 2;
                }
            }

            Udp.beginPacket(peerAddress, remote_udp_port);
            size_t written = Udp.write(buffer, BUFFER_SIZE);
            if (written != BUFFER_SIZE) {
                Serial.println("Net buffer is too small");
            }
            Udp.endPacket();

            /*
            if (rql.available() > 0) {
                Serial.printf("Running late behind (%d buffers)\n", rql.available());
            }*/

            seq += 1;
        }

        int size;
        // Nonblocking
        if ((size = Udp.parsePacket())) {
            last_receive = millis();

            if (size == EXIT_PACKET_SIZE
                && Udp.read(buffer, EXIT_PACKET_SIZE) == EXIT_PACKET_SIZE
                && containsOnly(buffer, 0xff, EXIT_PACKET_SIZE)) {
                // Exit sequence
                Serial.println("Received exit packet");
                Serial.println("Perf statistics");
                Serial.printf("  maxmem: %d blocks\n", AudioMemoryUsageMax());
                Serial.printf("  maxcpu: %f %%\n", AudioProcessorUsageMax());

                closeJacktripConnection();
                stopAudio();
            } else if (size != BUFFER_SIZE) {
                Serial.println("Received a malformed packet");
            } else {
                // We received an audio packet
//            Serial.println("Received packet");
                Udp.read(buffer, BUFFER_SIZE);
                uint32_t remaining = pql.play((const int16_t *) (buffer + PACKET_HEADER_SIZE), NUM_SAMPLES);

                if (remaining > 0) {
                    Serial.printf("%d samples dropped (L)", remaining);
                }

                if (NUM_CHANNELS > 1) {
                    remaining = pqr.play((const int16_t *) (buffer + PACKET_HEADER_SIZE + NUM_SAMPLES * 2),
                                         NUM_SAMPLES);
                    if (remaining > 0) {
                        Serial.printf("%d samples dropped (R)", remaining);
                    }
                }
            }
        }

        if (millis() - last_receive > 1000) {
            Serial.println("Nothing received for 1 second");
            last_receive = millis();
        }

        if (millis() - last_perf_report > PERF_REPORT_DELAY) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            last_perf_report = millis();
        }
    } else {
//        attemptJacktripConnection(2500);
//        if (connected) {
//            startAudio();
//        }
    }
}

EthernetLinkStatus startEthernet() {
    Ethernet.setSocketNum(4);
    if (BUFFER_SIZE > FNET_SOCKET_DEFAULT_SIZE) {
        Ethernet.setSocketSize(BUFFER_SIZE);
    }

#ifdef CONF_DHCP
    bool dhcpFailed = false;
    // start the Ethernet
    if (!Ethernet.begin(clientMAC)) {
        dhcpFailed = true;
    }
#endif

#ifdef CONF_MANUAL
    Ethernet.begin(mac, ip);
#endif

    if (Ethernet.linkStatus() != LinkON) {
        Serial.println("Ethernet cable is not connected.");
    } else {
        Serial.println("Ethernet connected.");
    }

#ifdef CONF_DHCP
    if (dhcpFailed) {
        Serial.println("DHCP conf failed");
        WAIT_INFINITE();
    }
#endif

    return Ethernet.linkStatus();
}

void startAudio() {
    audioShield.enable();
    audioShield.volume(0.5);
    audioShield.micGain(20);
    audioShield.inputSelect(AUDIO_INPUT_MIC);
    //audioShield.headphoneSelect(AUDIO_HEADPHONE_DAC);

    rql.begin();
    if (NUM_CHANNELS > 1) {
        rqr.begin();
    }

    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
}

void stopAudio() {
    rql.end();
    if (NUM_CHANNELS > 1) {
        rqr.end();
    }
    audioShield.disable();
}

void attemptJacktripConnection(uint16_t timeout) {
    // Query jacktrip udp port
    Serial.print("Attempting to connect to jacktrip server at ");
    peerAddress.printTo(Serial);
    Serial.printf(":%d... ", REMOTE_TCP_PORT);

    EthernetClient c = EthernetClient();
    c.setConnectionTimeout(timeout);
    if (c.connect(peerAddress, REMOTE_TCP_PORT)) {
        Serial.println("Succeeded!");
        connected = true;
    } else {
        Serial.println("Failed.");
        return;
    }

    uint32_t port = LOCAL_UDP_PORT;
    // port is sent little endian
    c.write((const uint8_t *) &port, 4);

    while (c.available() < 4) {}
    c.read((uint8_t *) &port, 4);
    remote_udp_port = port;
    Serial.printf("Remote port is %d\n", remote_udp_port);

    // start UDP
    Udp.begin(LOCAL_UDP_PORT);
}

void closeJacktripConnection() {
    Udp.stop();
    connected = false;
}

/**
 * Check that a buffer contains entries of only one value.
 * @param bufferToCheck
 * @param value
 * @param length
 * @return
 */
bool containsOnly(const uint8_t *bufferToCheck, uint8_t value, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (bufferToCheck[i] != value) {
            return false;
        }
    }
    return true;
}
