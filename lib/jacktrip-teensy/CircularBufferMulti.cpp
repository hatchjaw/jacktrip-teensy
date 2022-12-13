//
// Created by tar on 30/11/22.
//

#include "CircularBufferMulti.h"

template<typename T>
CircularBufferMulti<T>::CircularBufferMulti(uint8_t numChannels, uint16_t length) :
        kNumChannels{numChannels},
        kLength{length},
        kFloatLength{static_cast<float>(length)},
        kRwDeltaThresh(AUDIO_BLOCK_SAMPLES, AUDIO_BLOCK_SAMPLES * 2.f),
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
float CircularBufferMulti<T>::getReadPosition() {
    return readPos;
}

template<typename T>
void CircularBufferMulti<T>::clear() {
    for (int ch = 0; ch < kNumChannels; ++ch) {
        for (int s = 0; s < kLength; ++s) {
            buffer[ch][s] = 0;
        }
    }

    readPos = kFloatLength * .5f;
    writeIndex = 0;
    numBlockReads = 0;
    numBlockWrites = 0;
    numSampleWrites = 0;
    numSampleReads = 0;
    readPosAllTime = 0.f;
    readPosIncrement.set(1., true);
}

template<typename T>
void CircularBufferMulti<T>::printStats() {
    if (statTimer > kStatInterval) {
        Serial.printf("\nCircularBuffer: BLOCKS: writes %" PRId64 " %s reads %" PRId64 ", delta %" PRId64 ", ratio %"
                      ".7f\n",
                      numBlockWrites,
                      (numBlockWrites > numBlockReads ? ">" : (numBlockWrites < numBlockReads ? "<" : "==")),
                      numBlockReads,
                      numBlockWrites - numBlockReads,
                      static_cast<float>(numBlockWrites) / static_cast<float>(numBlockReads));

        auto fSampleWrites{static_cast<float>(numSampleWrites)};

        Serial.printf("CircularBuffer: SAMPLES: writes %" PRId64 " %s reads %f, delta %f, ratio %.7f\n",
                      numSampleWrites,
                      (fSampleWrites > readPosAllTime ? ">" : (fSampleWrites < readPosAllTime ? "<" : "==")),
                      readPosAllTime,
                      fSampleWrites - readPosAllTime,
                      fSampleWrites / readPosAllTime);

        Serial.printf("CircularBuffer: writeIndex: %d, readPos: %f, delta %f\n\n",
                      writeIndex, readPos, getReadWriteDelta());
        statTimer = 0;
    }
}

template<typename T>
void CircularBufferMulti<T>::write(const T **data, uint16_t len) {
    for (int n = 0; n < len; ++n, ++writeIndex, ++numSampleWrites) {
        if (writeIndex == kLength) {
            writeIndex = 0;
        }

        for (int ch = 0; ch < kNumChannels; ++ch) {
            buffer[ch][writeIndex] = data[ch][n];
        }
    }

    ++numBlockWrites;
    if (lastOp == WRITE) {
        ++consecutiveOpCount;
//        Serial.printf("CircularBuffer: Extra WRITE (%d)\n", consecutiveOpCount);
    } else {
        consecutiveOpCount = 1;
    }
    lastOp = WRITE;

    setReadPosIncrement();
}

template<typename T>
void CircularBufferMulti<T>::read(T **bufferToFill, uint16_t len) {
    auto initialReadPos{readPos};
//    auto initialReadIncrement{readPosIncrement.getCurrent()};

    for (uint16_t n = 0; n < len; n++) {
        // Wrap readPos.
        if (readPos >= kFloatLength) {
            readPos -= kFloatLength;
        }

        float readIdx{0.f};
        auto alpha = modff(readPos, &readIdx);
        // For each channel, get the next sample, interpolated around readPos.
        for (int ch = 0; ch < kNumChannels; ++ch) {
            bufferToFill[ch][n] = interpolateCubic(buffer[ch], static_cast<uint16_t>(readIdx), alpha);
        }

        // Try to keep read roughly a block behind write.
        // If readPos is more than a block behind writeIndex, there's a risk
        // of write overtaking read, so speed up.
        // If less than a block behind, there's a risk of reading garbage,
        // so slow down.
        auto rwDelta{getReadWriteDelta()};
        auto increment{readPosIncrement.getNext()};

        if (rwDelta < kRwDeltaThresh.first) {
//            readPosIncrement.set(rwDelta / kRwDeltaThresh.first, true);
//            readPosIncrement.set(.9f, true);
            increment = .99f;
//            increment = rwDelta / kRwDeltaThresh.first;

//            if (rwDelta < 10) {
//                increment = rwDelta / kRwDeltaThresh.first;
//            }

//            Serial.printf(
//                    "Read %" PRId64 " < loThresh behind write (readPos: %f, writeIndex: %d, rwDelta: %f, last+: %f)\n",
//                    numSampleReads, readPos, writeIndex, getReadWriteDelta(), increment);// readPosIncrement.getCurrent());
        }
        else if (rwDelta > kRwDeltaThresh.second) {
//            readPosIncrement.set(rwDelta / kRwDeltaThresh.second);
//            readPosIncrement.set(1.5f);
            increment = 1.01f;
//            Serial.printf(
//                    "Read %" PRId64 " > hiThresh behind write (readPos: %f, writeIndex: %d, rwDelta: %f, last+: %f)\n",
//                    numSampleReads, readPos, writeIndex, getReadWriteDelta(), increment);//readPosIncrement.getCurrent());
        }
//        else {
////            auto nextIncrement =
////                    numBlockReads == 0 ? 1.f : static_cast<float>(numBlockWrites) / static_cast<float>(numBlockReads);
////            readPosIncrement.set(nextIncrement);
////            Serial.printf(
////                    "Read %f ok (readPos: %f, writeIndex: %d, rwDelta: %f, last+: %f)\n",
////                    numSampleReads, readPos, writeIndex, getReadWriteDelta(), readPosIncrement.getCurrent());
//        }


//        Serial.printf("readPos increment %f\n", increment);
        readPos += increment;
        readPosAllTime += increment;
        ++numSampleReads;
    }

    // Just do a sort of wavetable thing if there haven't yet been any writes.
    if (0 == numBlockWrites) {
        readPos = initialReadPos;
        readPosAllTime = 0.;
    } else {
        ++numBlockReads;
        ++blocksReadSinceLastUpdate;
    }

    if (lastOp == READ) {
        ++consecutiveOpCount;
//        Serial.printf("CircularBuffer: Extra READ (%d)\n", consecutiveOpCount);
    } else {
        consecutiveOpCount = 1;
    }
    lastOp = READ;
}

template<typename T>
void CircularBufferMulti<T>::setReadPosIncrement() {
//    auto increment{numBlockReads == 0 ? 1.f : static_cast<float>(numBlockWrites) / static_cast<float>(numBlockReads)};
//    readPosIncrement.set(increment);
//    return;
    if (blocksReadSinceLastUpdate > 0 && numBlockWrites % 1000 == 0) {
        auto nextIncrement{1000.f / static_cast<float>(blocksReadSinceLastUpdate)};

        Serial.printf("blocks read per 1000 writes: %d, nextIncrement: %.7f\n", blocksReadSinceLastUpdate,
                      nextIncrement);

        blocksReadSinceLastUpdate = 0;
    }
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
T CircularBufferMulti<T>::interpolateCubic(T *channelData, uint16_t readIdx, float alpha) {
//    return channelData[readIdx];
    int r{readIdx};
    auto rm{r - 1}, rp{r + 1}, rpp{r + 2};
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
