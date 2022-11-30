//
// Created by tar on 30/11/22.
//

#include "CircularBufferMulti.h"

template<typename T>
CircularBufferMulti<T>::CircularBufferMulti(uint8_t numChannels, uint16_t length) :
        kNumChannels{numChannels},
        kLength{length},
        kFloatLength{static_cast<float>(length)},
        buffer{new T *[numChannels]} {
    for (int ch = 0; ch < kNumChannels; ++ch) {
        buffer[ch] = new T[kLength];
    }
    clear();
}

template<typename T>
CircularBufferMulti<T>::~CircularBufferMulti() {
    for (int i = 0; i < kNumChannels; ++i) {
        delete buffer[i];
    }
    delete[] buffer;
}

template<typename T>
int CircularBufferMulti<T>::getWriteIndex() {
    return writeIndex;
}

template<typename T>
int CircularBufferMulti<T>::getReadIndex() {
    return 0;//readIndex;
}

template<typename T>
void CircularBufferMulti<T>::clear() {
    for (int ch = 0; ch < kNumChannels; ++ch) {
        for (int s = 0; s < kLength; ++s) {
            buffer[ch][s] = 0;
        }
    }

//    readIndex = 0;
    readPos = 0.f;
    writeIndex = 0;
    numBlockReads = 0;
    numBlockWrites = 0;
}

template<typename T>
void CircularBufferMulti<T>::printStats() {
    if (statTimer > kStatInterval) {
        Serial.printf("CircularBuffer: writes %"
        PRId64
        " %s reads %"
        PRId64
        ", delta %"
        PRId64
        "\n",
                numBlockWrites,
                (numBlockWrites > numBlockReads ? ">" : (numBlockWrites < numBlockReads ? "<" : "==")),
                numBlockReads,
                numBlockWrites - numBlockReads);
        Serial.printf("CircularBuffer: writeIndex: %d, readIndex: %f, delta %f\n",
                      writeIndex, readPos, getReadWriteDelta());
        statTimer = 0;
    }
}

template<typename T>
void CircularBufferMulti<T>::write(const T **data, uint16_t len) {
    if (shouldPrintDebug()) {
        Serial.printf("Start of write method, writeIndex: %d\n", writeIndex);
    }

    for (int n = 0; n < len; ++n, ++writeIndex) {
        if (writeIndex == kLength) {
            writeIndex = 0;
        }

        if (shouldPrintDebug()) {
            Serial.printf("In loop, writeIndex: %d (readPos: %f)\n", writeIndex, readPos);
        }

//        if (writeIndex == readIndex) {
//            Serial.printf("Bumping readIndex. (writeIndex = %d, readIndex = %d)\n", writeIndex, readIndex);
//            ++readIndex;
//            if (readIndex == kLength) {
//                readIndex = 0;
//            }
//        }

        for (int ch = 0; ch < kNumChannels; ++ch) {
            buffer[ch][writeIndex] = data[ch][n];
        }
    }

    ++numBlockWrites;
//    if (lastOp == WRITE) {
//        ++consecutiveOpCount;
//        Serial.printf("CircularBuffer: Extra WRITE (%d)\n", consecutiveOpCount);
//    } else {
//        consecutiveOpCount = 1;
//    }
//    lastOp = WRITE;
}

template<typename T>
void CircularBufferMulti<T>::read(T **bufferToFill, uint16_t len) {
    if (shouldPrintDebug()) {
        Serial.printf("Start of read method, readPos: %f\n", readPos);
    }

    auto rwDelta{getReadWriteDelta()};
    auto initialReadPos{readPos};

    for (uint16_t n = 0; n < len; n++) {
        if (readPos >= kFloatLength) {
            readPos -= kFloatLength;
        }

//        if (shouldPrintDebug()) {
//            Serial.printf("In loop, readPos: %f (writeIndex: %d)\n", readPos, writeIndex);
//        }

        float readIdx{0.f};
        auto alpha = modff(readPos, &readIdx);
        for (int ch = 0; ch < kNumChannels; ++ch) {
            bufferToFill[ch][n] = interpolateCubic(buffer[ch], static_cast<uint16_t>(readIdx), alpha);
        }

        rwDelta = getReadWriteDelta();
        if (rwDelta < len) {
            // Could do this in proportion with the delta...
            readPos += .995f;
            Serial.printf("Read < 1 buffer behind write: slower (readPos: %f, writeIndex: %d, rwDelta: %f)\n",
                          readPos, writeIndex, getReadWriteDelta());
        } else if (rwDelta > 2 * len) {
            readPos += 1.005f;
            Serial.printf("Read > 2 buffers behind write,]: faster (readPos: %f, writeIndex: %d, rwDelta: %f)\n",
                          readPos, writeIndex, getReadWriteDelta());
        } else {
            readPos += 1.f;
        }
    }
    ++numBlockReads;
    // Just do a sort of wavetable thing if there haven't yet been any writes.
    if (0 == numBlockWrites) {
        readPos = initialReadPos;
    }

    return;

    // At this point, read should be *roughly* a block behind write.
    // If readIndex is more than a block behind writeIndex, speed up a bit.
    // If less than a block behind, there's a risk of reading garbage, so slow down.
    if (rwDelta >= 2 * len || rwDelta <= len) {
//        Serial.println("Resampling...");

        if (!isDebugging) {
            isDebugging = true;
            debugTimer = 0;
        }

        // Resample
//        auto fReadIndex{static_cast<float>(readIndex)};
        auto kFloatLength{static_cast<float>(kLength)};

        for (uint16_t n = 0; n < len; ++n) {
            if (readPos >= kFloatLength) {
                readPos -= kFloatLength;
            }
//            readIndex = static_cast<uint16_t>(floor(fReadIndex));
//            if (readIndex >= kLength) {
//                readIndex = 0;
//            }

            if (shouldPrintDebug()) {
                Serial.printf("In loop (resampling), readPos: %f (writeIndex: %d)\n", readPos, writeIndex);
            }

            // Separate the integer and fractional parts of the read index.
            float readIdx{0.f};
            auto alpha = modff(readPos, &readIdx);
//            Serial.printf("fReadIndex: %f, integerPart: %f, fractionalPart: %f\n", readPos, readIdx, alpha);

            for (int ch = 0; ch < kNumChannels; ++ch) {
//                bufferToFill[ch][n] = buffer[ch][readIndex];
                bufferToFill[ch][n] = interpolateCubic(buffer[ch], static_cast<uint16_t>(readIdx), alpha);
            }

            readPos += rwDelta <= len ? .9 : 1.1;
        }
    } else {
        isDebugging = false;
        auto initialReadPos{readPos};
        for (int n = 0; n < len; ++n) {
            if (readPos >= kLength) {
                readPos -= kLength;
            }
            if (shouldPrintDebug()) {
                Serial.printf("In loop, readPos: %f (writeIndex: %d)\n", readPos, writeIndex);
            }
            float readIdx{0.f};
            auto alpha = modff(readPos, &readIdx);
            for (int ch = 0; ch < kNumChannels; ++ch) {
                bufferToFill[ch][n] = interpolateCubic(buffer[ch], static_cast<uint16_t>(readIdx),
                                                       alpha);//buffer[ch][readIndex];
            }

            readPos += 1.f;
        }
        ++numBlockReads;
        // Just do a sort of wavetable thing if there haven't yet been any writes.
        if (0 == numBlockWrites) {
            readPos = initialReadPos;
        }
    }

//    if (lastOp == READ) {
//        ++consecutiveOpCount;
//        Serial.printf("CircularBuffer: Extra READ (%d)\n", consecutiveOpCount);
//    } else {
//        consecutiveOpCount = 1;
//    }
//    lastOp = READ;
}

template<typename T>
float CircularBufferMulti<T>::getReadWriteDelta() {
    auto fWrite{static_cast<float>(writeIndex)};
    if (readPos > fWrite) {
        return fWrite + kFloatLength - readPos;
    } else {
        return fWrite - readPos;
    }
}

template<typename T>
bool CircularBufferMulti<T>::shouldPrintDebug() {
    return debugTimer < 100;
}

template<typename T>
T CircularBufferMulti<T>::interpolateCubic(T *channelData, uint16_t readIdx, float alpha) {
//    return channelData[readIdx];
    uint16_t r{readIdx};
    auto rm{r-1}, rp{r+1}, rpp{r+2};
    if (r == 0) {
        rm = kLength - 1;
    } else if (r == kLength - 2) {
        rpp = 0;
    } else if (r == kLength - 1) {
        rp = 0;
        rpp = 1;
    }
    auto val = static_cast<float>(channelData[rm]) * (-alpha * (alpha - 1.f) * (alpha - 2.f) / 6.f)
               + static_cast<float>(channelData[r]) * ((alpha - 1.f) * (alpha + 1.f) * (alpha - 2.f) / 2.f)
               + static_cast<float>(channelData[rp]) * (-alpha * (alpha + 1.f) * (alpha - 2.f) / 2.f)
               + static_cast<float>(channelData[rpp]) * (alpha * (alpha + 1.f) * (alpha - 1.f) / 6.f);
//    if (statTimer == kStatInterval - 1) {
//        Serial.printf("Samples: %d %d %d %d\n", channelData[rm], channelData[r], channelData[rp], channelData[rpp]);
//        Serial.printf("Interpolated value: %f, rounded: %d\n", val, static_cast<T>(round(val)));
//    }
    return static_cast<T>(round(val));
}

template<typename T>
uint16_t CircularBufferMulti<T>::wrapIndex(uint16_t index, uint16_t length) {
    if (index >= length) {
        index -= length;
    } else if (index < 0) {
        index += length;
    }
    return index;
}
