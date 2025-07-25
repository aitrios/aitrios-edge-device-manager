/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(__LOG_MANAGER_STUB_H__)
#define __LOG_MANAGER_STUB_H__

#if defined(__cplusplus)
extern "C" {
#endif

struct SYS_client *stub_EVP_Agent_register_sys_client(void);
int stub_EVP_Agent_unregister_sys_client(struct SYS_client *c);
enum SYS_result stub_SYS_put_blob(struct SYS_client *c, const char *url,
                                  const struct SYS_http_header *headers,
                                  unsigned long long datalen, SYS_blob_cb cb,
                                  void *user);
enum SYS_result stub_SYS_put_blob_mstp(struct SYS_client *c,
                                       const char *storage_name,
                                       const char *filename,
                                       unsigned long long datalen,
                                       SYS_blob_cb cb, void *user);
enum SYS_result stub_SYS_send_telemetry(struct SYS_client *c, const char *topic,
                                        const char *value, SYS_telemetry_cb cb,
                                        void *user);
enum SYS_result stub_mstp_SYS_process_event(struct SYS_client *c, int ms);
enum SYS_result stub_telemetry_SYS_process_event(struct SYS_client *c, int ms);
enum SYS_result stub_SYS_process_event(struct SYS_client *c, int ms);
uint8_t *stub_get_decrypt_data(void);
void stub_clear_decrypt_data(void);
void stub_test_set_non_decrypt(void);
char *stub_get_storage_name(void);
char *stub_get_filename(void);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* !defined(__LOG_MANAGER_STUB_H__) */
