#include "IOput.h"

u8 key_num;

static void Key_AllRowsLow(void)
{
    ROW1 = 0;
    ROW2 = 0;
    ROW3 = 0;
    ROW4 = 0;
}

void output_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_12;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_12);

    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio);

    Key_AllRowsLow();
}

void input_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOB, &gpio);
}

static u8 Key_ScanRow(u8 row_index, u8 mode)
{
    Key_AllRowsLow();

    if(row_index == 0)
    {
        ROW1 = 1;
    }
    else if(row_index == 1)
    {
        ROW2 = 1;
    }
    else if(row_index == 2)
    {
        ROW3 = 1;
    }
    else
    {
        ROW4 = 1;
    }

    delay_ms(10);

    if(COL1)
    {
        while(COL1 && mode);
        return 0;
    }
    if(COL2)
    {
        while(COL2 && mode);
        return 1;
    }
    if(COL3)
    {
        while(COL3 && mode);
        return 2;
    }
    if(COL4)
    {
        while(COL4 && mode);
        return 3;
    }

    return KEY_NONE;
}

u8 key_scan(u8 mode)
{
    u8 col;

    col = Key_ScanRow(0, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) return KEY_1;
        if(col == 1) return KEY_2;
        if(col == 2) return KEY_3;
        return KEY_SET;
    }

    col = Key_ScanRow(1, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) return KEY_4;
        if(col == 1) return KEY_5;
        if(col == 2) return KEY_6;
        return KEY_ADD;
    }

    col = Key_ScanRow(2, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) return KEY_7;
        if(col == 1) return KEY_8;
        if(col == 2) return KEY_9;
        return KEY_DEC;
    }

    col = Key_ScanRow(3, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) return KEY_Asterisk;
        if(col == 1) return KEY_0;
        if(col == 2) return KEY_Hashtag;
        return KEY_SAVE;
    }

    return KEY_NONE;
}
