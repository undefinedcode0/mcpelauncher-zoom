#pragma once

class Transition {
private:
    float start;
    float end;
    long startTime;
    long endTime;
    float current;
    bool transitionInProgress;

public:
    void startTransition(float startVal, float endVal, int duration);
    void tick();
    float getCurrent() {
        return current;
    }
    bool inProgress() {
        return transitionInProgress;
    }
};