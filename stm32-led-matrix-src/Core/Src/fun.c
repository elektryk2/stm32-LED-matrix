#include "fun.h"
volatile uint16_t start =0, actual=0;
//void delay_us2 (uint16_t us)
//{
//
//	start=TIM1->CNT;
//	while (1)
//	{
//		actual=TIM1->CNT;
//		if(actual > (start+us))
//			break;  // wait for the counter to reach the us input in the parameter
//	}
//}


//void delay_us2 (uint16_t us)
//{
//
//	start=TIM1->CNT;
//
//		while (TIM1->CNT < (start+us));
//
//}


void delay_us(uint16_t au16_us)
{
    TIM1->CNT = 0;
    while (TIM1->CNT < au16_us);
}

void delay_ms(uint16_t au16_ms)
{
    while(au16_ms > 0)
    {
    	TIM1->CNT = 0;
    au16_ms--;
    while (TIM1->CNT < 1000);
    }
}

void delay_us2(uint16_t t)
{
 uint16_t t1=TIM1->CNT;
 uint16_t dt;
 do
 {
   dt=TIM1->CNT-t1;
 } while (dt<t);
}

void delay_ms2(uint16_t au16_ms)
{
    while(au16_ms > 0)
    {
		au16_ms--;
		delay_us2(1);
    }
}
