# Packages the native library into a signed APK without Gradle: aapt2 compiles the resources and
# links the manifest, the library is stored uncompressed and page-aligned so Android can map it
# straight from the APK, then apksigner signs it.
#
# Targets created: apk, apk-install, apk-run.

set(APP_BUILD_TOOLS_VERSION "35.0.0" CACHE STRING "Android build-tools version")
set(debug_keystore "$ENV{HOME}/.android/debug.keystore")
set(APP_KEYSTORE "${debug_keystore}" CACHE FILEPATH "APK signing keystore")
# apksigner password spec: "pass:<password>", "env:<variable>" or "file:<path>". Read a release
# password from the environment so it never lands in the build files.
set(APP_KEYSTORE_PASS "pass:android" CACHE STRING "APK signing keystore password (apksigner spec)")
set(APP_KEY_ALIAS "" CACHE STRING "Key alias in the keystore (empty: its only key)")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(debuggable_default ON)
else()
    set(debuggable_default OFF)
endif()
option(APP_DEBUGGABLE "Mark the APK debuggable" ${debuggable_default})
set(debug_mode_flag)
if(APP_DEBUGGABLE)
    set(debug_mode_flag --debug-mode)
endif()

set(build_tools "${APP_ANDROID_SDK}/build-tools/${APP_BUILD_TOOLS_VERSION}")
set(platform_jar "${APP_ANDROID_SDK}/platforms/android-${APP_ANDROID_TARGET_SDK}/android.jar")
foreach(required IN ITEMS "${build_tools}/aapt2" "${platform_jar}")
    if(NOT EXISTS "${required}")
        message(FATAL_ERROR "Missing Android SDK component: ${required}")
    endif()
endforeach()

find_program(APP_ADB adb HINTS "${APP_ANDROID_SDK}/platform-tools")
find_program(APP_ZIP zip REQUIRED)
find_program(APP_JAVA java
    HINTS "$ENV{JAVA_HOME}/bin" "/opt/homebrew/opt/openjdk@17/bin" "/opt/homebrew/opt/openjdk/bin")
if(NOT APP_JAVA)
    message(FATAL_ERROR "A Java runtime is required by apksigner; set JAVA_HOME")
endif()
cmake_path(GET APP_JAVA PARENT_PATH java_bin)

# versionCode from the project version: MAJOR*10000 + MINOR*100 + PATCH.
math(EXPR APP_VERSION_CODE
    "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")

function(app_add_apk library)
    set(APP_LIB_NAME "${library}")
    set(work "${CMAKE_CURRENT_BINARY_DIR}/apk")
    set(manifest "${work}/AndroidManifest.xml")
    set(resources "${work}/resources.zip")
    set(unsigned "${work}/unsigned.apk")
    set(aligned "${work}/aligned.apk")
    set(apk "${CMAKE_BINARY_DIR}/${APP_APK_NAME}.apk")
    set(lib_dir "${work}/lib/${ANDROID_ABI}")

    configure_file("${PROJECT_SOURCE_DIR}/android/AndroidManifest.xml.in" "${manifest}" @ONLY)
    file(GLOB_RECURSE resource_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/android/res/*")

    # A debug keystore is made on first use, as Android Studio would.
    if(NOT EXISTS "${APP_KEYSTORE}")
        if(NOT APP_KEYSTORE STREQUAL debug_keystore)
            message(FATAL_ERROR "Signing keystore not found: ${APP_KEYSTORE}")
        endif()
        cmake_path(GET APP_KEYSTORE PARENT_PATH keystore_dir)
        file(MAKE_DIRECTORY "${keystore_dir}")
        execute_process(
            COMMAND "${java_bin}/keytool" -genkeypair -keystore "${APP_KEYSTORE}"
                -storepass android -keypass android -alias androiddebugkey -keyalg RSA
                -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
    set(alias_flag)
    if(APP_KEY_ALIAS)
        set(alias_flag --ks-key-alias "${APP_KEY_ALIAS}")
    endif()

    add_custom_command(
        OUTPUT "${apk}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${work}/lib" "${resources}" "${unsigned}" "${aligned}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${lib_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${library}>" "${lib_dir}/"
        COMMAND "${build_tools}/aapt2" compile --dir "${PROJECT_SOURCE_DIR}/android/res"
            -o "${resources}"
        COMMAND "${build_tools}/aapt2" link -o "${unsigned}" --manifest "${manifest}"
            -I "${platform_jar}" ${debug_mode_flag} "${resources}"
        # Stored, not compressed, so the library is mapped from the APK (extractNativeLibs=false).
        COMMAND ${CMAKE_COMMAND} -E chdir "${work}" "${APP_ZIP}" -q -0 -r "${unsigned}" lib
        # 16 KB alignment for devices with 16 KB pages.
        COMMAND "${build_tools}/zipalign" -f -P 16 4 "${unsigned}" "${aligned}"
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${java_bin}:$ENV{PATH}"
            "${build_tools}/apksigner" sign --ks "${APP_KEYSTORE}"
            --ks-pass "${APP_KEYSTORE_PASS}" ${alias_flag} --out "${apk}" "${aligned}"
        DEPENDS ${library} "${manifest}" ${resource_files}
        COMMENT "Packaging ${APP_APK_NAME}.apk"
        VERBATIM)
    add_custom_target(apk ALL DEPENDS "${apk}")

    if(APP_ADB)
        add_custom_target(apk-install
            COMMAND "${APP_ADB}" install -r "${apk}"
            DEPENDS apk
            VERBATIM)
        add_custom_target(apk-run
            COMMAND "${APP_ADB}" shell am start -S -n "${APP_ID}/android.app.NativeActivity"
            DEPENDS apk-install
            VERBATIM)
    endif()
endfunction()
