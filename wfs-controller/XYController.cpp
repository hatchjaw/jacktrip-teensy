//
// Created by tar on 10/11/22.
//

#include "XYController.h"


XYController::XYController() = default;

void XYController::paint(Graphics &g) {
    Component::paint(g);
    g.fillAll(Colours::whitesmoke);

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), 1);

    // Nodes are automatically painted, as they are added as child components.
}

void XYController::resized() {
    for (auto &node: nodes) {
        node->setBounds();
    }
}

void XYController::mouseDown(const MouseEvent &event) {
    if (event.mods.isPopupMenu()) {
        // Maybe add a node
        PopupMenu m;
        if (event.originalComponent == this) {
            m.addItem(1, "Create node");
            m.showMenuAsync(PopupMenu::Options(), [this, event](int result) {
                if (result == 1) {
                    // Find position centered on the location of the click.
                    createNode(event.position);
                }
            });
        }
    }
}

void XYController::createNode(Point<float> position) {
    // Normalise the position to get the value.
    auto bounds{getBounds().toFloat()};
    Node::Value value{position.x / bounds.getWidth(), 1 - position.y / bounds.getHeight()};

    nodes.push_back(std::make_unique<Node>(value));
    addAndMakeVisible(*nodes.back());
    auto node{nodes.back().get()};
    node->setBounds();

    node->onMove = [this](Node *nodeBeingMoved) {
        // Set node bounds here
        nodeBeingMoved->setBounds();
        // And do value change callback
        if (onValueChange != nullptr) {
            // THIS IS APPALLING.
            uint i{0};
            for (auto it = nodes.begin(); it < nodes.end(); ++it, ++i) {
                if (it->get() == nodeBeingMoved) {
                    onValueChange(i, {nodeBeingMoved->value.x, nodeBeingMoved->value.y});
                    return;
                }
            }
        }
    };

    node->onRemove = [this](Node *nodeToRemove) {
        removeNode(nodeToRemove);
    };

    if (onValueChange != nullptr) {
        onValueChange(nodes.size() - 1, {node->value.x, node->value.y});
    }

    if (onAddNode != nullptr) {
        onAddNode();
    }

    repaint(node->getBounds());
}

void XYController::normalisePosition(Point<float> &position) {
    position.x /= static_cast<float>(getWidth());
    position.y = 1 - position.y / static_cast<float>(getHeight());
}

void XYController::removeNode(Component *const node) {
    for (auto it = nodes.begin(); it < nodes.end(); ++it) {
        if (it->get() == node) {
            nodes.erase(it);
            if (onRemoveNode != nullptr) {
                onRemoveNode();
            }
            return;
        }
    }
}

XYController::Node::Node(Value val) : value(val) {}

void XYController::Node::paint(Graphics &g) {
//    g.setColour(juce::Colours::grey);
//    g.drawRect(getLocalBounds(), 1);

    g.setColour(juce::Colours::steelblue);
    g.fillEllipse(getLocalBounds().toFloat());
    g.setColour(juce::Colours::steelblue.darker(.25));
    g.drawEllipse(getLocalBounds().withSizeKeepingCentre(getWidth() - 2, getHeight() - 2).toFloat(), 2.f);
    g.setColour(Colours::white);
    g.drawFittedText(String(value.x, 2) + newLine + String(value.y, 2), getLocalBounds(), Justification::centred, 2);
}

void XYController::Node::mouseDown(const MouseEvent &event) {
    if (event.mods.isPopupMenu()) {
        // Try to remove this node.
        PopupMenu m;
        m.addItem(1, "Remove node");
        m.showMenuAsync(PopupMenu::Options(), [this, event](int result) {
            if (result == 1 && onRemove != nullptr) {
                onRemove(this);
            }
        });
    }
}

void XYController::Node::mouseDrag(const MouseEvent &event) {
    if (event.mods.isLeftButtonDown()) {
        auto parent{getParentComponent()};
        auto parentBounds{parent->getBounds().toFloat()};

        auto parentBottomRight{Point<float>{parentBounds.getWidth(), parentBounds.getHeight()}};

        auto newVal{event.getEventRelativeTo(parent).position / parentBottomRight};

        // Set node value.
        value.x = clamp(newVal.x, 0., 1.);
        value.y = clamp(1 - newVal.y, 0., 1.);
        if (onMove != nullptr) {
            onMove(this);
        }
    }
}

float XYController::Node::clamp(float val, float min, float max) {
    if (val >= max) {
        val = max;
    } else if (val <= min) {
        val = min;
    }
    return val;
}

void XYController::Node::setBounds() {
    auto bounds{getParentComponent()->getBounds()};
    Component::setBounds(bounds.getWidth() * value.x - NODE_WIDTH / 2,
                         (bounds.getHeight() - bounds.getHeight() * value.y) - NODE_WIDTH / 2,
                         Node::NODE_WIDTH,
                         Node::NODE_WIDTH);
}
