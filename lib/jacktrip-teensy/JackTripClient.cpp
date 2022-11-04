//
// Created by Tommy Rushton on 16/09/22.
//

#include "JackTripClient.h"

JackTripClient::JackTripClient(IPAddress &serverIpAddress, uint16_t serverTcpPort) :
        AudioStream(2, inputQueueArray),
        clientIP(serverIpAddress),
        serverIP(serverIpAddress), // Assume client and server on same subnet
        serverTcpPort(serverTcpPort),
        timer(TeensyTimerTool::GPT1),
        udpBuffer(UDP_PACKET_SIZE * 4)
{
    // Generate a MAC address (from the program-once area of Teensy's flash
    // memory) to assign to the ethernet shield.
    teensyMAC(clientMAC);
    // Use the last byte of the MAC to set the last byte of the IP.
    // (Maybe needs a more sophisticated approach.)
    clientIP[3] += clientMAC[5];

    serverHeader = new JackTripPacketHeader;
}

uint8_t JackTripClient::begin(uint16_t port) {
    if (!active) {
        Serial.println("JackTripClient is not connected to any Teensy audio objects.");
        return 0;
    }

    if (LinkON != startEthernet()) {
        Serial.println("JackTripClient: failed to start ethernet connection.");
        return 0;
    }

    Serial.print("JackTripClient: IP is ");
    Serial.println(EthernetClass::localIP());

    Serial.printf("JackTripClient: Packet size is %d bytes\n", UDP_PACKET_SIZE);

#ifdef USE_TIMER
    auto timerPeriod = 1'000'000.f * static_cast<float>(AUDIO_BLOCK_SAMPLES) / AUDIO_SAMPLE_RATE_EXACT;
    Serial.printf("UDP send/receive timer period: %.3f\n", timerPeriod);
    timer.begin([this] { updateImpl(); }, timerPeriod);
#endif

    return EthernetUDP::begin(port);
}

EthernetLinkStatus JackTripClient::startEthernet() {
//    EthernetClass::setSocketNum(4);

    if (UDP_PACKET_SIZE > FNET_SOCKET_DEFAULT_SIZE) {
        EthernetClass::setSocketSize(UDP_PACKET_SIZE);
    }

    Serial.print("JackTripClient: MAC address is: ");
    for (int i = 0; i < 6; ++i) {
        Serial.printf(i < 5 ? "%02X:" : "%02X", clientMAC[i]);
    }
    Serial.println();

    EthernetClass::begin(clientMAC, clientIP);

    if (EthernetClass::linkStatus() != LinkON) {
        Serial.println("JackTripClient: Ethernet cable is not connected.");
    } else {
        Serial.println("JackTripClient: Ethernet connected.");
    }

    return EthernetClass::linkStatus();
}

bool JackTripClient::connect(uint16_t timeout) {
    if (!active) {
        Serial.println("JackTripClient is not connected to any Teensy audio objects.");
        return false;
    }

    // Attempt TCP handshake with JackTrip server.
    Serial.print("JackTripClient: Connecting to JackTrip server at ");
    Serial.print(serverIP);
    Serial.printf(":%d... ", serverTcpPort);

    EthernetClient c = EthernetClient();
    c.setConnectionTimeout(timeout);
    if (c.connect(serverIP, serverTcpPort)) {
        Serial.println("Succeeded!");
    } else {
        Serial.println();
        return false;
    }

    // Sending the local UDP port yields the remote UDP port in return.
    auto port = localPort();
    // Send the local port (little endian).
    if (4 != c.write((const uint8_t *) &port, 4)) {
        Serial.println("JackTripClient: failed to send UDP port to server.");
        c.close();
        return false;
    }
    // Patience...
    while (c.available() < 4) {}
    // Read the remote port.
    if (4 != c.read((uint8_t *) &port, 4)) {
        Serial.println("JackTripClient: failed to read UDP port from server.");
        c.close();
        connected = false;
        return false;
    } else {
        connected = true;
    }

    serverUdpPort = port;
    Serial.printf("JackTripClient: Server port is %d\n", serverUdpPort);

    lastReceive = 0;
    packetHeader.SeqNumber = 0;
    packetHeader.TimeStamp = 0;
    prevServerHeader.TimeStamp = 0;
    prevServerHeader.SeqNumber = 0;
    return connected;
}

void JackTripClient::stop() {
    connected = false;
    serverUdpPort = 0;
    udpBuffer.clear();
    packetStats.reset();
}

void JackTripClient::update(void) {
    updateImpl();
}

void JackTripClient::updateImpl() {
#ifndef USE_TIMER
    receivePackets();
#endif
    doAudioOutput();
#ifndef USE_TIMER
    sendPacket();

    if (showStats && connected) {
        packetStats.printStats();
        udpBuffer.printStats();
    }
#endif
}

bool JackTripClient::isConnected() const {
    return connected;
}

void JackTripClient::receivePackets() {
    if (!connected) return;

    int size;

    // Check for incoming UDP packets. Get as many packets as are available.
    while ((size = parsePacket())) {
        lastReceive = 0;

        if (size == EXIT_PACKET_SIZE && isExitPacket()) {
            // Exit sequence
            Serial.println("JackTripClient: Received exit packet");
            Serial.printf("  maxmem: %d blocks\n", AudioMemoryUsageMax());
            Serial.printf("  maxcpu: %f %%\n\n", AudioProcessorUsageMax());

            stop();
            return;
        } else if (size != UDP_PACKET_SIZE) {
            Serial.println("JackTripClient: Received a malformed packet");
        } else {
            // Read the UDP packet and write it into a circular buffer.
            uint8_t in[size];
            read(in, size);
            udpBuffer.write(in, size);

            // Read the header from the packet received from the server.
//            serverHeader = reinterpret_cast<JackTripPacketHeader *>(in);
            memcpy(serverHeader, in, sizeof(JackTripPacketHeader));

            if (showStats && packetStats.awaitingFirstReceive()) {
                Serial.println("===============================================================");
                Serial.printf("Received first packet: Timestamp: %" PRIu64 "; SeqNumber: %" PRIu16 "\n",
                              serverHeader->TimeStamp,
                              serverHeader->SeqNumber);
                packetHeader.TimeStamp = serverHeader->TimeStamp;
                packetHeader.SeqNumber = serverHeader->SeqNumber;
                Serial.println("===============================================================");
            }

            packetStats.registerReceive(*serverHeader);
        }
    }

    if (lastReceive > RECEIVE_TIMEOUT_MS) {
        Serial.printf("JackTripClient: Nothing received for %.1f s. Stopping.\n", RECEIVE_TIMEOUT_MS / 1000.f);
        stop();
    }
}

void JackTripClient::sendPacket() {
    // Might have received an exit packet, so check whether still connected.
    if (!connected) return;

    // Get the location in the UDP buffer to which audio samples should be
    // written.
    uint8_t packet[UDP_PACKET_SIZE];
    uint8_t *pos = packet + PACKET_HEADER_SIZE;

    // Copy audio to the UDP buffer.
    audio_block_t *inBlock[NUM_CHANNELS];
    for (int channel = 0; channel < NUM_CHANNELS; channel++) {
        inBlock[channel] = receiveReadOnly(channel);
        // Only proceed if a block was returned, i.e. something is connected
        // to one of this object's input channels.
        if (inBlock[channel]) {
            memcpy(pos, inBlock[channel]->data, CHANNEL_FRAME_SIZE);
            pos += CHANNEL_FRAME_SIZE;
            release(inBlock[channel]);
        }
    }

    packetHeader.SeqNumber++;
    packetHeader.TimeStamp += packetInterval;
    packetInterval = 0;

    // Copy the packet header to the UDP buffer.
    memcpy(packet, &packetHeader, PACKET_HEADER_SIZE);

    // Send the packet.
    beginPacket(serverIP, serverUdpPort);
    size_t written = write(packet, UDP_PACKET_SIZE);
    if (written != UDP_PACKET_SIZE) {
        Serial.println("JackTripClient: Net buffer is too small");
    }
    auto result = endPacket();
    if (0 == result) {
        Serial.println("JackTripClient: failed to send a packet.");
    }

    packetStats.registerSend(packetHeader);
}

void JackTripClient::doAudioOutput() {
    if (!connected) return;

    // Copy from UDP inBuffer to audio output.
    // Write samples to output.
    audio_block_t *outBlock[NUM_CHANNELS];
    uint8_t data[UDP_PACKET_SIZE];
    // Read a packet from the input UDP buffer
    udpBuffer.read(data, UDP_PACKET_SIZE);
    for (int channel = 0; channel < NUM_CHANNELS; channel++) {
        outBlock[channel] = allocate();
        // Only proceed if an audio block was allocated, i.e. the
        // current output channel is connected to something.
        if (outBlock[channel]) {
            // Get the start of the sample data in the packet. Cast to desired
            // bit-resolution.
            auto start = (const int16_t *) (data + PACKET_HEADER_SIZE + CHANNEL_FRAME_SIZE * channel);
            // Copy the samples to the output block.
            memcpy(outBlock[channel]->data, start, CHANNEL_FRAME_SIZE);
            // Finish up.
            transmit(outBlock[channel], channel);
            release(outBlock[channel]);
        }
    }
}

bool JackTripClient::isExitPacket() {
    uint8_t packet[EXIT_PACKET_SIZE];
    if (read(packet, EXIT_PACKET_SIZE) == EXIT_PACKET_SIZE) {
        for (size_t i = 0; i < EXIT_PACKET_SIZE; ++i) {
            if (packet[i] != 0xff) {
                return false;
            }
        }
        return true;
    }
    return false;
}

void JackTripClient::setShowStats(bool show, uint16_t intervalMS) {
    showStats = show;
    packetStats.setPrintInterval(intervalMS);
}

