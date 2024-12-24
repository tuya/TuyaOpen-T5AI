[install]
sudo apt-get install libmbedtls-dev

[compile]
gcc -o mbedtls_aes_xts mbedtls_aes_xts.c -l:libmbedcrypto.a -l:libmbedtls.a -l:libmbedx509.a

[run]
./mbedtls_aes_xts key.bin nvs_enc.bin nvs_enc_out.bin 0

