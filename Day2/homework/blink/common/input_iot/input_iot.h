
#ifndef INPUT_IO_H
#define INPUT_IO_H
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define USER_BUTTON_GPIO GPIO_NUM_5

typedef enum{
    LO_TO_HI = 1,
    HI_TO_LO = 2,
    ANY_EDGE = 3,
}   interrupt_type_edge_t;

typedef void (*input_callback_t) (int);
typedef void (*timeoutButton_t) (int);
extern TimerHandle_t xTimers;

void input_io_create(gpio_num_t gpio_num, interrupt_type_edge_t level);
int input_io_get_level(gpio_num_t gpio_num);
void input_set_callback(void *cb);
void input_set_timeout_callback(void *cb);


#endif /* INPUT_IO_H */
