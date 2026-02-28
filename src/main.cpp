#include <dlfcn.h>
#include <link.h>
#include <link.h>
#include <span>
#include <string>
#include "zoom.h"
#include "main.h"
#include "conf.h"
#include "imgui.h"
#include <libhat.hpp>
#include <span>
#include <cstddef>

Zoom zoom;

unsigned long CameraApi_tryGetFOV(void* t) {
    return zoom.CameraAPI_tryGetFOV(t);
}

extern "C" [[gnu::visibility("default")]] void mod_preinit() {}

extern "C" [[gnu::visibility("default")]] void mod_init() {
    Conf::load();

    auto gw = dlopen("libmcpelauncher_gamewindow.so", 0);
    game_window_is_mouse_locked = (decltype(game_window_is_mouse_locked))dlsym(gw, "game_window_is_mouse_locked");
    game_window_get_primary_window = (decltype(game_window_get_primary_window))dlsym(gw, "game_window_get_primary_window");
    game_window_add_window_creation_callback = (decltype(game_window_add_window_creation_callback))dlsym(gw, "game_window_add_window_creation_callback");
    game_window_add_keyboard_callback = (decltype(game_window_add_keyboard_callback))dlsym(gw, "game_window_add_keyboard_callback");
    game_window_add_mouse_scroll_callback = (decltype(game_window_add_mouse_scroll_callback))dlsym(gw, "game_window_add_mouse_scroll_callback");

    auto mcLib = dlopen("libminecraftpe.so", 0);

    std::span<std::byte> range1, range2;
    uintptr_t mc_base = 0;

    auto callback = [&](const dl_phdr_info& info) {
        if(auto h = dlopen(info.dlpi_name, RTLD_NOLOAD); dlclose(h), h != mcLib)
            return 0;
        mc_base = info.dlpi_addr;
        range1 = {reinterpret_cast<std::byte*>(info.dlpi_addr + info.dlpi_phdr[1].p_vaddr), info.dlpi_phdr[1].p_memsz};
        range2 = {reinterpret_cast<std::byte*>(info.dlpi_addr + info.dlpi_phdr[2].p_vaddr), info.dlpi_phdr[2].p_memsz};
        return 1;
    };

    dl_iterate_phdr(
        [](dl_phdr_info* info, size_t, void* data) {
            return (*static_cast<decltype(callback)*>(data))(*info);
        },
        &callback);

    auto CameraAPI_typeinfo_name = hat::find_pattern(range1, hat::object_to_signature("9CameraAPI")).get();
    auto CameraAPI_typeinfo = hat::find_pattern(range2, hat::object_to_signature(CameraAPI_typeinfo_name)).get() - sizeof(void*);
    auto CameraAPI_vtable = hat::find_pattern(range2, hat::object_to_signature(CameraAPI_typeinfo)).get() + sizeof(void*);
    auto CameraAPI_tryGetFOV = reinterpret_cast<unsigned long (**)(void*)>(CameraAPI_vtable) + 13;

    // --- sanity checks ---
    uintptr_t vtable_offset = reinterpret_cast<uintptr_t>(CameraAPI_vtable) - mc_base;
    uintptr_t hook_offset   = reinterpret_cast<uintptr_t>(CameraAPI_tryGetFOV) - mc_base;
    fprintf(stderr, "[zoom] mc_base:              0x%lx\n", mc_base);
    fprintf(stderr, "[zoom] CameraAPI_vtable:     base+0x%lx (expected base+0x13010010)\n", vtable_offset);
    fprintf(stderr, "[zoom] hook target [+13]:    base+0x%lx (expected base+0x13010078)\n", hook_offset);

    // Validate vtable header: vtable[-2] should be 0 (top-offset), vtable[-1] should be typeinfo
    auto vtable_raw = reinterpret_cast<void**>(CameraAPI_vtable);
    void* top_offset  = vtable_raw[-2];
    void* ti_ptr      = vtable_raw[-1];
    fprintf(stderr, "[zoom] vtable[-2] (top-offset, expect 0x0): %p\n", top_offset);
    fprintf(stderr, "[zoom] vtable[-1] (typeinfo ptr):           %p\n", ti_ptr);
    fprintf(stderr, "[zoom] CameraAPI_typeinfo (expected match): %p\n", (void*)CameraAPI_typeinfo);

    if(top_offset != nullptr) {
        fprintf(stderr, "[zoom] ERROR: vtable[-2] is not 0 — wrong vtable location (false match). Aborting hook.\n");
        return;
    }
    if(ti_ptr != (void*)CameraAPI_typeinfo) {
        fprintf(stderr, "[zoom] ERROR: vtable[-1] does not point to CameraAPI typeinfo — wrong vtable location (false match). Aborting hook.\n");
        return;
    }

    fprintf(stderr, "[zoom] vtable looks correct, installing hook at index 13\n");
    // --- end sanity checks ---

    CameraAPI_tryGetFOV_orig = *CameraAPI_tryGetFOV;

    *CameraAPI_tryGetFOV = [](void* t) -> unsigned long {
        unsigned long result = CameraAPI_tryGetFOV_orig(t);
        fprintf(stderr, "[zoom] result as ptr: 0x%lx\n", result);
        for(int i = 0; i < 40; i++) {
            float f = *reinterpret_cast<float*>(result + i * 4);
            if(f > 50.0f && f < 150.0f)
                fprintf(stderr, "[zoom]   [%d] (+0x%x) = %f  <-- CANDIDATE\n", i, i * 4, f);
        }
        return result;
    };

    initImgui();
    game_window_add_window_creation_callback(NULL, [](void* user) {
        game_window_add_keyboard_callback(game_window_get_primary_window(), NULL, [](void* user, int keyCode, int action) -> bool {
            if(Conf::changingKey) {
                if(action == 0) {
                    Conf::zoomKey = keyCode;
                    showKeyWindow();
                }
                return true;
            }
            if(action != 1) {
                zoom.onKeyboard(keyCode, action);
            }
            return false;
        });
        game_window_add_mouse_scroll_callback(game_window_get_primary_window(), NULL, [](void* user, double x, double y, double dx, double dy) -> bool {
            return zoom.onMouseScroll(dy);
        });
    });
}
