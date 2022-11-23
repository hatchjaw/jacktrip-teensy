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

    for (auto *reader: readers) {
        reader->releaseResources();
    }

    readers.clear(true);

    tempBuffer.setSize(2, 0);

    isPrepared = false;
}

void MultiChannelAudioSource::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    const ScopedLock sl{lock};

//    jassert(readers.size() >= bufferToFill.buffer->getNumChannels());

    if (!stopped) {
        for (int i = 0; i < readers.size(); ++i) {
            auto writePointer = bufferToFill.buffer->getWritePointer(i);
            // Set up a single-channel temp buffer
            tempBuffer.setDataToReferTo(&writePointer,
                                        1,
                                        bufferToFill.buffer->getNumSamples());
            AudioSourceChannelInfo channelInfo(&tempBuffer, bufferToFill.startSample, bufferToFill.numSamples);
            readers.getUnchecked(i)->getNextAudioBlock(channelInfo);
        }

        if (!playing) {
            // just stopped playing, so fade out the last block...
            for (int i = bufferToFill.buffer->getNumChannels(); --i >= 0;)
                bufferToFill.buffer->applyGainRamp(i, bufferToFill.startSample, jmin(256, bufferToFill.numSamples),
                                                   1.0f, 0.0f);

            if (bufferToFill.numSamples > 256)
                bufferToFill.buffer->clear(bufferToFill.startSample + 256, bufferToFill.numSamples - 256);
        }

        if (readers.getUnchecked(0)->getNextReadPosition() > readers.getUnchecked(0)->getTotalLength() + 1
            && !readers.getUnchecked(0)->isLooping()) {
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
    for (auto *reader: readers) {
        reader->setNextReadPosition(newPosition);
    }
}

int64 MultiChannelAudioSource::getNextReadPosition() const {
    if (readers.isEmpty()) {
        return 0;
    }

    return readers.getUnchecked(0)->getNextReadPosition();
}

int64 MultiChannelAudioSource::getTotalLength() const {
    const ScopedLock sl{lock};

    if (readers.isEmpty()) {
        return 0;
    }

    int64 length{0};
    for (auto *reader: readers) {
        if (reader->getTotalLength() > length) {
            length = reader->getTotalLength();
        }
    }
    return length;
}

bool MultiChannelAudioSource::isLooping() const {
    const ScopedLock sl{lock};

    return !readers.isEmpty() && readers.getUnchecked(0)->isLooping();
}

void MultiChannelAudioSource::addSource(File &file) {
    if (auto *reader = formatManager.createReaderFor(file)) {
        readers.add(new juce::AudioFormatReaderSource(reader, true));
    } else {
        jassertfalse;
    }

//    if (reader != nullptr) {
//        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
//        readerSource->setLooping(true);
//
////                        transport->setSource(newSource.get(), 0, nullptr, reader->sampleRate);
////                        transport->setGain(.5f);
////                        transport->setLooping(true);
////                        transport->prepareToPlay(blockSize, sampleRate);
////                        transport->start();
////                        mixer->addInputSource(transport.get(), false);
//
//        transportSources.push_back(std::make_unique<AudioTransportSource>());
//        auto *transport{transportSources.back().get()};
//        transport->setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
//        transport->setGain(.5f);
//        transport->prepareToPlay(blockSize, sampleRate);
//        transport->start();
//        mixer->addInputSource(transport, false);

//                        transportSources.back()->setSource(newSource.get(), 0, nullptr, reader->sampleRate);
//                        transportSources.back()->prepareToPlay(blockSize, sampleRate);
//                        transportSources.back()->setGain(.5f);
//                        transportSources.back()->setLooping(true);
//                        transportSources.back()->start();
//                        mixer->addInputSource(transportSources.back().get(), false);
//                        readerSource = std::move(newSource);

}

void MultiChannelAudioSource::removeSource(uint index) {
    readers.remove(static_cast<int>(index), true);
}

void MultiChannelAudioSource::start() {
    if (!playing) {
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