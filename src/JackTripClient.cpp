//
// Created by tar on 16/09/22.
//

#include "JackTripClient.h"

JackTripClient::JackTripClient(uint8_t numChannels,
                               IPAddress &serverIpAddress,
                               uint16_t serverTcpPort) :
        AudioStream{numChannels, new audio_block_t *[numChannels]},
        kNumChannels{numChannels},
        kUdpPacketSize{PACKET_HEADER_SIZE + kNumChannels * AUDIO_BLOCK_SAMPLES * sizeof(uint16_t)},
        kAudioPacketSize{AUDIO_BLOCK_SAMPLES * kNumChannels * 2u},
        // Assume client and server on same subnet
        clientIP{serverIpAddress},
        serverIP{serverIpAddress},
        serverTcpPort{serverTcpPort},
#ifdef USE_TIMER
        timer(TeensyTimerTool::GPT1),
#endif
        udpBuffer(kUdpPacketSize * 16),
        audioBuffer(kNumChannels, AUDIO_BLOCK_SAMPLES * 8),
        audioBlock(new int16_t *[kNumChannels]) {

    // Generate a MAC address (from the program-once area of Teensy's flash
    // memory) to assign to the ethernet shield.
    teensyMAC(clientMAC);
    // Use the last byte of the MAC to set the last byte of the IP.
    // (Maybe needs a more sophisticated approach.)
    clientIP[3] += clientMAC[5];

    serverHeader = new JackTripPacketHeader;

    for (int ch = 0; ch < kNumChannels; ++ch) {
        audioBlock[ch] = new int16_t[AUDIO_BLOCK_SAMPLES];
    }
}

JackTripClient::~JackTripClient() {
    delete serverHeader;
    for (int ch = 0; ch < kNumChannels; ++ch) {
        delete[] audioBlock[ch];
    }
    delete[] audioBlock;
}

uint8_t JackTripClient::begin(uint16_t port) {
    if (!active) {
        Serial.println("JackTripClient is not connected to any Teensy audio objects.");
        return 0;
    }

    if (kUdpPacketSize > FNET_SOCKET_DEFAULT_SIZE) {
        Serial.printf("JackTripClient: UDP packet size (%d) is greater than the default socket size (%d). "
                      "Increasing to match.\n", kUdpPacketSize, FNET_SOCKET_DEFAULT_SIZE);
        EthernetClass::setSocketSize(kUdpPacketSize);
    }

    Serial.print("JackTripClient: MAC address is: ");
    for (int i = 0; i < 6; ++i) {
        Serial.printf(i < 5 ? "%02X:" : "%02X", clientMAC[i]);
    }
    Serial.println();

    EthernetClass::begin(clientMAC, clientIP);

    if (EthernetClass::linkStatus() != LinkON) {
        Serial.println("JackTripClient: Ethernet link could not be established.");
        return 0;
    } else {
        Serial.println("JackTripClient: Ethernet connected.");
    }

    Serial.print("JackTripClient: IP is ");
    Serial.println(EthernetClass::localIP());

    Serial.printf("JackTripClient: Packet size is %d bytes\n", kUdpPacketSize);

#ifdef USE_TIMER
    auto timerPeriod = 1'000'000.f * static_cast<float>(AUDIO_BLOCK_SAMPLES) / AUDIO_SAMPLE_RATE_EXACT;
    Serial.printf("UDP send/receive timer period: %.3f\n", timerPeriod);
    timer.begin([this] { updateImpl(); }, timerPeriod);
#endif

    return EthernetUDP::begin(port);
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
    // Send the local port.
    if (4 != c.write(reinterpret_cast<const uint8_t *>(&port), 4)) {
        Serial.println("JackTripClient: failed to send UDP port to server.");
        c.close();
        return false;
    }
    // Patience...
    while (c.available() < 4) {}
    // Read the remote port.
    if (4 != c.read(reinterpret_cast<uint8_t *>(&port), 4)) {
        Serial.println("JackTripClient: failed to read UDP port from server.");
        c.close();
        connected = false;
        return false;
    } else {
        connected = true;
//        if (onConnected != nullptr) {
//            (*onConnected)();
//        }
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
    audioBuffer.clear();
    packetStats.reset();
}

void JackTripClient::update(void) {
#ifdef USE_TIMER
    doAudioOutputFromAudio();
#else
    updateImpl();
#endif
}

void JackTripClient::updateImpl() {
    auto received{receivePackets()};

//    if (received < 1) {
////        Serial.printf("Received %d packets\n", received);
//        if (received == 0) {
//            delayMicroseconds(100);
////            Serial.print("trying again...");
//            received = receivePackets();
////            Serial.printf(" Received %d packets\n", received);
//        }
//    }
#ifndef USE_TIMER
    doAudioOutputFromAudio();
#endif
    sendPacket();

    if (showStats && connected) {
        packetStats.printStats();
//        udpBuffer.printStats();
        audioBuffer.printStats();
    }
}

bool JackTripClient::isConnected() const {
    return connected;
}

int JackTripClient::receivePackets() {
    if (!connected) return -1;

    auto received{0}, size{0};

    // Check for incoming UDP packets. Get as many packets as are available.
    RECEIVE_CONDITION ((size = parsePacket()) > 0) {
        ++received;
        lastReceive = 0;

        if (size == EXIT_PACKET_SIZE && isExitPacket()) {
            // Exit sequence
            Serial.println("JackTripClient: Received exit packet");
            Serial.printf("  maxmem: %d blocks\n", AudioMemoryUsageMax());
            Serial.printf("  maxcpu: %f %%\n\n", AudioProcessorUsageMax());

            stop();
            return received;
        } else if (size != kUdpPacketSize) {
            Serial.println("JackTripClient: Received a malformed packet");
        } else {
            // Read the UDP packet and write it into a circular buffer.
            uint8_t in[size];
            auto bytesRead = read(in, size);
//            Serial.printf("Read %d bytes\n", bytesRead);
//            udpBuffer.write(in, size);

            // Convert to audio and write that to a circular buffer.
            const int16_t *audio[kNumChannels];
            for (int ch = 0; ch < kNumChannels; ++ch) {
                audio[ch] = reinterpret_cast<int16_t *>(in + PACKET_HEADER_SIZE + CHANNEL_FRAME_SIZE * ch);
            }
            audioBuffer.write(audio, AUDIO_BLOCK_SAMPLES);

            // Read the header from the packet received from the server.
            serverHeader = reinterpret_cast<JackTripPacketHeader *>(in);

            if (packetStats.awaitingFirstReceive()) { //|| timestampInterval > 1000) {
//                timestampInterval = 0;

                if (packetStats.awaitingFirstReceive() && showStats) {
                    Serial.println("===============================================================");
                    Serial.printf("Received first packet: Timestamp: %" PRIu64 "; SeqNumber: %" PRIu16 "\n",
                                  serverHeader->TimeStamp,
                                  serverHeader->SeqNumber);
                    packetHeader.TimeStamp = serverHeader->TimeStamp;
                    packetHeader.SeqNumber = serverHeader->SeqNumber;
                    Serial.println("===============================================================");
                }
            }

            packetStats.registerReceive(*serverHeader);
        }
    }

    if (lastReceive > RECEIVE_TIMEOUT_MS) {
        Serial.printf("JackTripClient: Nothing received for %.1f s. Stopping.\n", RECEIVE_TIMEOUT_MS / 1000.f);
        stop();
    }

    return received;
}

void JackTripClient::sendPacket() {
    // Might have received an exit packet, so check whether still connected.
    if (!connected) return;

    // Get the location in the UDP buffer to which audio samples should be
    // written.
    uint8_t packet[kUdpPacketSize];
    uint8_t *pos = packet + PACKET_HEADER_SIZE;

    // Copy audio to the UDP buffer.
    audio_block_t *inBlock[num_inputs];
    for (int channel = 0; channel < num_inputs; channel++) {
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
    size_t written = write(packet, kUdpPacketSize);
    if (written != kUdpPacketSize) {
        written += write(packet + written, kUdpPacketSize - written);
        if (written != kUdpPacketSize) {
            Serial.printf("JackTripClient: Net buffer is too small (wrote %d of %d bytes)\n", written, kUdpPacketSize);
        }
    }
    auto result = endPacket();
    if (0 == result) {
        Serial.println("JackTripClient: failed to send a packet.");
    }

    packetStats.registerSend(packetHeader);
}

void JackTripClient::doAudioOutputFromUDP() {
    if (!connected) return;

    // Copy from UDP inBuffer to audio output.
    // Write samples to output.
    audio_block_t *outBlock[kNumChannels];
    uint8_t data[kUdpPacketSize];
    // Read a packet from the input UDP buffer
    udpBuffer.read(data, kUdpPacketSize);
    for (int channel = 0; channel < kNumChannels; channel++) {
        outBlock[channel] = allocate();
        // Only proceed if an audio block was allocated, i.e. the
        // current output channel is connected to something.
        if (outBlock[channel]) {
            // Get the start of the sample data in the packet. Cast to desired
            // bit-resolution.
            auto start = (const int16_t *) (data + PACKET_HEADER_SIZE + CHANNEL_FRAME_SIZE * channel);
            // Copy the samples to the output block.
            memcpy(outBlock[channel]->data, start, CHANNEL_FRAME_SIZE);
#ifdef JACKTRIPCLIENT_DEBUG
            auto header = reinterpret_cast<JackTripPacketHeader *>(data);
            // Indicate the first sample in each packet.
            outBlock[channel]->data[0] = INT16_MIN;
            // Indicate this packet's timestamp.
            outBlock[channel]->data[15] = static_cast<int16_t>(header->TimeStamp);
#endif
            // Finish up.
            transmit(outBlock[channel], channel);
            release(outBlock[channel]);
        }
    }
}

void JackTripClient::doAudioOutputFromAudio() {
    audioBuffer.read(audioBlock, AUDIO_BLOCK_SAMPLES);
    audio_block_t *outBlock[kNumChannels];
    for (int ch = 0; ch < kNumChannels; ++ch) {
        outBlock[ch] = allocate();
        if (outBlock[ch]) {
            // Copy the samples to the output block.
            memcpy(outBlock[ch]->data, audioBlock[ch], CHANNEL_FRAME_SIZE);
#ifdef JACKTRIPCLIENT_DEBUG
            // Indicate the first sample in each block.
            outBlock[ch]->data[0] = INT16_MIN;
#endif
            // Finish up.
            transmit(outBlock[ch], ch);
            release(outBlock[ch]);
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

void JackTripClient::setOnConnected(std::function<void(void)> callback) {
    *onConnected = callback;
}
