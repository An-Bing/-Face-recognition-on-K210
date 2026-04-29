#ifndef __IOPUT_H
#define __IOPUT_H

#include "sys.h"
#include "delay.h"
#include "lcd.h"

#define COL1    PAin(0)
#define COL2    PAin(1)
#define COL3    PAin(2)
#define COL4    PAin(3)

#define ROW1    PAout(7)
#define ROW2    PAout(6)
#define ROW3    PAout(5)
#define ROW4    PAout(4)

#define DOOR_LOCK   PAout(8)
#define ALARM_IO    PAout(12)
#define PIR_IN      PBin(1)

#define KEY_0           0
#define KEY_1           1
#define KEY_2           2
#define KEY_3           3
#define KEY_4           4
#define KEY_5           5
#define KEY_6           6
#define KEY_7           7
#define KEY_8           8
#define KEY_9           9
#define KEY_Asterisk    10
#define KEY_Hashtag     11
#define KEY_SET         12
#define KEY_ADD         13
#define KEY_DEC         14
#define KEY_SAVE        15
#define KEY_NONE        16

extern u8 key_num;

void output_init(void);
void input_init(void);
u8 key_scan(u8 mode);

#endif
