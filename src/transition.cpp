#include "transition.h"
#include "util.h"

void Transition::startTransition(float startVal, float endVal, int durationMs) {
    if(transitionInProgress) {
        start = current;
    } else {
        start = startVal;
    }
    end = endVal;
    transitionInProgress = true;
    startTime = getEpochTime();
    endTime = startTime + durationMs;
}

void Transition::tick() {
    if(transitionInProgress) {
        int64_t time = getEpochTime();
        int64_t timeSinceStart = time - startTime;
        double currentProgress = (double)timeSinceStart / (double)(endTime - startTime);
        if(currentProgress >= 1.0) {
            current = end;
            transitionInProgress = false;
            return;
        }
        current = start + (end - start) * (float)currentProgress;
    }
}