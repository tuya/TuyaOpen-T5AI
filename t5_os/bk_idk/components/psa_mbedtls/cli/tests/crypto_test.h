// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int aes_cbc_main(void);
int aes_gcm_main(void);
int ecdh_main(void);
int ecdsa_main(void);
int hmac_main(void);
int psa_tls_client_main(void);
int sha256_main(void);
int sha384_main(void);

int aes_cbc_perf_main(void);
int aes_gcm_perf_main(void);
int ecdh_perf_main(void);
int ecdsa_perf_main(void);
int hmac_perf_main(void);
int sha256_perf_main(void);
int sha384_perf_main(void);

/*key manage test begin*/
int generate_key_manage(void);
int aes_encrypt_decrypt(void);
int get_key_attributes(void);
int destroy_key(uint32_t id);
/*key manage test finish*/

void crypto_lock(void);
void crypto_unlock(void);
uint64_t crypto_get_time(void);
uint32_t crypto_diff_time(uint64_t end, uint64_t begin);
void crypto_perf_log(const char* type, const char* cpu_freq, uint32_t key_len, uint32_t len, uint32_t time);
int crypto_set_cpu_freq(uint32_t freq);

#ifdef __cplusplus
}
#endif
