//
// Created by tar on 29/11/22.
//

#include "NtpReceiver.h"

uint8_t NtpReceiver::begin(uint16_t port) {
    if (1 != EthernetUDP::begin(port)) {
        return 0;
    }

//    setSyncProvider([this] {getNtpTime();});
}

time_t NtpReceiver::getNtpTime() {
    while (parsePacket() > 0); // discard any previously received packets
    Serial.println("Transmit NTP Request");
    auto ip = IPAddress{192, 168, 10, 255};
    sendNtpPacket(ip);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
        int size = parsePacket();
        if (size >= kNtpPacketSize) {
            Serial.println("Receive NTP Response");
            read(packetBuffer, kNtpPacketSize);  // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long) packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long) packetBuffer[43];
            return secsSince1900 - 2208988800UL + 1 * SECS_PER_HOUR;
        }
    }
    Serial.println("No NTP Response :-(");
    return 0; // return 0 if unable to get the time
}

void NtpReceiver::sendNtpPacket(IPAddress &address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, kNtpPacketSize);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    beginPacket(address, 123); //NTP requests are to port 123
    write(packetBuffer, kNtpPacketSize);
    endPacket();
}
