/*
 * 74HC595.h
 *
 *  Created on: 11.04.2023
 *      Author: Tran Minh Nhat - 2014008
 */

#ifndef MAIN_74HC595_H_
#define MAIN_74HC595_H_
#include "driver/gpio.h"

typedef struct {
    int16_t dataPin;
    int16_t latchPin;
    int16_t clkPin;
}IC74HC595_t;

void Init74HC595(IC74HC595_t *IC74HC595, int16_t dataPin, int16_t latchPin, int16_t clkPin);
void clock74HC595(IC74HC595_t *const IC74HC595);
void latch74HC595(IC74HC595_t *const IC74HC595);
void writeByte74HC595(IC74HC595_t *const IC74HC595, uint8_t data);
void write4Byte74HC595(IC74HC595_t *IC74HC595, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4);
void delay_ms(int ms);

#endif