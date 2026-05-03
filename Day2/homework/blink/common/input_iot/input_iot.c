#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "input_iot.h"

input_callback_t input_callback = NULL;

static void IRAM_ATTR gpio_input_handler(void *arg)
{
    int gpio_num = (uint32_t) arg;
    if (input_callback) {
        input_callback(gpio_num);
    }
}

void input_io_create(gpio_num_t gpio_num, interrupt_type_edge_t level)
{
    esp_rom_gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(gpio_num, level);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *)gpio_num);
}

int input_io_get_level(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

void input_set_callback(void *cb)
{
    input_callback = (input_callback_t) cb;
}

int input_io_press_time(gpio_num_t gpio_num)
{
    uint16_t time = 0;
    while (gpio_get_level(gpio_num) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        time += 100;
    }
    return time;
}
