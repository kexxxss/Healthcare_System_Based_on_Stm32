#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
 


void TIM3_Int_Init(u16 arr,u16 psc);
void TIM2_Int_Init(u16 arr,u16 psc);
void TIM7_Int_Init(u16 arr,u16 psc);
void dis_DrawCurve(u32* data,u8 x);
 
#endif
