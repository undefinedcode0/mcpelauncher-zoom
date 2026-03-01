#include "zoom.h"
#include "conf.h"
#include "main.h"
#include "util.h"

// The hook writes the FOV float directly to offset +0x18 on the object
// returned by CameraAPI vtable[13]. CameraAPI_tryGetFOV_orig returns a
// pointer to that object; we read the current FOV from it, store it as
// lastClientZoom, and overwrite it when zoom is active.

float Zoom::applyFOV(float fov) {
  lastClientZoom = fov;
  if (!Conf::animated) {
    return zoomKeyDown ? zoomLevel : fov;
  }
  if (transition.inProgress() || zoomKeyDown) {
    transition.tick();
    float current = transition.getCurrent();
    if (current > 0.0f)
      return current;
  }
  return fov;
}

bool Zoom::isZooming() { return zoomKeyDown || transition.inProgress(); }

float Zoom::getCurrentFOV() {
  return transition.inProgress() ? transition.getCurrent() : zoomLevel;
}

bool Zoom::onMouseScroll(double dy) {
  if (zoomKeyDown &&
      game_window_is_mouse_locked(game_window_get_primary_window())) {
    if (dy > 0) {
      if (zoomLevel > zoomMin) {
        float newLevel = zoomLevel - zoomStep;
        if (Conf::animated) {
          transition.startTransition(zoomLevel, newLevel, 100);
        }
        zoomLevel = newLevel;
      }
    } else {
      if (zoomLevel < zoomMax) {
        float newLevel = zoomLevel + zoomStep;
        if (Conf::animated) {
          transition.startTransition(zoomLevel, newLevel, 100);
        }
        zoomLevel = newLevel;
      }
    }
    return true;
  }
  return false;
}

void Zoom::onKeyboard(int keyCode, int action) {
  if (keyCode == Conf::zoomKey) {
    float diff = lastClientZoom > zoomLevel ? lastClientZoom - zoomLevel
                                            : zoomLevel - lastClientZoom;
    int duration = clamp(100, (int)(diff / 0.15f), 250);
    switch (action) {
    case 0:
      if (game_window_is_mouse_locked(game_window_get_primary_window()) &&
          !zoomKeyDown) {
        zoomKeyDown = true;
        if (Conf::animated) {
          transition.startTransition(lastClientZoom, zoomLevel, duration);
        }
      }
      break;
    case 2:
      if (zoomKeyDown) {
        zoomKeyDown = false;
        if (Conf::animated) {
          transition.startTransition(zoomLevel, lastClientZoom, duration);
        }
      }
      break;
    default:
      break;
    }
  }
}