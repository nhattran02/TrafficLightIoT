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
#define INTERVAL 200
#define WAIT vTaskDelay(INTERVAL/portTICK_PERIOD_MS)

static const char *TAG = "TrafficLightIoT";

static void SPIFFS_Directory(char * path);
void tft(void *pvParameters);

void app_main(void){

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 10,
		.format_if_mount_failed =true
	};
	esp_err_t ret =esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret ==ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret== ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total,&used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}
	xTaskCreate(tft, "TFT", 4096, NULL, 2, NULL);
}

void tft(void *pvParameters)
{
	// set font file
	FontxFile fx16[2];
	FontxFile fx24[2];
	FontxFile fx32[2];
	InitFontx(fx16,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
	ST7735_t dev;
	spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
	lcdInit(&dev, SCREEN_WIDTH, SCREEN_HEIGHT, OFFSET_X, OFFSET_Y);
	//Effect
 	
	while (1) {
		lcdFillScreen(&dev, GREEN);
		WAIT;
		lcdDrawFillRect(&dev, 0, 0, 20, 150, RED);
		WAIT;
	}
}

/**
 * It prints the contents of the directory.
 * 
 * @param path The path to the directory to list.
 */
static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(TAG,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}
