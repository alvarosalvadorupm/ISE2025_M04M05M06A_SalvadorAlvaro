#ifndef __SNTP_H
#define __SNTP_H

	#include "stm32f4xx_hal.h"
	#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
	#include "stdbool.h"
	#include "stdio.h"
	#include <time.h>
	#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE
	#include "RTC.h"
	
	void init_SNTP(void);
	
#endif /* __SNTP_H */
