/*Author: Tran Minh Nhat - 2014008*/

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
#include "esp_spiffs.h"
#include "st7735s.h"
#include "fontx.h"


#define LED                     2 
#define BUTTON_UP               15
#define BUTTON_DOWN             4
#define BUTTON_ENTER            13
#define EX_UART_NUM             UART_NUM_0
#define PATTERN_CHR_NUM         (3)       
#define BUF_SIZE                (1024)
#define RD_BUF_SIZE             (BUF_SIZE)
#define SCREEN_WIDTH            130
#define SCREEN_HEIGHT           160
#define OFFSET_X                0
#define OFFSET_Y                0
#define GPIO_MOSI               23
#define GPIO_SCLK               19
#define GPIO_CS                 22
#define GPIO_DC                 21
#define GPIO_RESET              18
#define INTERVAL                2000    
#define WAIT                    vTaskDelay(INTERVAL/portTICK_PERIOD_MS)
#define LONG_PRESS_DURATION     2000/portTICK_PERIOD_MS
#define SHORT_PRESS_DURATION    200/portTICK_PERIOD_MS


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
static void IntroDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option3Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option4Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SetTimeLightDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void GlobalConfig(void);
static void OptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option);
static void DisableButtonInterrupt();
static void EnsableButtonInterrupt();

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
    TickType_t start_time = 0;
    TickType_t current_time;
    int pinNumber;
    int option_counting = 1;
    bool is_long_press = false;
    bool is_short_press = false;
    //gpio_isr_handler_add(pinNumber, gpio_interrupt_handler, (void*)pinNumber); 
	while(1){
        if(xQueueReceive(button_queue, &pinNumber, portMAX_DELAY)){
            //disable the interrupt
            gpio_isr_handler_remove(pinNumber);
            if(gpio_get_level(pinNumber)==1){
                start_time = xTaskGetTickCount();
                is_long_press = false;
                is_short_press = false;
            }
            while(gpio_get_level(pinNumber)==1){
                current_time = xTaskGetTickCount();
                if((current_time - start_time > LONG_PRESS_DURATION) && !is_long_press){
                    //long press detected
                    switch(pinNumber){
                        case BUTTON_UP:
                            printf("Long press button up!\n");

                            break;
                        case BUTTON_DOWN:
                            printf("Long press button down!\n");

                            break;
                        case BUTTON_ENTER:
                            printf("Long press button enter!\n");
                            xQueueSend(long_press_queue, &pinNumber, NULL);
                            break;
                        default:
                            break;
                    } 
                    is_long_press = true;
                    is_short_press = false;
                }else if((current_time - start_time < SHORT_PRESS_DURATION) && !is_short_press){
                    //short press detected
                    switch(pinNumber){
                        case BUTTON_UP:
                            printf("Short press button up!\n");
                            if(option_counting<=1){
                                option_counting=1;
                            }else{
                                --option_counting ;
                            }
                            xQueueSend(option_queue, &option_counting, NULL);
                            break;
                        case BUTTON_DOWN:
                            printf("Short press button down!\n");
                            if(option_counting>=4){
                                option_counting=4;
                            }else{
                                ++option_counting ;
                            }
                            xQueueSend(option_queue, &option_counting, NULL);
                            break;
                        case BUTTON_ENTER:
                            printf("Short press button enter!\n");
                            xQueueSend(option_chosen_queue, &option_counting, NULL);
                            break;
                        default:
                            break;
                    }
                    is_short_press = true;
                    is_long_press = false;
                }
                vTaskDelay(50/portTICK_PERIOD_MS); //debounce
            }
            gpio_isr_handler_add(pinNumber, gpio_interrupt_handler, (void*)pinNumber); 
	    }
    }
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
                    if(strncmp(dtmp, "!SETTIME!", strlen("!SETTIME!"))==0){
                        gpio_set_level(LED, 1);

                    }else if(strncmp(dtmp, "!ADJ!", strlen("!ADJ!"))==0){
                        gpio_set_level(LED, 0);

                    }else if(strncmp(dtmp, "!SLOW!", strlen("!SLOW!"))==0){
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

    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &(uart_config_t) {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    });
    uart_set_pin(EX_UART_NUM, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //spiff config 
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
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
    int option_counting = 1; //init 1
    int option_chosen = 1;
    int pin_number;
	static FontxFile fx16[2];
	InitFontx(fx16,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic

	static FontxFile fx24[2];
	InitFontx(fx24,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic

	static ST7735_t dev;
	spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
	lcdInit(&dev, SCREEN_WIDTH, SCREEN_HEIGHT, OFFSET_X, OFFSET_Y);
    //Show my custom
    IntroDisplay(&dev, fx24, SCREEN_WIDTH, SCREEN_HEIGHT);
    OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
    //for test:
    //SetTimeLightDisplay(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);

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
                        printf("Im here!\n");
                        // xTaskCreate(&time_display_task, "Time Display Task", 4096, NULL, 2, NULL);
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
                        while(!xQueueReceive(long_press_queue, &pin_number, portTICK_PERIOD_MS)){
                            SetTimeLightDisplay(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                        }
                        lcdFillScreen(&dev, BLACK);
                        break;
                    default: 
                        break;
                }
            }
        }
	}
}

static void IntroDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    DisableButtonInterrupt();
    const uint16_t colors[] = {WHITE, WHITE, WHITE};
    const char *strings[] = {"Traffic", "Light", "Control"};
    const uint8_t x[] = {50, 75, 100};
    const uint8_t y[] = {155, 120, 85};

    lcdSetFontDirection(dev, DIRECTION270);
    lcdFillScreen(dev, BLACK);

    for (int i = 0; i < 3; i++) {
        lcdDrawString(dev, fx, x[i], y[i], (uint8_t*)strings[i], colors[i]);
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);    
    lcdFillScreen(dev, BLACK);
    const char *str[] = {"Start in: 3", "Start in: 2", "Start in: 1"};
    for(int i = 0; i < 3 ; i++){
        lcdDrawString(dev, fx, 70, 150, (uint8_t*)str[i], colors[i]);
        vTaskDelay(500/portTICK_PERIOD_MS);    
        lcdFillScreen(dev, BLACK);
    }
    EnsableButtonInterrupt();
}
static void Option1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height) 
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"Set TimeLight",
        (const uint8_t*)"Manual Adjust",
        (const uint8_t*)"  Slow Mode  ",
        (const uint8_t*)"  Terminal   ",
    };
    const uint16_t colors[] = {BLACK, WHITE, WHITE, WHITE};
    const uint16_t bgColors[] = {WHITE, BLACK, BLACK, BLACK};
    const int offsets[] = {30, 50, 70, 90};
    const int captionXOffsets[] = {45, 65, 85, 105};

    const int numCaptions = sizeof(captions) / sizeof(captions[0]);
    for (int i = 0; i < numCaptions; ++i) {
        const uint8_t* const caption = captions[i];
        const uint16_t color = colors[i];
        const uint16_t bgColor = bgColors[i];
        const int offset = offsets[i];
        const int captionXOffset = captionXOffsets[i];
        lcdDrawFillRect(dev, offset, 20, offset + fontHeight, 140, bgColor);
        lcdDrawString(dev, fx, captionXOffset, 130, caption, color);
    }

}


static void Option2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height) 
{
    lcdSetFontDirection(dev, DIRECTION270);
     static uint8_t ascii[30];
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"Set TimeLight",
        (const uint8_t*)"Manual Adjust",
        (const uint8_t*)"  Slow Mode  ",
        (const uint8_t*)"  Terminal   ",
    };
    const uint16_t colors[] = {WHITE, BLACK, WHITE, WHITE};
    const uint16_t bgColors[] = {BLACK, WHITE, BLACK, BLACK};
    const int offsets[] = {30, 50, 70, 90};
    const int captionXOffsets[] = {45, 65, 85, 105};

    const int numCaptions = sizeof(captions) / sizeof(captions[0]);
    for (int i = 0; i < numCaptions; ++i) {
        const uint8_t* const caption = captions[i];
        const uint16_t color = colors[i];
        const uint16_t bgColor = bgColors[i];
        const int offset = offsets[i];
        const int captionXOffset = captionXOffsets[i];
        lcdDrawFillRect(dev, offset, 20, offset + fontHeight, 140, bgColor);
        lcdDrawString(dev, fx, captionXOffset, 130, caption, color);
    }
}

static void Option3Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height) 
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"Set TimeLight",
        (const uint8_t*)"Manual Adjust",
        (const uint8_t*)"  Slow Mode  ",
        (const uint8_t*)"  Terminal   ",
    };
    const uint16_t colors[] = {WHITE, WHITE, BLACK, WHITE};
    const uint16_t bgColors[] = {BLACK, BLACK, WHITE, BLACK};
    const int offsets[] = {30, 50, 70, 90};
    const int captionXOffsets[] = {45, 65, 85, 105};

    const int numCaptions = sizeof(captions) / sizeof(captions[0]);
    for (int i = 0; i < numCaptions; ++i) {
        const uint8_t* const caption = captions[i];
        const uint16_t color = colors[i];
        const uint16_t bgColor = bgColors[i];
        const int offset = offsets[i];
        const int captionXOffset = captionXOffsets[i];
        lcdDrawFillRect(dev, offset, 20, offset + fontHeight, 140, bgColor);
        lcdDrawString(dev, fx, captionXOffset, 130, caption, color);
    }
}

static void Option4Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height) 
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "TrafficLight Control");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"Set TimeLight",
        (const uint8_t*)"Manual Adjust",
        (const uint8_t*)"  Slow Mode  ",
        (const uint8_t*)"  Terminal   ",
    };
    const uint16_t colors[] = {WHITE, WHITE, WHITE, BLACK};
    const uint16_t bgColors[] = {BLACK, BLACK, BLACK, WHITE};
    const int offsets[] = {30, 50, 70, 90};
    const int captionXOffsets[] = {45, 65, 85, 105};

    const int numCaptions = sizeof(captions) / sizeof(captions[0]);
    for (int i = 0; i < numCaptions; ++i) {
        const uint8_t* const caption = captions[i];
        const uint16_t color = colors[i];
        const uint16_t bgColor = bgColors[i];
        const int offset = offsets[i];
        const int captionXOffset = captionXOffsets[i];
        lcdDrawFillRect(dev, offset, 20, offset + fontHeight, 140, bgColor);
        lcdDrawString(dev, fx, captionXOffset, 130, caption, color);
    }
}

static void SetTimeLightDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{

    lcdSetFontDirection(dev, DIRECTION270);
    lcdFillScreen(dev, BLACK);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "    Set TimeLight   ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);    
    strcpy((char*)ascii, "Phase 1:");
    lcdDrawString(dev, fx, 40, 160, ascii, WHITE);
    strcpy((char*)ascii, "G1:");
    lcdDrawString(dev, fx, 60, 140, ascii, GREEN);
    strcpy((char*)ascii, "999");
    lcdDrawString(dev, fx, 60, 116, ascii, GREEN);

    strcpy((char*)ascii, "Y1:");
    lcdDrawString(dev, fx, 60, 60, ascii, YELLOW);
    strcpy((char*)ascii, "999");
    lcdDrawString(dev, fx, 60, 36, ascii, YELLOW);

    strcpy((char*)ascii, "Phase 2:");
    lcdDrawString(dev, fx, 85, 160, ascii, WHITE);
    strcpy((char*)ascii, "G2:");
    lcdDrawString(dev, fx, 110, 140, ascii, GREEN);
    strcpy((char*)ascii, "999");
    lcdDrawString(dev, fx, 110, 116, ascii, GREEN);

    strcpy((char*)ascii, "Y2:");
    lcdDrawString(dev, fx, 110, 60, ascii, YELLOW);
    strcpy((char*)ascii, "999");
    lcdDrawString(dev, fx, 110, 36, ascii, YELLOW);
}
static void DisableButtonInterrupt()
{
    gpio_isr_handler_remove(BUTTON_UP);
    gpio_isr_handler_remove(BUTTON_DOWN);
    gpio_isr_handler_remove(BUTTON_ENTER);
}
static void EnsableButtonInterrupt()
{
    gpio_isr_handler_add(BUTTON_UP, gpio_interrupt_handler, (void*)BUTTON_UP); 
    gpio_isr_handler_add(BUTTON_DOWN, gpio_interrupt_handler, (void*)BUTTON_DOWN); 
    gpio_isr_handler_add(BUTTON_ENTER, gpio_interrupt_handler, (void*)BUTTON_ENTER); 
}