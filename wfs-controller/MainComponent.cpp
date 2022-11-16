#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(ValueTree &tree) :
        valueTree(tree) {
    addAndMakeVisible(xyController);
    xyController.onValueChange = [this](uint nodeIndex, Point<float> position) {
        valueTree.setProperty("/source/" + String{nodeIndex} + "/x", position.x, nullptr);
        valueTree.setProperty("/source/" + String{nodeIndex} + "/y", position.y, nullptr);
    };
    xyController.onAddNode = [this] { addSource(); };
    xyController.onRemoveNode = [this] { removeSource(); };

    for (uint i = 0; i < NUM_MODULES; ++i) {
        auto cb{new ComboBox};
        addAndMakeVisible(cb);
        cb->addItemList(TEENSY_IPS, 1);
        cb->onChange = [this, i] {
            auto ip{TEENSY_IPS[moduleSelectors[i]->getSelectedId() - 1]};
            valueTree.setProperty("/module/" + String(i), ip, nullptr);
        };
//        LookAndFeel_V4 &lf = cb->getLookAndFeel();
//        cb->setLookAndFeel(lf);
        moduleSelectors.add(cb);
    }

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
    settingsButton.onClick = [this] { showSettings(); };

    setSize(800, 800);

    formatManager.registerBasicFormats();

    setAudioChannels(0, 2);

    // Use JACK for output
    // TODO: warn if JACK not found
    auto setup{deviceManager.getAudioDeviceSetup()};
    setup.bufferSize = 32;
    auto &deviceTypes{deviceManager.getAvailableDeviceTypes()};
    for (auto type: deviceTypes) {
        type->scanForDevices();
        if (type->getDeviceNames().contains("JACK Audio Connection Kit", true)) {
            setup.outputDeviceName = "JACK Audio Connection Kit";
            break;
        }
    }
    deviceManager.setAudioDeviceSetup(setup, true);

    // TODO: Autoconnect to jacktrip clients.
//    auto client =
//    jack_connect();
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
    auto moduleSelectorWidth{static_cast<float>(xyController.getWidth()) / static_cast<float>(NUM_MODULES)};
    for (int i{0}; i < moduleSelectors.size(); ++i) {
        moduleSelectors[i]->setBounds(
                xyPadX + i * moduleSelectorWidth,
                xyController.getBottom() + padding,
                moduleSelectorWidth,
                30
        );
    }
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

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRateReported) {
    blockSize = samplesPerBlockExpected;
    this->sampleRate = sampleRateReported;
//    for (auto &source: transportSources) {
//        source->prepareToPlay(samplesPerBlockExpected, sampleRate);
//        source->setGain(.5f);
//        source->setLooping(true);
//    }
}

void MainComponent::releaseResources() {
    for (auto &source: transportSources) {
        source->releaseResources();
    }
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    // TODO: each source occupies a channel; fix the panning accordingly.
    for (auto &source: transportSources) {
        if (source->isPlaying()) {
            source->getNextAudioBlock(bufferToFill);
        }
    }
}

void MainComponent::addSource() {
    fileChooser = std::make_unique<FileChooser>("Select an audio file",
                                                File("~/Documents"),
                                                "*.wav;*.aiff;*.flac;*.ogg");
    fileChooser->launchAsync(
            FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
            [this](const FileChooser &chooser) {
                auto file = chooser.getResult();
                if (file != File{}) {
                    auto *reader = formatManager.createReaderFor(file);

                    if (reader != nullptr) {
                        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                        transportSources.push_back(std::make_unique<AudioTransportSource>());
                        transportSources.back()->setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                        transportSources.back()->prepareToPlay(blockSize, sampleRate);
                        transportSources.back()->setGain(.5f);
                        transportSources.back()->setLooping(true);
                        transportSources.back()->start();
                        readerSource = std::move(newSource);
                    }
                }
            }
    );
}

void MainComponent::removeSource() {
    auto &source{transportSources.back()};
    source->stop();
    source->releaseResources();
    transportSources.erase(transportSources.end());
}
