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
                    // Find position centered on the location of the click.
//                    auto position = event.position - Point<float>(Node::NODE_WIDTH / 2, Node::NODE_WIDTH / 2);
                    createNode(event.position);
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

//    if (event.mods.isLeftButtonDown() && onValueChange != nullptr) {
//        onValueChange(0, event.position);
//    }
}

void XYController::createNode(Point<float> position) {
//    normalisePosition(position);

    // Normalise the position to get the value.
    Node::Value val{position.x / getWidth(), 1 - position.y / getHeight()};
    position -= Point<float>(Node::NODE_WIDTH / 2, Node::NODE_WIDTH / 2);

    nodes.push_back(std::make_unique<Node>(val));
    addAndMakeVisible(*nodes.back());
//    nodes.back()->constrainDragPosition();
    // Allow XYController to listen for mouse events on Nodes.
    auto node = nodes.back().get();
    auto bounds = getBounds().toFloat();
    node->setBounds((bounds.getWidth() * val.x) - Node::NODE_WIDTH,
                    (bounds.getHeight() - bounds.getHeight() * val.y) - Node::NODE_WIDTH / 2,
                    Node::NODE_WIDTH,
                    Node::NODE_WIDTH);
    node->addMouseListener(this, true);

    node->onMove = [this, node]() {
        // Set node bounds here
        auto bounds = getBounds().toFloat();
        node->setBounds((bounds.getWidth() * node->value.x) - Node::NODE_WIDTH / 2,
                        (bounds.getHeight() - bounds.getHeight() * node->value.y) - Node::NODE_WIDTH / 2,
                        Node::NODE_WIDTH,
                        Node::NODE_WIDTH);
        // And do value change callback
        if (onValueChange != nullptr) {
            onValueChange(0, {node->value.x, node->value.y});
        }
    };

    if (onValueChange != nullptr) {
        onValueChange(0, {node->value.x, node->value.y});
    }

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

void XYController::setNormalisableRanges(NormalisableRange<double> xRange, NormalisableRange<double> yRange) {
    X.range = xRange;
    Y.range = yRange;
}

XYController::Node::Node(Value val) {
    value = val;
//    setBounds(position.toInt().x, position.toInt().y, NODE_WIDTH, NODE_WIDTH);
//    constrainer = std::make_unique<ComponentBoundsConstrainer>();
}//:
//        position(normalisedPosition) {}

void XYController::Node::paint(Graphics &g) {
//    g.setColour(juce::Colours::grey);
//    g.drawRect(getLocalBounds(), 1);

    g.setColour(juce::Colours::steelblue);
    g.fillEllipse(getLocalBounds().toFloat());
    g.setColour(juce::Colours::steelblue.darker(.25));
    g.drawEllipse(getLocalBounds().withSizeKeepingCentre(getWidth() - 2, getHeight() - 2).toFloat(), 2.f);
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
//    dragger.startDraggingComponent(this, event);

//    if (event.mods.isLeftButtonDown()) {
//        mouseDownWithinTarget = event.getEventRelativeTo(this).getMouseDownPosition();
//    }
}

void XYController::Node::mouseDrag(const MouseEvent &event) {
//    Component::mouseDrag(event);
    if (event.mods.isLeftButtonDown()) {
        auto parent = getParentComponent();
        auto parentBounds = parent->getBounds().toFloat();

        auto parentBottomRight = Point<float>{parentBounds.getWidth(), parentBounds.getHeight()};

        auto newVal = event.getEventRelativeTo(parent).position / parentBottomRight;
        // Set value here.
        value.x = clamp(newVal.x, 0., 1.);
        value.y = clamp(1 - newVal.y, 0., 1.);
        if (onMove != nullptr) {
            onMove();
        }
//        setBounds(event.position.toInt().x, event.position.toInt().y, NODE_WIDTH, NODE_WIDTH);
//        repaint();

//        auto bounds = getBounds();
//
//        bounds += event.getEventRelativeTo(this).getPosition() - mouseDownWithinTarget;
//
//        if (constrainer != nullptr)
//            constrainer->setBoundsForComponent(this, bounds, false, false, false, false);
//        else
//            setBounds(bounds);
    }
//    dragger.dragComponent(this, event, nullptr);


}

float XYController::Node::clamp(float val, float min, float max) {
    if (val > max) {
        val = max;
    } else if (val < min) {
        val = min;
    }
    return val;
}
