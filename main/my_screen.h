#ifndef _MY_SCREEN_H_
#define _MY_SCREEN_H_

/* This is defining the pins that the screen is connected to. */
#define SCREEN_WIDTH 130
#define SCREEN_HEIGHT 160
#define OFFSET_X 0
#define OFFSET_Y 0
#define GPIO_MOSI 23
#define GPIO_SCLK 19
#define GPIO_CS 22
#define GPIO_DC 21
#define GPIO_RESET 18
#define INTERVAL 500
#define WAIT vTaskDelay(INTERVAL/portTICK_PERIOD_MS)

static void SPIFFS_Directory(char * path);
void screen_task(void *pvParameters);
TickType_t Option1Display(ST7735_t * dev, FontxFile *fx, int width, int height);
TickType_t Option2Display(ST7735_t * dev, FontxFile *fx, int width, int height);
TickType_t Option3Display(ST7735_t * dev, FontxFile *fx, int width, int height);
TickType_t Option4Display(ST7735_t * dev, FontxFile *fx, int width, int height);



#endif