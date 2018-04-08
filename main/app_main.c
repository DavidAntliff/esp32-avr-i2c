/*
 * MIT License
 *
 * Copyright (c) 2018 David Antliff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "soc/rtc.h"
#include "esp_log.h"

#include "led.h"
#include "i2c_master.h"
#include "avr_support.h"

#define TAG "app_main"

static void do_avr_reset(const char * topic, bool value, void * context)
{
    if (value)
    {
        ESP_LOGW(TAG, "AVR restart requested");
        avr_support_reset();
    }
}

static void do_avr_cp(const char * topic, bool value, void * context)
{
    avr_support_set_cp_pump(value ? AVR_PUMP_STATE_ON : AVR_PUMP_STATE_OFF);
}

static void do_avr_pp(const char * topic, bool value, void * context)
{
    avr_support_set_pp_pump(value ? AVR_PUMP_STATE_ON : AVR_PUMP_STATE_OFF);
}

static void do_avr_alarm(const char * topic, bool value, void * context)
{
    avr_support_set_alarm(value ? AVR_ALARM_STATE_ON : AVR_ALARM_STATE_OFF);
}

// brief delay during startup sequence
static void _delay(void)
{
    vTaskDelay(100 / portTICK_RATE_MS);
}

static void avr_test_sequence(void)
{
    static int state = 0;

    // simple state machine for testing
    switch (state)
    {
    case 0:
        avr_support_set_cp_pump(AVR_PUMP_STATE_OFF);
        avr_support_set_pp_pump(AVR_PUMP_STATE_OFF);
        avr_support_set_alarm(AVR_ALARM_STATE_OFF);
        break;
    case 1:
        avr_support_set_alarm(AVR_ALARM_STATE_ON);
        break;
    case 2:
        avr_support_set_cp_pump(AVR_PUMP_STATE_ON);
        break;
    case 3:
        avr_support_set_cp_pump(AVR_PUMP_STATE_OFF);
        avr_support_set_pp_pump(AVR_PUMP_STATE_ON);
        break;
    case 4:
        avr_support_set_cp_pump(AVR_PUMP_STATE_ON);
        avr_support_set_alarm(AVR_ALARM_STATE_OFF);
        break;
    case 5:
        avr_support_set_cp_pump(AVR_PUMP_STATE_OFF);
        break;
    case 6:
        avr_support_set_cp_pump(AVR_PUMP_STATE_ON);
        avr_support_set_pp_pump(AVR_PUMP_STATE_OFF);
        break;
    default:
        state = 0;
        break;
    }
    state = (state + 1) % 7;
}

void app_main()
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("i2c", ESP_LOG_DEBUG);
    esp_log_level_set("avr_support", ESP_LOG_DEBUG);
    esp_log_level_set("app_main", ESP_LOG_DEBUG);

    // Priority of queue consumer should be higher than producers
    UBaseType_t avr_priority = 5;

    // round to nearest MHz (stored value is only precise to MHz)
    uint32_t apb_freq = (rtc_clk_apb_freq_get() + 500000) / 1000000 * 1000000;
    ESP_LOGI(TAG, "APB CLK %u Hz", apb_freq);
    ESP_LOGI(TAG, "Core ID %d", xPortGetCoreID());

    // Onboard LED
    led_init(CONFIG_ONBOARD_LED_GPIO);

    // I2C bus
    _delay();
    i2c_master_info_t * i2c_master_info = i2c_master_init(I2C_MASTER_NUM, CONFIG_I2C_MASTER_SDA_GPIO, CONFIG_I2C_MASTER_SCL_GPIO, I2C_MASTER_FREQ_HZ);

    _delay();
    int num_i2c_devices = i2c_master_scan(i2c_master_info);
    ESP_LOGI(TAG, "%d I2C devices detected", num_i2c_devices);

    // I2C devices - AVR, Light Sensor, LCD
    _delay();
    avr_support_init(i2c_master_info, avr_priority);
    avr_support_reset();

    bool running = true;

    TickType_t last_wake_time = xTaskGetTickCount();

    while (running)
    {
        last_wake_time = xTaskGetTickCount();

        avr_test_sequence();

        vTaskDelayUntil(&last_wake_time, 1000 / portTICK_RATE_MS);
    }

    i2c_master_close(i2c_master_info);

    ESP_LOGE(TAG, "Restarting...");
    vTaskDelay(1000 / portTICK_RATE_MS);
    esp_restart();
}
