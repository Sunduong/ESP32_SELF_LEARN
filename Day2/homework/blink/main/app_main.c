/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
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
#include "driver/uart.h"

static const char *TAG = "example";

static TickType_t time_pressed = 0;
static TickType_t time_released = 0;

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BIT_EVENT_BUTTON_PRESSED_DONE (1<<0)

//Requirment: Short press is < 1 second, normal press is 1-3 seconds, long press is > 3 seconds, and timeout press is > 5s

TimerHandle_t xDebounceTimer;
EventGroupHandle_t xEventGroup;
TimerHandle_t xBlinkTimer; 

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

//UART handle
#define EX_UART_NUM UART_NUM_1
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart1_queue; 

static int blink_period = 100; // Default blink period in milliseconds

void vBlinkTimerCallback(TimerHandle_t xTimer)
{
    static bool led_on = false;
    led_on = !led_on;
    output_io_set_level(BLINK_GPIO, led_on ? 1 : 0);
}

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            xTimerStart(xBlinkTimer, 0);
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
            //Event of UART receiving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                dtmp[event.size] = 0;
                dtmp[strcspn((char*)dtmp, "\r\n")] = 0;
                uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);

                int new_period = atoi((char*)dtmp);

                if (new_period > 0)
                {
                    blink_period = new_period;
                    xTimerChangePeriod(xBlinkTimer, pdMS_TO_TICKS(blink_period), 0);
                    char msg[50];
                    int len = snprintf(msg, sizeof(msg), "Timer period changed to %d ms\n", blink_period);
                    uart_write_bytes(EX_UART_NUM, msg, len);
                }
                break;
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
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

    xBlinkTimer = xTimerCreate("Blink Timer by UART", (blink_period), pdTRUE, NULL, vBlinkTimerCallback);
    configASSERT(xBlinkTimer);
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart1_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    //Set UART pins (using UART1 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(EX_UART_NUM, 20);
    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 3072, NULL, 12, NULL);
}
