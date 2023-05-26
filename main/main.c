/*
 * main.c
 *
 *  Created on: 12.03.2023
 *      Author: Tran Minh Nhat - 2014008
 */
 
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
#include "74HC595.h"

#define LED                     2 
#define BUTTON_UP               35 
#define BUTTON_DOWN             34 
#define BUTTON_ENTER            36 
#define LED_RED_PHASE_1         0
#define LED_YELLOW_PHASE_1      2
#define LED_GREEN_PHASE_1       4
#define LED_RED_PHASE_2         27
#define LED_YELLOW_PHASE_2      26
#define LED_GREEN_PHASE_2       25
#define EX_UART_NUM             UART_NUM_0
#define PATTERN_CHR_NUM         (3) 

#define DATA_PIN_595            5 
#define LATCH_PIN_595           17
#define CLOCK_PIN_595           16

#define GPIO_MOSI               23
#define GPIO_SCLK               19
#define GPIO_CS                 22
#define GPIO_DC                 21
#define GPIO_RESET              18

#define BUF_SIZE                (1024)
#define RD_BUF_SIZE             (BUF_SIZE)
#define SCREEN_WIDTH            130
#define SCREEN_HEIGHT           160
#define OFFSET_X                0
#define OFFSET_Y                0

#define INTERVAL                2000    
#define WAIT                    vTaskDelay(INTERVAL/portTICK_PERIOD_MS)
#define DEBOUNCE                20
#define NOTPRESS                -1

typedef enum {
    G1_CHOSEN = 0,
    Y1_CHOSEN,
    G2_CHOSEN,
    Y2_CHOSEN,
}time_light_chosen_t;


const int LED7Seg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
TaskHandle_t TaskHandler_uart;
TaskHandle_t TaskHandler_LED;

static QueueHandle_t uart0_queue;


int G1 = 10, Y1 = 5, G2 = 8, Y2 = 4;
int R1 = 12, R2 = 15;
int G1_save = 10, Y1_save = 5, G2_save = 8, Y2_save = 4;
int R1_save = 12, R2_save = 15;
char G1_str[3], Y1_str[3], G2_str[3], Y2_str[3];
time_light_chosen_t time_light_chosen = G1_CHOSEN; 
bool isRED1on = false;
bool isGREEN1on = false;
bool isYELLOW1on = false;
bool isRED2on = false; 
bool isGREEN2on = false; 
bool isYELLOW2on = false;
bool isInSlowMode = false;
bool isLEDTaskRunning = true;

static void uart_event_task(void *);
static void screen_task(void *);
static void LED_task(void *);
static void IntroDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option3Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void Option4Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void EXITDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void PHASE1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void PHASE2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SAVEDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void BACKDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SubManualAdjOptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option);
static void Sub2ManualAdjOptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option);
static void SubRED1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SubGREEN1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SubYELLOW1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SubRED2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SubGREEN2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SubYELLOW2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void BACK2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SlowModeDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SetTimeLightDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height);
static void SavedDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height, int idx);
static void Init_Hardware(void);
static void OptionSelect(ST7735_t * , FontxFile *, int , int , int );
static void ManualAdjOptionSelect(ST7735_t * , FontxFile *, int , int , int );
static int  DetectButton();
static void LEDAllOff();


void app_main(void)
{   
    Init_Hardware();

	xTaskCreate(&screen_task, "TFT Screen", 1024*4, NULL, 3, NULL);
	xTaskCreate(&LED_task, "LED task", 1024*4, NULL, 3, &TaskHandler_LED);      
    // xTaskCreate(&uart_event_task, "UART Task", 4096, NULL, 2, &TaskHandler_uart);
}

static void LED_task(void *pvParameters)
{
    static IC74HC595_t IC74HC595;
    Init74HC595(&IC74HC595, DATA_PIN_595, LATCH_PIN_595, CLOCK_PIN_595);
    int idx;

    while(1){
        // xSemaphoreGive(red_phase1_sem);
        idx = R1_save;
        gpio_set_level(LED_RED_PHASE_1, 1);
        gpio_set_level(LED_GREEN_PHASE_2, 1);

        while(idx >= 1){
            for(int i = G2_save; i >= 1; i--){
                write4Byte74HC595(&IC74HC595, LED7Seg[i/10], LED7Seg[i%10], LED7Seg[idx/10], LED7Seg[idx%10]);
                idx --;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            gpio_set_level(LED_GREEN_PHASE_2, 0);
            gpio_set_level(LED_YELLOW_PHASE_2, 1);
            for(int i = Y2_save; i >= 1; i--){
                write4Byte74HC595(&IC74HC595, LED7Seg[i/10], LED7Seg[i%10], LED7Seg[idx/10], LED7Seg[idx%10]);
                idx --;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            gpio_set_level(LED_YELLOW_PHASE_2, 0);
        }
        gpio_set_level(LED_RED_PHASE_1, 0);
        idx = R2_save;

        gpio_set_level(LED_RED_PHASE_2, 1);
        gpio_set_level(LED_GREEN_PHASE_1, 1);
        while (idx >= 1){
            for(int i = G1_save; i >= 1; i--){
                write4Byte74HC595(&IC74HC595, LED7Seg[idx/10], LED7Seg[idx%10], LED7Seg[i/10], LED7Seg[i%10]);
                idx --;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            gpio_set_level(LED_GREEN_PHASE_1, 0);
            gpio_set_level(LED_YELLOW_PHASE_1, 1);

            for(int i = Y1_save; i >= 1; i--){
                write4Byte74HC595(&IC74HC595, LED7Seg[idx/10], LED7Seg[idx%10], LED7Seg[i/10], LED7Seg[i%10]);
                idx --;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            gpio_set_level(LED_YELLOW_PHASE_1, 0);
        }
        gpio_set_level(LED_RED_PHASE_2, 0);
    } 
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

static void ManualAdjOptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option)
{
        switch(option){
        case 1:
            PHASE1Display(dev, fx, width, height);
            break;
        case 2:
            PHASE2Display(dev, fx, width, height);
            break;
        case 3:
            SAVEDisplay(dev, fx, width, height);
            break;
        case 4: 
            EXITDisplay(dev, fx, width, height);
            break;
        default:
            break;
    }
}


static void SubManualAdjOptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option)
{
        switch(option){
        case 1:
            SubRED1Display(dev, fx, width, height);
            break;
        case 2:
            SubGREEN1Display(dev, fx, width, height);
            break;
        case 3:
            SubYELLOW1Display(dev, fx, width, height);
            break;
        case 4: 
            BACKDisplay(dev, fx, width, height);
            break;
        default:
            break;
    }
}

static void Sub2ManualAdjOptionSelect(ST7735_t * dev, FontxFile *fx, int width, int height, int option)
{
    switch(option){
        case 1:
            SubRED2Display(dev, fx, width, height);
            break;
        case 2:
            SubGREEN2Display(dev, fx, width, height);
            break;
        case 3:
            SubYELLOW2Display(dev, fx, width, height);
            break;
        case 4: 
            BACK2Display(dev, fx, width, height);
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
        printf("\n---CONTROL TRAFFIC LIGHT VIA TERMINAL---\n");
        printf("1. Set TimeLight:     !SETTIME! \n");
        printf("2. Manual Adjust:     !ADJ! \n");
        printf("3. Slow Mode:         !SLOW! \n");
        printf("4. Show Current Time: !SHOW! \n");
        printf("Enter your command: \n");
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            //if(event.data)
            switch(event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    printf("%s", dtmp);
                    if(strncmp(dtmp, "!SETTIME!", strlen("!SETTIME!"))==0){
                        printf("------------SET TIME------------\n");
                        printf("To set time using the command: !PHASE<x>_G1=<num1>_Y1=<num2>!\n");
                        printf("Example: Phase 1 have G1 = 3 and Y1 = 7 -> !PHASE1_G1=3_Y1=7!\n");
                        printf("Enter your command: \n");
                        while (1)
                        {
                            vTaskDelay(10/portTICK_PERIOD_MS);
                        }
                        

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

static void Init_Hardware(void)
{
    // gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED_PHASE_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_YELLOW_PHASE_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN_PHASE_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED_PHASE_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_YELLOW_PHASE_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN_PHASE_2, GPIO_MODE_OUTPUT);

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
    //spiff config _
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
    int option_counting_1 = 1; 
    int option_counting_2 = 1;
    int option_counting_3 = 1;

    int button_state = NOTPRESS;
    bool exit_loop = false;
	static FontxFile fx16[2];
    static IC74HC595_t IC74HC595;
    Init74HC595(&IC74HC595, DATA_PIN_595, LATCH_PIN_595, CLOCK_PIN_595);
	InitFontx(fx16,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	static FontxFile fx24[2];
	InitFontx(fx24,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic

	static ST7735_t dev;
	spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
	lcdInit(&dev, SCREEN_WIDTH, SCREEN_HEIGHT, OFFSET_X, OFFSET_Y);
    IntroDisplay(&dev, fx24, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_1);
	while (1) {
        vTaskDelay(pdMS_TO_TICKS(10)); //prevent watchdog timer trigger   
        if(DetectButton()==BUTTON_DOWN){
            option_counting_1 = (option_counting_1 < 4) ? option_counting_1 + 1 : 4;
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_1);
        }else if(DetectButton()==BUTTON_UP){
            option_counting_1 = (option_counting_1 > 1) ? option_counting_1 - 1 : 1;
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_1);
        }else if(DetectButton()==BUTTON_ENTER){
            switch (option_counting_1){
            case 1:
                lcdFillScreen(&dev, BLACK);
                exit_loop = false;
                while(!exit_loop){
                    vTaskDelay(pdMS_TO_TICKS(10)); //prevent watchdog timer trigger   
                    SetTimeLightDisplay(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT); 
                    while((button_state = DetectButton()) == -1){
                        vTaskDelay(pdMS_TO_TICKS(10)); //prevent watchdog timer trigger   
                    }
                    if(button_state == BUTTON_ENTER){
                        switch (time_light_chosen){
                            case G1_CHOSEN: time_light_chosen = Y1_CHOSEN; break;
                            case Y1_CHOSEN: time_light_chosen = G2_CHOSEN; break;
                            case G2_CHOSEN: time_light_chosen = Y2_CHOSEN; break;
                            case Y2_CHOSEN:
                                time_light_chosen = G1_CHOSEN;
                                //saving data
                                R2 = G1 + Y1;
                                R1 = G2 + Y2;
                                G1_save = G1;
                                Y1_save = Y1;
                                R1_save = R1;
                                G2_save = G2;
                                Y2_save = Y2;
                                R2_save = R2;
                                if(isLEDTaskRunning){
                                    vTaskDelete(TaskHandler_LED);
                                    isLEDTaskRunning = false;
                                }
                                LEDAllOff();
                                gpio_set_level(LED_YELLOW_PHASE_1, 1);
                                gpio_set_level(LED_YELLOW_PHASE_2, 1);

                                for(int i = 3; i >= 1; i--){
                                    SavedDisplay(&dev, fx24, SCREEN_WIDTH, SCREEN_HEIGHT, i);
                                    write4Byte74HC595(&IC74HC595, LED7Seg[i/10], LED7Seg[i%10], LED7Seg[i/10], LED7Seg[i%10]);
                                    vTaskDelay(pdMS_TO_TICKS(1000));
                                }
                                gpio_set_level(LED_YELLOW_PHASE_1, 0);
                                gpio_set_level(LED_YELLOW_PHASE_2, 0);

                                isLEDTaskRunning = true;
	                            xTaskCreate(&LED_task, "LED task", 1024*4, NULL, 3, &TaskHandler_LED);      
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
                lcdFillScreen(&dev, BLACK);
                PHASE1Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                while (1){
                    vTaskDelay(pdMS_TO_TICKS(10)); //prevent watchdog timer trigger   

                    if(DetectButton() == BUTTON_DOWN){
                        option_counting_2 = (option_counting_2 < 4) ? option_counting_2 + 1 : 4;
                        ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_2);
                    }else if(DetectButton() == BUTTON_UP){
                        option_counting_2 = (option_counting_2 > 1) ? option_counting_2 - 1 : 1;
                        ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_2);
                    }else if(DetectButton() == BUTTON_ENTER){
                        switch (option_counting_2){
                        case 1:
                            lcdFillScreen(&dev, BLACK);
                            SubRED1Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                            while (1){
                                vTaskDelay(pdMS_TO_TICKS(10)); //prevent watchdog timer trigger   

                                if(DetectButton() == BUTTON_DOWN){
                                    option_counting_3 = (option_counting_3 < 4) ? option_counting_3 + 1 : 4;
                                    SubManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                }else if(DetectButton() == BUTTON_UP){
                                    option_counting_3 = (option_counting_3 > 1) ? option_counting_3 - 1 : 1;
                                    SubManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                }else if(DetectButton() == BUTTON_ENTER){
                                    switch(option_counting_3){
                                        case 1:
                                            isRED1on = !isRED1on;
                                            if(isRED1on){
                                                isGREEN1on = false;
                                                isYELLOW1on = false;
                                            }
                                            SubManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                            
                                            break;
                                        case 2:
                                            isGREEN1on = !isGREEN1on;
                                            if(isGREEN1on){
                                                isRED1on = false;
                                                isYELLOW1on = false;
                                            }
                                            SubManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                            break;
                                        case 3:
                                            isYELLOW1on = !isYELLOW1on;
                                            if(isYELLOW1on){
                                                isRED1on = false;
                                                isGREEN1on = false;
                                            }
                                            SubManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                            break;
                                        case 4:
                                            // goto BACK;
                                            break;
                                        default:
                                            break;
                                    }
                                    if(option_counting_3 == 4){
                                        option_counting_3 = 1;
                                        PHASE1Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                                        break;
                                    }
                                }
                            }
                            break;
                        case 2:
                            lcdFillScreen(&dev, BLACK);
                            SubRED2Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                            while (1){
                                vTaskDelay(pdMS_TO_TICKS(10)); //prevent watchdog timer trigger   

                                if(DetectButton() == BUTTON_DOWN){
                                    option_counting_3 = (option_counting_3 < 4) ? option_counting_3 + 1 : 4;
                                    Sub2ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                }else if(DetectButton() == BUTTON_UP){
                                    option_counting_3 = (option_counting_3 > 1) ? option_counting_3 - 1 : 1;
                                    Sub2ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                }else if(DetectButton() == BUTTON_ENTER){
                                    switch(option_counting_3){
                                        case 1:
                                            isRED2on = !isRED2on;
                                            if(isRED2on){
                                                isGREEN2on = false;
                                                isYELLOW2on = false;
                                            }
                                            Sub2ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                            
                                            break;
                                        case 2:
                                            isGREEN2on = !isGREEN2on;
                                            if(isGREEN2on){
                                                isRED2on = false;
                                                isYELLOW2on = false;
                                            }
                                            Sub2ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                            break;
                                        case 3:
                                            isYELLOW2on = !isYELLOW2on;
                                            if(isYELLOW2on){
                                                isRED2on = false;
                                                isGREEN2on = false;
                                            }
                                            Sub2ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_3);
                                            break;
                                        case 4:
                                            // goto BACK;
                                            break;
                                        default:
                                            break;
                                    }
                                    if(option_counting_3 == 4){
                                        option_counting_3 = 1;
                                        PHASE2Display(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                                        break;
                                    }
                                }
                            }
                            break;
                        case 3:
                            //save and apply manual adjust
                            if(isLEDTaskRunning){
                                vTaskDelete(TaskHandler_LED);
                                isLEDTaskRunning = false;
                            }
                            LEDAllOff();
                            gpio_set_level(LED_YELLOW_PHASE_1, 1);
                            gpio_set_level(LED_YELLOW_PHASE_2, 1);
                            for(int i = 3; i >= 1; i--){
                                    SavedDisplay(&dev, fx24, SCREEN_WIDTH, SCREEN_HEIGHT, i);
                                    write4Byte74HC595(&IC74HC595, LED7Seg[i/10], LED7Seg[i%10], LED7Seg[i/10], LED7Seg[i%10]);
                                    vTaskDelay(pdMS_TO_TICKS(1000));
                            }
                            gpio_set_level(LED_YELLOW_PHASE_1, 0);
                            gpio_set_level(LED_YELLOW_PHASE_2, 0);
                            write4Byte74HC595(&IC74HC595, LED7Seg[88/10], LED7Seg[88%10], LED7Seg[88/10], LED7Seg[88%10]);
                            if(isRED1on){
                                gpio_set_level(LED_RED_PHASE_1, 1);
                                gpio_set_level(LED_GREEN_PHASE_1, 0);
                                gpio_set_level(LED_YELLOW_PHASE_1, 0);
                            }else if(isGREEN1on){
                                gpio_set_level(LED_RED_PHASE_1, 0);
                                gpio_set_level(LED_GREEN_PHASE_1, 1);
                                gpio_set_level(LED_YELLOW_PHASE_1, 0);
                            }else if(isYELLOW1on){
                                gpio_set_level(LED_RED_PHASE_1, 0);
                                gpio_set_level(LED_GREEN_PHASE_1, 0);
                                gpio_set_level(LED_YELLOW_PHASE_1, 1);
                            }

                            if(isRED2on){
                                gpio_set_level(LED_RED_PHASE_2, 1);
                                gpio_set_level(LED_GREEN_PHASE_2, 0);
                                gpio_set_level(LED_YELLOW_PHASE_2, 0);
                            }else if(isGREEN2on){
                                gpio_set_level(LED_RED_PHASE_2, 0);
                                gpio_set_level(LED_GREEN_PHASE_2, 1);
                                gpio_set_level(LED_YELLOW_PHASE_2, 0);
                            }else if(isYELLOW2on){
                                gpio_set_level(LED_RED_PHASE_2, 0);
                                gpio_set_level(LED_GREEN_PHASE_2, 0);
                                gpio_set_level(LED_YELLOW_PHASE_2, 1);
                            }
                            lcdFillScreen(&dev, BLACK);
                            ManualAdjOptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_2);
                            
                            break;
                        case 4:
                            if(isLEDTaskRunning){
                                vTaskDelete(TaskHandler_LED);
                                isLEDTaskRunning = false;
                            }
                            LEDAllOff();
                            gpio_set_level(LED_YELLOW_PHASE_1, 1);
                            gpio_set_level(LED_YELLOW_PHASE_2, 1);
                            for(int i = 3; i >= 1; i--){
                                    SavedDisplay(&dev, fx24, SCREEN_WIDTH, SCREEN_HEIGHT, i);
                                    write4Byte74HC595(&IC74HC595, LED7Seg[i/10], LED7Seg[i%10], LED7Seg[i/10], LED7Seg[i%10]);
                                    vTaskDelay(pdMS_TO_TICKS(1000));
                            }
                            gpio_set_level(LED_YELLOW_PHASE_1, 0);
                            gpio_set_level(LED_YELLOW_PHASE_2, 0);
                            
                            isLEDTaskRunning = true;
	                        xTaskCreate(&LED_task, "LED task", 1024*4, NULL, 3, &TaskHandler_LED);      

                            // lcdFillScreen(&dev, BLACK);
                            break;
                        default:
                            break;
                        }
                        
                        if(option_counting_2 == 4){
                            option_counting_2 = 1;
                            break;
                        }
                    }
                }
                lcdFillScreen(&dev, BLACK);
                break;
            case 3:
                lcdFillScreen(&dev, BLACK);
                SlowModeDisplay(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT);
                while (1){
                    gpio_set_level(LED_YELLOW_PHASE_1, 1);
                    gpio_set_level(LED_YELLOW_PHASE_2, 1);
                    gpio_set_level(LED_RED_PHASE_1, 0);
                    gpio_set_level(LED_RED_PHASE_2, 0);
                    gpio_set_level(LED_GREEN_PHASE_1, 0);
                    gpio_set_level(LED_GREEN_PHASE_2, 0);                    
                    write4Byte74HC595(&IC74HC595, LED7Seg[88/10], LED7Seg[88%10], LED7Seg[88/10], LED7Seg[88%10]);                    
                    if(DetectButton() == BUTTON_ENTER) {
                        if(isLEDTaskRunning){
                            vTaskDelete(TaskHandler_LED);
                            isLEDTaskRunning = false;
                        }
                        LEDAllOff();
                        isLEDTaskRunning = true;
	                    xTaskCreate(&LED_task, "LED task", 1024*4, NULL, 3, &TaskHandler_LED);          
                        break;                
                    }
                }

                //handler
                lcdFillScreen(&dev, BLACK);
                break;
            case 4: //terminal
                lcdFillScreen(&dev, BLACK);
                xTaskCreate(&uart_event_task, "UART Task", 4096, NULL, 2, &TaskHandler_uart);
                
                while(1){
                    vTaskDelay(10/portTICK_PERIOD_MS);
                }   
                lcdFillScreen(&dev, BLACK);
                break;
            default:
                break;
            }
            OptionSelect(&dev, fx16, SCREEN_WIDTH, SCREEN_HEIGHT, option_counting_1);
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

static void SavedDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height, int idx)
{

    lcdSetFontDirection(dev, DIRECTION270);
    lcdFillScreen(dev, BLACK);
    char * string = "SAVE";
    switch (idx)
    {
    case 1:
        string = "SAVE IN 1";
        break;
    case 2:
        string = "SAVE IN 2";
        break;
    case 3:
        string = "SAVE IN 3";
        break;
    default:
        break;
    }
    lcdDrawString(dev, fx, 75, 130, (uint8_t*)string, WHITE);
    
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


static void SubRED1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 1 ",
        (const uint8_t*)"GREEN PHASE 1",
        (const uint8_t*)"YELLOW PHASE 1",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {BLACK, WHITE, WHITE, WHITE};
    uint16_t bgColors[] = {WHITE, BLACK, BLACK, BLACK};
    if(isRED1on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN1on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW1on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }
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

static void SubRED2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 2 ",
        (const uint8_t*)"GREEN PHASE 2",
        (const uint8_t*)"YELLOW PHASE 2",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {BLACK, WHITE, WHITE, WHITE};
    uint16_t bgColors[] = {WHITE, BLACK, BLACK, BLACK};
    if(isRED2on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN2on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW2on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }
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

static void SubGREEN1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 1 ",
        (const uint8_t*)"GREEN PHASE 1",
        (const uint8_t*)"YELLOW PHASE 1",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {WHITE, BLACK, WHITE, WHITE};
    uint16_t bgColors[] = {BLACK, WHITE, BLACK, BLACK};
    if(isRED1on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN1on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW1on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }

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

static void SubGREEN2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 2 ",
        (const uint8_t*)"GREEN PHASE 2",
        (const uint8_t*)"YELLOW PHASE 2",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {WHITE, BLACK, WHITE, WHITE};
    uint16_t bgColors[] = {BLACK, WHITE, BLACK, BLACK};
    if(isRED2on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN2on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW2on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }

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

static void SubYELLOW1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 1 ",
        (const uint8_t*)"GREEN PHASE 1",
        (const uint8_t*)"YELLOW PHASE 1",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {WHITE, WHITE, BLACK, WHITE};
    uint16_t bgColors[] = {BLACK, BLACK, WHITE, BLACK};
    if(isRED1on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN1on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW1on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }

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

static void SubYELLOW2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 2 ",
        (const uint8_t*)"GREEN PHASE 2",
        (const uint8_t*)"YELLOW PHASE 2",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {WHITE, WHITE, BLACK, WHITE};
    uint16_t bgColors[] = {BLACK, BLACK, WHITE, BLACK};
    if(isRED2on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN2on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW2on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }

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

static void PHASE1Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"   PHASE 1   ",
        (const uint8_t*)"   PHASE 2   ",
        (const uint8_t*)"    SAVE     ",
        (const uint8_t*)"    EXIT     ",

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


static void PHASE2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"   PHASE 1   ",
        (const uint8_t*)"   PHASE 2   ",
        (const uint8_t*)"    SAVE     ",
        (const uint8_t*)"    EXIT     ",

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

static void SAVEDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"   PHASE 1   ",
        (const uint8_t*)"   PHASE 2   ",
        (const uint8_t*)"    SAVE     ",
        (const uint8_t*)"    EXIT     ",

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

static void BACKDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 1 ",
        (const uint8_t*)"GREEN PHASE 1",
        (const uint8_t*)"YELLOW PHASE 1",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {WHITE, WHITE, WHITE, BLACK};
    uint16_t bgColors[] = {BLACK, BLACK, BLACK, WHITE};
    if(isRED1on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN1on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW1on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }
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


static void BACK2Display(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)" RED PHASE 2 ",
        (const uint8_t*)"GREEN PHASE 2",
        (const uint8_t*)"YELLOW PHASE 2",
        (const uint8_t*)"     BACK    ",

    };
    uint16_t colors[] = {WHITE, WHITE, WHITE, BLACK};
    uint16_t bgColors[] = {BLACK, BLACK, BLACK, WHITE};
    if(isRED2on){
        colors[0] = WHITE;
        bgColors[0] = RED;
    }else if(isGREEN2on){
        colors[1] = BLACK;
        bgColors[1] = GREEN;
    }else if(isYELLOW2on){
        colors[2] = BLACK;
        bgColors[2] = YELLOW;
    }
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

static void EXITDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    strcpy((char*)ascii, "   Manual Adjust    ");
    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);

    const uint8_t fontHeight = getFortHeight(fx);
    const uint8_t* const captions[] = {
        (const uint8_t*)"   PHASE 1   ",
        (const uint8_t*)"   PHASE 2   ",
        (const uint8_t*)"    SAVE     ",
        (const uint8_t*)"    EXIT     ",

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
    return NOTPRESS; //not press
}
static void  LEDAllOff()
{
    gpio_set_level(LED_RED_PHASE_1, 0);
    gpio_set_level(LED_RED_PHASE_2, 0);
    gpio_set_level(LED_GREEN_PHASE_1, 0);
    gpio_set_level(LED_GREEN_PHASE_2, 0);
    gpio_set_level(LED_YELLOW_PHASE_1, 0);
    gpio_set_level(LED_YELLOW_PHASE_2, 0);
}

static void SlowModeDisplay(ST7735_t * const dev, const FontxFile * const fx, const int width, const int height)
{
    lcdSetFontDirection(dev, DIRECTION270);
    static uint8_t ascii[30];
    // strcpy((char*)ascii, "   Manual Adjust    ");
    strcpy((char*)ascii, "      Slow Mode     ");

    lcdDrawString(dev, fx, 15, 160, ascii, GREEN);
    lcdDrawLine(dev, 15, 160, 15, 0, WHITE);
    strcpy((char*)ascii, "    SLOW     ");
    lcdDrawString(dev, fx, 80, 130, ascii, YELLOW);
}