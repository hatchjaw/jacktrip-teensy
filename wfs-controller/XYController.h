//
// Created by tar on 10/11/22.
//

#ifndef JACKTRIP_TEENSY_XYCONTROLLER_H
#define JACKTRIP_TEENSY_XYCONTROLLER_H

#include <JuceHeader.h>

class XYController : public Component {
public:
    explicit XYController();

    void paint(Graphics &g) override;

    void mouseDrag(const MouseEvent &event) override;

    void mouseDown(const MouseEvent &event) override;

    void resized() override;

    std::function<void(Point<float>)> onValueChange;

protected:
    class Node : public Component {
    public:
        explicit Node(Point<float> position);

        void paint(Graphics &g) override;

        void resized() override;

        void mouseDown(const MouseEvent &event) override;

        void mouseDrag(const MouseEvent &event) override;

//        void constrainDragPosition();

//        Point<float> position;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
        static constexpr float NODE_WIDTH{30.f};

//        std::unique_ptr<ParameterAttachment> positionAttachment;

        ComponentDragger dragger;

        friend class XYController;
    };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XYController)

    std::vector<std::unique_ptr<Node>> nodes;

    void createNode(Point<float> position);

    void normalisePosition(Point<float> &position);

    void removeNode(Component *node);

    ComponentBoundsConstrainer* constrainer;
};


#endif //JACKTRIP_TEENSY_XYCONTROLLER_H
