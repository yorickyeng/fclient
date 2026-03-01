
#!/bin/bash

#ABI=armaebi-v7a
#ABI=x86
ABI=arm64-v8a
#ABI=x86_64

ANDROID_NDK=$HOME/Library/Android/sdk/ndk/28.2.13676358
TOOL_CHAIN=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
CMAKE=$HOME/Library/Android/sdk/cmake/3.22.1/bin/cmake

mkdir -p ${ABI}
cd ${ABI}

${CMAKE} ../../spdlog \
-DCMAKE_SYSTEM_NAME=Android \
-DCMAKE_SYSTEM_VERSION=21 \
-DANDROID_ABI=${ABI} \
-DCMAKE_TOOLCHAIN_FILE=${TOOL_CHAIN} \
-DCMAKE_CXX_FLAGS=-DSPDLOG_COMPILED_LIB

${CMAKE} --build .