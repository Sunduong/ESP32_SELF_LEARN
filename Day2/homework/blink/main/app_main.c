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
static TickType_t time_pressed = 0;
static TickType_t time_released = 0;

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BIT_EVENT_BUTTON_PRESSED_DONE (1<<0)

//Requirment: Short press is < 1 second, normal press is 1-3 seconds, long press is > 3 seconds, and timeout press is > 5s

TimerHandle_t xDebounceTimer;
EventGroupHandle_t xEventGroup;

void vDebounceTimerCallback(TimerHandle_t xTimer)
{
    configASSERT(xTimer);
    if (input_io_get_level(USER_BUTTON_GPIO) == 0) //pressed
    {
        time_pressed = xTaskGetTickCount();
        xTimerStart(xTimers, 0);
    }
    else //released
    {
        time_released = xTaskGetTickCount();
        xEventGroupSetBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED_DONE);
        xTimerStop(xTimers, 0);
        output_io_set_level(BLINK_GPIO, 0);
    }
}

static void configure_button(void)
{
    input_io_create(USER_BUTTON_GPIO, ANY_EDGE);
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

void button_timeout_callback(gpio_num_t gpio_num)
{
    if (gpio_num == USER_BUTTON_GPIO)
    {
        printf("Button timeout event detected in callback, gpio_num = %d\n", gpio_num);
        output_io_set_level(BLINK_GPIO, 1);
    }
}

void vTask1(void *pvParameters)
{
    while(1)
    {
        EventBits_t uxBits = xEventGroupWaitBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED_DONE, pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & BIT_EVENT_BUTTON_PRESSED_DONE)
        {
            TickType_t delta_time = time_released - time_pressed;
            if (delta_time < pdMS_TO_TICKS(1000))
            {
                printf("Short press detected, delta_time = %ld ms\n", delta_time * portTICK_PERIOD_MS);
            }
            else if (delta_time < pdMS_TO_TICKS(3000))
            {
                printf("Normal press detected, delta_time = %ld ms\n", delta_time * portTICK_PERIOD_MS);
            }
            else if (delta_time < pdMS_TO_TICKS(5000))
            {
                printf("Long press detected, delta_time = %ld ms\n", delta_time * portTICK_PERIOD_MS);
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
    output_io_set_level(BLINK_GPIO, 0);
    input_set_callback(button_callback);
    input_set_timeout_callback(button_timeout_callback);


    xTaskCreate(vTask1, "Task 1", 4096*2, NULL, 4, NULL);
}
