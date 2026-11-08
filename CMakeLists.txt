cmake_minimum_required(VERSION 3.25)

# Selects the Android NDK toolchain before project().
include(cmake/AndroidToolchain.cmake)

project(NdkImGui
    VERSION 0.1.0
    DESCRIPTION "Dear ImGui on Android with the NDK alone"
    LANGUAGES C CXX)

# The app's identity, used by the manifest and the APK's name. Change these first.
set(APP_NAME "NDK ImGui")
set(APP_ID "com.example.ndkimgui")
set(APP_APK_NAME "NdkImGui")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerOptions)
include(Dependencies)
include(AndroidPackaging)

file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(app SHARED ${app_sources})
target_include_directories(app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(app PRIVATE app::options native_app_glue imgui android log)
# android_main is looked up by name, and the glue's entry point must not be stripped.
target_link_options(app PRIVATE "-u" "ANativeActivity_onCreate")

app_add_apk(app)
