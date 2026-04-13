#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include "pico/stdlib.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_y;

QueueHandle_t xQueueR;
QueueHandle_t xQueueY;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_PIN_R)
        xSemaphoreGiveFromISR(xSemaphore_r, 0);
    else if (gpio == BTN_PIN_Y)
        xSemaphoreGiveFromISR(xSemaphore_y, 0);
}

void btn_r_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    int state = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_r, portMAX_DELAY) == pdTRUE) {
            state = !state;
            xQueueSend(xQueueR, &state, 0);
        }
    }
}

void btn_y_task(void *p) {
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int state = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_y, portMAX_DELAY) == pdTRUE) {
            state = !state;
            xQueueSend(xQueueY, &state, 0);
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int state = 0;
    while (true) {
        xQueueReceive(xQueueR, &state, 0);

        if (state) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int state = 0;
    while (true) {
        xQueueReceive(xQueueY, &state, 0);

        if (state) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

int main() {
    stdio_init_all();

    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_y = xSemaphoreCreateBinary();
    xQueueR = xQueueCreate(4, sizeof(int));
    xQueueY = xQueueCreate(4, sizeof(int));

    xTaskCreate(btn_r_task, "BTN_Task R", 256, NULL, 1, NULL);
    xTaskCreate(btn_y_task, "BTN_Task Y", 256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_Task R", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Task Y", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true);
}