#include "App.hpp"

#include <GLES3/gl3.h>
#include <android/configuration.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>

namespace app {

namespace {

// ImGui settles hover and release states over a couple of frames after an event.
constexpr int kFramesAfterEvent = 3;
constexpr auto kSettleTime = std::chrono::seconds(1);
constexpr int kSettlePollMs = 100;
constexpr int kCaretBlinkMs = 500;
constexpr float kFontSize = 16;     // dp
constexpr float kBaseDensity = 160; // dpi of one dp per pixel.
constexpr ImVec4 kBackground{0.10f, 0.10f, 0.12f, 1};
// Roboto ships on every Android phone; ImGui's own font is a fallback.
constexpr const char* kSystemFont = "/system/fonts/Roboto-Regular.ttf";

} // namespace

App::App(android_app* native)
    : m_native(native)
    , m_jni(native) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // The working directory is not writable on Android.
    std::error_code error;
    if (std::filesystem::exists(kSystemFont, error)) {
        io.Fonts->AddFontFromFileTTF(kSystemFont);
    } else {
        io.Fonts->AddFontDefault();
    }
    ImGui_ImplOpenGL3_Init("#version 300 es");
    applyScale();
    requestFrames();
}

App::~App() {
    ImGui_ImplOpenGL3_Shutdown();
    if (m_window != nullptr) {
        ImGui_ImplAndroid_Shutdown();
    }
    ImGui::DestroyContext();
}

void App::onWindowCreated(ANativeWindow* window) {
    if (m_window != nullptr) {
        ImGui_ImplAndroid_Shutdown();
    }
    m_window = window;
    ImGui_ImplAndroid_Init(window);
    m_display.attach(window);
    onInsetsChanged();
}

void App::onWindowDestroyed() {
    m_display.detach();
    m_jni.setKeyboardVisible(false);
}

void App::onConfigChanged() {
    applyScale(); // The display size setting may have changed.
    onInsetsChanged();
}

void App::onInsetsChanged() {
    m_settleUntil = Clock::now() + kSettleTime;
    requestFrames();
}

std::int32_t App::onInput(const AInputEvent* event) {
    requestFrames();
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        // Unhandled, Back leaves the app as Android expects.
        if (AKeyEvent_getKeyCode(event) == AKEYCODE_BACK) {
            return 0;
        }
        if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {
            // Control characters (Enter, Backspace) arrive as keys through the backend.
            if (const char32_t character = m_jni.unicodeChar(event);
                character >= U' ' && character != U'\x7f') {
                ImGui::GetIO().AddInputCharacter(static_cast<unsigned int>(character));
            }
        }
    }
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

void App::applyScale() {
    const std::int32_t density = AConfiguration_getDensity(m_native->config);
    m_scale = density > 0 && density != ACONFIGURATION_DENSITY_NONE
                  ? static_cast<float>(density) / kBaseDensity
                  : 1;
    ImGuiStyle style;
    ImGui::StyleColorsDark(&style);
    // Fingers are less precise than a mouse.
    style.TouchExtraPadding = {4, 4};
    style.ScrollbarSize = 18;
    style.GrabMinSize = 16;
    style.ScaleAllSizes(m_scale);
    style.FontSizeBase = kFontSize;
    style.FontScaleDpi = m_scale;
    ImGui::GetStyle() = style;
}

void App::requestFrames() {
    m_pendingFrames = kFramesAfterEvent;
}

int App::idleTimeoutMs() const {
    if (Clock::now() < m_settleUntil) {
        return kSettlePollMs;
    }
    return ImGui::GetIO().WantTextInput ? kCaretBlinkMs : -1;
}

bool App::frame() {
    if (!m_display.hasSurface()) {
        return false;
    }
    m_safeArea = m_jni.safeArea();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    drawUi();
    ImGui::Render();

    const ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, static_cast<GLsizei>(io.DisplaySize.x),
               static_cast<GLsizei>(io.DisplaySize.y));
    glClearColor(kBackground.x, kBackground.y, kBackground.z, kBackground.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    m_display.present();

    // The keyboard follows ImGui's text fields; its arrival moves the safe area.
    if (io.WantTextInput != m_keyboardShown) {
        m_keyboardShown = io.WantTextInput;
        m_jni.setKeyboardVisible(m_keyboardShown);
        onInsetsChanged();
    }
    if (m_pendingFrames > 0) {
        --m_pendingFrames;
    }
    return m_pendingFrames > 0;
}

void App::drawUi() {
    // One window filling what the system bars, the cutout and the keyboard leave.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({viewport->Pos.x + m_safeArea.left, viewport->Pos.y + m_safeArea.top});
    ImGui::SetNextWindowSize({viewport->Size.x - m_safeArea.left - m_safeArea.right,
                              viewport->Size.y - m_safeArea.top - m_safeArea.bottom});
    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Main", nullptr, kFlags);

    ImGui::TextUnformatted("Dear ImGui on Android, from C++ alone");
    ImGui::Text("%.0f x %.0f px, %.2f px per dp", viewport->Size.x, viewport->Size.y, m_scale);
    ImGui::Separator();

    if (ImGui::Button("Tap me")) {
        ++m_taps;
    }
    ImGui::SameLine();
    ImGui::Text("%d taps", m_taps);
    ImGui::SliderFloat("Value", &m_value, 0, 1);
    ImGui::InputText("Text", m_text.data(), m_text.size());
    ImGui::Checkbox("Dear ImGui demo", &m_showDemo);

    ImGui::End();
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

} // namespace app
