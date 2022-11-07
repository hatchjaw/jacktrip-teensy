//
// Created by tar on 30/10/22.
//

#include "CircularBuffer.h"

template<typename T>
CircularBuffer<T>::CircularBuffer(uint16_t length) :
        length(length),
        buffer(new T[length]) {
    clear();
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
void CircularBuffer<T>::clear() {
    for (int i = 0; i < length; ++i) {
        buffer[i] = 0;
    }
    readIndex = 0;
    writeIndex = 0;
    numReads = 0;
    numWrites = 0;
}

template<typename T>
void CircularBuffer<T>::printStats() {
    if (statTimer > STAT_INTERVAL) {
        Serial.printf("CircularBuffer: writes %d %s reads %d, delta %d\n",
                      numWrites,
                      (numWrites > numReads ? ">" : (numWrites < numReads ? "<" : "==")),
                      numReads,
                      numWrites - numReads);
        statTimer = 0;
    }
}

template<typename T>
void CircularBuffer<T>::write(const T *data, uint16_t len) {
    for (int i = 0; i < len; ++i, ++writeIndex) {
        if (writeIndex == length) {
            writeIndex = 0;
        }

        buffer[writeIndex] = data[i];
    }

    ++numWrites;
//    if (lastOp == WRITE) {
//        ++consecutiveOpCount;
//        Serial.printf("CircularBuffer: Extra WRITE (%d)\n", consecutiveOpCount);
//    } else {
//        consecutiveOpCount = 1;
//    }
//    lastOp = WRITE;
}

template<typename T>
void CircularBuffer<T>::read(T *bufferToFill, uint16_t len) {
    for (int i = 0; i < len; ++i, ++readIndex) {
        if (readIndex == length) {
            readIndex = 0;
        }
        bufferToFill[i] = buffer[readIndex];
    }

    ++numReads;
//    if (lastOp == READ) {
//        ++consecutiveOpCount;
//        Serial.printf("CircularBuffer: Extra READ (%d)\n", consecutiveOpCount);
//    } else {
//        consecutiveOpCount = 1;
//    }
//    lastOp = READ;
}
