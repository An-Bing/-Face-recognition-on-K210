#include "ds1302.h"
#include "delay.h"

#define DS1302_RST      PBout(12)
#define DS1302_IO_OUT   PBout(13)
#define DS1302_IO_IN    PBin(13)
#define DS1302_SCLK     PBout(14)

#define DS1302_REG_SECOND   0x80
#define DS1302_REG_MINUTE   0x82
#define DS1302_REG_HOUR     0x84
#define DS1302_REG_DAY      0x86
#define DS1302_REG_MONTH    0x88
#define DS1302_REG_WEEK     0x8A
#define DS1302_REG_YEAR     0x8C
#define DS1302_REG_WP       0x8E

static void DS1302_IO_ModeOut(void)
{
    GPIOB->CRH &= ~(0x0Fu << 20);
    GPIOB->CRH |=  (0x03u << 20);
}

static void DS1302_IO_ModeIn(void)
{
    GPIOB->CRH &= ~(0x0Fu << 20);
    GPIOB->CRH |=  (0x04u << 20);
}

static void DS1302_WriteByte(u8 data)
{
    u8 i;

    DS1302_IO_ModeOut();
    for(i = 0; i < 8; i++)
    {
        DS1302_IO_OUT = data & 0x01;
        delay_us(2);
        DS1302_SCLK = 1;
        delay_us(2);
        DS1302_SCLK = 0;
        data >>= 1;
    }
}

static u8 DS1302_ReadByte(void)
{
    u8 i;
    u8 data = 0;

    DS1302_IO_ModeIn();
    for(i = 0; i < 8; i++)
    {
        data >>= 1;
        if(DS1302_IO_IN)
        {
            data |= 0x80;
        }
        DS1302_SCLK = 1;
        delay_us(2);
        DS1302_SCLK = 0;
        delay_us(2);
    }
    return data;
}

static void DS1302_WriteReg(u8 reg, u8 data)
{
    DS1302_RST = 0;
    DS1302_SCLK = 0;
    delay_us(2);
    DS1302_RST = 1;
    delay_us(2);
    DS1302_WriteByte(reg);
    DS1302_WriteByte(data);
    DS1302_RST = 0;
}

static u8 DS1302_ReadReg(u8 reg)
{
    u8 data;

    DS1302_RST = 0;
    DS1302_SCLK = 0;
    delay_us(2);
    DS1302_RST = 1;
    delay_us(2);
    DS1302_WriteByte(reg | 0x01);
    data = DS1302_ReadByte();
    DS1302_RST = 0;
    return data;
}

static u8 DecToBcd(u8 value)
{
    return ((value / 10) << 4) | (value % 10);
}

static u8 BcdToDec(u8 value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
}

u8 DS1302_TimeIsValid(const DS1302_TIME *time)
{
    if(time == 0)
    {
        return 0;
    }

    if(time->month < 1 || time->month > 12)
    {
        return 0;
    }
    if(time->day < 1 || time->day > 31)
    {
        return 0;
    }
    if(time->hour > 23 || time->minute > 59 || time->second > 59)
    {
        return 0;
    }

    return 1;
}

void DS1302_ReadTime(DS1302_TIME *time)
{
    if(time == 0)
    {
        return;
    }

    time->second = BcdToDec(DS1302_ReadReg(DS1302_REG_SECOND) & 0x7F);
    time->minute = BcdToDec(DS1302_ReadReg(DS1302_REG_MINUTE) & 0x7F);
    time->hour   = BcdToDec(DS1302_ReadReg(DS1302_REG_HOUR) & 0x3F);
    time->day    = BcdToDec(DS1302_ReadReg(DS1302_REG_DAY) & 0x3F);
    time->month  = BcdToDec(DS1302_ReadReg(DS1302_REG_MONTH) & 0x1F);
    time->week   = BcdToDec(DS1302_ReadReg(DS1302_REG_WEEK) & 0x07);
    time->year   = BcdToDec(DS1302_ReadReg(DS1302_REG_YEAR));
}

void DS1302_WriteTime(const DS1302_TIME *time)
{
    if(time == 0)
    {
        return;
    }

    DS1302_WriteReg(DS1302_REG_WP, 0x00);
    DS1302_WriteReg(DS1302_REG_SECOND, DecToBcd(time->second));
    DS1302_WriteReg(DS1302_REG_MINUTE, DecToBcd(time->minute));
    DS1302_WriteReg(DS1302_REG_HOUR,   DecToBcd(time->hour));
    DS1302_WriteReg(DS1302_REG_DAY,    DecToBcd(time->day));
    DS1302_WriteReg(DS1302_REG_MONTH,  DecToBcd(time->month));
    DS1302_WriteReg(DS1302_REG_WEEK,   DecToBcd(time->week));
    DS1302_WriteReg(DS1302_REG_YEAR,   DecToBcd(time->year));
    DS1302_WriteReg(DS1302_REG_WP, 0x80);
}

void DS1302_Init(void)
{
    GPIO_InitTypeDef gpio;
    DS1302_TIME default_time;
    DS1302_TIME check_time;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio);

    DS1302_IO_ModeOut();
    DS1302_RST = 0;
    DS1302_SCLK = 0;
    DS1302_IO_OUT = 0;

    DS1302_ReadTime(&check_time);
    if(!DS1302_TimeIsValid(&check_time))
    {
        default_time.year = 24;
        default_time.month = 1;
        default_time.day = 1;
        default_time.hour = 0;
        default_time.minute = 0;
        default_time.second = 0;
        default_time.week = 1;
        DS1302_WriteTime(&default_time);
    }
}
