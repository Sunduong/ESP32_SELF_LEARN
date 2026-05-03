/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "input_iot.h"
#include "output_iot.h"

static const char *TAG = "example";

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define USER_BUTTON_GPIO GPIO_NUM_5
#define BIT_EVENT_BUTTON_PRESSED (1<<0)

//Requirment: Short press is < 1 second, normal press is 1-3 seconds, long press is > 3 seconds, and timeout press is > 5s
#define BIT_EVENT_SHORT_PRESS       (1<<1)
#define BIT_EVENT_NORMAL_PRESS      (1<<2)
#define BIT_EVENT_LONG_PRESS        (1<<3)
#define BIT_EVENT_TIMEOUT_PRESS     (1<<4)

TimerHandle_t xDebounceTimer;
EventGroupHandle_t xEventGroup;

void vDebounceTimerCallback(TimerHandle_t xTimer)
{
    configASSERT(xTimer);
    if (input_io_get_level(USER_BUTTON_GPIO) == 0) {
        xEventGroupSetBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED);
    }
}

static void configure_button(void)
{
    input_io_create(USER_BUTTON_GPIO, HI_TO_LO);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    output_io_create(BLINK_GPIO);
    /* Increase drive strength for better LED brightness */
    gpio_set_drive_capability(BLINK_GPIO, GPIO_DRIVE_CAP_3);
}

void button_callback(gpio_num_t gpio_num)
{
    if (gpio_num == USER_BUTTON_GPIO)
    {
        // Just collect data to handle in the other task, avoid doing heavy processing in the ISR
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
    }
}

void vTask1(void *pvParameters)
{
    while(1)
    {
        EventBits_t uxBits = xEventGroupWaitBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & BIT_EVENT_BUTTON_PRESSED)
        {
            //button is pressed
            if (input_io_get_level(USER_BUTTON_GPIO) == 0)
            {
                uint16_t time = input_io_press_time(USER_BUTTON_GPIO);
                if (time < 1000)
                {
                    printf("Short press detected, time=%dms\n", time);
                }
                else if (time < 3000)
                {
                    printf("Normal press detected, time=%dms\n", time);
                }
                else if (time < 5000)
                {
                    printf("Long press detected, time=%dms\n", time);
                }
                else
                {
                    printf("Timeout press detected, time=%dms\n", time);
                }

            }
        }
    }
}

void app_main(void)
{
    xEventGroup = xEventGroupCreate();
    configASSERT(xEventGroup);
    xDebounceTimer = xTimerCreate("Debounce Timer", pdMS_TO_TICKS(50), pdFALSE, NULL, vDebounceTimerCallback);
    configASSERT(xDebounceTimer);
    /* Configure the peripheral according to the LED type */
    configure_led();
    configure_button();
    input_set_callback(button_callback);


    xTaskCreate(vTask1, "Task 1", 4096*2, NULL, 4, NULL);
}
