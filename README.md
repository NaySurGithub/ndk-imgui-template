# NDK ImGui template

A minimal Android app in C++23 with [Dear ImGui](https://github.com/ocornut/imgui) on OpenGL ES 3.
No Kotlin, no Java, no Gradle: the activity is the framework's `NativeActivity`, and CMake builds
and signs the APK with the SDK's own tools (`aapt2`, `zipalign`, `apksigner`).

It is the setup of [SlopTube](https://github.com/NaySurGithub/SlopTube), reduced to what every app
needs:

- **Render on demand.** The loop sleeps in `ALooper_pollOnce` until an event arrives and draws a
  few frames after it, so an idle screen costs no CPU or GPU time.
- **Density.** Sizes and the font follow the phone's display density (`AConfiguration_getDensity`),
  with larger touch areas than ImGui's mouse defaults.
- **Safe area.** The UI stays clear of the system bars, the display cutout and the keyboard
  (`WindowInsets` through JNI).
- **Keyboard.** The soft keyboard opens and closes with ImGui's text fields, and typed characters
  reach them (`InputMethodManager` and `KeyEvent.getUnicodeChar` through JNI, since the NDK has
  neither).
- **Lifecycle.** The GL context outlives the window, so going to the background and coming back,
  or rotating the phone, keeps every texture.
- **Font.** Roboto from `/system/fonts`, ImGui's own font if it is missing.

## Requirements

| Tool | Version |
| --- | --- |
| CMake | 3.25+ |
| Ninja | any |
| Android SDK | platform 35, build-tools 35.0.0 |
| Android NDK | r27 or newer (the newest installed is used) |
| JDK | 17 (for `apksigner` and `keytool` only) |

The SDK is found through `ANDROID_HOME`, `ANDROID_SDK_ROOT`, Homebrew's
`android-commandlinetools`, `~/Library/Android/sdk` or `~/Android/Sdk`. The first configure
downloads Dear ImGui.

## Building

```sh
cmake --preset android-release
cmake --build --preset android-release                 # build/android-release/NdkImGui.apk
cmake --build build/android-release --target apk-run   # install and launch over adb
```

`android-debug` builds a debuggable APK. Both are signed with `~/.android/debug.keystore`,
created on first use. To sign with your own key:

```sh
cmake --preset android-release -DAPP_KEYSTORE=/path/to/release.keystore \
  -DAPP_KEYSTORE_PASS=env:KEYSTORE_PASSWORD
```

## Layout

| Path | Contents |
| --- | --- |
| `CMakeLists.txt` | App name, application id and version: change these first |
| `cmake/AndroidToolchain.cmake` | Finds the SDK and NDK, minimum and target API levels, ABI |
| `cmake/AndroidPackaging.cmake` | Turns the library into a signed APK; `apk`, `apk-install`, `apk-run` targets |
| `cmake/Dependencies.cmake` | Dear ImGui (pinned) and `android_native_app_glue` |
| `android/` | Manifest template and resources (theme, placeholder launcher icon) |
| `src/main.cpp` | `android_main`: the event loop |
| `src/App.*` | ImGui setup, input, frames and the example screen (`App::drawUi`) |
| `src/Display.*` | EGL display and GL ES 3 context |
| `src/Jni.*` | Keyboard, typed characters and insets through JNI |

Source files in `src/` are picked up automatically when the build runs again.

## Adding Java

Some Android features only reach Java classes (a foreground service, a media session,
notifications). The way SlopTube does it: set `android:hasCode="true"`, subclass `NativeActivity`
in a small Java file, compile it with `javac` and `d8` from CMake, and add `classes.dex` to the
APK next to the library. See SlopTube's
[`cmake/AndroidPackaging.cmake`](https://github.com/NaySurGithub/SlopTube/blob/main/cmake/AndroidPackaging.cmake).

## Limitations

- arm64 only (`APP_ANDROID_ABI`).
- Keyboards that send whole words at once (`ACTION_MULTIPLE`) or compose text are not handled: a
  `NativeActivity` only receives key events, which covers Latin keyboards typing one character
  at a time.
- ImGui windows scroll with their scrollbar or by dragging it; there is no kinetic scrolling.

## Licence

MIT. Dear ImGui is MIT as well.
