// Entry point, called by android_native_app_glue on its own thread: runs the event loop and
// forwards lifecycle and input events to App. The loop sleeps until an event arrives unless the
// app asks for the next frame.

#include "App.hpp"

#include <android/looper.h>
#include <android_native_app_glue.h>

namespace {

void handleCommand(android_app* native, std::int32_t command) {
    auto& app = *static_cast<app::App*>(native->userData);
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        app.onWindowCreated(native->window);
        break;
    case APP_CMD_TERM_WINDOW:
        app.onWindowDestroyed();
        break;
    case APP_CMD_CONFIG_CHANGED:
        app.onConfigChanged();
        break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONTENT_RECT_CHANGED:
        app.onInsetsChanged();
        break;
    default:
        break;
    }
}

std::int32_t handleInput(android_app* native, AInputEvent* event) {
    return static_cast<app::App*>(native->userData)->onInput(event);
}

} // namespace

void android_main(android_app* native) {
    app::App app(native);
    native->userData = &app;
    native->onAppCmd = handleCommand;
    native->onInputEvent = handleInput;

    bool animating = true;
    while (native->destroyRequested == 0) {
        int timeout = animating ? 0 : app.idleTimeoutMs();
        while (true) {
            android_poll_source* source = nullptr;
            const int result =
                ALooper_pollOnce(timeout, nullptr, nullptr, reinterpret_cast<void**>(&source));
            if (source != nullptr) {
                source->process(native, source);
            }
            if (result == ALOOPER_POLL_TIMEOUT || result == ALOOPER_POLL_ERROR ||
                native->destroyRequested != 0) {
                break;
            }
            timeout = 0; // Drain everything already queued before drawing.
        }
        if (native->destroyRequested == 0) {
            animating = app.frame();
        }
    }
    native->userData = nullptr;
}
