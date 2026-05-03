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
// #include "input_iot.h"
// #include "output_iot.h"

static const char *TAG = "example";
#define BLINK_GPIO CONFIG_BLINK_GPIO

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    /* Increase drive strength for better LED brightness */
    gpio_set_drive_capability(BLINK_GPIO, GPIO_DRIVE_CAP_3);
}

TimerHandle_t xTimers[2];

void vTimerCallback(TimerHandle_t xTimer)
{
    configASSERT(xTimer);
    int ulCount = (uint32_t) pvTimerGetTimerID(xTimer);
    if (ulCount == 0) {
        static int x = 0;
        gpio_set_level(BLINK_GPIO, x);
        x = !x;
    } else if (ulCount == 1) {
        printf("Hello\n");
    }
}


void vTask1(void *pvParameters)
{
    while (1) {
        printf("Task 1 is running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    configure_led();
    xTimers[0] = xTimerCreate("TimerBlink", pdMS_TO_TICKS(2000),
                                pdTRUE,
                                (void *) 0,
                                vTimerCallback);
    xTimers[1] = xTimerCreate("TimerLog", pdMS_TO_TICKS(5000),
                                pdTRUE,
                                (void *) 1,
                                vTimerCallback);
    xTimerStart(xTimers[0], 0);
    xTimerStart(xTimers[1], 0);
    xTaskCreate(vTask1, "Task 1", 4096, NULL, 4, NULL);
}
