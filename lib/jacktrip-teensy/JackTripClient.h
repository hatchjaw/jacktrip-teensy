//
// Created by Tommy Rushton on 16/09/22.
//

#ifndef JACKTRIP_TEENSY_JACKTRIPCLIENT_H
#define JACKTRIP_TEENSY_JACKTRIPCLIENT_H

#include <Audio.h>
#include <NativeEthernet.h>
#include <TeensyID.h>
#include <TeensyTimerTool.h>
#include "PacketHeader.h"
#include "CircularBuffer.h"

#undef CONF_DHCP

#define DO_DIAGNOSTICS

/**
 * Inputs: signals produced by other audio components, to be sent to peers over
 *   the JackTrip protocol to do with as they will.
 * Outputs: audio signals received over the JackTrip protocol.
 */
class JackTripClient : public AudioStream, EthernetUDP {
public:
    JackTripClient(IPAddress &clientIpAddress, IPAddress &serverIpAddress);

    /**
     * Set up the client.
     * @param port local UDP port on which to listen for packets.
     * @return <em>true</em> on success, <em>false</em> on failure.
     */
    uint8_t begin(uint16_t port) override;

    bool isConnected() const;

    /**
     * Connect the client to the server.
     * @param timeout
     * @return <em>true</em> on success, <em>false</em> on failure.
     */
    bool connect(uint16_t timeout = 1000);

    void stop() override;

private:
    static constexpr uint8_t NUM_CHANNELS{2};
    static constexpr uint16_t UDP_PACKET_SIZE{PACKET_HEADER_SIZE + NUM_CHANNELS * AUDIO_BLOCK_SAMPLES * sizeof(uint16_t)};
    static constexpr uint32_t RECEIVE_TIMEOUT_MS{5000};
    static constexpr uint32_t DIAGNOSTIC_PRINT_INTERVAL{5000};
    static constexpr uint16_t AUDIO_BUFFER_SIZE{AUDIO_BLOCK_SAMPLES * NUM_CHANNELS * 2};
    /**
     * Size in bytes of one channel's worth of samples.
     */
    static constexpr uint8_t CHANNEL_FRAME_SIZE{AUDIO_BLOCK_SAMPLES * sizeof(uint16_t)};

    /**
     * Remote server tcp port for initial handshake.
     */
    const uint16_t REMOTE_TCP_PORT{4464};
    /**
     * Size, in bytes, of JackTrip's exit packet
     */
    static constexpr uint8_t EXIT_PACKET_SIZE{63};

    /**
     * Attempt to establish an ethernet connection.
     * @return Connection status
     */
    EthernetLinkStatus startEthernet();

    /**
     * "The heart of your object is it's update() function.
     * The library will call your update function every 128 samples,
     * or approximately every 2.9 milliseconds."
     */
    void update(void) override;

    /**
     * Receive a JackTrip packet containing audio to route to this object's
     * outputs.
     * NB assumes that a new packet is ready each time it is called. This may
     * well be a dangerous assumption.
     */
    void receivePackets();

    /**
     * Check whether a packet received from the JackTrip server is an exit
     * packet.
     * @return
     */
    bool isExitPacket();

    /**
     * Send a JackTrip packet containing audio routed to this object's inputs.
     */
    void sendPackets();

    /**
     * Copy audio samples from incoming UDP data to Teensy audio output.
     */
    void doAudioOutput();

    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte clientMAC[6]{};
    /**
     * IP to assign to Teensy.
     */
    IPAddress clientIP;
    /**
     * IP of the jacktrip server.
     */
    IPAddress serverIP;

    /**
     * UDP port of the jacktrip server.
     */
    uint32_t serverUdpPort{0};

    /*volatile*/ bool connected{false};

    elapsedMillis lastReceive{0};

    /**
     * UDP packet buffer (in/out)
     */
    uint8_t buffer[UDP_PACKET_SIZE]{};

    /**
     * "The final required component is inputQueueArray[], which should be a
     * private variable.
     * The size must match the number passed to the AudioStream constructor."
     */
    audio_block_t *inputQueueArray[2]{};
    /**
     * The header to send with every outgoing JackTrip packet.
     * TimeStamp and SeqNumber should be incremented accordingly.
     */
    JackTripPacketHeader packetHeader{
            0,
            0,
            // Teensy's default block size is 128. Can be overridden with
            // compiler flag -DAUDIO_BLOCK_SAMPLES (see platformio.ini).
            AUDIO_BLOCK_SAMPLES,
            samplingRateT::SR44,
            16,
            NUM_CHANNELS,
            NUM_CHANNELS
    };

    bool awaitingFirstPacket{true};

    int initialDiagnostic{0};
    elapsedMillis diagnosticElapsed{0};
    elapsedMicros packetInterval{0};
    JackTripPacketHeader prevServerHeader{};
    JackTripPacketHeader *serverHeader;

    TeensyTimerTool::PeriodicTimer timer;

    void timerCallback();

    CircularBuffer<uint8_t> udpBuffer;
    CircularBuffer<int16_t> audioBuffer;
};


#endif //JACKTRIP_TEENSY_JACKTRIPCLIENT_H
