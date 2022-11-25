//
// Created by tar on 23/11/22.
//

#include "MultiChannelAudioSource.h"

MultiChannelAudioSource::MultiChannelAudioSource() {
    formatManager.registerBasicFormats();
}

void MultiChannelAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRateToUse) {
    tempBuffer.setSize(2, samplesPerBlockExpected);

    const ScopedLock sl{lock};

    blockSize = samplesPerBlockExpected;
    sampleRate = sampleRateToUse;

    isPrepared = true;
}

void MultiChannelAudioSource::releaseResources() {
    const ScopedLock sl{lock};

    for (auto *reader: sources) {
        reader->releaseResources();
    }

    sources.clear(true);

    tempBuffer.setSize(2, 0);

    isPrepared = false;
}

void MultiChannelAudioSource::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    const ScopedLock sl{lock};

    if (!sources.isEmpty() && !stopped) {
        for (int i = 0; i < sources.size(); ++i) {
            auto writePointer = bufferToFill.buffer->getWritePointer(i);
            // Set up a single-channel temp buffer
            tempBuffer.setDataToReferTo(&writePointer, 1, bufferToFill.buffer->getNumSamples());
            AudioSourceChannelInfo channelInfo(&tempBuffer, bufferToFill.startSample, bufferToFill.numSamples);
            sources.getUnchecked(i)->getNextAudioBlock(channelInfo);
        }

        if (!playing) {
            DBG("MultiChannelAudioSource: Just stopped playing...");
            // just stopped playing, so fade out the last block...
            for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;)
                bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, jmin(256, bufferToFill.numSamples),
                                                   1.0f, 0.0f);

            if (bufferToFill.numSamples > 256)
                bufferToFill.buffer->clear(bufferToFill.startSample + 256, bufferToFill.numSamples - 256);
        }

        if (!sources.isEmpty()
            && sources.getUnchecked(0)->getNextReadPosition() > sources.getUnchecked(0)->getTotalLength() + 1
            && !sources.getUnchecked(0)->isLooping()) {
            playing = false;
            sendChangeMessage();
        }

        stopped = !playing;

        for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;) {
            bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, bufferToFill.numSamples, lastGain, gain);
        }
    } else {
        bufferToFill.clearActiveBufferRegion();
        stopped = true;
    }

    lastGain = gain;
}

void MultiChannelAudioSource::setNextReadPosition(int64 newPosition) {
    for (auto *reader: sources) {
        reader->setNextReadPosition(newPosition);
    }
}

int64 MultiChannelAudioSource::getNextReadPosition() const {
    if (sources.isEmpty()) {
        return 0;
    }

    return sources.getUnchecked(0)->getNextReadPosition();
}

int64 MultiChannelAudioSource::getTotalLength() const {
    const ScopedLock sl{lock};

    if (sources.isEmpty()) {
        return 0;
    }

    int64 length{0};
    for (auto *reader: sources) {
        if (reader->getTotalLength() > length) {
            length = reader->getTotalLength();
        }
    }
    return length;
}

bool MultiChannelAudioSource::isLooping() const {
    const ScopedLock sl{lock};

    return !sources.isEmpty() && sources.getUnchecked(0)->isLooping();
}

void MultiChannelAudioSource::addSource(File &file) {
    if (auto *source = formatManager.createReaderFor(file)) {
        DBG("MultiChannelAudioSource: Adding source " << sources.size());
        auto *reader = new juce::AudioFormatReaderSource(source, true);
        reader->setLooping(true);
        sources.add(reader);
    } else {
        jassertfalse;
    }
}

void MultiChannelAudioSource::removeSource(uint index) {
    const ScopedLock sl{lock};

    DBG("MultiChannelAudioSource: Removing source " << String(index));

    auto idx = static_cast<int>(index);
    // Come on, mate; release resources *then* remove.
    sources.getUnchecked(idx)->releaseResources();
    sources.remove(idx, true);

    if (sources.isEmpty()) {
        stop();
    }
}

void MultiChannelAudioSource::start() {
    if (!sources.isEmpty() && !playing) {
        {
            const ScopedLock sl{lock};
            playing = true;
            stopped = false;
        }

        sendChangeMessage();
    }
}

void MultiChannelAudioSource::stop() {
    if (playing) {
        playing = false;

        int n = 500;
        while (--n >= 0 && !stopped)
            Thread::sleep(2);

        sendChangeMessage();
    }
}