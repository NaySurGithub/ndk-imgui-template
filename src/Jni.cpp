#include "Jni.hpp"

#include <android/input.h>
#include <android_native_app_glue.h>

namespace app {

namespace {

// Deletes the local references made in its scope: the app's thread never returns to Java, so
// they would otherwise pile up.
class LocalFrame {
public:
    explicit LocalFrame(JNIEnv* env, jint capacity = 16)
        : m_env(env) {
        m_env->PushLocalFrame(capacity);
    }

    ~LocalFrame() { m_env->PopLocalFrame(nullptr); }

    LocalFrame(const LocalFrame&) = delete;
    LocalFrame& operator=(const LocalFrame&) = delete;

private:
    JNIEnv* m_env;
};

jmethodID method(JNIEnv* env, jobject object, const char* name, const char* signature) {
    return env->GetMethodID(env->GetObjectClass(object), name, signature);
}

} // namespace

Jni::Jni(android_app* app)
    : m_app(app) {
    m_app->activity->vm->AttachCurrentThread(&m_env, nullptr);
}

Jni::~Jni() {
    m_app->activity->vm->DetachCurrentThread();
}

jobject Jni::decorView() {
    jobject activity = m_app->activity->clazz;
    jobject window = m_env->CallObjectMethod(
        activity, method(m_env, activity, "getWindow", "()Landroid/view/Window;"));
    return m_env->CallObjectMethod(window,
                                   method(m_env, window, "getDecorView", "()Landroid/view/View;"));
}

void Jni::setKeyboardVisible(bool visible) {
    if (visible == m_keyboardVisible) {
        return;
    }
    m_keyboardVisible = visible;

    const LocalFrame frame(m_env);
    jobject activity = m_app->activity->clazz;
    jstring service = m_env->NewStringUTF("input_method");
    jobject manager = m_env->CallObjectMethod(
        activity,
        method(m_env, activity, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;"),
        service);
    jobject view = decorView();
    if (visible) {
        m_env->CallBooleanMethod(
            manager, method(m_env, manager, "showSoftInput", "(Landroid/view/View;I)Z"), view, 0);
    } else {
        jobject token = m_env->CallObjectMethod(
            view, method(m_env, view, "getWindowToken", "()Landroid/os/IBinder;"));
        m_env->CallBooleanMethod(
            manager, method(m_env, manager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z"),
            token, 0);
    }
}

char32_t Jni::unicodeChar(const AInputEvent* keyEvent) {
    const LocalFrame frame(m_env);
    jclass type = m_env->FindClass("android/view/KeyEvent");
    jmethodID constructor = m_env->GetMethodID(type, "<init>", "(JJIIII)V");
    jobject event =
        m_env->NewObject(type, constructor, static_cast<jlong>(AKeyEvent_getDownTime(keyEvent)),
                         static_cast<jlong>(AKeyEvent_getEventTime(keyEvent)),
                         AKeyEvent_getAction(keyEvent), AKeyEvent_getKeyCode(keyEvent),
                         AKeyEvent_getRepeatCount(keyEvent), AKeyEvent_getMetaState(keyEvent));
    const jint character =
        m_env->CallIntMethod(event, m_env->GetMethodID(type, "getUnicodeChar", "(I)I"),
                             AKeyEvent_getMetaState(keyEvent));
    return static_cast<char32_t>(character);
}

Insets Jni::safeArea() {
    const LocalFrame frame(m_env);
    jobject view = decorView();
    jobject windowInsets = m_env->CallObjectMethod(
        view, method(m_env, view, "getRootWindowInsets", "()Landroid/view/WindowInsets;"));
    if (windowInsets == nullptr) {
        return {}; // Not attached yet.
    }
    jclass typeClass = m_env->FindClass("android/view/WindowInsets$Type");
    const auto type = [&](const char* name) {
        return m_env->CallStaticIntMethod(typeClass,
                                          m_env->GetStaticMethodID(typeClass, name, "()I"));
    };
    jobject insets = m_env->CallObjectMethod(
        windowInsets, method(m_env, windowInsets, "getInsets", "(I)Landroid/graphics/Insets;"),
        type("systemBars") | type("displayCutout") | type("ime"));
    jclass insetsClass = m_env->GetObjectClass(insets);
    const auto side = [&](const char* name) {
        return static_cast<float>(
            m_env->GetIntField(insets, m_env->GetFieldID(insetsClass, name, "I")));
    };
    return {
        .left = side("left"), .top = side("top"), .right = side("right"), .bottom = side("bottom")};
}

} // namespace app
