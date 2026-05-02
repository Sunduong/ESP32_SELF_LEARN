/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "input_iot.h"

static const char *TAG = "example";

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define GPIO_USER_BUTTON GPIO_NUM_5

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    /* Increase drive strength for better LED brightness */
    gpio_set_drive_capability(BLINK_GPIO, GPIO_DRIVE_CAP_3);
}

void input_event_callback(int pin)
{
    if (pin == GPIO_USER_BUTTON)
    {
        static uint32_t last_press = 0;
        uint32_t now = xTaskGetTickCountFromISR();
        if (now - last_press > pdMS_TO_TICKS(200)) {
            static int x = 0;
            gpio_set_level(BLINK_GPIO, x);
            x = 1 - x;
            last_press = now;
        }
    }
}

void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
    configure_led();

    input_io_create(GPIO_USER_BUTTON, HI_TO_LO);
    input_set_callback(input_event_callback);
}
