//
// Created by Tommy Rushton on 16/09/22.
//

#ifndef JACKTRIP_TEENSY_JACKTRIPCLIENT_H
#define JACKTRIP_TEENSY_JACKTRIPCLIENT_H

#include <Audio.h>
#include <NativeEthernet.h>
#include "PacketHeader.h"
//#include <PacketHeader.h> // Might be nice to include this from jacktrip

#define CONF_MANUAL

/**
 * Inputs: signals produced by other audio components, to be sent to peers over
 *   the JackTrip protocol to do with as they will.
 * Outputs: audio signals received over the JackTrip protocol,
 */
class JackTripClient : public AudioStream, EthernetUDP {
public:
    JackTripClient(IPAddress &clientIpAddress, IPAddress &serverIpAddress);

    /**
     * Set up the client.
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
    static const uint8_t NUM_CHANNELS{2};
    // "Can't use 2 channels and 512 samples due to a bug in the udp
    // implementation (can't send packets bigger than 2048)"
    // TODO: check the above, and ideally derive the below from the JackTrip server.
    // Using 128 because Teensy's AudioStream::update() is called every 128
    // samples. Stored as a 16-bit value to match the type expected by
    // JackTripPacketHeader
    static const uint16_t NUM_SAMPLES{128};
    static const uint32_t UDP_BUFFER_SIZE{PACKET_HEADER_SIZE + NUM_CHANNELS * NUM_SAMPLES * 2};

    /**
     * Remote server tcp port for initial handshake.
     */
    const uint16_t REMOTE_TCP_PORT{4464};
    /**
     * Size, in bytes, of JackTrip's exit packet
     */
    const uint8_t EXIT_PACKET_SIZE{63};

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
    void receivePacket();

    /**
     * Check whether a packet received from the JackTrip server is an exit
     * packet.
     * @return
     */
    bool isExitPacket();

    /**
     * Send a JackTrip packet containing audio routed to this object's inputs.
     */
    void sendPacket();

    /**
     * MAC address to assign to Teensy's ethernet shield.
     * TODO: Might not work for multiple clients?..
     */
    byte clientMAC[6]{0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE0};
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

    bool connected{false};

    uint32_t lastReceive{0};
    /**
     * UDP packet buffer (in/out)
     */
    uint8_t buffer[UDP_BUFFER_SIZE]{};

    /**
     * "The final required component is inputQueueArray[], which should be a
     * private variable.
     * The size must match the number passed to the AudioStream constructor."
     * TODO: maybe don't need this?
     */
    audio_block_t *inputQueueArray[2]{};
    /**
     * The header to send with every outgoing JackTrip packet.
     * TimeStamp and SeqNumber should be incremented accordingly.
     */
    JackTripPacketHeader packetHeader{
            0,
            0,
            NUM_SAMPLES,
            samplingRateT::SR44,
            16,
            NUM_CHANNELS,
            NUM_CHANNELS
    };
};


#endif //JACKTRIP_TEENSY_JACKTRIPCLIENT_H
