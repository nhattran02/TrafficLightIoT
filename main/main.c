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


#define LED                 2 
#define BUTTON_UP           15
#define BUTTON_DOWN         4
#define BUTTON_ENTER        13
#define EX_UART_NUM         UART_NUM_0
#define PATTERN_CHR_NUM     (3)       
#define BUF_SIZE            (1024)
#define RD_BUF_SIZE         (BUF_SIZE)
#define SCREEN_WIDTH        130
#define SCREEN_HEIGHT       160
#define OFFSET_X            0
#define OFFSET_Y            0
#define GPIO_MOSI           23
#define GPIO_SCLK           19
#define GPIO_CS             22
#define GPIO_DC             21
#define GPIO_RESET          18
#define INTERVAL            2000
#define WAIT                vTaskDelay(INTERVAL/portTICK_PERIOD_MS)
#define LONG_PRESS_DELAY    3000

static QueueHandle_t uart0_queue;
static QueueHandle_t button_queue;
static QueueHandle_t option_queue;
static QueueHandle_t option_chosen_queue;
static QueueHandle_t long_press_queue;

TaskHandle_t TaskHandler_uart;

static void button_task(void *pvParameters);
static void uart_event_task(void *pvParameters);
static void IRAM_ATTR gpio_interrupt_handler(void *args);
static void screen_task(void *pvParameters);
static void Option1Display(ST7735_t * dev, FontxFile *fx, int width, int height);
static void Option2Display(ST7735_t * dev, FontxFile *fx, int width, int height);
static void Option3Display(ST7735_t * dev, FontxFile *fx, int width, int height);
static void Option4Display(ST7735_t * dev, FontxFile *fx, int width, int height);
static void SetTimeLightDisplay(ST7735_t * dev, FontxFile *fx, int width, int height);

static void GlobalConfig(void);
static void OptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option);
bool isEqualString(char *str1, char *str2);


void app_main(void)
{
    GlobalConfig();
    //Create tasks
	xTaskCreate(&button_task, "Button",    4096, NULL, 2, NULL);
	xTaskCreate(&screen_task, "TFT Screen", 4096, NULL, 2, NULL);
	//xTaskCreate(&uart_event_task, "UART Task", 4096, NULL, 2, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_UP, gpio_interrupt_handler, (void *)BUTTON_UP);
    gpio_isr_handler_add(BUTTON_DOWN, gpio_interrupt_handler, (void *)BUTTON_DOWN);
    gpio_isr_handler_add(BUTTON_ENTER, gpio_interrupt_handler, (void *)BUTTON_ENTER);

}
static void OptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option)
{
    switch(option){
        case 0:
        case 1:
            Option1Display(dev, fx, width, height);
            break;
        case 2:
            Option2Display(dev, fx, width, height);
            break;
        case 3:
            Option3Display(dev, fx, width, height);
            break;
        case 4:
        case 5:
            Option4Display(dev, fx, width, height);
            break;
        default:
            break;
    }
}

static void button_task(void *pvParameters)
{
    TickType_t start_time;
    TickType_t current_time;
    int pinNumber;
    int option_counting = 1;
	while(1){
        if(xQueueReceive(button_queue, &pinNumber, portMAX_DELAY)){
            //disable the interrupt
            gpio_isr_handler_remove(pinNumber);
            if(gpio_get_level(pinNumber)==1 && pinNumber == BUTTON_UP){
                vTaskDelay(100/portTICK_PERIOD_MS);
                if(gpio_get_level(pinNumber)==1 && pinNumber == BUTTON_UP){
                    //do something
                    if(option_counting<=1){
                        option_counting=1;
                    }else{
                        --option_counting ;
                    }
                    xQueueSend(option_queue, &option_counting, NULL);
                }
            }else if(gpio_get_level(pinNumber)==1 && pinNumber == BUTTON_DOWN){
                vTaskDelay(100/portTICK_PERIOD_MS);
                if(gpio_get_level(pinNumber)==1 && pinNumber == BUTTON_DOWN){
                    //do something
                    if(option_counting>=4){
                        option_counting=4;
                    }else{
                        ++option_counting ;
                    }
                    xQueueSend(option_queue, &option_counting, NULL);
                }
            }else if(gpio_get_level(pinNumber)==1 && pinNumber == BUTTON_ENTER){
                vTaskDelay(100/portTICK_PERIOD_MS);
                if(gpio_get_level(pinNumber)==1 && pinNumber == BUTTON_ENTER){

                    xQueueSend(option_chosen_queue, &option_counting, NULL);
                }
            }

            gpio_isr_handler_add(pinNumber, gpio_interrupt_handler, (void*)pinNumber); 

	    }
    }
}
bool isEqualString(char *str1, char *str2)
{
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

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    xQueueSendFromISR(button_queue, &pinNumber, NULL);
}

static void GlobalConfig(void)
{
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON_UP, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON_DOWN, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON_ENTER, GPIO_MODE_INPUT);
    gpio_pullup_dis(BUTTON_UP);
    gpio_pullup_dis(BUTTON_DOWN);
    gpio_pullup_dis(BUTTON_ENTER);
    gpio_pulldown_en(BUTTON_UP);
    gpio_pulldown_en(BUTTON_DOWN);
    gpio_pulldown_en(BUTTON_ENTER);
    gpio_set_intr_type(BUTTON_UP, GPIO_INTR_POSEDGE);
    gpio_set_intr_type(BUTTON_DOWN, GPIO_INTR_POSEDGE);
    gpio_set_intr_type(BUTTON_ENTER, GPIO_INTR_POSEDGE);

    //uart config 
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
    //spiff config 
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 10,
		.format_if_mount_failed =true
	};
	esp_vfs_spiffs_register(&conf);
    button_queue = xQueueCreate(1, sizeof(int));
    option_queue = xQueueCreate(1, sizeof(int));
    option_chosen_queue = xQueueCreate(1, sizeof(int));
    long_press_queue = xQueueCreate(1, sizeof(int));
}
static void screen_task(void *pvParameters)
{
    //Set font file
    int option_counting = 1;
    int option_chosen = 1;
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
    OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
	while (1) {
        if(xQueueReceive(option_queue, &option_counting, portTICK_PERIOD_MS)){
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
            //printf("Display option %d !\n", option_counting);

        }else{
            if(xQueueReceive(option_chosen_queue, &option_chosen, portTICK_PERIOD_MS)){
                //printf("Option %d is chosen!\n", option_chosen);
                switch(option_chosen){
                    case 1:
                        SetTimeLightDisplay(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                        lcdFillScreen(&dev, BLACK);
                        break;
                    case 2:
                        lcdFillScreen(&dev, BLACK);
                        break;
                    case 3:
                        lcdFillScreen(&dev, BLACK);
                        break;
                    case 4:      
                        xTaskCreate(&uart_event_task, "UART Task", 4096, NULL, 2, NULL);
                        lcdFillScreen(&dev, BLACK);
                        break;
                    default: 
                        break;
                }
            }
        }
	}
}
static void Option1Display(ST7735_t * dev, FontxFile *fx, int width, int height) 
{
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
}
static void Option2Display(ST7735_t * dev, FontxFile *fx, int width, int height) 
{
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

}
static void Option3Display(ST7735_t * dev, FontxFile *fx, int width, int height) 
{
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

}


static void Option4Display(ST7735_t * dev, FontxFile *fx, int width, int height) 
{
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

}

static void SetTimeLightDisplay(ST7735_t * dev, FontxFile *fx, int width, int height){
    lcdSetFontDirection(dev, DIRECTION270);
    //Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
    uint8_t ascii[30];
    lcdFillScreen(dev, BLACK);
    //Effect
    strcpy((char*)ascii, "   Set TimeLight   ");
    lcdDrawString(dev, fx, 15, 160, ascii, RED);
    lcdDrawLine(dev, 15, 160, 15, 0, GREEN);
}