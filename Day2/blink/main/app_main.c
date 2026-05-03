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
#define BLINK_GPIO          CONFIG_BLINK_GPIO
#define GPIO_USER_BUTTON    GPIO_NUM_5

TimerHandle_t xTimers[2];
TimerHandle_t xDebounceTimer;
EventGroupHandle_t xEventGroup;

#define BIT_EVENT_BUTTON_PRESSED    (1<<0)
#define BIT_EVENT_UART_RECEIVED     (1<<4)

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    output_io_create(BLINK_GPIO);
    /* Increase drive strength for better LED brightness */
    gpio_set_drive_capability(BLINK_GPIO, GPIO_DRIVE_CAP_3);
}

static void configure_button(void)
{
    ESP_LOGI(TAG, "Example configured to use GPIO User Button!");
    input_io_create(GPIO_USER_BUTTON, HI_TO_LO);
}

void vDebounceTimerCallback(TimerHandle_t xTimer)
{
    configASSERT(xTimer);
    if (input_io_get_level(GPIO_USER_BUTTON) == 0) {
        xEventGroupSetBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED);
    }
}

void vTask1(void *pvParameters)
{
    while (1) {
        EventBits_t uxBits = xEventGroupWaitBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED | BIT_EVENT_UART_RECEIVED, 
                                                pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & BIT_EVENT_BUTTON_PRESSED) {
            printf("Button event received in Task 1\n");
            printf("Toggling the LED, LED state: %d\n", input_io_get_level(GPIO_NUM_11));
            // output_io_toggle(BLINK_GPIO); 
            /*Trong embedded, state phải do software quản lý, 
            không dựa vào đọc lại hardware (trừ khi thật sự cần) */
            static int x = 0;
            gpio_set_level(BLINK_GPIO, x);
            x = !x;
            printf("Toggled the LED, new LED state: %d\n", input_io_get_level(GPIO_NUM_11));
        }
        if (uxBits & BIT_EVENT_UART_RECEIVED) {
            printf("UART event received in Task 1\n");
        }
    }
}

void IRAM_ATTR button_callback(int gpio_num)
{
    if (gpio_num == GPIO_USER_BUTTON)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTimerResetFromISR(xDebounceTimer,
                           &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

void app_main(void)
{
    xEventGroup = xEventGroupCreate();
    configASSERT(xEventGroup);

    xDebounceTimer = xTimerCreate("Debounce Timer",
                                 pdMS_TO_TICKS(50),
                                 pdFALSE,
                                 NULL,
                                 vDebounceTimerCallback);
    configASSERT(xDebounceTimer);

    input_set_callback(button_callback);
    configure_button();
    configure_led();
    input_io_create(GPIO_NUM_11, HI_TO_LO);
    output_io_set_level(BLINK_GPIO, 0); /* Ensure known initial LED state */
    
    xTaskCreate(vTask1, "Task 1", 4096*2, NULL, 4, NULL);
}
