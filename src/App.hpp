#pragma once

#include "Display.hpp"
#include "Jni.hpp"

#include <array>
#include <chrono>
#include <cstdint>

struct AInputEvent;
struct ANativeWindow;
struct android_app;

namespace app {

// The application: owns Dear ImGui and the GL display, and draws a frame only when something
// changed, so an idle screen costs no CPU or GPU time.
class App {
public:
    explicit App(android_app* native);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void onWindowCreated(ANativeWindow* window);
    void onWindowDestroyed();
    void onConfigChanged();
    void onInsetsChanged();
    std::int32_t onInput(const AInputEvent* event);

    // Draws a frame; returns whether the next one must follow at once.
    bool frame();

    // How long the event loop may sleep without an event (-1: until one comes).
    [[nodiscard]] int idleTimeoutMs() const;

private:
    using Clock = std::chrono::steady_clock;

    void applyScale();
    void drawUi();
    void requestFrames();

    android_app* m_native;
    Display m_display;
    Jni m_jni;
    ANativeWindow* m_window = nullptr;
    float m_scale = 1;
    Insets m_safeArea;
    int m_pendingFrames = 0;
    bool m_keyboardShown = false;
    // Insets and the keyboard reach their final place some time after the event that moves them.
    Clock::time_point m_settleUntil{};

    // State of the example screen.
    bool m_showDemo = false;
    int m_taps = 0;
    float m_value = 0.5f;
    std::array<char, 128> m_text{};
};

} // namespace app
