#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "st7735s.h"
#include "fontx.h"
#include "my_screen.h"

/**
 * It prints the contents of the directory.
 * 
 * @param path The path to the directory to list.
 */
static void SPIFFS_Directory(char * path){
    DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		//ESP_LOGI("ST7735","d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}
void screen_task(void *pvParameters){
    //Set font file
	FontxFile fx24[2];
	FontxFile fx16[2];
	FontxFile fx32[2];
	InitFontx(fx16,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
	ST7735_t dev;
	spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
	lcdInit(&dev, SCREEN_WIDTH, SCREEN_HEIGHT, OFFSET_X, OFFSET_Y);
    lcdFillScreen(&dev, BLACK);
    //Show my custom

	while (1) {
        Option1Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
        WAIT;
        Option2Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
        WAIT;
        Option3Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
        WAIT;
        Option4Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
        WAIT;
	}
}

TickType_t Option1Display(ST7735_t * dev, FontxFile *fx, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();
    lcdSetFontDirection(dev, DIRECTION270);
    //Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
    uint8_t ascii[30];

    //Effect
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, RED);

    lcdDrawFillRect(dev, 30, 20, 30 + fontHeight, 140, CYAN);
    strcpy((char*)ascii, "Set TimeLight");
    lcdDrawString(dev, fx, 45, 130, ascii, BLACK);

    lcdDrawFillRect(dev, 50, 20, 50 + fontHeight, 140, RED);
    strcpy((char*)ascii, "Manual Adjust");
    lcdDrawString(dev, fx, 65, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 70, 20, 70 + fontHeight, 140, RED);
    strcpy((char*)ascii, "  Slow Mode  ");
    lcdDrawString(dev, fx, 85, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 90, 20, 90 + fontHeight, 140, RED);
    strcpy((char*)ascii, "  Terminal   ");
    lcdDrawString(dev, fx, 105, 130, ascii, WHITE);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	return diffTick;
}
TickType_t Option2Display(ST7735_t * dev, FontxFile *fx, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();
    lcdSetFontDirection(dev, DIRECTION270);
    //Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
    uint8_t ascii[30];

    //Effect
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, RED);

    lcdDrawFillRect(dev, 30, 20, 30 + fontHeight, 140, RED);
    strcpy((char*)ascii, "Set TimeLight");
    lcdDrawString(dev, fx, 45, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 50, 20, 50 + fontHeight, 140, CYAN);
    strcpy((char*)ascii, "Manual Adjust");
    lcdDrawString(dev, fx, 65, 130, ascii, BLACK);

    lcdDrawFillRect(dev, 70, 20, 70 + fontHeight, 140, RED);
    strcpy((char*)ascii, "  Slow Mode  ");
    lcdDrawString(dev, fx, 85, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 90, 20, 90 + fontHeight, 140, RED);
    strcpy((char*)ascii, "  Terminal   ");
    lcdDrawString(dev, fx, 105, 130, ascii, WHITE);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	return diffTick;
}
TickType_t Option3Display(ST7735_t * dev, FontxFile *fx, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();
    lcdSetFontDirection(dev, DIRECTION270);
    //Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
    uint8_t ascii[30];

    //Effect
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, RED);

    lcdDrawFillRect(dev, 30, 20, 30 + fontHeight, 140, RED);
    strcpy((char*)ascii, "Set TimeLight");
    lcdDrawString(dev, fx, 45, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 50, 20, 50 + fontHeight, 140, RED);
    strcpy((char*)ascii, "Manual Adjust");
    lcdDrawString(dev, fx, 65, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 70, 20, 70 + fontHeight, 140, CYAN);
    strcpy((char*)ascii, "  Slow Mode  ");
    lcdDrawString(dev, fx, 85, 130, ascii, BLACK);

    lcdDrawFillRect(dev, 90, 20, 90 + fontHeight, 140, RED);
    strcpy((char*)ascii, "  Terminal   ");
    lcdDrawString(dev, fx, 105, 130, ascii, WHITE);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	return diffTick;
}

TickType_t Option4Display(ST7735_t * dev, FontxFile *fx, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();
    lcdSetFontDirection(dev, DIRECTION270);
    //Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
    uint8_t ascii[30];

    //Effect
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, RED);

    lcdDrawFillRect(dev, 30, 20, 30 + fontHeight, 140, RED);
    strcpy((char*)ascii, "Set TimeLight");
    lcdDrawString(dev, fx, 45, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 50, 20, 50 + fontHeight, 140, RED);
    strcpy((char*)ascii, "Manual Adjust");
    lcdDrawString(dev, fx, 65, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 70, 20, 70 + fontHeight, 140, RED);
    strcpy((char*)ascii, "  Slow Mode  ");
    lcdDrawString(dev, fx, 85, 130, ascii, WHITE);

    lcdDrawFillRect(dev, 90, 20, 90 + fontHeight, 140, CYAN);
    strcpy((char*)ascii, "  Terminal   ");
    lcdDrawString(dev, fx, 105, 130, ascii, BLACK);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	return diffTick;
}
