//
// Created by tar on 02/11/22.
//

#include "PacketStats.h"

PacketStats::PacketStats() {
    reset();
}

void PacketStats::reset() {
    totalReceived = 0;
    totalSent = 0;
    lastReceived = JackTripPacketHeader{};
    lastSent = JackTripPacketHeader{};
    receiveInterval = PacketDelta{};
    receiveSequence = PacketDelta{};
    sendInterval = PacketDelta{};
    sendSequence = PacketDelta{};
}

void PacketStats::printStats() {
    if (!awaitingFirstReceive() && elapsed > printInterval) {
        Serial.printf("\n"
                      "        |   Total   |   Last Timestamp    | Last SeqNum | TS delta min/mean/max | "
                      "SN delta min/mean/max\n"
                      "receive | %9" PRId32 " | %16" PRIu64 "    | %11" PRIu16 " | %d/%.4f/%d µs | %d/%.4f/%d\n"
                      "   send | %9" PRId32 " | %16" PRIu64 "    | %11" PRIu16 " | %d/%.4f/%d µs | %d/%.4f/%d\n"
                      "  delta | %9" PRId32 " | %16" PRId64 " µs | %11" PRId16 "\n\n",
                      totalReceived,
                      lastReceived.TimeStamp, lastReceived.SeqNumber,
                      receiveInterval.min, receiveInterval.mean, receiveInterval.max,
                      receiveSequence.min, receiveSequence.mean, receiveSequence.max,
                      totalSent,
                      lastSent.TimeStamp, lastSent.SeqNumber,
                      sendInterval.min, sendInterval.mean, sendInterval.max,
                      sendSequence.min, sendSequence.mean, sendSequence.max,
                      totalReceived - totalSent,
                      static_cast<int64_t>(lastReceived.TimeStamp) - static_cast<int64_t>(lastSent.TimeStamp),
                      static_cast<int16_t>(lastReceived.SeqNumber) - static_cast<int16_t>(lastSent.SeqNumber)
        );

        elapsed = 0;
    }
}

bool PacketStats::awaitingFirstReceive() const {
    return totalReceived == 0;
}

void PacketStats::setPrintInterval(uint32_t intervalMS) {
    printInterval = intervalMS;
}

void PacketStats::registerReceive(JackTripPacketHeader &header) {
    ++totalReceived;

//    if (totalReceived < 1000) {
//        Serial.printf("Seq: %" PRId32 "; Timestamp: %" PRIu64 "\n", header.SeqNumber, header.TimeStamp);
//    }

    if (totalReceived > IGNORE_JUNK) {
        auto seqNumDelta = static_cast<int>(header.SeqNumber) - static_cast<int>(lastReceived.SeqNumber);
        if (header.SeqNumber == 0 && seqNumDelta == -UINT16_MAX) {
            seqNumDelta = 1;
        }
        if (seqNumDelta != 1) {
            Serial.printf("PACKET DROPPED (RECEIVE): prev %d current %d dropped %d\n\n", lastReceived.SeqNumber,
                          header.SeqNumber, header.SeqNumber - lastReceived.SeqNumber - 1);
        }

        auto fTotal = static_cast<float>(totalReceived - IGNORE_JUNK);

        if (lastReceived.TimeStamp != 0) {
            auto interval = static_cast<int32_t>(header.TimeStamp - lastReceived.TimeStamp);
            auto fInterval = static_cast<float>(interval);
            if (interval < receiveInterval.min) {
                receiveInterval.min = interval;
            } else if (interval > receiveInterval.max) {
                receiveInterval.max = interval;
            }
            receiveInterval.mean = (receiveInterval.mean * (fTotal - 1) + fInterval) / fTotal;
        }

        auto fSequence = static_cast<float>(seqNumDelta);
        if (seqNumDelta < receiveSequence.min) {
            receiveSequence.min = seqNumDelta;
        } else if (seqNumDelta > receiveSequence.max) {
            receiveSequence.max = seqNumDelta;
        }
        receiveSequence.mean = (receiveSequence.mean * (fTotal - 1) + fSequence) / fTotal;
    }

    lastReceived = header;
}

void PacketStats::registerSend(JackTripPacketHeader &header) {
    ++totalSent;

    if (totalSent > IGNORE_JUNK) {
        auto seqNumDelta = static_cast<int>(header.SeqNumber) - static_cast<int>(lastSent.SeqNumber);
        if (header.SeqNumber == 0 && seqNumDelta == -UINT16_MAX) {
            seqNumDelta = 1;
        }
        if (seqNumDelta != 1) {
            Serial.printf("PACKET DROPPED (SEND): prev %d current %d dropped %d\n\n", lastSent.SeqNumber,
                          header.SeqNumber, header.SeqNumber - lastSent.SeqNumber);
        }

        auto fTotal = static_cast<float>(totalSent - IGNORE_JUNK);

        if (lastSent.TimeStamp != 0) {
            auto interval = static_cast<int32_t>(header.TimeStamp - lastSent.TimeStamp);
            auto fInterval = static_cast<float>(interval);
            if (interval < sendInterval.min) {
                sendInterval.min = interval;
            } else if (interval > sendInterval.max) {
                sendInterval.max = interval;
            }
            sendInterval.mean = (sendInterval.mean * (fTotal - 1) + fInterval) / fTotal;
        }

        auto fSequence = static_cast<float>(seqNumDelta);
        if (seqNumDelta < sendSequence.min) {
            sendSequence.min = seqNumDelta;
        } else if (seqNumDelta > sendSequence.max) {
            sendSequence.max = seqNumDelta;
        }
        sendSequence.mean = (sendSequence.mean * (fTotal - 1) + fSequence) / fTotal;
    }

    lastSent = header;
}
