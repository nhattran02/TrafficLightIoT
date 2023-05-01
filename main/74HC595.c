/*
 * 74HC595.c
 *
 *  Created on: 11.04.2023
 *      Author: Tran Minh Nhat - 2014008
 */

#include "74HC595.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * Init IC 74HC595
 *
 * @param   IC74HC595                 Pointer to IC74HC595_t configuration structure
 * @param   dataPin                   data pin
 * @param   latchPin                  latch pin
 * @param   clkPin                    clock pin
 * 
 * @return
 * None
 */
void Init74HC595(IC74HC595_t *IC74HC595, int16_t dataPin, int16_t latchPin, int16_t clkPin)
{
    
    gpio_reset_pin(dataPin);
	gpio_set_direction(dataPin, GPIO_MODE_OUTPUT);
	gpio_set_level(dataPin, 0);

    gpio_reset_pin(latchPin);
	gpio_set_direction(latchPin, GPIO_MODE_OUTPUT);
	gpio_set_level(latchPin, 0);

    gpio_reset_pin(clkPin);
	gpio_set_direction(clkPin, GPIO_MODE_OUTPUT);
	gpio_set_level(clkPin, 0);

    IC74HC595->dataPin = dataPin;
    IC74HC595->latchPin = latchPin;
    IC74HC595->clkPin = clkPin;
}

/**
 * Make a clock in shift clock pin
 *
 * @param   IC74HC595                 Pointer to IC74HC595_t configuration structure
 *
 * @return
 *          - ESP_OK                  if success
 *          - ESP_FAIL                if fails
 */
void clock74HC595(IC74HC595_t *IC74HC595)
{
    gpio_set_level(IC74HC595->clkPin, 1);
    gpio_set_level(IC74HC595->clkPin, 0);
}
/**
 * Make a clock in latch clock pin
 *
 * @param   IC74HC595                 Pointer to IC74HC595_t configuration structure
 *
 * @return
 *          - ESP_OK                  if success
 *          - ESP_FAIL                if fails
 */
void latch74HC595(IC74HC595_t *const IC74HC595)
{
    gpio_set_level(IC74HC595->latchPin, 1);
    gpio_set_level(IC74HC595->latchPin, 0);
}

/**
 * Write 1 byte to IC 74HC595
 *
 * @param   IC74HC595                 Pointer to IC74HC595_t configuration structure
 * @param   data                      1 byte data need to write into chip
 *
 * @return
 *          - ESP_OK                  if success
 *          - ESP_FAIL                if fails
 */
void writeByte74HC595(IC74HC595_t *IC74HC595, uint8_t data)
{

    uint8_t idx;
    for(idx = 0; idx < 8; idx++){
        if(data & (0x01 << (7 - idx))){
            gpio_set_level(IC74HC595->dataPin, 1);
            // vTaskDelay(pdMS_TO_TICKS(1000)); 
        }else {
            gpio_set_level(IC74HC595->dataPin, 0);
            // vTaskDelay(pdMS_TO_TICKS(1000)); 
        }
        clock74HC595(IC74HC595);
        // vTaskDelay(pdMS_TO_TICKS(1000)); 

    }
    // vTaskDelay(pdMS_TO_TICKS(1000)); 
    // latch74HC595(IC74HC595);

}

/**
 * Write 4 byte to IC 74HC595
 *
 * @param   IC74HC595                 Pointer to IC74HC595_t configuration structure
 * @param   data                      1 byte data need to write into chip
 *
 * @return
 *          - ESP_OK                  if success
 *          - ESP_FAIL                if fails
 */
void write4Byte74HC595(IC74HC595_t *IC74HC595, uint8_t data4, uint8_t data3, uint8_t data2, uint8_t data1)
{

    writeByte74HC595(IC74HC595, data4);
    writeByte74HC595(IC74HC595, data3);
    writeByte74HC595(IC74HC595, data2);
    writeByte74HC595(IC74HC595, data1);
    latch74HC595(IC74HC595);
}



void delay_ms(int ms) 
{
	int _ms = ms + (portTICK_PERIOD_MS - 1);
	TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
	vTaskDelay(xTicksToDelay);
}





