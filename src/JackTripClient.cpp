//
// Created by Tommy Rushton on 16/09/22.
//

#include "JackTripClient.h"

JackTripClient::JackTripClient(IPAddress &clientIpAddress, IPAddress &serverIpAddress) :
        AudioStream(2, inputQueueArray),
        clientIP(clientIpAddress),
        serverIP(serverIpAddress) {}

uint8_t JackTripClient::begin(uint16_t port) {
    if (LinkON != startEthernet()) {
        return 0;
    }

    Serial.print("JackTripClient: IP is ");
    Ethernet.localIP().printTo(Serial);
    Serial.println();

    Serial.printf("JackTripClient: Packet size is %d bytes\n", UDP_BUFFER_SIZE);

    return EthernetUDP::begin(port);
}

EthernetLinkStatus JackTripClient::startEthernet() {
    Ethernet.setSocketNum(4);

    if (UDP_BUFFER_SIZE > FNET_SOCKET_DEFAULT_SIZE) {
        Ethernet.setSocketSize(UDP_BUFFER_SIZE);
    }

#ifdef CONF_DHCP
    bool dhcpFailed = false;
// start the Ethernet
if (!Ethernet.begin(clientMAC)) {
    dhcpFailed = true;
}
#endif

#ifdef CONF_MANUAL
    Ethernet.begin(clientMAC, clientIP);
#endif

    if (Ethernet.linkStatus() != LinkON) {
        Serial.println("JackTripClient: Ethernet cable is not connected.");
    } else {
        Serial.println("JackTripClient: Ethernet connected.");
    }

#ifdef CONF_DHCP
    if (dhcpFailed) {
    Serial.println("DHCP conf failed");
    WAIT_INFINITE();
}
#endif

    return Ethernet.linkStatus();
}

bool JackTripClient::connect(uint16_t timeout) {
    // Attempt handshake with JackTrip server via TCP port.
    Serial.print("JackTripClient: Connecting to JackTrip server at ");
    serverIP.printTo(Serial);
    Serial.printf(":%d... ", REMOTE_TCP_PORT);

    EthernetClient c = EthernetClient();
    c.setConnectionTimeout(timeout);
    if (c.connect(serverIP, REMOTE_TCP_PORT)) {
        Serial.println("Succeeded!");
        connected = true;
    } else {
        Serial.println("Failed.");
        return false;
    }

    // TODO: check value/type of port
    auto port = localPort();
    // port is sent little endian
    c.write((const uint8_t *) &port, 4);

    while (c.available() < 4) {}
    c.read((uint8_t *) &port, 4);
    serverUdpPort = port;
    Serial.printf("JackTripClient: Server port is %d\n", serverUdpPort);

    // TODO: better connection success check
    return connected;// && 1 == udp.begin(LOCAL_UDP_PORT);
}

void JackTripClient::stop() {
    EthernetUDP::stop();
    connected = false;
}

void JackTripClient::update(void) {
    if (connected) {
        sendPacket();
        receivePacket();
    }
}

bool JackTripClient::isConnected() const {
    return connected;
}

void JackTripClient::receivePacket() {
    int size;
    // Nonblocking
    if ((size = parsePacket())) {
        lastReceive = millis();

        if (size == EXIT_PACKET_SIZE && isExitPacket()) {
            // Exit sequence
            Serial.println("JackTripClient: Received exit packet");

            stop();
        } else if (size != UDP_BUFFER_SIZE) {
            Serial.println("JackTripClient: Received a malformed packet");
        } else {
//            Serial.println("JackTripClient: Received audio packet");
            this->read(buffer, UDP_BUFFER_SIZE);

            // Size in memory of one channel's worth of samples.
            auto channelFrameSize = NUM_SAMPLES * sizeof(uint16_t);

            // Write samples to output.
            audio_block_t *outBlock[NUM_CHANNELS];
            for (int channel = 0; channel < NUM_CHANNELS; channel++) {
                outBlock[channel] = allocate();
                // Get the start of the sample data for the current channel.
                auto start = (const int16_t *) (buffer + PACKET_HEADER_SIZE + channelFrameSize * channel);
                // Copy the samples to the appropriate channel of the output block.
                memcpy(outBlock[channel]->data, start, channelFrameSize);
                // Finish up.
                transmit(outBlock[channel], channel);
                release(outBlock[channel]);
            }
        }
    }

    if (millis() - lastReceive > 1000) {
        Serial.println("JackTripClient: Nothing received for 1 second");
        lastReceive = millis();
    }
}

bool JackTripClient::isExitPacket() {
    if (this->read(buffer, EXIT_PACKET_SIZE) == EXIT_PACKET_SIZE) {
        for (size_t i = 0; i < EXIT_PACKET_SIZE; ++i) {
            if (buffer[i] != 0xff) {
                return false;
            }
        }
        return true;
    }
    return false;
}

void JackTripClient::sendPacket() {
    // Copy the packet header to the UDP buffer.
    memcpy(buffer, &packetHeader, PACKET_HEADER_SIZE);
    uint8_t *pos = buffer + PACKET_HEADER_SIZE;
    // Size in memory of one channel's worth of samples.
    auto channelFrameSize = AUDIO_BLOCK_SAMPLES * sizeof(uint16_t);

    // Copy audio to the buffer.
    audio_block_t *inBlock[NUM_CHANNELS];
    for (int channel = 0; channel < NUM_CHANNELS; channel++) {
        inBlock[channel] = receiveReadOnly(channel);
        memcpy(pos, inBlock[channel]->data, channelFrameSize);
        pos += channelFrameSize;
        release(inBlock[channel]);
    }

    // Send the packet.
    this->beginPacket(serverIP, serverUdpPort);
    size_t written = this->write(buffer, UDP_BUFFER_SIZE);
    if (written != UDP_BUFFER_SIZE) {
        Serial.println("JackTripClient: Net buffer is too small");
    }
    this->endPacket();

    packetHeader.TimeStamp += 1;
    packetHeader.SeqNumber += 1;
}
