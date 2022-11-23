#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(ValueTree &tree) :
        multiChannelSource(std::make_unique<MultiChannelAudioSource>()),
        valueTree(tree) {

    // TODO: generalise this -- use a compiler flag? Shared with teensies?
    // TODO: use this in MultiChannelAudioSource?
    setAudioChannels(0, 2);

    // Use JACK for output
    // TODO: warn if JACK not found
    auto setup{deviceManager.getAudioDeviceSetup()};
    setup.bufferSize = 32;
    auto &deviceTypes{deviceManager.getAvailableDeviceTypes()};
    for (auto type: deviceTypes) {
        auto typeName{type->getTypeName()};
        if (typeName == "JACK") {
            deviceManager.setCurrentAudioDeviceType(typeName, true);
            type->scanForDevices();
            auto names{type->getDeviceNames()};
            if (names.contains("system", true)) {
                setup.outputDeviceName = "system";
                // Should really check for an error string here.
                auto result{deviceManager.setAudioDeviceSetup(setup, true)};
                break;
            }
        }
    }

    addAndMakeVisible(xyController);
    xyController.onValueChange = [this](uint nodeIndex, Point<float> position) {
        valueTree.setProperty("/source/" + String{nodeIndex} + "/x", position.x, nullptr);
        valueTree.setProperty("/source/" + String{nodeIndex} + "/y", position.y, nullptr);
    };
    xyController.onAddNode = [this] { addSource(); };
    xyController.onRemoveNode = [this](uint nodeIndex) { removeSource(nodeIndex); };

    for (uint i = 0; i < NUM_MODULES; ++i) {
        auto cb{new ComboBox};
        addAndMakeVisible(cb);
        cb->onChange = [this, cb, i] {
            auto ip{cb->getText()};
            DBG("setting property: /module/" + String(i) + ": " + ip);
            valueTree.setProperty("/module/" + String(i), ip, nullptr);
        };
        moduleSelectors.add(cb);
    }

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
    settingsButton.onClick = [this] { showSettings(); };

    addAndMakeVisible(connectToModulesButton);
    connectToModulesButton.setButtonText("Refresh ports");
    connectToModulesButton.onClick = [this] { refreshPorts(); };

    setSize(800, 800);

    refreshPorts();
}

void MainComponent::refreshPorts() {
    jack.connect();
    auto clients{jack.getJackTripClients()};
    // Update the module selector lists.
    for (auto *selector: moduleSelectors) {
        // Cache the current value.
        auto text{selector->getText()};
        // Refresh the list
        selector->clear();
        selector->addItemList(clients, 1);
        // Try to restore the old value.
        for (int i = 0; i < clients.size(); ++i) {
            if (clients[i] == text) {
                selector->setSelectedItemIndex(i);
            }
        }
    }
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
    connectToModulesButton.setBounds(xyController.getRight() - 100, xyController.getY() - 25, 100, 20);
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
//    blockSize = samplesPerBlockExpected;
//    sampleRate = sampleRateReported;
//
//    mixer->prepareToPlay(samplesPerBlockExpected, sampleRateReported);
    multiChannelSource->prepareToPlay(samplesPerBlockExpected, sampleRateReported);
}

void MainComponent::releaseResources() {
//    for (auto &source: transportSources) {
//        source->releaseResources();
//    }
//    mixer->releaseResources();
    multiChannelSource->releaseResources();
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    // TODO: each source occupies a channel; fix the panning accordingly.
//    for (auto &source: transportSources) {
//        if (source->isPlaying()) {
//            source->getNextAudioBlock(bufferToFill);
//        }
//    }
//    const ScopedLock sl{lock};
//    mixer->getNextAudioBlock(bufferToFill);

    multiChannelSource->getNextAudioBlock(bufferToFill);
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
                    multiChannelSource->addSource(file);
                    multiChannelSource->start();
                }
            }
    );
}

void MainComponent::removeSource(uint sourceIndex) {
//    auto &source{transportSources.back()};
//    source->stop();
//    source->releaseResources();
//    transportSources.erase(transportSources.end());

//    auto source{transportSources[sourceIndex].get()};//.end()};
//    mixer->removeInputSource(source);
//    transportSources.erase(sourceIndex);
    multiChannelSource->removeSource(sourceIndex);
}
