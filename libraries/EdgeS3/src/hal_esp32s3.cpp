// hal_esp32s3.cpp
// ESP32-S3 backend for hal/hal.h (D1 task 12), built on Arduino-ESP32.
// Behaviour follows the Pico backend (project_v3/platform/pico/hal_pico.c);
// differences are noted where the hardware forces them.
#include <Arduino.h>
#include <stdio.h>
#include "esp_timer.h"
#include "sdkconfig.h"

#include "hal/hal.h"
#include "platform_esp32s3.h"

// ---------- time ----------
// esp_timer_get_time() is a documented ESP-IDF exception: Arduino micros() is
// 32-bit and wraps after about 71 minutes, while the core relies on 64-bit time.

uint64_t hal_time_us(void) {
    return (uint64_t)esp_timer_get_time();
}

uint32_t hal_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// ---------- delay ----------

void hal_delay_ms(uint32_t ms) {
    // delay() is vTaskDelay(), which can return up to one tick early;
    // finish on the timer so the caller always waits at least ms.
    int64_t end = esp_timer_get_time() + (int64_t)ms * 1000;
    delay(ms);
    while (esp_timer_get_time() < end) delay(1);
}

void hal_delay_us(uint32_t us) {
    delayMicroseconds(us);
}

// ---------- console ----------

static ssize_t console_write(void *cookie, const char *buf, size_t len) {
    (void)cookie;
    return (ssize_t)Serial.write((const uint8_t *)buf, len);
}

void platform_console_begin(uint32_t wait_ms) {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < wait_ms) delay(10);

    // Arduino sends printf to UART0, not the USB console, so give C stdio a
    // stream that writes to Serial. Tasks created later inherit it.
    cookie_io_functions_t io = { NULL, console_write, NULL, NULL };
    FILE *f = fopencookie(NULL, "w", io);
    if (!f) return;
    setvbuf(f, NULL, _IOLBF, 256);
    _GLOBAL_REENT->_stdout = f;
    stdout = f;
}

int hal_console_getc(uint32_t timeout_us) {
    int64_t end = esp_timer_get_time() + timeout_us;
    for (;;) {
        int ch = Serial.read();
        if (ch >= 0) return ch;
        if (esp_timer_get_time() >= end) return HAL_ERR_TIMEOUT;
        delay(1);
    }
}

// ---------- GPIO ----------

bool hal_gpio_valid(int pin) {
    if (pin < 0 || pin > 48) return false;
    if (pin >= 22 && pin <= 25) return false;   // not present on the ESP32-S3
    if (pin == 19 || pin == 20) return false;   // native USB, which carries the console
    if (pin >= 26 && pin <= 32) return false;   // SPI flash and quad PSRAM
#if CONFIG_SPIRAM_MODE_OCT
    if (pin >= 33 && pin <= 37) return false;   // octal PSRAM
#endif
    return true;
}

static uint64_t s_pwm_pins;   // pins currently attached to LEDC

int hal_gpio_mode(int pin, hal_gpio_mode_t mode) {
    if (!hal_gpio_valid(pin)) return HAL_ERR_PIN;
    s_pwm_pins &= ~(1ULL << pin);   // pinMode() releases any LEDC attachment
    switch (mode) {
    case HAL_GPIO_OUTPUT:
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        break;
    case HAL_GPIO_INPUT_PULLUP:   pinMode(pin, INPUT_PULLUP);   break;
    case HAL_GPIO_INPUT_PULLDOWN: pinMode(pin, INPUT_PULLDOWN); break;
    default:                      pinMode(pin, INPUT);          break;
    }
    return HAL_OK;
}

int hal_gpio_write(int pin, int level) {
    if (!hal_gpio_valid(pin)) return HAL_ERR_PIN;
    digitalWrite(pin, level ? HIGH : LOW);
    return HAL_OK;
}

int hal_gpio_read(int pin) {
    if (!hal_gpio_valid(pin)) return HAL_ERR_PIN;
    return digitalRead(pin) ? 1 : 0;
}

int hal_gpio_toggle(int pin) {
    if (!hal_gpio_valid(pin)) return HAL_ERR_PIN;
    // Arduino-ESP32 outputs keep their input buffer, so the pin can be read
    // back directly; the Pico has to rely on its input synchroniser instead.
    int next = !digitalRead(pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, next ? HIGH : LOW);
    s_pwm_pins &= ~(1ULL << pin);
    return next;
}

// ---------- PWM ----------

static const uint32_t kPwmFreqHz = 37463;   // Pico: 150 MHz / 4 / 1001
static const uint8_t  kPwmBits   = 10;

int hal_pwm_set(int pin, float duty) {
    if (!hal_gpio_valid(pin)) return HAL_ERR_PIN;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    if (!(s_pwm_pins & (1ULL << pin))) {
        if (!ledcAttach(pin, kPwmFreqHz, kPwmBits)) return HAL_ERR_UNSUPPORTED;
        s_pwm_pins |= 1ULL << pin;
    }
    uint32_t level = (uint32_t)(duty * 1000.0f);   // same 1/1000 steps as the Pico
    ledcWrite(pin, (level * ((1u << kPwmBits) - 1) + 500) / 1000);
    return HAL_OK;
}

int hal_pwm_stop(int pin) {
    if (!hal_gpio_valid(pin)) return HAL_ERR_PIN;
    if (s_pwm_pins & (1ULL << pin)) {
        ledcDetach(pin);
        s_pwm_pins &= ~(1ULL << pin);
    }
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    return HAL_OK;
}
