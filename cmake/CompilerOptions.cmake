# Warnings and size optimisations, applied through the app::options target.

add_library(app_options INTERFACE)
add_library(app::options ALIAS app_options)

target_compile_options(app_options INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion
    -Wno-missing-field-initializers -Wnon-virtual-dtor -Woverloaded-virtual -Wimplicit-fallthrough
    -fno-rtti
    $<$<CONFIG:Release,MinSizeRel>:-ffunction-sections -fdata-sections -fvisibility=hidden>)
target_link_options(app_options INTERFACE
    $<$<CONFIG:Release,MinSizeRel>:-Wl,--gc-sections -Wl,--icf=all -Wl,-O2 -s>)

include(CheckIPOSupported)
check_ipo_supported(RESULT APP_IPO_SUPPORTED OUTPUT ipo_error)
if(APP_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
endif()
