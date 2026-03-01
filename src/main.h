#pragma once
#include <cstdint>

inline bool (*game_window_is_mouse_locked)(void *window);
inline void (*game_window_add_window_creation_callback)(
    void *user, void (*callback)(void *user));
inline void *(*game_window_get_primary_window)();
inline void (*game_window_add_keyboard_callback)(void *window, void *user,
                                                 bool (*callback)(void *user,
                                                                  int keyCode,
                                                                  int action));
inline void (*game_window_add_mouse_scroll_callback)(
    void *window, void *user,
    bool (*callback)(void *user, double x, double y, double dx, double dy));

struct FovResult {
  uint64_t rax;
  uint64_t rdx;
};
inline FovResult (*CameraAPI_tryGetFOV_orig)(void *);