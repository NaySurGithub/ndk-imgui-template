#pragma once

#include <jni.h>

struct AInputEvent;
struct android_app;

namespace app {

// Space taken by the system bars, the display cutout and the keyboard, in pixels.
struct Insets {
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

// The few framework calls the NDK has no C API for, made through JNI from the app's thread.
// Only framework classes are used, so the app needs no Java code of its own.
class Jni {
public:
    explicit Jni(android_app* app);
    ~Jni();

    Jni(const Jni&) = delete;
    Jni& operator=(const Jni&) = delete;

    // ANativeActivity_showSoftInput does not show the keyboard on most phones;
    // InputMethodManager does.
    void setKeyboardVisible(bool visible);

    // The character a key event types (0 for none): the NDK only gives key codes.
    [[nodiscard]] char32_t unicodeChar(const AInputEvent* keyEvent);

    [[nodiscard]] Insets safeArea();

private:
    [[nodiscard]] jobject decorView();

    android_app* m_app;
    JNIEnv* m_env = nullptr;
    bool m_keyboardVisible = false;
};

} // namespace app
