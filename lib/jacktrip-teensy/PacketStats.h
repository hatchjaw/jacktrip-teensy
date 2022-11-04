//
// Created by tar on 03/11/22.
//

#ifndef JACKTRIP_TEENSY_PACKETSTATS_H
#define JACKTRIP_TEENSY_PACKETSTATS_H

#include "Arduino.h"
#include "PacketHeader.h"

class PacketStats {
public:
    PacketStats();

    void reset();

    void printStats();

    bool awaitingFirstReceive() const;

    void setPrintInterval(uint32_t intervalMS);

    void registerReceive(JackTripPacketHeader &header);

    void registerSend(JackTripPacketHeader &header);

private:
    struct PacketDelta{
        int32_t min{INT32_MAX}, max{INT32_MIN};
        float mean{0.f};
    };
    const int IGNORE_JUNK{100};
    int32_t totalSent{0};
    int32_t totalReceived{0};
    JackTripPacketHeader lastSent{};
    JackTripPacketHeader lastReceived{};
    PacketDelta receiveInterval;
    PacketDelta receiveSequence;
    PacketDelta sendInterval;
    PacketDelta sendSequence;
    elapsedMillis elapsed;
    uint32_t printInterval{1'000};
};


#endif //JACKTRIP_TEENSY_PACKETSTATS_H
