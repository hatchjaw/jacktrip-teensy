#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <JuceHeader.h>
#include "XYController.h"
#include "JackConnector.h"

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
    const StringArray TEENSY_IPS{
            "192.168.10.182", // 1
            "192.168.10.28",  // 2
            "192.168.10.30",  // 3
            "192.168.10.149", // 4
            "192.168.10.31",  // 5
            "192.168.10.196", // 6
            "192.168.10.126", // 7
            "192.168.10.156", // 8
    };

    juce::TextButton settingsButton;
    SafePointer <DialogWindow> settingsWindow;

    juce::TextButton connectToModulesButton;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::vector<std::unique_ptr<juce::AudioTransportSource>> transportSources;
    std::unique_ptr<juce::MixerAudioSource> mixer;

    std::unique_ptr<juce::FileChooser> fileChooser;

    XYController xyController;
    OwnedArray<ComboBox> moduleSelectors;

    JackConnector jack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)

    void showSettings();

    ValueTree &valueTree;

    void addSource();

    void removeSource();

    int blockSize{0};
    double sampleRate{0.};

    void refreshPorts();
};
