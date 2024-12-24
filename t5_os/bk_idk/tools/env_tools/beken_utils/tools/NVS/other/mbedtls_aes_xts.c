#include "mbedtls/aes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATAUNIT_SIZE 32

typedef unsigned char u8;

void generate_iv(u8 *iv, int unit_num) {
    memset(iv, 0, 16);
    for (int k = 0; k < 16 && k < DATAUNIT_SIZE; k++) {
        if (8 * k >= 32) continue;
        iv[15 - k] = (unit_num >> (8 * k)) & 0xFF;
    }
    for (int j = 0; j < 8; j++) {
        u8 temp = iv[j];
        iv[j] = iv[15 - j];
        iv[15 - j] = temp;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <key_file> <input_file> <output_file> <mode>\n", argv[0]);
        fprintf(stderr, "mode: 0 for encrypt, 1 for decrypt\n");
        return 1;
    }

    const char *key_file = argv[1];
    const char *input_file = argv[2];
    const char *output_file = argv[3];
    int mode = atoi(argv[4]);

    FILE *f_key = fopen(key_file, "rb");
    if (!f_key) {
        perror("fopen key");
        return 1;
    }

    unsigned char key[64]; // AES-XTS 512-bit key (two 256-bit keys)
    if (fread(key, 1, sizeof(key), f_key) != sizeof(key)) {
        perror("fread key");
        fclose(f_key);
        return 1;
    }
    fclose(f_key);

    FILE *f_in = fopen(input_file, "rb");
    if (!f_in) {
        perror("fopen input");
        return 1;
    }

    FILE *f_out = fopen(output_file, "wb");
    if (!f_out) {
        perror("fopen output");
        fclose(f_in);
        return 1;
    }

    mbedtls_aes_xts_context ctx;
    unsigned char iv[16];  // Initialization vector
    unsigned char buffer[DATAUNIT_SIZE];
    unsigned char output[DATAUNIT_SIZE];
    int unit_num = 0;

    // Initialize the AES-XTS context
    mbedtls_aes_xts_init(&ctx);

    // Set the AES-XTS key (both encryption and decryption keys)
    if (mode == 0) {
        mbedtls_aes_xts_setkey_enc(&ctx, key, 512); // 512 bits for encryption key
    } else {
        mbedtls_aes_xts_setkey_dec(&ctx, key, 512); // 512 bits for decryption key
    }

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, DATAUNIT_SIZE, f_in)) > 0) {
        // Generate IV
        generate_iv(iv, unit_num);
        unit_num++;

        // Encrypt or decrypt using AES-XTS
        mbedtls_aes_crypt_xts(&ctx, mode == 0 ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT,
                              bytes_read, iv, buffer, output);

        // Write output to file
        if (fwrite(output, 1, bytes_read, f_out) != bytes_read) {
            perror("fwrite output");
            fclose(f_in);
            fclose(f_out);
            return 1;
        }
    }

    // Clean up
    mbedtls_aes_xts_free(&ctx);
    fclose(f_in);
    fclose(f_out);

    return 0;
}
