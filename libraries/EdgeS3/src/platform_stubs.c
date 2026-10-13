// platform_stubs.c
// ESP32-S3 placeholders for the platform services the shared core calls but
// that later tasks port. Each returns a documented "unsupported" result so the
// core runs and reports the gap instead of failing to link:
//
//   model transfer (task 20)   no uploaded model: MODEL_IDLE, size 0, NULL
//
// Each task deletes its part of this file when it adds the real driver.
#include <stdio.h>

#include "mqtt/mqtt_model_transfer.h"

// ---------- model transfer ----------

void model_transfer_init(void) {}
void model_transfer_poll(void) {}
model_status_t model_transfer_status(void) { return MODEL_IDLE; }
uint32_t model_transfer_model_size(void) { return 0; }
const uint8_t *model_transfer_get_model_ptr(void) { return NULL; }
