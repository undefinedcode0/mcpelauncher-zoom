#pragma once
#include "transition.h"
#include <stdlib.h>

class Zoom {
private:
  bool zoomKeyDown = false;
  float zoomLevel = 30.0f;     // target zoom FOV in degrees (vertical)
  float zoomMin = 20.0f;       // minimum zoom (most zoomed in)
  float zoomMax = 40.0f;       // maximum zoom (least zoomed in)
  float zoomStep = 2.0f;       // scroll step in degrees
  float lastClientZoom = 0.0f; // last FOV reported by the game

  Transition transition;

public:
  void applyFOV(unsigned long result);
  bool onMouseScroll(double dy);
  void onKeyboard(int keyCode, int action);

  bool isZooming();
  float getCurrentFOV();
};