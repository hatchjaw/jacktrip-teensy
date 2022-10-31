//
// Created by tar on 30/10/22.
//

#ifndef JACKTRIP_TEENSY_CIRCULARBUFFER_H
#define JACKTRIP_TEENSY_CIRCULARBUFFER_H


#include <cstdint>

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

private:
    uint16_t length;
    T *buffer;
    uint16_t writeIndex{0}, readIndex{0};
};

template
class CircularBuffer<uint8_t>;

template
class CircularBuffer<int16_t>;

#endif //JACKTRIP_TEENSY_CIRCULARBUFFER_H
