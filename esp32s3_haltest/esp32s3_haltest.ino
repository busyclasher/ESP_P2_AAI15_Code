// esp32s3_haltest.ino
// D1 task 12: HAL timing and console checks on the ESP32-S3, driven by
// parity/hal_test.py. It links the same EdgeCore and EdgeS3 libraries as the
// firmware in esp32s3_edge/ but runs no RTOS tasks of its own.
//
// Console commands: SELFTEST runs the checks; any other line is echoed back as
// "[ECHO] <line>". A "[TICK] <ms>" line is printed every 1000 ms.
#include <string.h>
#include <EdgeS3.h>

// Defined in the core (mqtt/mqtt_config_parser.c); the Pico declares them in main.c.
extern "C" void control_bind_flags(bool *running, bool *cfg_ready);
extern "C" bool control_mqtt_start(const char *broker, uint16_t port,
                                   const char *client_id, const char *node_id);

static int s_fail;

static void check(const char *name, bool ok, const char *detail) {
    printf("[HALTEST] %s %s %s\n", ok ? "PASS" : "FAIL", name, detail);
    if (!ok) s_fail++;
}

static void run_selftest(void) {
    char d[64];
    s_fail = 0;
    printf("[HALTEST] start\n");

    uint64_t a = hal_time_us();
    uint64_t b = hal_time_us();
    snprintf(d, sizeof d, "%llu -> %llu us", (unsigned long long)a, (unsigned long long)b);
    check("time-monotonic", b >= a, d);

    uint64_t us = hal_time_us();
    uint32_t ms = hal_time_ms();
    snprintf(d, sizeof d, "ms=%lu us/1000=%llu", (unsigned long)ms, (unsigned long long)(us / 1000));
    check("time-ms-matches-us", ms >= us / 1000 && ms - us / 1000 <= 1, d);

    uint64_t t0 = hal_time_us();
    hal_delay_ms(100);
    uint64_t dt = hal_time_us() - t0;
    snprintf(d, sizeof d, "%llu us", (unsigned long long)dt);
    check("delay-ms-100", dt >= 100000 && dt <= 102000, d);

    t0 = hal_time_us();
    hal_delay_ms(1);
    dt = hal_time_us() - t0;
    snprintf(d, sizeof d, "%llu us", (unsigned long long)dt);
    check("delay-ms-1", dt >= 1000 && dt <= 2100, d);

    t0 = hal_time_us();
    hal_delay_us(500);
    dt = hal_time_us() - t0;
    snprintf(d, sizeof d, "%llu us", (unsigned long long)dt);
    check("delay-us-500", dt >= 500 && dt <= 600, d);

    t0 = hal_time_us();
    int ch = hal_console_getc(20000);
    dt = hal_time_us() - t0;
    snprintf(d, sizeof d, "ret=%d after %llu us", ch, (unsigned long long)dt);
    check("console-getc-timeout", ch == HAL_ERR_TIMEOUT && dt >= 20000, d);

    static const int reserved[] = { -1, 19, 20, 22, 26, 32, 49, 99 };
    bool all = true;
    for (unsigned i = 0; i < sizeof reserved / sizeof reserved[0]; i++) {
        int p = reserved[i];
        all = all && !hal_gpio_valid(p) && hal_gpio_mode(p, HAL_GPIO_OUTPUT) == HAL_ERR_PIN
                  && hal_gpio_write(p, 1) == HAL_ERR_PIN && hal_gpio_read(p) == HAL_ERR_PIN
                  && hal_gpio_toggle(p) == HAL_ERR_PIN && hal_pwm_set(p, 0.5f) == HAL_ERR_PIN;
    }
    check("gpio-reserved-pins-refused", all, "-1 19 20 22 26 32 49 99");

    int r = hal_gpio_mode(4, HAL_GPIO_INPUT_PULLDOWN);
    int lvl = hal_gpio_read(4);
    snprintf(d, sizeof d, "mode=%d read=%d", r, lvl);
    check("gpio-input-pulldown", r == HAL_OK && lvl == 0, d);

    r = hal_gpio_mode(4, HAL_GPIO_INPUT_PULLUP);
    lvl = hal_gpio_read(4);
    snprintf(d, sizeof d, "mode=%d read=%d", r, lvl);
    check("gpio-input-pullup", r == HAL_OK && lvl == 1, d);

    r = hal_gpio_mode(4, HAL_GPIO_OUTPUT);
    int t1 = hal_gpio_toggle(4), l1 = hal_gpio_read(4);
    int t2 = hal_gpio_toggle(4), l2 = hal_gpio_read(4);
    snprintf(d, sizeof d, "toggle %d/%d then %d/%d", t1, l1, t2, l2);
    check("gpio-output-toggle", r == HAL_OK && t1 == 1 && l1 == 1 && t2 == 0 && l2 == 0, d);

    int p1 = hal_pwm_set(4, 0.5f), p2 = hal_pwm_set(4, 0.25f), p3 = hal_pwm_stop(4);
    snprintf(d, sizeof d, "set=%d set=%d stop=%d read=%d", p1, p2, p3, hal_gpio_read(4));
    check("pwm-set-and-stop", p1 == HAL_OK && p2 == HAL_OK && p3 == HAL_OK && hal_gpio_read(4) == 0, d);
    hal_gpio_mode(4, HAL_GPIO_INPUT);

    // The whole core is linked; services not ported yet answer with the
    // documented errors from libraries/EdgeS3/src/platform_stubs.c.
    static bool running = false, cfg_ready = false;
    scheduler_reset();
    rules_reset();
    control_bind_flags(&running, &cfg_ready);

    app_cfg_t cfg;
    memset(&cfg, 0, sizeof cfg);
    strcpy(cfg.name, "S3GPIO");
    cfg.proto = proto_gpio;
    cfg.mode = sample_mode_poll;
    cfg.freq_hz = 1.0f;
    cfg.gpio_mode = "digital";
    cfg.gpio_pull = "down";
    cfg.gpio_pin = 4;
    int slot = scheduler_add_config(&cfg);
    scheduler_run_all(false);
    snprintf(d, sizeof d, "scheduler_add_config=%d", slot);
    check("unported-gpio-driver-refuses", slot < 0, d);

    bool mq = control_mqtt_start("127.0.0.1", 1883, "s3-node", "s3-node");
    check("unported-mqtt-reports-offline", !mq, mq ? "started" : "mqtt_init=false");

    inference_mgr_init();
    bool model = inference_mgr_load_base_model();
    check("unported-tflite-reports-no-model", !model && !inference_mgr_is_ready(),
          model ? "loaded" : "load_base_model=false");

    printf("[HALTEST] done failures=%d\n", s_fail);
}

void setup() {
    platform_console_begin(3000);
    printf("[BOOT] esp32s3_edge task 12 HAL test\n");
    run_selftest();
}

void loop() {
    static char line[128];
    static size_t n;
    static uint32_t next_tick = hal_time_ms() + 1000;

    int ch;
    while ((ch = hal_console_getc(0)) >= 0) {
        if (ch == '\r') continue;
        if (ch != '\n') {
            if (n < sizeof line - 1) line[n++] = (char)ch;
            continue;
        }
        line[n] = 0;
        n = 0;
        if (strcmp(line, "SELFTEST") == 0) run_selftest();
        else printf("[ECHO] %s\n", line);
    }

    if ((int32_t)(hal_time_ms() - next_tick) >= 0) {
        printf("[TICK] %lu\n", (unsigned long)hal_time_ms());
        next_tick += 1000;
    }
    hal_delay_ms(2);
}
