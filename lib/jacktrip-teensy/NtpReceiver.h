//
// Created by tar on 29/11/22.
//

#ifndef JACKTRIP_TEENSY_NTPRECEIVER_H
#define JACKTRIP_TEENSY_NTPRECEIVER_H

#include <NativeEthernet.h>
#include <TimeLib.h>

class NtpReceiver : public EthernetUDP {
public:
    uint8_t begin(uint16_t port) override;
private:
    time_t getNtpTime();
    void sendNtpPacket(IPAddress &address);
    static constexpr int kNtpPacketSize{48}; // NTP time is in the first 48 bytes of message
    byte packetBuffer[kNtpPacketSize]; //buffer to hold incoming & outgoing packets
};


#endif //JACKTRIP_TEENSY_NTPRECEIVER_H
