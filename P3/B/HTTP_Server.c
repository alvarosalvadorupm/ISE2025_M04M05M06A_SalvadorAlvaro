/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server.c
 * Purpose: HTTP Server example
 *----------------------------------------------------------------------------*/

#include <stdio.h>

#include "main.h"

#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE

#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
//#include "Board_LED.h"                  // ::Board Support:LED
#include "LEDs.h"                  			// ::Board Support:LED
#include "Board_Buttons.h"              // ::Board Support:Buttons
//#include "Board_ADC.h"                  // ::Board Support:A/D Converter
#include "adc.h"
//#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "Thlcd.h"                      // LCD
//#include "GLCD_Config.h"                // Keil.MCBSTM32F400::Board Support:Graphic LCD
#include "RTC.h"
#include "SNTP.h"
#include "LPM.h"

// Main stack size must be multiple of 8 Bytes
#define APP_MAIN_STK_SZ (1024U)
uint64_t app_main_stk[APP_MAIN_STK_SZ / 8];
const osThreadAttr_t app_main_attr = {
  .stack_mem  = &app_main_stk[0],
  .stack_size = sizeof(app_main_stk)
};

//extern GLCD_FONT GLCD_Font_6x8;
//extern GLCD_FONT GLCD_Font_16x24;

extern uint16_t AD_in          (uint32_t ch);
extern uint8_t  get_button     (void);
extern void     netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len);

extern bool LEDrun;
extern char lcd_text[2][20+1];
extern char rtc_text[2][20+1];

extern osThreadId_t TID_Display;
extern osThreadId_t TID_Led;
extern osThreadId_t TID_RTC;
extern osThreadId_t TID_ALARM;

bool LEDrun;
char lcd_text[2][20+1] = { "LCD line 1",
                           "LCD line 2" };

char rtc_text[2][20+1] = {0};

ADC_HandleTypeDef adchandle; //handler definition
													 
/* Thread IDs */
osThreadId_t TID_Display;
osThreadId_t TID_Led;
osThreadId_t TID_RTC;
osThreadId_t TID_ALARM;
osThreadId_t TID_PULSADOR;
osThreadId_t TID_LPM;

/* Timers IDs */
osTimerId_t tim_timer_1sec;
osTimerId_t tim_timer_6sec;
osTimerId_t tim_timer_3min;
osTimerId_t tim_timer_100ms;
osTimerId_t tim_timer_15s;

/* Thread declarations */
static void BlinkLed (void *arg);
static void Display  (void *arg);
static void Date_Time_RTC (void *arg);
static void Alarm(void *arg);
static void Pulsador(void *arg);

/* Buffers used for displaying Time and Date */
uint8_t aShowTime[10] = {0};
uint8_t aShowDate[10] = {0};

uint8_t Parp_LED_R = 0;
uint32_t flag = 0x00;

__NO_RETURN void app_main (void *arg);

/* Read analog inputs */
uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;

  if (ch == 0) {
		val = ADC_getVoltage(&adchandle , 13 ); //get values from channel 13->ADC123_IN13
  }
  return ((uint16_t)val);
}

/* Read digital inputs */
uint8_t get_button (void) {
  return ((uint8_t)Buttons_GetState ());
}

/* IP address change notification */
void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {

  (void)if_num;
  (void)val;
  (void)len;

  if (option == NET_DHCP_OPTION_IP_ADDRESS) {
    /* IP address change, trigger LCD update */
    osThreadFlagsSet (TID_Display, 0x01);
  }
}

void Timer_1sec_Callback (void){
	
	if(Parp_LED_R < 4){
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		Parp_LED_R++;
	}else{
		Parp_LED_R = 0;
		osTimerStop(tim_timer_1sec);
	}
	
}

void Timer_6sec_Callback(void){
	
	init_SNTP();
	osTimerStart(tim_timer_3min, 180000);
	
}

void Timer_3min_Callback(void){
	
	init_SNTP();
	osTimerStart(tim_timer_1sec, 500);
	
}

void Timer_100ms_Callback(void){
  
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
  
}

void Timer_15s_Callback(void){
  
  HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
  Encender_Apagar_LEDs(0x04); // Encendemos el LED Rojo antes de entrar en el Modo Sleep
  osTimerStop(tim_timer_100ms);
  SleepMode_Measure();
  
}

int init_Timers(void){
	
	osStatus_t status;
	
	tim_timer_1sec  = osTimerNew((osTimerFunc_t)&Timer_1sec_Callback, osTimerPeriodic, NULL, NULL);
	tim_timer_6sec  = osTimerNew((osTimerFunc_t)&Timer_6sec_Callback, osTimerOnce, NULL, NULL);
	tim_timer_3min  = osTimerNew((osTimerFunc_t)&Timer_3min_Callback, osTimerPeriodic, NULL, NULL);
	tim_timer_100ms = osTimerNew((osTimerFunc_t)&Timer_100ms_Callback, osTimerPeriodic, NULL, NULL);
  tim_timer_15s   = osTimerNew((osTimerFunc_t)&Timer_15s_Callback, osTimerOnce, NULL, NULL);
  
  if(tim_timer_1sec != NULL || tim_timer_6sec != NULL || tim_timer_3min != NULL || tim_timer_100ms != NULL){
    //Se inicializa con un intervalo de xxx ms
    //status = osTimerStart(tim_timer_id,xxxU);
    if(status != osOK){
      return -1;
    }
  }
	return 0;
}


static void Low_Power_Mode (void *arg){
  
  (void)arg;
  
  osTimerStart(tim_timer_100ms, 100);
  osTimerStart(tim_timer_15s, 15000);
  
}

/*----------------------------------------------------------------------------
  Thread 'PULSADOR': Funcion para gestionar el pulador de usuario
 *---------------------------------------------------------------------------*/

static __NO_RETURN void Pulsador (void *arg){
	
	(void)arg;
	
	while(1){
		
		osThreadFlagsWait(0x01U, osFlagsWaitAny, osWaitForever);
		
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    osTimerStart(tim_timer_100ms, 100);
//    osTimerStart(tim_timer_15s, 15000);
		RTC_DateConfig(0, 1, 1, 1);
		RTC_TimeConfig(0, 0, 0);
		
	}
}

/*----------------------------------------------------------------------------
  Thread 'ALARM': Funcion para gestionar los booleanos de las alarmas
 *---------------------------------------------------------------------------*/

static __NO_RETURN void Alarm (void *arg){
	
	(void)arg;
	
	while(1){
		
		osThreadFlagsWait(0x01U, osFlagsWaitAny, osWaitForever);
		
		for(uint8_t i = 0; i < 5; i++){
				
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
			osDelay(1000);
			
		}
	}
}

/*----------------------------------------------------------------------------
  Thread 'RTC': Funcion para obtener dia y fecha del RTC
 *---------------------------------------------------------------------------*/
static __NO_RETURN void Date_Time_RTC (void *arg) {
	
	(void)arg;
	
	osTimerStart(tim_timer_6sec, 6000);
  
	while(1){
		RTC_Hora_Fecha(aShowTime, aShowDate);
		
		// Copiar el contenido de aShowTime en rtc_text[0], para poder tenerlo asi en un array de caracteres
    memcpy(rtc_text[0], aShowTime, sizeof(aShowTime));
    rtc_text[0][sizeof(aShowTime)] = '\0';  // Asegurar terminaci�n
		
    // Copiar el contenido de aShowDate en rtc_text[1]
    memcpy(rtc_text[1], aShowDate, sizeof(aShowDate));
    rtc_text[1][sizeof(aShowDate)] = '\0';  // Asegurar terminaci�n
		
		osThreadFlagsSet (TID_Display, 0x02);
	}
	
}

/*----------------------------------------------------------------------------
  Thread 'Display': LCD display handler
 *---------------------------------------------------------------------------*/
static __NO_RETURN void Display (void *arg) {

  (void)arg;
  
	//Inicializacion del LCD
  Init_PinesGPIO();
  LCD_Reset();
  LCD_Init();
	cleanLCD();
  LCD_Update();

  while(1) {
    /* Wait for signal from DHCP */
    flag = osThreadFlagsWait (0x03U, osFlagsWaitAny, osWaitForever);
		
		cleanLCD();
		
		if(flag == 0x01){

			/* Display user text line 1 */
			write_lcd(lcd_text[0],1, strlen(lcd_text[0]));
			/* Display user text line 2 */
			write_lcd(lcd_text[1],2, strlen(lcd_text[1]));
		}else if(flag == 0x02){
			
			/* Display user text line 1 */
			write_lcd(rtc_text[0],1, strlen(rtc_text[0]));
			/* Display user text line 2 */
			write_lcd(rtc_text[1],2, strlen(rtc_text[1]));
		}
		
//    LCD_Update();
    
  }
}

/*----------------------------------------------------------------------------
  Thread 'BlinkLed': Blink the LEDs on an eval board
 *---------------------------------------------------------------------------*/
static __NO_RETURN void BlinkLed (void *arg) {	// Funci�n que hace que los leds vayan parpadeando
  const uint8_t led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
                                0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  uint32_t cnt = 0U;

  (void)arg;

  LEDrun = false;
  while(1) {
    /* Every 100 ms */
    if (LEDrun == true) {
//      LED_SetOut (led_val[cnt]);
			Encender_Apagar_LEDs(led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0U;
      }
    }
    osDelay (100);
  }
}

/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/
__NO_RETURN void app_main (void *arg) {
  (void)arg;

	// Inicializacion de los LEDs
  Init_LEDs();
	
	// Inicializacion de los ADCs
	ADC1_pins_F429ZI_config(); //specific PINS configuration
	ADC_Init_Single_Conversion(&adchandle , ADC1); //ADC1 configuration
	
	// Inicializacion del RTC
	RTC_Config();
	
	// Inicializacion Timers
	init_Timers();
	
	// Inicializacion Pulsador Usuario
	Pulsador_B1();
	
  netInitialize ();			// Inicializa un monton de cosas del Ethernet

  TID_Led     = osThreadNew (BlinkLed, NULL, NULL);
  TID_Display   = osThreadNew (Display,  NULL, NULL);
  TID_RTC       = osThreadNew (Date_Time_RTC, NULL, NULL);
  TID_ALARM     = osThreadNew (Alarm, NULL, NULL);
  TID_PULSADOR  = osThreadNew (Pulsador, NULL, NULL);
  TID_LPM       = osThreadNew (Low_Power_Mode, NULL, NULL);
  
  osThreadExit();
}
