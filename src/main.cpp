#include "main.h"
#include "conf.h"
#include "imgui.h"
#include "zoom.h"
#include <cstddef>
#include <dlfcn.h>
#include <libhat.hpp>
#include <link.h>
#include <span>
#include <string>

Zoom zoom;

extern "C" [[gnu::visibility("default")]] void mod_preinit() {}

uintptr_t mc_base = 0;

extern "C" [[gnu::visibility("default")]] void mod_init() {
  Conf::load();

  auto gw = dlopen("libmcpelauncher_gamewindow.so", 0);
  game_window_is_mouse_locked = (decltype(game_window_is_mouse_locked))dlsym(
      gw, "game_window_is_mouse_locked");
  game_window_get_primary_window =
      (decltype(game_window_get_primary_window))dlsym(
          gw, "game_window_get_primary_window");
  game_window_add_window_creation_callback =
      (decltype(game_window_add_window_creation_callback))dlsym(
          gw, "game_window_add_window_creation_callback");
  game_window_add_keyboard_callback =
      (decltype(game_window_add_keyboard_callback))dlsym(
          gw, "game_window_add_keyboard_callback");
  game_window_add_mouse_scroll_callback =
      (decltype(game_window_add_mouse_scroll_callback))dlsym(
          gw, "game_window_add_mouse_scroll_callback");

  auto mcLib = dlopen("libminecraftpe.so", 0);
  auto callback = [&](const dl_phdr_info &info) {
    if (auto h = dlopen(info.dlpi_name, RTLD_NOLOAD); dlclose(h), h != mcLib)
      return 0;
    mc_base = info.dlpi_addr;
    return 1;
  };

  dl_iterate_phdr(
      [](dl_phdr_info *info, size_t, void *data) {
        return (*static_cast<decltype(callback) *>(data))(*info);
      },
      &callback);

  auto fovFn = reinterpret_cast<float (**)(void *)>(
      reinterpret_cast<uintptr_t>(mc_base) + 0x130F3590);
  CameraAPI_tryGetFOV_orig = *fovFn;
  *fovFn = [](void *t) -> float {
    float fov = CameraAPI_tryGetFOV_orig(t);
    if (fov <= 0.0f)
      return fov;

    void *p1 =
        *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(t) + 0x180);
    if (!p1)
      return fov;
    void *p2 =
        *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(p1) + 0xC8);
    if (!p2)
      return fov;
    float *fovPtr =
        reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(p2) + 0x8);

    zoom.applyFOV(fov, fovPtr);
    return *fovPtr;
  };

  initImgui();
  game_window_add_window_creation_callback(NULL, [](void *user) {
    game_window_add_keyboard_callback(
        game_window_get_primary_window(), NULL,
        [](void *user, int keyCode, int action) -> bool {
          if (Conf::changingKey) {
            if (action == 0) {
              Conf::zoomKey = keyCode;
              showKeyWindow();
            }
            return true;
          }
          if (action != 1) {
            zoom.onKeyboard(keyCode, action);
          }
          return false;
        });
    game_window_add_mouse_scroll_callback(
        game_window_get_primary_window(), NULL,
        [](void *user, double x, double y, double dx, double dy) -> bool {
          return zoom.onMouseScroll(dy);
        });
  });
}
