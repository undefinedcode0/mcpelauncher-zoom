#include "zoom.h"
#include "conf.h"
#include "main.h"
#include "util.h"

// The hook writes the FOV float directly to offset +0x18 on the object
// returned by CameraAPI vtable[13]. CameraAPI_tryGetFOV_orig returns a
// pointer to that object; we read the current FOV from it, store it as
// lastClientZoom, and overwrite it when zoom is active.

void Zoom::writeFOV(float fov) {
  if (!cachedFovPtr)
    return;
  if (!Conf::animated) {
    *cachedFovPtr = zoomKeyDown ? zoomLevel : fov > 0.0f ? fov : lastClientZoom;
  } else if (transition.inProgress()) {
    transition.tick();
    *cachedFovPtr = transition.getCurrent();
  } else if (zoomKeyDown) {
    *cachedFovPtr = zoomLevel;
  } else {
    *cachedFovPtr = fov > 0.0f ? fov : lastClientZoom;
  }
}

void Zoom::applyFOV(float fov, float *fovPtr) {
  cachedFovPtr = fovPtr;
  if (fov > 0.0f)
    lastClientZoom = fov;
  writeFOV(fov);
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
    int duration = clamp(100, (int)(diff / 0.0015f), 250);
    switch (action) {
    case 0:
      if (game_window_is_mouse_locked(game_window_get_primary_window()) &&
          !zoomKeyDown) {
        zoomKeyDown = true;
        if (Conf::animated) {
          transition.startTransition(lastClientZoom, zoomLevel, duration);
        }
        writeFOV(0); // apply immediately
      }
      break;
    case 2:
      if (zoomKeyDown) {
        zoomKeyDown = false;
        if (Conf::animated) {
          transition.startTransition(zoomLevel, lastClientZoom, duration);
        }
        writeFOV(0); // restore immediately
      }
      break;
    default:
      break;
    }
  }
}