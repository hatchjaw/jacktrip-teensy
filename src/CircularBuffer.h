//
// Created by tar on 30/10/22.
//

#ifndef JACKTRIP_TEENSY_CIRCULARBUFFER_H
#define JACKTRIP_TEENSY_CIRCULARBUFFER_H

#include <Arduino.h>

template<typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(uint16_t length);

    ~CircularBuffer();

    void write(const T *data, uint16_t len);

    void read(T *bufferToFill, uint16_t len);

    int getWriteIndex();

    int getReadIndex();

    void clear();

    void printStats();

private:
    enum OperationType{
        UNKNOWN,
        READ,
        WRITE
    };
    const uint32_t kStatInterval{2500};
    uint16_t length;
    T *buffer;
    uint16_t writeIndex{0}, readIndex{0};
    int32_t numReads{0}, numWrites{0};
    OperationType lastOp{UNKNOWN};
    uint8_t consecutiveOpCount{1};
    elapsedMillis statTimer;
};

template
class CircularBuffer<uint8_t>;

template
class CircularBuffer<int16_t>;

#endif //JACKTRIP_TEENSY_CIRCULARBUFFER_H
