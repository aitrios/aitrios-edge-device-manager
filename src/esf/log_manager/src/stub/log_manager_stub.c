/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <evp/sdk_sys.h>
#include <malloc.h>
#include <sdk_backdoor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sys.h>

#ifdef LOG_MANAGER_ENCRYPT_ENABLE
#include "sec_mod_public.h"
#include "sec_util_public.h"
#endif  // LOG_MANAGER_ENCRYPT_ENABLE

// enum
enum SYS_event_type {
  SYS_EVENT_TYPE_MSTP,
  SYS_EVENT_TYPE_TELEMETRY,
  SYS_EVENT_TYPE_HTTP,
};

// stub structure declaration
struct SYS_stub_http {
  bool enable;  // HTTP enable
  enum SYS_result (*cb)(struct SYS_client *c, struct SYS_blob_data *blob,
                        enum SYS_callback_reason reason, void *user);
  unsigned long long
      datalen;        // data size (remaining size management after transfer)
  void *blob_buffer;  // blob address
  size_t len;         // blob size
  enum SYS_callback_reason reason;  // callback reason
  void *user;                       // user setting
};

struct SYS_stub_mstp {
  bool enable;  // MSTP enable
  enum SYS_result (*cb)(struct SYS_client *c, struct SYS_blob_data *blob,
                        enum SYS_callback_reason reason, void *user);
  uint64_t datalen;   // data size (remaining size management after transfer)
  void *blob_buffer;  // blob address
  size_t len;         // blob size
  enum SYS_callback_reason reason;  // callback reason
  void *user;                       // user setting
};

struct SYS_stub_telemetry {
  bool enable;  // telemetry enable
  void (*cb)(struct SYS_client *c, enum SYS_callback_reason reason, void *user);
  enum SYS_callback_reason reason;  // callback reason
  void *user;                       // user setting
};

struct SYS_stub {
  struct SYS_stub_mstp mstp_data;
  struct SYS_stub_telemetry telemetry_data;
  struct SYS_stub_http http_data;
  struct SYS_client *c;
};

static struct SYS_stub sys_stub;
static struct SYS_stub s_sys_stub_http;
static struct SYS_stub s_sys_stub_telemetry;
static int stub_sys_process_event_fail_no = 0;
static int stub_sys_put_blob_mstp_fail_no = 0;

static uint8_t *static_test_tmp_blob = NULL;
static size_t static_test_tmp_blob_size = 0;
static uint8_t *static_decrypted_test_blob = NULL;
static bool static_test_non_decrypted_flag = false;
static char *static_stub_SYS_put_blob_mstp_storage_name = NULL;
static char *static_stub_SYS_put_blob_mstp_filename = NULL;

static uint8_t *static_test_tmp_http_blob = NULL;
static size_t static_test_tmp_http_blob_size = 0;
static char *static_stub_SYS_put_blob_url = NULL;
static uint8_t *static_uploaded_http_blob = NULL;

static bool static_startup_flag = false;
static bool static_startup_flag_http = false;

static bool stub_backdoor_get_agent_status_disconnected = false;

// define
#define LOG_MANAGER_BLOB_WORKER_BUFFER_SIZE (2048 + 128)

static void stub_test_decrypt_data(uint8_t *data, uint32_t data_size,
                                   uint8_t *decrypt_data);

/* stub function */
// providing a Client ID
struct SYS_client *stub_EVP_Agent_register_sys_client(void) {
  struct SYS_client *c = NULL;
  c = malloc(sizeof(*c));  // memory allocation
  if (c != NULL) {
    memset(c, 0, sizeof(*c));  // initialization
  }
  printf("%s:return EVP_Agent_register_sys_client();\n", __func__);
  return c;
}

// discarding the client ID
int stub_EVP_Agent_unregister_sys_client(struct SYS_client *c) {
  free(c);  // 　memory release
  c = NULL;
  printf("%s:return EVP_Agent_unregister_sys_client();\n", __func__);
  return 0;
}

enum SYS_result stub_SYS_put_blob(struct SYS_client *c, const char *url,
                                  const struct SYS_http_header *headers,
                                  unsigned long long datalen, SYS_blob_cb cb,
                                  void *user) {
  enum SYS_result result = SYS_RESULT_OK;

  if (static_stub_SYS_put_blob_url) {
    free(static_stub_SYS_put_blob_url);
    static_stub_SYS_put_blob_url = NULL;
  }

  static_stub_SYS_put_blob_url = strdup(url);
  if (!static_stub_SYS_put_blob_url) {
    result = SYS_RESULT_ERROR_NO_MEM;
  }

  // initialization
  if (static_test_tmp_http_blob) {
    free(static_test_tmp_http_blob);
  }

  static_test_tmp_http_blob = calloc(1, datalen);
  static_test_tmp_http_blob_size = 0;

  s_sys_stub_http.http_data.enable = true;
  s_sys_stub_http.http_data.blob_buffer = NULL;
  s_sys_stub_http.http_data.len = 0;
  s_sys_stub_http.http_data.reason = SYS_REASON_MORE_DATA;
  // set the information required for blob upload in the stub structure
  s_sys_stub_http.http_data.datalen = datalen;
  s_sys_stub_http.http_data.cb = cb;
  s_sys_stub_http.http_data.user = user;
  s_sys_stub_http.c = c;

  static_startup_flag_http = true;

  return result;
}

// configuring STP upload
enum SYS_result stub_SYS_put_blob_mstp(struct SYS_client *c
                                       __attribute__((unused)),
                                       const char *storage_name,
                                       const char *filename, uint64_t datalen,
                                       SYS_blob_cb cb, void *user) {
  enum SYS_result result = SYS_RESULT_OK;

  if (stub_sys_put_blob_mstp_fail_no) {
    return SYS_RESULT_ERROR_NO_MEM;
  }

  if (static_stub_SYS_put_blob_mstp_storage_name) {
    free(static_stub_SYS_put_blob_mstp_storage_name);
    static_stub_SYS_put_blob_mstp_storage_name = NULL;
  }
  if (static_stub_SYS_put_blob_mstp_filename) {
    free(static_stub_SYS_put_blob_mstp_filename);
    static_stub_SYS_put_blob_mstp_filename = NULL;
  }

  static_stub_SYS_put_blob_mstp_storage_name = strdup(storage_name);
  static_stub_SYS_put_blob_mstp_filename = strdup(filename);
  if (!static_stub_SYS_put_blob_mstp_storage_name ||
      !static_stub_SYS_put_blob_mstp_filename) {
    result = SYS_RESULT_ERROR_NO_MEM;
  }

  // initialization
  if (static_test_tmp_blob) {
    free(static_test_tmp_blob);
  }
  static_test_tmp_blob = calloc(1, datalen);
  static_test_tmp_blob_size = 0;

  sys_stub.mstp_data.enable = true;
  sys_stub.mstp_data.blob_buffer = NULL;
  sys_stub.mstp_data.len = 0;
  sys_stub.mstp_data.reason = SYS_REASON_MORE_DATA;

  // set the information required for blob upload in the stub structure
  sys_stub.mstp_data.datalen = datalen;
  sys_stub.mstp_data.cb = cb;
  sys_stub.mstp_data.user = user;
  sys_stub.c = c;

  static_startup_flag = true;

  return result;
}

// telemetry data transmission settings
enum SYS_result stub_SYS_send_telemetry(struct SYS_client *c
                                        __attribute__((unused)),
                                        const char *topic, const char *value,
                                        SYS_telemetry_cb cb, void *user) {
  if (value == NULL) {
    return SYS_RESULT_ERROR_BAD_PARAMS;
  }

  enum SYS_result result = SYS_RESULT_OK;
  char *valuedup = strdup(value), *topicdup = strdup(topic);

  if (!valuedup || !topicdup) {
    result = SYS_RESULT_ERROR_NO_MEM;
  }

  // initialization
  s_sys_stub_telemetry.telemetry_data.enable = true;
  s_sys_stub_telemetry.telemetry_data.reason = SYS_REASON_FINISHED;
  // set the information required for send telemetry in the stub structure
  s_sys_stub_telemetry.telemetry_data.cb = cb;
  s_sys_stub_telemetry.telemetry_data.user = user;
  s_sys_stub_telemetry.c = c;

  printf("%s:topic=%s, value=%s\n", __func__, topicdup, valuedup);

  free(topicdup);
  free(valuedup);
  return result;
}

// event processing (executing HTTP)
enum SYS_result stub_http_SYS_process_event(struct SYS_client *c
                                            __attribute__((unused)),
                                            int ms __attribute__((unused))) {
  enum SYS_result result = SYS_RESULT_OK;
  enum SYS_callback_reason reason = SYS_REASON_FINISHED;
  struct SYS_blob_data blob = {0};

  if (static_startup_flag_http == false) {
    return result;
  }

  if (s_sys_stub_http.http_data.datalen > 0) {
    reason = SYS_REASON_MORE_DATA;

    if (s_sys_stub_http.http_data.datalen <
        LOG_MANAGER_BLOB_WORKER_BUFFER_SIZE) {
      blob.len = s_sys_stub_http.http_data.datalen;
    } else {
      blob.len = LOG_MANAGER_BLOB_WORKER_BUFFER_SIZE;
    }

    blob.blob_buffer = calloc(1, blob.len);
    s_sys_stub_http.http_data.datalen -= blob.len;
  } else {
    reason = SYS_REASON_FINISHED;
  }

  s_sys_stub_http.http_data.reason = reason;

  // callback
  result = (*s_sys_stub_http.http_data.cb)(c, &blob,
                                           s_sys_stub_http.http_data.reason,
                                           s_sys_stub_http.http_data.user);

  if (reason == SYS_REASON_MORE_DATA) {
    memcpy(static_test_tmp_http_blob + static_test_tmp_http_blob_size,
           blob.blob_buffer, blob.len);
    static_test_tmp_http_blob_size += blob.len;

    free(blob.blob_buffer);
    blob.blob_buffer = NULL;

  } else if (reason == SYS_REASON_FINISHED) {
    free(static_uploaded_http_blob);
    static_uploaded_http_blob = calloc(1, static_test_tmp_http_blob_size);

    memcpy(static_uploaded_http_blob, static_test_tmp_http_blob,
           static_test_tmp_http_blob_size);

    free(static_test_tmp_http_blob);
    static_test_tmp_http_blob = NULL;
    static_test_tmp_http_blob_size = 0;
    static_startup_flag_http = false;

    s_sys_stub_http.http_data.enable = false;
  }

  return result;
}

// event processing (executing STP)
enum SYS_result stub_mstp_SYS_process_event(struct SYS_client *c
                                            __attribute__((unused)),
                                            int ms __attribute__((unused))) {
  enum SYS_result result = SYS_RESULT_OK;
  enum SYS_callback_reason reason = SYS_REASON_FINISHED;
  struct SYS_blob_data blob = {0};

  if (static_startup_flag == false) {
    return result;
  }

  if (sys_stub.mstp_data.datalen > 0) {
    reason = SYS_REASON_MORE_DATA;

    if (sys_stub.mstp_data.datalen < LOG_MANAGER_BLOB_WORKER_BUFFER_SIZE) {
      blob.len = sys_stub.mstp_data.datalen;
    } else {
      blob.len = LOG_MANAGER_BLOB_WORKER_BUFFER_SIZE;
    }

    blob.blob_buffer = calloc(1, blob.len);
    sys_stub.mstp_data.datalen -= blob.len;
  } else {
    reason = SYS_REASON_FINISHED;
  }

  sys_stub.mstp_data.reason = reason;

  // callback
  result = (*sys_stub.mstp_data.cb)(c, &blob, sys_stub.mstp_data.reason,
                                    sys_stub.mstp_data.user);

  if (reason == SYS_REASON_MORE_DATA) {
    memcpy(static_test_tmp_blob + static_test_tmp_blob_size, blob.blob_buffer,
           blob.len);
    static_test_tmp_blob_size += blob.len;

    blob.blob_buffer = NULL;

  } else if (reason == SYS_REASON_FINISHED) {
    free(static_decrypted_test_blob);
    static_decrypted_test_blob = calloc(1, static_test_tmp_blob_size);

    if (static_test_non_decrypted_flag == true) {
      memcpy(static_decrypted_test_blob, static_test_tmp_blob,
             static_test_tmp_blob_size);
    } else {
      stub_test_decrypt_data(static_test_tmp_blob, static_test_tmp_blob_size,
                             static_decrypted_test_blob);
    }

    free(static_test_tmp_blob);
    static_test_tmp_blob = NULL;
    static_test_tmp_blob_size = 0;
    static_startup_flag = false;

    sys_stub.mstp_data.enable = false;
  }

  return result;
}

// event processing (executing Telemetry)
enum SYS_result stub_telemetry_SYS_process_event(struct SYS_client *c
                                                 __attribute__((unused)),
                                                 int ms
                                                 __attribute__((unused))) {
  enum SYS_result result = SYS_RESULT_OK;

  // after a callback is raised with reason=SYS_REASON_ERROR
  // prevent callback from being called
  if (SYS_REASON_ERROR == s_sys_stub_telemetry.telemetry_data.reason) {
    printf("%s:return SYS_process_event()\n", __func__);
    return result;
  }
  (*s_sys_stub_telemetry.telemetry_data.cb)(
      c, s_sys_stub_telemetry.telemetry_data.reason,
      s_sys_stub_telemetry.telemetry_data.user);

  s_sys_stub_telemetry.telemetry_data.enable = false;

  return result;
}

// event processing
enum SYS_result stub_SYS_process_event(struct SYS_client *c, int ms) {
  if (stub_sys_process_event_fail_no == 1) {
    return SYS_RESULT_TIMEDOUT;
  } else if (stub_sys_process_event_fail_no == 2) {
    stub_sys_process_event_fail_no = 0;
    return SYS_RESULT_ERRNO;
  } else if (stub_sys_process_event_fail_no == 3) {
    stub_sys_process_event_fail_no = 0;
    return SYS_RESULT_SHOULD_EXIT;
  }
  if (ms != -1) {
    usleep(ms);
  }

  enum SYS_result result = SYS_RESULT_OK;

  if ((c == sys_stub.c) && sys_stub.mstp_data.enable) {
    result = stub_mstp_SYS_process_event(c, ms);
  }

  if ((c == s_sys_stub_http.c) && s_sys_stub_http.http_data.enable) {
    result = stub_http_SYS_process_event(c, ms);
  }

  if ((c == s_sys_stub_telemetry.c) &&
      s_sys_stub_telemetry.telemetry_data.enable) {
    result = stub_telemetry_SYS_process_event(c, ms);
  }

  return result;
}

void stub_setup_fail_sys_process_event(int no) {
  stub_sys_process_event_fail_no = no;
}
void stub_setup_fail_sys_put_blob_mstp(int no) {
  stub_sys_put_blob_mstp_fail_no = no;
}

void stub_clear_sys_parameter(void) {
  stub_sys_process_event_fail_no = 0;
  stub_sys_put_blob_mstp_fail_no = 0;

  if (static_test_tmp_blob) {
    free(static_test_tmp_blob);
    static_test_tmp_blob = NULL;
    static_test_tmp_blob_size = 0;
  }
}

uint8_t *stub_get_decrypt_data(void) { return static_decrypted_test_blob; }

char *stub_get_storage_name(void) {
  return static_stub_SYS_put_blob_mstp_storage_name;
}

char *stub_get_filename(void) { return static_stub_SYS_put_blob_mstp_filename; }

uint8_t *stub_get_http_blob(void) { return static_uploaded_http_blob; }
char *stub_get_http_url(void) { return static_stub_SYS_put_blob_url; }

void stub_clear_decrypt_data(void) {
  free(static_decrypted_test_blob);
  static_decrypted_test_blob = NULL;
  static_test_non_decrypted_flag = false;

  free(static_stub_SYS_put_blob_mstp_storage_name);
  static_stub_SYS_put_blob_mstp_storage_name = NULL;
  free(static_stub_SYS_put_blob_mstp_filename);
  static_stub_SYS_put_blob_mstp_filename = NULL;

  free(static_uploaded_http_blob);
  static_uploaded_http_blob = NULL;
  free(static_stub_SYS_put_blob_url);
  static_stub_SYS_put_blob_url = NULL;
  free(static_test_tmp_http_blob);
  static_test_tmp_http_blob = NULL;
  static_test_tmp_http_blob_size = 0;
}

static void stub_test_decrypt_data(uint8_t *data, uint32_t data_size,
                                   uint8_t *decrypt_data) {
// This func does nothing because encryption is not performed by
// LogManager.
#ifdef LOG_MANAGER_ENCRYPT_ENABLE
  uint8_t p_key[SECUTIL_KEY_SIZE_DETAIL_LOG / 8] = {0};
  uint16_t key_length = 0;

  EsfSecUtilStatus ret = EsfSecUtilInit();
  if (ret != kEsfSecUtilSuccess) {
    printf("%s:EsfSecUtilInit failed. ret:%x\n", __func__, ret);
    goto process_fin;
  }

  uint32_t total_length = EsfSecUtilGetBlockFlashDataSize(data_size);
  printf("%s:total_length:%u\n", __func__, total_length);
  if (data_size < total_length) {
    printf("%s:Parameter error. total_length over than data_size.\n", __func__);
    goto process_fin;
  }

  ret = SecMod_GetKeySize(SEC_MOD_KEY_ID_DETAIL_LOG, &key_length);
  if (ret != kEsfSecUtilSuccess) {
    printf("%s:SecMod_GetKeySize failed. ret:%x\n", __func__, ret);
    goto process_fin;
  }

  ret = SecMod_GetKey(SEC_MOD_KEY_ID_DETAIL_LOG, p_key, &key_length);
  if (ret != kEsfSecUtilSuccess) {
    printf("%s:SecMod_GetKey failed. ret:%x\n", __func__, ret);
    goto process_fin;
  }

  uint8_t iv_output[SECUTIL_AES_BLOCK_SIZE] = {0};
  memcpy(iv_output, data, SECUTIL_AES_BLOCK_SIZE);
  uint8_t *data_skip_iv = data + SECUTIL_AES_BLOCK_SIZE;
  ret = EsfSecUtilDecryptAesCbc(SECUTIL_KEY_SIZE_DETAIL_LOG, p_key,
                                total_length, iv_output, data_skip_iv,
                                decrypt_data);
  if (ret != kEsfSecUtilSuccess) {
    printf("%s:EsfSecUtilDecryptAesCbc failed. ret:%x\n", __func__, ret);
    goto process_fin;
  }

process_fin:
  if (ret != kEsfSecUtilSuccess) {
    stub_clear_decrypt_data();
  }
  (void)EsfSecUtilDeinit();

  // Free the static_decrypted_test_blob upon normal termination at
  // the end of the test.
#endif  // LOG_MANAGER_ENCRYPT_ENABLE
}
void stub_test_set_non_decrypt(void) { static_test_non_decrypted_flag = true; }

enum evp_agent_status stub_EVP_getAgentStatus(void) {
  if (stub_backdoor_get_agent_status_disconnected) {
    return EVP_AGENT_STATUS_DISCONNECTED;
  }
  return EVP_AGENT_STATUS_CONNECTED;
}

void StubSetupDisconnectedStatus(void) {
  stub_backdoor_get_agent_status_disconnected = true;
}

void StubClearBackdoorParameter(void) {
  stub_backdoor_get_agent_status_disconnected = false;
}
