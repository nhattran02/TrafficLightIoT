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
#define BUTTON_NOTPRESS         -1
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
#define DEBOUNCE                10

static QueueHandle_t uart0_queue;


TaskHandle_t TaskHandler_uart;
int G1=5, Y1=5, G2=5, Y2=5;
char G1_str[3], Y1_str[3], G2_str[3], Y2_str[3];
typedef enum {
    G1_CHOSEN=0,
    Y1_CHOSEN,
    G2_CHOSEN,
    Y2_CHOSEN,
}time_light_chosen_t;

time_light_chosen_t time_light_chosen = G1_CHOSEN; 

static void uart_event_task(void *pvParameters);
static void screen_task(void *pvParameters);
static void IntroDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option3Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option4Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SetTimeLightDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void GlobalConfig(void);
static void OptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option);
static int DetectButton();

void app_main(void)
{
    GlobalConfig();
	// xTaskCreate(&button_task, "Button",    4096, NULL, 2, NULL);
	xTaskCreate(&screen_task, "TFT Screen", 4096, NULL, 2, NULL);
	//xTaskCreate(&uart_event_task, "UART Task", 4096, NULL, 2, NULL);
}

static void OptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option)
{
    switch(option){
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
            Option4Display(dev, fx, width, height);
            break;
        default:
            break;
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

static void GlobalConfig(void)
{
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON_UP, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON_DOWN, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON_ENTER, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_UP);
    gpio_pullup_en(BUTTON_DOWN);
    gpio_pullup_en(BUTTON_ENTER);
    gpio_pulldown_dis(BUTTON_UP);
    gpio_pulldown_dis(BUTTON_DOWN);
    gpio_pulldown_dis(BUTTON_ENTER);

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

}

static void screen_task(void *pvParameters)
{
    int option_counting = 1; 
    int button_state = BUTTON_NOTPRESS;
    bool exit_loop = false;
	static FontxFile fx16[2];
	InitFontx(fx16,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	static FontxFile fx24[2];
	InitFontx(fx24,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic

	static ST7735_t dev;
	spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
	lcdInit(&dev, SCREEN_WIDTH, SCREEN_HEIGHT, OFFSET_X, OFFSET_Y);
    IntroDisplay(&dev, fx24, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
	while (1) {
        if(DetectButton()==BUTTON_DOWN){
            option_counting = (option_counting < 4) ? option_counting + 1 : 4;
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
        }else if(DetectButton()==BUTTON_UP){
            option_counting = (option_counting > 1) ? option_counting - 1 : 1;
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
        }else if(DetectButton()==BUTTON_ENTER){
            switch (option_counting){
            case 1:
                lcdFillScreen(&dev, BLACK);
                exit_loop = false;
                while(!exit_loop){
                    SetTimeLightDisplay(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT); 
                    while((button_state = DetectButton()) == -1);
                    if(button_state == BUTTON_ENTER){
                        switch (time_light_chosen){
                            case G1_CHOSEN: time_light_chosen = Y1_CHOSEN; break;
                            case Y1_CHOSEN: time_light_chosen = G2_CHOSEN; break;
                            case G2_CHOSEN: time_light_chosen = Y2_CHOSEN; break;
                            case Y2_CHOSEN:
                                time_light_chosen = G1_CHOSEN;
                                //saving data
                                lcdFillScreen(&dev, BLACK);
                                exit_loop = true;
                                break;
                            default: break;
                        }
                    }else if(button_state == BUTTON_DOWN){
                        switch (time_light_chosen){
                            case G1_CHOSEN: G1 = (G1 > 0) ? G1 - 1 : 0; break;
                            case Y1_CHOSEN: Y1 = (Y1 > 0) ? Y1 - 1 : 0; break;
                            case G2_CHOSEN: G2 = (G2 > 0) ? G2 - 1 : 0; break;
                            case Y2_CHOSEN: Y2 = (Y2 > 0) ? Y2 - 1 : 0; break;
                            default: break;
                        }
                    }else if(button_state == BUTTON_UP){
                        switch (time_light_chosen){
                            case G1_CHOSEN: G1 = (G1 < 999) ? G1 + 1 : 999; break;
                            case Y1_CHOSEN: Y1 = (Y1 < 999) ? Y1 + 1 : 999; break;
                            case G2_CHOSEN: G2 = (G2 < 999) ? G2 + 1 : 999; break;
                            case Y2_CHOSEN: Y2 = (Y2 < 999) ? Y2 + 1 : 999; break;
                            default: break;
                        }
                    }
                }
                break;
            case 2:
                //handler 
                lcdFillScreen(&dev, BLACK);
                break;
            case 3:
                //handler
                lcdFillScreen(&dev, BLACK);
                break;
            case 4:
                //handler
                lcdFillScreen(&dev, BLACK);
                break;
            default:
                break;
            }
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting);
        }
	}
}

static void IntroDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
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
    static uint8_t ascii[30];
    lcdSetFontDirection(dev, DIRECTION270);
    itoa(G1, G1_str, 10); 
    itoa(Y1, Y1_str, 10); 
    itoa(G2, G2_str, 10); 
    itoa(Y2, Y2_str, 10);

    strcpy((char*)ascii, "    Set TimeLight   ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE); 
    switch (time_light_chosen){
    case G1_CHOSEN:
        strcpy((char*)ascii, "Phase 1:");
        lcdDrawFillRect(dev, 45, 90, 61, 145, WHITE);
        lcdDrawString(dev, fx, 40, 160, ascii, WHITE);
        strcpy((char*)ascii, "G1:");
        lcdDrawString(dev, fx, 60, 140, ascii, BLACK);
        strcpy((char*)ascii, G1_str);
        lcdDrawString(dev, fx, 60, 116, ascii, BLACK);

        strcpy((char*)ascii, "Y1:");
        lcdDrawString(dev, fx, 60, 60, ascii, YELLOW);
        strcpy((char*)ascii, Y1_str);
        lcdDrawString(dev, fx, 60, 36, ascii, YELLOW);

        strcpy((char*)ascii, "Phase 2:");
        lcdDrawString(dev, fx, 85, 160, ascii, WHITE);
        strcpy((char*)ascii, "G2:");
        lcdDrawString(dev, fx, 110, 140, ascii, GREEN);
        strcpy((char*)ascii, G2_str);
        lcdDrawString(dev, fx, 110, 116, ascii, GREEN);

        strcpy((char*)ascii, "Y2:");
        lcdDrawString(dev, fx, 110, 60, ascii, YELLOW);
        strcpy((char*)ascii, Y2_str);
        lcdDrawString(dev, fx, 110, 36, ascii, YELLOW);
        break;
    case Y1_CHOSEN:  
        strcpy((char*)ascii, "Phase 1:");
        lcdDrawFillRect(dev, 45, 90, 61, 145, BLACK);
        lcdDrawString(dev, fx, 40, 160, ascii, WHITE);
        strcpy((char*)ascii, "G1:");
        lcdDrawString(dev, fx, 60, 140, ascii, GREEN);
        strcpy((char*)ascii, G1_str);
        lcdDrawString(dev, fx, 60, 116, ascii, GREEN);

        lcdDrawFillRect(dev, 45, 10, 61, 65, WHITE); 
        strcpy((char*)ascii, "Y1:");
        lcdDrawString(dev, fx, 60, 60, ascii, BLACK);
        strcpy((char*)ascii, Y1_str);
        lcdDrawString(dev, fx, 60, 36, ascii, BLACK);

        strcpy((char*)ascii, "Phase 2:");
        lcdDrawString(dev, fx, 85, 160, ascii, WHITE);
        strcpy((char*)ascii, "G2:");
        lcdDrawString(dev, fx, 110, 140, ascii, GREEN);
        strcpy((char*)ascii, G2_str);
        lcdDrawString(dev, fx, 110, 116, ascii, GREEN);

        strcpy((char*)ascii, "Y2:");
        lcdDrawString(dev, fx, 110, 60, ascii, YELLOW);
        strcpy((char*)ascii, Y2_str);
        lcdDrawString(dev, fx, 110, 36, ascii, YELLOW);
        break;
    case G2_CHOSEN:   
        strcpy((char*)ascii, "Phase 1:");
        lcdDrawString(dev, fx, 40, 160, ascii, WHITE);
        strcpy((char*)ascii, "G1:");
        lcdDrawString(dev, fx, 60, 140, ascii, GREEN);
        strcpy((char*)ascii, G1_str);
        lcdDrawString(dev, fx, 60, 116, ascii, GREEN);

        lcdDrawFillRect(dev, 45, 10, 61, 65, BLACK); 
        strcpy((char*)ascii, "Y1:");
        lcdDrawString(dev, fx, 60, 60, ascii, YELLOW);
        strcpy((char*)ascii, Y1_str);
        lcdDrawString(dev, fx, 60, 36, ascii, YELLOW);

        strcpy((char*)ascii, "Phase 2:");
        lcdDrawString(dev, fx, 85, 160, ascii, WHITE);
        lcdDrawFillRect(dev, 93, 90, 109, 145, WHITE); 
        strcpy((char*)ascii, "G2:");
        lcdDrawString(dev, fx, 110, 140, ascii, BLACK);
        strcpy((char*)ascii, G2_str);
        lcdDrawString(dev, fx, 110, 116, ascii, BLACK);

        strcpy((char*)ascii, "Y2:");
        lcdDrawString(dev, fx, 110, 60, ascii, YELLOW);
        strcpy((char*)ascii, Y2_str);
        lcdDrawString(dev, fx, 110, 36, ascii, YELLOW);
        break;
    case Y2_CHOSEN:  
        strcpy((char*)ascii, "Phase 1:");
        lcdDrawString(dev, fx, 40, 160, ascii, WHITE);
        strcpy((char*)ascii, "G1:");
        lcdDrawString(dev, fx, 60, 140, ascii, GREEN);
        strcpy((char*)ascii, G1_str);
        lcdDrawString(dev, fx, 60, 116, ascii, GREEN);

        strcpy((char*)ascii, "Y1:");
        lcdDrawString(dev, fx, 60, 60, ascii, YELLOW);
        strcpy((char*)ascii, Y1_str);
        lcdDrawString(dev, fx, 60, 36, ascii, YELLOW);

        strcpy((char*)ascii, "Phase 2:");
        lcdDrawString(dev, fx, 85, 160, ascii, WHITE);
        lcdDrawFillRect(dev, 93, 90, 109, 145, BLACK); 
        strcpy((char*)ascii, "G2:");
        lcdDrawString(dev, fx, 110, 140, ascii, GREEN);
        strcpy((char*)ascii, G2_str);
        lcdDrawString(dev, fx, 110, 116, ascii, GREEN);

        lcdDrawFillRect(dev, 93, 10, 109, 65, WHITE); 
        strcpy((char*)ascii, "Y2:");
        lcdDrawString(dev, fx, 110, 60, ascii, BLACK);
        strcpy((char*)ascii, Y2_str);
        lcdDrawString(dev, fx, 110, 36, ascii, BLACK);
        break;
    default:
        break;
    }
}
static int DetectButton()
{
    gpio_num_t buttons[] = {BUTTON_ENTER, BUTTON_DOWN, BUTTON_UP};
    int button_count = sizeof(buttons)/sizeof(gpio_num_t);
    for(int i = 0; i < button_count; i ++){
        if(gpio_get_level(buttons[i])==0){
            vTaskDelay(DEBOUNCE/portTICK_PERIOD_MS);
            if(gpio_get_level(buttons[i])==0){
                return buttons[i];
            }
        }
    }
    return -1; //not press
}
