# Finds the Android SDK and NDK and selects the NDK's toolchain. Included before project().

set(APP_ANDROID_MIN_SDK 30 CACHE STRING "Minimum supported Android API level")
set(APP_ANDROID_TARGET_SDK 35 CACHE STRING "Target Android API level")
set(APP_ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI")

if(NOT APP_ANDROID_SDK)
    foreach(candidate IN ITEMS
            "$ENV{ANDROID_HOME}"
            "$ENV{ANDROID_SDK_ROOT}"
            "/opt/homebrew/share/android-commandlinetools"
            "$ENV{HOME}/Library/Android/sdk"
            "$ENV{HOME}/Android/Sdk")
        if(candidate AND EXISTS "${candidate}/platforms")
            set(APP_ANDROID_SDK "${candidate}" CACHE PATH "Android SDK root")
            break()
        endif()
    endforeach()
endif()
if(NOT APP_ANDROID_SDK)
    message(FATAL_ERROR "Android SDK not found; set ANDROID_HOME or APP_ANDROID_SDK")
endif()

# The newest installed NDK, unless one is given.
if(NOT APP_ANDROID_NDK)
    file(GLOB ndk_versions LIST_DIRECTORIES true "${APP_ANDROID_SDK}/ndk/*")
    if(NOT ndk_versions)
        message(FATAL_ERROR "No NDK in ${APP_ANDROID_SDK}/ndk; install one with sdkmanager")
    endif()
    list(SORT ndk_versions COMPARE NATURAL ORDER DESCENDING)
    list(GET ndk_versions 0 ndk_latest)
    set(APP_ANDROID_NDK "${ndk_latest}" CACHE PATH "Android NDK root")
endif()

set(CMAKE_TOOLCHAIN_FILE "${APP_ANDROID_NDK}/build/cmake/android.toolchain.cmake")
set(ANDROID_ABI "${APP_ANDROID_ABI}")
set(ANDROID_PLATFORM "android-${APP_ANDROID_MIN_SDK}")
set(ANDROID_STL "c++_static")
set(ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES ON)
