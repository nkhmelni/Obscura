/**
 * Encryption Pass Sample
 *
 * This sample demonstrates various features of the Encryption Pass:
 * - Global variable encryption
 * - Local-to-Global promotion (L2G)
 * - Per-variable annotations (NO_ENC, L2G, NO_L2G)
 *
 * Compile with CMake or directly:
 *   clang -fpass-plugin=path/to/libObscura.dylib \
 *         -DENC_FULL -DL2G_ENABLE \
 *         -I path/to/include -include config.h \
 *         main.c -o sample
 */

#include <stdio.h>
#include <stdint.h>
#include "config.h"

/*============================================================================*
 * Global Variables - Encrypted by default
 *============================================================================*/

// These will be encrypted
static int32_t secret_key = 0xDEADBEEF;
static int32_t api_token = 0x12345678;
static float magic_ratio = 3.14159f;

// Lookup table - array encryption
static int32_t lookup_table[8] = {
    0x10, 0x20, 0x30, 0x40,
    0x50, 0x60, 0x70, 0x80
};

/*============================================================================*
 * Global Variables - Excluded with NO_ENC
 *============================================================================*/

// This won't be encrypted (NO_ENC annotation)
NO_ENC static int32_t public_version = 0x010203;

// Public configuration - not encrypted
NO_ENC static int32_t debug_level = 0;

/*============================================================================*
 * Functions demonstrating L2G
 *============================================================================*/

/**
 * Demonstrates automatic L2G promotion.
 * With L2G_ENABLE, local constants are promoted to globals and encrypted.
 */
int32_t process_with_l2g(int32_t input) {
    // These local constants will be promoted (L2G_ENABLE)
    int32_t multiplier = 0x9E3779B9;
    int32_t mask = 0xFFFF0000;

    // Perform some operations
    int32_t result = input * multiplier;
    result = result ^ mask;

    return result;
}

/**
 * Demonstrates explicit L2G annotation.
 * The L2G marker ensures promotion regardless of L2G_ENABLE.
 */
int32_t process_with_marker(int32_t input) {
    // Explicitly marked for promotion
    L2G int32_t round_constant = 0xB7E15163;

    // Promoted but NOT encrypted (combined annotations)
    L2G NO_ENC int32_t shift_amount = 13;

    // Excluded from automatic promotion
    NO_L2G int32_t loop_counter = 0;

    int32_t result = input;
    for (loop_counter = 0; loop_counter < 4; loop_counter++) {
        result = (result << shift_amount) | (result >> (32 - shift_amount));
        result ^= round_constant;
    }

    return result;
}

/**
 * Demonstrates filter behavior.
 * Depending on your filter settings, some variables may be skipped.
 */
void demonstrate_types(void) {
    // Different integer sizes
    int8_t  val8  = 0x12;
    int16_t val16 = 0x1234;
    int32_t val32 = 0x12345678;
    int64_t val64 = 0x123456789ABCDEF0LL;

    // Floating point
    float  fval = 2.71828f;
    double dval = 1.41421;

    printf("int8:   0x%02X\n", val8);
    printf("int16:  0x%04X\n", val16);
    printf("int32:  0x%08X\n", val32);
    printf("int64:  0x%016llX\n", val64);
    printf("float:  %f\n", fval);
    printf("double: %f\n", dval);
}

/*============================================================================*
 * Main
 *============================================================================*/

int main(void) {
    printf("=== Encryption Pass Sample ===\n\n");

    // Encrypted globals
    printf("Secret Key:  0x%08X\n", secret_key);
    printf("API Token:   0x%08X\n", api_token);
    printf("Magic Ratio: %f\n", magic_ratio);
    printf("\n");

    // Array access
    int32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += lookup_table[i];
    }
    printf("Lookup Sum:  0x%X\n", sum);
    printf("\n");

    // Non-encrypted globals
    printf("Version:     %d.%d.%d (not encrypted)\n",
           (public_version >> 16) & 0xFF,
           (public_version >> 8) & 0xFF,
           public_version & 0xFF);
    printf("Debug Level: %d (not encrypted)\n", debug_level);
    printf("\n");

    // L2G demonstrations
    printf("L2G auto:    0x%08X\n", process_with_l2g(0x12345678));
    printf("L2G marker:  0x%08X\n", process_with_marker(0x12345678));
    printf("\n");

    // Type demonstrations
    printf("--- Type Demonstrations ---\n");
    demonstrate_types();

    return 0;
}
