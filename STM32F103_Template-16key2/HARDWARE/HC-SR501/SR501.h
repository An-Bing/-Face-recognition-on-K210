#ifndef __SR501_H__
#define	__SR501_H__
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"


#define            HC_SR501            PBin(1)
#define            HC_SR501_Pin        GPIO_Pin_1
#define            HC_SR50PORT         GPIOB
#define            HC_SR50CLKLINE      RCC_APB2Periph_GPIOB


extern void HC_SR501Configuration(void);
extern FunctionalState HC_SR501_Status(void);
#endif


