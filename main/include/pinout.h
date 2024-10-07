#ifndef _PINOUT_H
#define _PINOUT_H

//gerenal include
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/spi_master.h"
#include <stdbool.h>
#include <stdint.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


//local library include
#include "bk4819.h"
#include "ST7920_SERIAL.h"
#include "bk4819-regs.h"
#include <esp_timer.h>
#include <driver/ledc.h>
#include "f6x8m.h"
#include "f10x16f.h"
#include "font.h"
#include "bitmap.h"



//pinout declaration
#define PIN_BK4819_SCL   GPIO_NUM_25
#define PIN_BK4819_SCN   GPIO_NUM_12
#define PIN_BK4819_DATA  GPIO_NUM_8
#define PIN_ADVANCE GPIO_NUM_12
#define PIN_RETURN GPIO_NUM_13


#define SCLK_PIN 2 	//E Pin in Display 22
#define CS_PIN 0		//RS Pin in Display 20
#define SID_PIN 1		//R/W Pin in Display 21
#define RST_PIN 3 		//RST Pin in Display 23
 

#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0

#define DEBOUNCE_TIME_US 500


#define battery_min 3.0 //  Minimum battery
#define battery_max 4.2 //   Maximum battery
#define R1 470300.0 // 47K Resistor divisor     
#define R2 100300.0 // 10k Resistor divisor 
#define adc_offset 0.01 // a Ajustar
#define vol_offset 0.0 // a Ajustar

//LEDC config deprecated
/*
#define ESP_INTR_FLAG_DEFAULT 0
#define DEBOUNCE_TIME_US 1500000
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (23) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_9_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (110) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (40000) // Frequency in Hertz. Set frequency at 4 kHz
*/
#endif