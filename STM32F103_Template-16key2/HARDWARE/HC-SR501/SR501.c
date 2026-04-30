#include "SR501.h"

void HC_SR501Configuration(void)
{
	GPIO_InitTypeDef    GPIO;
    
    //Enable APB2 Bus
    RCC_APB2PeriphClockCmd(HC_SR50CLKLINE, ENABLE);
    
    //Register IO 
    GPIO.GPIO_Pin   = HC_SR501_Pin;
    GPIO.GPIO_Mode  = GPIO_Mode_IPD;
    GPIO_Init(HC_SR50PORT, &GPIO);
}

FunctionalState HC_SR501_Status(void)
{
	if (HC_SR501 == 1)
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}	
