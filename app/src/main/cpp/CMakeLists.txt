# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("fclient")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
    # List C/C++ source files with relative paths to this CMakeLists.txt.
    native-lib.cpp)

set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../")
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" ABSOLUTE)

set(SPDLOG_ROOT "${PROJECT_ROOT}/libs/spdlog")
set(MBEDTLS_ROOT "${PROJECT_ROOT}/libs/mbedtls")

add_library(spdlog STATIC IMPORTED)
add_library(mbedx509 SHARED IMPORTED)
add_library(mbedtls SHARED IMPORTED)
add_library(mbedcrypto SHARED IMPORTED)
add_library(everest SHARED IMPORTED)
add_library(p256m SHARED IMPORTED)

set_target_properties(spdlog PROPERTIES IMPORTED_LOCATION
    "${SPDLOG_ROOT}/build/${CMAKE_ANDROID_ARCH_ABI}/libspdlog.a")
set_target_properties(mbedx509 PROPERTIES IMPORTED_LOCATION
    "${MBEDTLS_ROOT}/build/${CMAKE_ANDROID_ARCH_ABI}/library/libmbedx509.so")
set_target_properties(mbedtls PROPERTIES IMPORTED_LOCATION
    "${MBEDTLS_ROOT}/build/${CMAKE_ANDROID_ARCH_ABI}/library/libmbedtls.so")
set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION
    "${MBEDTLS_ROOT}/build/${CMAKE_ANDROID_ARCH_ABI}/library/libmbedcrypto.so")
set_target_properties(everest PROPERTIES IMPORTED_LOCATION
    "${MBEDTLS_ROOT}/build/${CMAKE_ANDROID_ARCH_ABI}/3rdparty/everest/libeverest.so")
set_target_properties(p256m PROPERTIES IMPORTED_LOCATION
    "${MBEDTLS_ROOT}/build/${CMAKE_ANDROID_ARCH_ABI}/3rdparty/p256-m/libp256m.so")

target_include_directories(fclient PUBLIC
    "${SPDLOG_ROOT}/spdlog/include"
    "${MBEDTLS_ROOT}/mbedtls/include"
)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(fclient
    spdlog
    everest
    p256m
    mbedx509
    mbedtls
    mbedcrypto
    log
    android
)