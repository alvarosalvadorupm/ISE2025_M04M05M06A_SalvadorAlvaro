#ifndef __LPM_H
#define __LPM_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "LEDs.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

#define SLEEP_MODE

#if !defined (SLEEP_MODE) && !defined (STOP_MODE) && !defined (STOP_UNDERDRIVE_MODE) && !defined (STANDBY_MODE)\
 && !defined (STANDBY_RTC_MODE) && !defined (STANDBY_RTC_BKPSRAM_MODE)
 #error "Please select first the target STM32F4xx Low Power mode to be measured (in stm32f4xx_lp_modes.h file)"
#endif

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void SleepMode_Measure(void);

#endif /* __LPM_H */
