#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(ValueTree &tree) :
        valueTree(tree) {

    setSize(800, 800);

    formatManager.registerBasicFormats();

    addAndMakeVisible(xyController);
    xyController.onValueChange = [this] (uint nodeIndex, Point<float> position) {
        valueTree.setProperty("/source/" + String{nodeIndex} + "/x", position.x, nullptr);
        valueTree.setProperty("/source/" + String{nodeIndex} + "/y", position.y, nullptr);
    };

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
    settingsButton.onClick = [this] { showSettings(); };
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::ghostwhite);
}

void MainComponent::resized() {
    auto bounds{getLocalBounds()};
    auto padding{5}, xyPadX{75}, xyPadY{100};
    xyController.setBounds(xyPadX, xyPadY / 2, bounds.getWidth() - 2 * xyPadX, bounds.getHeight() - 2 * xyPadY);
    settingsButton.setBounds(padding, bounds.getBottom() - padding - 20, 50, 20);
}

void MainComponent::showSettings() {
    DialogWindow::LaunchOptions settings;
    auto deviceSelector = new AudioDeviceSelectorComponent(
            deviceManager,
            0,     // minimum input channels
            256,   // maximum input channels
            0,     // minimum output channels
            256,   // maximum output channels
            false, // ability to select midi inputs
            false, // ability to select midi output device
            false, // treat channels as stereo pairs
            false // hide advanced options
    );

    settings.content.setOwned(deviceSelector);

    Rectangle<int> area(0, 0, 600, 400);

    settings.content->setSize(area.getWidth(), area.getHeight());

    settings.dialogTitle = "Audio Settings";
    settings.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    settings.escapeKeyTriggersCloseButton = true;
    settings.useNativeTitleBar = true;
    settings.resizable = false;

    settingsWindow = settings.launchAsync();

    if (settingsWindow != nullptr)
        settingsWindow->centreWithSize(600, 400);
}

MainComponent::~MainComponent() {
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    transportSource.setGain(1.f);
    transportSource.setLooping(true);
}

void MainComponent::releaseResources() {
    transportSource.releaseResources();
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    if (transportSource.isPlaying()) {
        transportSource.getNextAudioBlock(bufferToFill);
    }
}