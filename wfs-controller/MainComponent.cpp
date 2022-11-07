#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
    setSize(800, 800);

    formatManager.registerBasicFormats();

    addAndMakeVisible(sliderX);
    sliderX.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    sliderX.setValue(.5f);
    sliderX.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    sliderX.setNormalisableRange({0., 1., .01});
    sliderX.onValueChange = [this] {
        sendOSC();
        repaint();
    };

    addAndMakeVisible(sliderY);
    sliderY.setSliderStyle(Slider::SliderStyle::LinearVertical);
    sliderY.setValue(.5f);
    sliderY.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    sliderY.setNormalisableRange({0., 1., .01});
    sliderY.onValueChange = [this] {
        sendOSC();
        repaint();
    };

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
    settingsButton.onClick = [this] { showSettings(); };

    // Prepare to send OSC messages over UDP multicast.
    socket = std::make_unique<DatagramSocket>();
    // Got to bind to the local address of the appropriate network interface.
    // TODO: make these specifiable via the UI
    socket->bindToPort(8888, "192.168.10.10");
    // TODO: also make multicast IP and port specifiable via the UI.
    osc.connectToSocket(*socket, "230.0.0.20", 41814);
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
    auto bounds{getBounds()};
    auto padding{5}, markerWidth{20};
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::aliceblue);
    g.fillEllipse(padding + (bounds.getWidth() - 2 * padding - markerWidth) * sliderX.getValue(),
                  padding + (bounds.getHeight() - 40 - padding - markerWidth / 2) * (1 - sliderY.getValue()),
                  markerWidth, markerWidth);
}

void MainComponent::resized() {
    auto bounds{getLocalBounds()};
    auto padding{5};
    sliderX.setBounds(padding, bounds.getBottom() - padding - 40, bounds.getWidth() - 2 * padding, 20);
    sliderY.setBounds(padding, padding, 20, bounds.getHeight() - 30 - padding);
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
    osc.disconnect();
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

void MainComponent::sendOSC() {
    OSCBundle bundle;
    bundle.addElement(OSCMessage{"/wfs/pos/0", (float)sliderX.getValue()});
    bundle.addElement(OSCMessage{"/wfs/pos/1", (float)sliderY.getValue()});
    osc.send(bundle);
}