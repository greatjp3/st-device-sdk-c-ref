/* ***************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/


#include "device_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/pwm.h"

inline uint16_t get_servo_duty_cycle(double angle) {
  if (angle > 180) return -1;
  return (uint16_t)((0.025 + (0.12 - 0.025) * (angle / 180.0)) *
                    (double)UINT16_MAX);
}

void push_momentary(void)
{
    // guppy1 
    printf("1\n");
    pwm_set_duty(0, 1000);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("2\n");
    pwm_set_duty(0, 1600);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    pwm_set_duty(0, 0);
    pwm_start();
    printf("3\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
    // tetra
    printf("4\n");
    pwm_set_duty(1, 1000);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("5\n");
    pwm_set_duty(1, 1600);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    pwm_set_duty(1, 0);
    pwm_start();
    printf("6\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
    // //gold fish
    printf("7\n");
    pwm_set_duty(2, 1000);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("8\n");
    pwm_set_duty(2, 1600);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("9\n");
    pwm_set_duty(2, 1000);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("10\n");
    pwm_set_duty(2, 1600);
    pwm_start();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("11\n");
    pwm_set_duty(2, 0);
    pwm_start();
    printf("12\n");
    pwm_stop(0x0);
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void change_switch_state(int switch_state)
{
    if (switch_state == SWITCH_OFF) {
        gpio_set_level(GPIO_OUTPUT_TETRA, MAINLED_GPIO_ON);
        gpio_set_level(GPIO_OUTPUT_GUPPY, MAINLED_GPIO_ON);
    } else {
        gpio_set_level(GPIO_OUTPUT_TETRA, MAINLED_GPIO_OFF);
        gpio_set_level(GPIO_OUTPUT_GUPPY, MAINLED_GPIO_OFF);
    }
}

int get_button_event(int* button_event_type, int* button_event_count)
{
    static uint32_t button_count = 0;
    static uint32_t button_last_state = BUTTON_GPIO_RELEASED;
    static TimeOut_t button_timeout;
    static TickType_t long_press_tick = pdMS_TO_TICKS(BUTTON_LONG_THRESHOLD_MS);
    static TickType_t button_delay_tick = pdMS_TO_TICKS(BUTTON_DELAY_MS);

    uint32_t gpio_level = 0;

    gpio_level = gpio_get_level(GPIO_INPUT_BUTTON);
    if (button_last_state != gpio_level) {
        /* wait debounce time to ignore small ripple of currunt */
        vTaskDelay( pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS) );
        gpio_level = gpio_get_level(GPIO_INPUT_BUTTON);
        if (button_last_state != gpio_level) {
            printf("Button event, val: %d, tick: %u\n", gpio_level, (uint32_t)xTaskGetTickCount());
            button_last_state = gpio_level;
            if (gpio_level == BUTTON_GPIO_PRESSED) {
                button_count++;
            }
            vTaskSetTimeOutState(&button_timeout);
            button_delay_tick = pdMS_TO_TICKS(BUTTON_DELAY_MS);
            long_press_tick = pdMS_TO_TICKS(BUTTON_LONG_THRESHOLD_MS);
        }
    } else if (button_count > 0) {
        if ((gpio_level == BUTTON_GPIO_PRESSED)
                && (xTaskCheckForTimeOut(&button_timeout, &long_press_tick ) != pdFALSE)) {
            *button_event_type = BUTTON_LONG_PRESS;
            *button_event_count = 1;
            button_count = 0;
            return true;
        } else if ((gpio_level == BUTTON_GPIO_RELEASED)
                && (xTaskCheckForTimeOut(&button_timeout, &button_delay_tick ) != pdFALSE)) {
            *button_event_type = BUTTON_SHORT_PRESS;
            *button_event_count = button_count;
            button_count = 0;
            return true;
        }
    }

    return false;
}

void led_blink(int switch_state, int delay, int count)
{
    for (int i = 0; i < count; i++) {
        vTaskDelay(delay / portTICK_PERIOD_MS);
        change_switch_state(1 - switch_state);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        change_switch_state(switch_state);
    }
}

void change_led_mode(int noti_led_mode)
{
    static TimeOut_t led_timeout;
    static TickType_t led_tick = -1;
    static int last_led_mode = -1;
    static int led_state = SWITCH_OFF;

    if (last_led_mode != noti_led_mode) {
        last_led_mode = noti_led_mode;
        vTaskSetTimeOutState(&led_timeout);
        led_tick = 0;
    }

    switch (noti_led_mode)
    {
        case LED_ANIMATION_MODE_IDLE:
            break;
        case LED_ANIMATION_MODE_SLOW:
            if (xTaskCheckForTimeOut(&led_timeout, &led_tick ) != pdFALSE) {
                led_state = 1 - led_state;
                change_switch_state(led_state);
                vTaskSetTimeOutState(&led_timeout);
                if (led_state == SWITCH_ON) {
                    led_tick = pdMS_TO_TICKS(200);
                } else {
                    led_tick = pdMS_TO_TICKS(800);
                }
            }
            break;
        case LED_ANIMATION_MODE_FAST:
            if (xTaskCheckForTimeOut(&led_timeout, &led_tick ) != pdFALSE) {
                led_state = 1 - led_state;
                change_switch_state(led_state);
                vTaskSetTimeOutState(&led_timeout);
                led_tick = pdMS_TO_TICKS(100);
            }
            break;
        default:
            break;
    }
}

void iot_gpio_init(void)
{
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = 1 << GPIO_OUTPUT_GUPPY;
	io_conf.pull_down_en = 1;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
    io_conf.pin_bit_mask = 1 << GPIO_OUTPUT_TETRA;
    gpio_config(&io_conf);
	io_conf.pin_bit_mask = 1 << GPIO_OUTPUT_MAINLED_0;
	gpio_config(&io_conf);

	io_conf.pin_bit_mask = 1 << GPIO_OUTPUT_NOUSE1;
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = 1 << GPIO_OUTPUT_NOUSE2;
	gpio_config(&io_conf);


	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1 << GPIO_INPUT_BUTTON;
	io_conf.pull_down_en = (BUTTON_GPIO_RELEASED == 0);
	io_conf.pull_up_en = (BUTTON_GPIO_RELEASED == 1);
	gpio_config(&io_conf);

	gpio_set_intr_type(GPIO_INPUT_BUTTON, GPIO_INTR_ANYEDGE);

	gpio_install_isr_service(0);

	gpio_set_level(GPIO_OUTPUT_GUPPY, MAINLED_GPIO_ON);
    gpio_set_level(GPIO_OUTPUT_TETRA, MAINLED_GPIO_ON);
	gpio_set_level(GPIO_OUTPUT_MAINLED_0, 0);
}


void iot_pwm_init(void)
{
    uint32_t pin_no[3];
    uint32_t duty[3];
    pin_no[0] = (uint32_t) GPIO_FEED_GUPPY;
    duty[0] = (uint32_t) 0;
    pin_no[1] = (uint32_t) GPIO_FEED_TETRA;
    duty[1] = (uint32_t) 0;
    pin_no[2] = (uint32_t) GPIO_FEED_GOLDFISH;
    duty[2] = (uint32_t) 0;

    if( pwm_init((uint32_t) 20000, duty, 3, pin_no) != ESP_OK){
        printf("pwm_init fail\n");
    };

    float phase[3] = { 0.0f, 0.0f, 0.0f};
    if( pwm_set_phases(phase) != ESP_OK){
        printf("pwm_set_phase fail\n");
    };
    pwm_start();
}
