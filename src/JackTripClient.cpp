//
// Created by Tommy Rushton on 16/09/22.
//

#include "JackTripClient.h"

JackTripClient::JackTripClient(IPAddress &clientIpAddress, IPAddress &serverIpAddress) :
        AudioStream(2, inputQueueArray),
        clientIP(clientIpAddress),
        serverIP(serverIpAddress){
    // Generate a MAC address (from the program-once area of Teensy's flash
    // memory) to assign to the ethernet shield.
    teensyMAC(clientMAC);
    clientIP[3] += clientMAC[5];
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
    Serial.println(Ethernet.localIP());

    Serial.printf("JackTripClient: Packet size is %d bytes\n", UDP_BUFFER_SIZE);

    return EthernetUDP::begin(port);
}

EthernetLinkStatus JackTripClient::startEthernet() {
    Ethernet.setSocketNum(4);

    if (UDP_BUFFER_SIZE > FNET_SOCKET_DEFAULT_SIZE) {
        Ethernet.setSocketSize(UDP_BUFFER_SIZE);
    }

    Serial.print("JackTripClient: MAC address is: ");
    for (int i = 0; i < 6; ++i) {
        Serial.printf(i < 5 ? "%02X:" : "%02X", clientMAC[i]);
    }
    Serial.println();

#ifdef CONF_DHCP
    bool dhcpFailed = false;
    // Start ethernet
    if (!Ethernet.begin(clientMAC)) {
        dhcpFailed = true;
    }
#else
    Ethernet.begin(clientMAC, clientIP);
#endif

    if (Ethernet.linkStatus() != LinkON) {
        Serial.println("JackTripClient: Ethernet cable is not connected.");
    } else {
        Serial.println("JackTripClient: Ethernet connected.");
    }

#ifdef CONF_DHCP
    if (dhcpFailed) {
        Serial.println("JackTripClient: DHCP configuration failed");
        return Unknown;
    }
#endif

    return Ethernet.linkStatus();
}

bool JackTripClient::connect(uint16_t timeout) {
    if (!active) {
        Serial.println("JackTripClient is not connected to any Teensy audio objects.");
        return false;
    }

    // Attempt handshake with JackTrip server via TCP port.
    Serial.print("JackTripClient: Connecting to JackTrip server at ");
    Serial.print(serverIP);
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

    // Apparently just sending the local UDP port yields the remote UDP port
    // in return...
    auto port = localPort();
    // Send the local port (little endian).
    if (4 != c.write((const uint8_t *) &port, 4)) {
        Serial.println("JackTripClient: failed to send UDP port to server.");
        c.close();
        connected = false;
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
    }

    serverUdpPort = port;
    Serial.printf("JackTripClient: Server port is %d\n", serverUdpPort);

    lastReceive = millis();
    return connected;
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
        // Only proceed if a block was returned, i.e. something is connected
        // to one of this object's input channels.
        if (inBlock[channel]) {
            memcpy(pos, inBlock[channel]->data, channelFrameSize);
            pos += channelFrameSize;
            release(inBlock[channel]);
        }
    }

    // Send the packet.
    beginPacket(serverIP, serverUdpPort);
    size_t written = write(buffer, UDP_BUFFER_SIZE);
    if (written != UDP_BUFFER_SIZE) {
        Serial.println("JackTripClient: Net buffer is too small");
    }
    endPacket();

//    packetHeader.TimeStamp++; // This isn't necessary.
    packetHeader.SeqNumber++;
}

void JackTripClient::receivePacket() {
    int size;

    // parsePacket() does the read, essentially. Afterwards there's no data
    // remaining in the UDP stream. But how does this work so smoothly?...
    // TODO: override parsePacket()?
    if ((size = parsePacket())) {
        lastReceive = millis();

        if (size == EXIT_PACKET_SIZE && isExitPacket()) {
            // Exit sequence
            Serial.println("JackTripClient: Received exit packet");
            Serial.printf("  maxmem: %d blocks\n", AudioMemoryUsageMax());
            Serial.printf("  maxcpu: %f %%\n", AudioProcessorUsageMax());

            stop();
        } else if (size != UDP_BUFFER_SIZE) {
            Serial.println("JackTripClient: Received a malformed packet");
        } else {
            // Read the UDP packet into the buffer.
            read(buffer, UDP_BUFFER_SIZE);

            // Size in memory of one channel's worth of samples.
            auto channelFrameSize = AUDIO_BLOCK_SAMPLES * sizeof(uint16_t);

//            auto *header = new JackTripPacketHeader;
//            memcpy(header, buffer, PACKET_HEADER_SIZE);
////            if (header->SeqNumber - prevHeader.SeqNumber != 1) {
//                Serial.println(header->SeqNumber);
//                Serial.println(header->TimeStamp);
////            }
////            prevHeader.SeqNumber = header->SeqNumber;
////            prevHeader.TimeStamp = header->TimeStamp;

            // Write samples to output.
            audio_block_t *outBlock[NUM_CHANNELS];
            for (int channel = 0; channel < NUM_CHANNELS; channel++) {
                outBlock[channel] = allocate();
                // Only proceed if an audio block was allocated, i.e. the
                // current output channel is connected to something.
                if (outBlock[channel]) {
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
    }

//    if (millis() - lastReceive > RECEIVE_TIMEOUT) {
//        Serial.printf("JackTripClient: Nothing received for %.1f s\n", RECEIVE_TIMEOUT / 1000.f);
//        Serial.printf("JackTripClient: (Is JACK definitely running on the server?)\n");
//        stop();
//        lastReceive = millis();
//    }

    if (millis() - lastReceive > RECEIVE_TIMEOUT) {
        Serial.printf("JackTripClient: Nothing received for %.1f s\n", RECEIVE_TIMEOUT / 1000.f);
        lastReceive = millis();
    }
}

bool JackTripClient::isExitPacket() {
    if (read(buffer, EXIT_PACKET_SIZE) == EXIT_PACKET_SIZE) {
        for (size_t i = 0; i < EXIT_PACKET_SIZE; ++i) {
            if (buffer[i] != 0xff) {
                return false;
            }
        }
        return true;
    }
    return false;
}
