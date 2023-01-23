//
// Created by tar on 30/11/22.
//

#ifndef JACKTRIP_TEENSY_CIRCULARBUFFERMULTI_H
#define JACKTRIP_TEENSY_CIRCULARBUFFERMULTI_H


#include <Arduino.h>
#include "SmoothedParameter.h"

template<typename T>
class CircularBufferMulti {
public:
    CircularBufferMulti(uint8_t numChannels, uint16_t length);

    ~CircularBufferMulti();

    void write(const T **data, uint16_t len);

    void read(T **bufferToFill, uint16_t len);

    int getWriteIndex();

    float getReadPosition();

    void clear();

    void printStats();

private:
    enum OperationType {
        UNKNOWN,
        READ,
        WRITE
    };
    static constexpr uint16_t BLOCKS_PER_READ_INCREMENT_UPDATE{1000};
    static constexpr uint8_t VISUALISER_LENGTH{100};
    const uint32_t kStatInterval{2500};
    const uint8_t kNumChannels;
    const uint16_t kLength;
    const float kFloatLength;
    const std::pair<float, float> kRwDeltaThresh;

    float getReadWriteDelta();

    void setReadPosIncrement();

    T interpolateCubic(T *channelData, uint16_t readIdx, float alpha);

    uint16_t wrapIndex(uint16_t index, uint16_t length);

    T **buffer;
    uint16_t writeIndex{0};
    float readPos{0.f};
    SmoothedParameter<float> readPosIncrement{1.f};
    uint64_t numBlockReads{0}, numBlockWrites{0}, numSampleWrites{0}, numSampleReads{0};
    uint32_t blocksReadSinceLastUpdate{0}, blocksWrittenSinceLastUpdate{0};
    float readPosAllTime{0.f};
    OperationType lastOp{UNKNOWN};
    uint8_t consecutiveOpCount{1};
    elapsedMillis statTimer{0};
    elapsedMillis debugTimer{100};
    char visualiser[VISUALISER_LENGTH + 1]{};
};

template
class CircularBufferMulti<uint8_t>;

template
class CircularBufferMulti<int16_t>;

#endif //JACKTRIP_TEENSY_CIRCULARBUFFERMULTI_H
