#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "st7735s.h"
#include "fontx.h"
#include "my_screen.h"

#define LED 2 
#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM    (3)       
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;
TaskHandle_t TaskHandler_uart;

void button_task(void *pvParameters);
static void uart_event_task(void *pvParameters);
bool isEqualString(char *str1, char *str2);

void app_main(void){
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
	uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 10,
		.format_if_mount_failed =true
	};
	esp_vfs_spiffs_register(&conf);

	xTaskCreate(&screen_task, "TFT Screen", 4096, NULL, 2, NULL);
	//xTaskCreate(&button_task, "Button", 4096, NULL, 2, NULL);
	xTaskCreate(&uart_event_task, "UART Task", 4096, NULL, 2, NULL);
}
void button_task(void *pvParameters){
	while(1){
		gpio_set_level(LED, 1);
		vTaskDelay(1000/portTICK_PERIOD_MS);
		gpio_set_level(LED, 0);
		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}
bool isEqualString(char *str1, char *str2){
    bool check=true;
    for(int i=0;i<strlen(str2);i++){
        if(str1[i]!=str2[i]){
            check=false;
            break;
        }
    }
    return check;
}

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    char* dtmp = (char*) malloc(RD_BUF_SIZE);
    while(1) {
        //Waiting for UART event.
        printf("\n----------CONTROLL TRAFFIC LIGHT VIA TERMINAL----------\n");
        printf("1. Set TimeLight:     !SETTIME! \n");
        printf("2. Manual Adjust:     !ADJ! \n");
        printf("3. Slow Mode:         !SLOW! \n");
        printf("4. Show Current Time: !SHOW! \n");

        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            //if(event.data)
            switch(event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    printf("%s", dtmp);
                    if(isEqualString(dtmp, "!SETTIME!")){
                        gpio_set_level(LED, 1);

                    }else if(isEqualString(dtmp, "!ADJ!")){
                        gpio_set_level(LED, 0);

                    }else if(isEqualString(dtmp, "!SLOW!")){
                        for(int i=0;i<5;i++){
                            gpio_set_level(LED, 1);
                            vTaskDelay(500/portTICK_PERIOD_MS);
                            gpio_set_level(LED, 0);
                            vTaskDelay(500/portTICK_PERIOD_MS);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}


