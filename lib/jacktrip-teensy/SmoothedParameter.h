//
// Created by tar on 01/12/22.
//

#ifndef JACKTRIP_TEENSY_SMOOTHEDPARAMETER_H
#define JACKTRIP_TEENSY_SMOOTHEDPARAMETER_H

#include <Arduino.h>

template<typename T>
class SmoothedParameter {
public:
    explicit SmoothedParameter(T initialValue);

    void set(T targetValue, bool skipSmoothing = false);

    T getNext();

    T &getCurrent();

private:
    static constexpr T MULTIPLIER{.01f}, THRESHOLD{1e-6};
    T current, target;
};


template
class SmoothedParameter<float>;

#endif //JACKTRIP_TEENSY_SMOOTHEDPARAMETER_H
