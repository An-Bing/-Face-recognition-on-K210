#ifndef __DS1302_H
#define __DS1302_H

#include "sys.h"

typedef struct
{
    u8 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 week;
} DS1302_TIME;

void DS1302_Init(void);
void DS1302_ReadTime(DS1302_TIME *time);
void DS1302_WriteTime(const DS1302_TIME *time);
u8 DS1302_TimeIsValid(const DS1302_TIME *time);

#endif
