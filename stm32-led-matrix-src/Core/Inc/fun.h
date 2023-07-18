#ifndef __FUN_H
#define __FUN_H



#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

void delay_us2 (uint16_t us);
void delay_ms2 (uint16_t us);

void delay_ms(uint16_t au16_ms);
void delay_us(uint16_t au16_us);

#define delayMicroseconds(x)  delay_us2(x)
//#define delay(x)  delay_ms2(x)

#define delay(x)  HAL_Delay(x)

#define micros() TIM1->CNT
#define millis() HAL_GetTick()


#ifdef __cplusplus
}
#endif
#endif
