//
// Created by tar on 10/11/22.
//

#include "XYController.h"


XYController::XYController() = default;

void XYController::paint(Graphics &g) {
    Component::paint(g);
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);

    for (auto &node: nodes) {
//        g.setColour(juce::Colours::steelblue);
//        g.fillEllipse(position.getX() - NODE_WIDTH / 2,
//                      position.getY() - NODE_WIDTH / 2,
//                      NODE_WIDTH, NODE_WIDTH);
//        node->setBounds(node->position.x * getWidth(), getHeight() - node->position.y * getHeight(), 30, 30);
    }
}

void XYController::resized() {
    for (auto &node: nodes) {
//        node->setBounds(node->position.x * getWidth(), getHeight() - node->position.y * getHeight(), 30, 30);
    }
}

void XYController::mouseDown(const MouseEvent &event) {
    if (event.mods.isPopupMenu()) {
        // Maybe add a node
        PopupMenu m;
        if (event.originalComponent == this) {
            m.addItem(1, "Create source");
            m.showMenuAsync(PopupMenu::Options(), [this, event](int result) {
                if (result == 1) {
                    createNode(event.position - Point<float>(Node::NODE_WIDTH / 2, Node::NODE_WIDTH / 2));
                }
            });
        } else if (auto node = dynamic_cast<Node *>(event.originalComponent)) {
            // Maybe remove a node
            m.addItem(1, "Remove source");
            m.showMenuAsync(PopupMenu::Options(), [this, node](int result) {
                if (result == 1) {
                    removeNode(node);
                }
            });
        }
    }
}

void XYController::mouseDrag(const MouseEvent &event) {
//    if (auto node = dynamic_cast<Node *>(event.originalComponent)) {
//        auto pos = event.position;
//        normalisePosition(pos);
//        node->position = pos;
//        node->setBounds(node->position.x * getWidth(), getHeight() - node->position.y * getHeight(), 30, 30);
//    }

    if (onValueChange != nullptr) {
        onValueChange(event.position);
    }
}

void XYController::createNode(Point<float> position) {
//    normalisePosition(position);
    nodes.push_back(std::make_unique<Node>(position));
    addAndMakeVisible(*nodes.back());
//    nodes.back()->constrainDragPosition();
    // Allow XYController to listen for mouse events on Nodes.
    nodes.back()->addMouseListener(this, true);

    repaint();
}

void XYController::normalisePosition(Point<float> &position) {
    position.x /= static_cast<float>(getWidth());
    position.y = 1 - position.y / static_cast<float>(getHeight());
}

void XYController::removeNode(Component *const node) {
    for (auto it = nodes.begin(); it < nodes.end(); ++it) {
        if (it->get() == node) {
            nodes.erase(it);
        }
    }
}

XYController::Node::Node(Point<float> position) {
    setBounds(position.toInt().x, position.toInt().y, NODE_WIDTH, NODE_WIDTH);
}//:
//        position(normalisedPosition) {}

void XYController::Node::paint(Graphics &g) {
//    g.setColour(juce::Colours::grey);
//    g.drawRect(getLocalBounds(), 1);

    g.setColour(juce::Colours::steelblue);
    g.fillEllipse(getLocalBounds().toFloat());
}

void XYController::Node::resized() {
    Component::resized();
}

void XYController::Node::mouseDown(const MouseEvent &event) {
//    if (event.mods.isPopupMenu()) {
//        // Try to remove this node.
//        PopupMenu m;
//        m.addItem(1, "Remove source");
//        m.showMenuAsync(PopupMenu::Options(), [this, event](int result) {
//            if (result == 1) {
////                createNode(event.position - Point<float>(Node::NODE_WIDTH / 2, Node::NODE_WIDTH / 2));
//            }
//        });
//    }
    dragger.startDraggingComponent(this, event);
}

void XYController::Node::mouseDrag(const MouseEvent &event) {
//    Component::mouseDrag(event);
//    if (event.mods.isLeftButtonDown()) {
//        position = normalisePosition()
//    }
    dragger.dragComponent(this, event, nullptr);
}
