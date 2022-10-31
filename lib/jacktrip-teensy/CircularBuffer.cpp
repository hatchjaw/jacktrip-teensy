//
// Created by tar on 30/10/22.
//

#include "CircularBuffer.h"

template<typename T>
CircularBuffer<T>::CircularBuffer(uint16_t length) :
        length(length),
        buffer(new T[length]) {
    for (int i = 0; i < length; ++i) {
        buffer[i] = 0;
    }
}

template<typename T>
CircularBuffer<T>::~CircularBuffer() {
    delete[] buffer;
}

template<typename T>
int CircularBuffer<T>::getWriteIndex() {
    return writeIndex;
}

template<typename T>
int CircularBuffer<T>::getReadIndex() {
    return readIndex;
}

template<typename T>
void CircularBuffer<T>::write(const T *data, uint16_t len) {
    for (int i = 0; i < len; ++i, ++writeIndex) {
        if (writeIndex == length) {
            writeIndex = 0;
        }
        buffer[writeIndex] = data[i];
    }
}

template<typename T>
void CircularBuffer<T>::read(T *bufferToFill, uint16_t len) {
    for (int i = 0; i < len; ++i, ++readIndex) {
        if (readIndex == length) {
            readIndex = 0;
        }
        bufferToFill[i] = buffer[readIndex];
    }
}
