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

    std::function<void(uint nodeIndex, Point<float>)> onValueChange;

    void setNormalisableRanges(NormalisableRange<double> xRange, NormalisableRange<double> yRange);

protected:
    struct Axis {
        NormalisableRange<double> range;
    };

    class Node : public Component {
    private:
        struct Value {
            float x, y;
        };
    public:
        explicit Node(Value val);

        void paint(Graphics &g) override;

        void resized() override;

        void mouseDown(const MouseEvent &event) override;

        void mouseDrag(const MouseEvent &event) override;

        std::function<void()> onMove;

//        void constrainDragPosition();

//        Point<float> position;
    private:
//        struct Value {
//            float x, y;
//        };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
        static constexpr float NODE_WIDTH{50.f};

//        ComponentDragger dragger;
        Value value{.5, .5};

        friend class XYController;

        Point<int> mouseDownWithinTarget;
        std::unique_ptr<ComponentBoundsConstrainer> constrainer;

        float clamp(float val, float min, float max);
    };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XYController)

    std::vector<std::unique_ptr<Node>> nodes;

    void createNode(Point<float> value);

    void normalisePosition(Point<float> &position);

    void removeNode(Component *node);

//    ComponentBoundsConstrainer *constrainer;

    Axis X, Y;
};


#endif //JACKTRIP_TEENSY_XYCONTROLLER_H
