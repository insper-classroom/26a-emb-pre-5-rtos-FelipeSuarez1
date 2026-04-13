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

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

void btn_callback(uint gpio, uint32_t events) {
    uint pin = gpio;
    xQueueSendFromISR(xQueueBtn, &pin, 0);
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    uint pin;
    while (true) {
        if (xQueueReceive(xQueueBtn, &pin, portMAX_DELAY) == pdTRUE) {
            if (pin == BTN_PIN_R)
                xSemaphoreGive(xSemaphoreLedR);
            else if (pin == BTN_PIN_Y)
                xSemaphoreGive(xSemaphoreLedY);
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int state = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE)
            state = !state;

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
        if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE)
            state = !state;

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

    xQueueBtn = xQueueCreate(4, sizeof(uint));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    xTaskCreate(btn_task,   "BTN_Task",   256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_Task R", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Task Y", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true);
}