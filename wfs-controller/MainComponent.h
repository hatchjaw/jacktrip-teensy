#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <JuceHeader.h>
#include "XYController.h"
#include "JackConnector.h"
#include "MultiChannelAudioSource.h"

#ifndef NUM_AUDIO_SOURCES
#define NUM_AUDIO_SOURCES 2
#endif

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent {
public:
    void prepareToPlay(int samplesPerBlockExpected, double sampleRateReported) override;

    void releaseResources() override;

    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;

    //==============================================================================
    explicit MainComponent(ValueTree &tree);

    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics &) override;

    void resized() override;

private:
    //==============================================================================
    static constexpr uint NUM_MODULES{8};

    void addSource(uint sourceIndex);

    void removeSource(uint sourceIndex);

    void refreshDevicesAndPorts();

    void showSettings();

    juce::TextButton settingsButton;
    SafePointer <DialogWindow> settingsWindow;

    juce::TextButton connectToModulesButton;

    std::unique_ptr<MultiChannelAudioSource> multiChannelSource;

    std::unique_ptr<juce::FileChooser> fileChooser;

    XYController xyController;
    OwnedArray<ComboBox> moduleSelectors;

    JackConnector jack;

    ValueTree &valueTree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
