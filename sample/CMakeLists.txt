cmake_minimum_required(VERSION 3.20)
project(EncryptionSample C)

#===----------------------------------------------------------------------===//
# Encryption Pass Configuration
#===----------------------------------------------------------------------===//

# Path to the Encryption Pass plugin (adjust to your installation)
set(ENC_PLUGIN "$ENV{HOME}/encryption-pass/lib/libEncryption.dylib" CACHE PATH "Path to libEncryption.dylib")
set(ENC_INCLUDE "$ENV{HOME}/encryption-pass/include" CACHE PATH "Path to enc_options.h")

# Verify plugin exists
if(NOT EXISTS ${ENC_PLUGIN})
    message(FATAL_ERROR "Encryption plugin not found at: ${ENC_PLUGIN}\n"
                        "Please set ENC_PLUGIN to the correct path.")
endif()

#===----------------------------------------------------------------------===//
# Encryption Options
#===----------------------------------------------------------------------===//

# Encryption level and options
add_compile_options(
    -fpass-plugin=${ENC_PLUGIN}
    -DENC_FULL                    # Both Lite and Deep encryption
    -DENC_FULL_TIMES=3            # 3 iterations of each level
    -DENC_DEEP_INLINE             # Inline Deep decryption
    -DL2G_ENABLE                  # Enable Local-to-Global promotion
    -I${ENC_INCLUDE} -include ${ENC_INCLUDE}/enc_options.h
)

#===----------------------------------------------------------------------===//
# Build Configuration
#===----------------------------------------------------------------------===//

# macOS configuration
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

# Create executable
add_executable(sample main.c)
