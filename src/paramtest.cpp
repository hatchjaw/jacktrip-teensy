#include <Audio.h>
#include "Gain/Gain.h"

AudioControlSGTL5000 audioShield;
Gain g;

void setup() {
    while (!Serial);

    if (CrashReport) {
        Serial.println(CrashReport);
        CrashReport.clear();
    }

    audioShield.enable();
    AudioMemory(16);

    g.setParamValue("gain", .5);
    Serial.println("Gain set.");
    g.setParamValue("gain?", .5);
    Serial.println("Gain set again.");
}

void loop() {}