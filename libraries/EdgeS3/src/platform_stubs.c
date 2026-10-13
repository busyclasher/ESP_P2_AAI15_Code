// platform_stubs.c
// ESP32-S3 placeholders for the platform services the shared core calls but
// that later tasks port. Each returns a documented "unsupported" result so the
// core runs and reports the gap instead of failing to link:
//
//   bus drivers (tasks 14-17) init_cfg() returns -1, so the scheduler prints
//                              "init_cfg FAILED" and refuses the sensor
//   MQTT (task 19)             publishing returns false, as on the Pico while
//                              the broker is unreachable; mqtt_init() returns false
//   model transfer (task 20)   no uploaded model: MODEL_IDLE, size 0, NULL
//   TFLite Micro (task 20)     load and invoke return TFLITE_ERR_NOT_LOADED
//
// Each task deletes its part of this file when it adds the real driver.
#include <stdio.h>

#include "bus/bus_if.h"
#include "mqtt/mqtt_telemetry.h"
#include "mqtt/mqtt_model_transfer.h"
#include "inference_engine/tflite_wrapper.h"

// ---------- bus drivers ----------

static int bus_not_ported(const sensor_cfg_t *cfg, void **ctx_out) {
    (void)ctx_out;
    printf("[S3] %s driver not ported yet\n", cfg && cfg->proto ? cfg->proto : "bus");
    return -1;
}

const bus_t bus_uart = { "uart", bus_not_ported, NULL, NULL, NULL, NULL };

// ---------- MQTT ----------

bool mqtt_init(const char *broker_host, uint16_t broker_port, const char *client_id,
               mqtt_cfg_block_cb_t cfg_cb, mqtt_cmd_cb_t cmd_cb, mqtt_node_msg_cb_t node_cb) {
    (void)broker_host; (void)broker_port; (void)client_id;
    (void)cfg_cb; (void)cmd_cb; (void)node_cb;
    printf("[S3] MQTT not ported yet\n");
    return false;
}

void mqtt_poll(void) {}
bool mqtt_is_connected(void) { return false; }
void mqtt_set_node_id(const char *node_id) { (void)node_id; }
void mqtt_publish_birth(bool retained) { (void)retained; }
void mqtt_publish_heartbeat(uint32_t uptime_s) { (void)uptime_s; }

bool mqtt_pub_text(const char *topic, const char *fmt, ...) {
    (void)topic; (void)fmt;
    return false;
}

bool mqtt_pub_bytes(const char *sensor_name, const uint8_t *bytes, size_t len, int qos, bool retain) {
    (void)sensor_name; (void)bytes; (void)len; (void)qos; (void)retain;
    return false;
}

bool mqtt_pub_line(const char *sensor_name, const char *line, int qos, bool retain) {
    (void)sensor_name; (void)line; (void)qos; (void)retain;
    return false;
}

bool mqtt_pub_event(const char *rule_name, const char *event_type, double value,
                    const char *msg, uint32_t ts_ms) {
    (void)rule_name; (void)event_type; (void)value; (void)msg; (void)ts_ms;
    return false;
}

bool mqtt_pub_cfg_status(const char *name, bool ok) { (void)name; (void)ok; return false; }
bool mqtt_pub_rule_status(const char *name, bool ok) { (void)name; (void)ok; return false; }

bool mqtt_follow_remote_source(const char *other_node_id, const char *remote_source,
                               const char *local_alias) {
    (void)other_node_id; (void)remote_source; (void)local_alias;
    return false;
}

// ---------- model transfer ----------

void model_transfer_init(void) {}
void model_transfer_poll(void) {}
model_status_t model_transfer_status(void) { return MODEL_IDLE; }
uint32_t model_transfer_model_size(void) { return 0; }
const uint8_t *model_transfer_get_model_ptr(void) { return NULL; }

// ---------- TFLite Micro ----------

static const tflite_model_info_t s_no_model;

tflite_err_t tflite_load_model(const uint8_t *model_data, uint32_t model_size) {
    (void)model_data; (void)model_size;
    printf("[S3] TFLite Micro not ported yet\n");
    return TFLITE_ERR_NOT_LOADED;
}

tflite_err_t tflite_invoke(const float *inputs, float *outputs) {
    (void)inputs; (void)outputs;
    return TFLITE_ERR_NOT_LOADED;
}

const tflite_model_info_t *tflite_get_info(void) { return &s_no_model; }
void tflite_unload_model(void) {}
bool tflite_is_ready(void) { return false; }
