// esp32s3_edge.ino
// ESP32-S3 firmware of the Configurable Edge Platform (D1). The shared code is
// the EdgeCore library; libraries/EdgeS3 supplies the HAL backend and the
// FreeRTOS runtime from the Week 6 design review. setup() starts the tasks and
// loop() is MonitorTask.
#include <EdgeS3.h>

void setup() {
    platform_console_begin(3000);
    printf("[BOOT] esp32s3_edge\n");
    runtime_start();
}

void loop() {
    runtime_monitor_tick();
    delay(1000);
}
