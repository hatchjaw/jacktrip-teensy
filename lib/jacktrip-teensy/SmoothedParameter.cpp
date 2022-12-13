//
// Created by tar on 01/12/22.
//

#include "SmoothedParameter.h"

template<typename T>
SmoothedParameter<T>::SmoothedParameter(T initialValue) {
    target = initialValue;
    current = target;
}

template<typename T>
void SmoothedParameter<T>::set(T targetValue, bool skipSmoothing) {
    target = targetValue;
    if (skipSmoothing) {
        current = target;
    }
}

template<typename T>
T SmoothedParameter<T>::getNext() {
    if (current != target) {
        auto delta = target - current;
        auto absDelta = abs(delta);
        if (absDelta < THRESHOLD) {
            current = target;
        } else {
            current += MULTIPLIER * delta;
        }
    }

    return current;
}

template<typename T>
T &SmoothedParameter<T>::getCurrent() {
    return current;
}
