cmake_minimum_required(VERSION 3.20)
project(ObscuraSample C)

# Obscura plugin paths (adjust to your installation)
set(OBSCURA_PLUGIN "$ENV{HOME}/obscura/lib/libObscura.dylib" CACHE PATH "Path to libObscura.dylib")
set(OBSCURA_INCLUDE "$ENV{HOME}/obscura/include" CACHE PATH "Path to config.h")

# Apply Obscura to all source files
add_compile_options(
    -fpass-plugin=${OBSCURA_PLUGIN}
    -DENC_FULL -DENC_FULL_TIMES=3 -DENC_DEEP_INLINE -DL2G_ENABLE
    -I${OBSCURA_INCLUDE} -include ${OBSCURA_INCLUDE}/config.h
)

# macOS configuration
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

add_executable(sample main.c)
