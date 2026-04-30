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

    delay_ms(1);

    if(COL1)
    {
        return 0;
    }
    if(COL2)
    {
        return 1;
    }
    if(COL3)
    {
        return 2;
    }
    if(COL4)
    {
        return 3;
    }

    return KEY_NONE;
}

u8 key_scan(u8 mode)
{
    u8 col;
    u8 raw_key = KEY_NONE;
    static u8 last_key = KEY_NONE;

    col = Key_ScanRow(0, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) raw_key = KEY_1;
        else if(col == 1) raw_key = KEY_2;
        else if(col == 2) raw_key = KEY_3;
        else raw_key = KEY_SET;
        goto KEY_DONE;
    }

    col = Key_ScanRow(1, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) raw_key = KEY_4;
        else if(col == 1) raw_key = KEY_5;
        else if(col == 2) raw_key = KEY_6;
        else raw_key = KEY_ADD;
        goto KEY_DONE;
    }

    col = Key_ScanRow(2, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) raw_key = KEY_7;
        else if(col == 1) raw_key = KEY_8;
        else if(col == 2) raw_key = KEY_9;
        else raw_key = KEY_DEC;
        goto KEY_DONE;
    }

    col = Key_ScanRow(3, mode);
    if(col != KEY_NONE)
    {
        if(col == 0) raw_key = KEY_Asterisk;
        else if(col == 1) raw_key = KEY_0;
        else if(col == 2) raw_key = KEY_Hashtag;
        else raw_key = KEY_SAVE;
        goto KEY_DONE;
    }

KEY_DONE:
    if(raw_key == KEY_NONE)
    {
        last_key = KEY_NONE;
        return KEY_NONE;
    }
    if(raw_key == last_key)
    {
        return KEY_NONE;
    }
    last_key = raw_key;
    return raw_key;
}


