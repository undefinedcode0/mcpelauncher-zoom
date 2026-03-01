#pragma once
#include "transition.h"
#include <stdlib.h>

class Zoom {
private:
  bool zoomKeyDown = false;
  float zoomLevel = 0.35f;     // ~20° in radians
  float zoomMin = 0.20f;       // ~11°
  float zoomMax = 0.70f;       // ~40°
  float zoomStep = 0.05f;      // scroll step
  float lastClientZoom = 0.0f; // last FOV reported by the game

  Transition transition;

public:
  float applyFOV(float fov);
  bool onMouseScroll(double dy);
  void onKeyboard(int keyCode, int action);

  bool isZooming();
  float getCurrentFOV();
};