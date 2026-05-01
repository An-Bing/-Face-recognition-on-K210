#include "usart.h"
#include "delay.h"
#include "timer.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];
u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];
vu16 USART2_RX_STA = 0;

static volatile u8 esp_cmd_ready = 0;
static volatile u8 esp_cmd = 0;
static volatile u8 esp_value = 0;
static u8 esp_in_frame = 0;
static u8 esp_frame_len = 0;
static u8 esp_frame[8];

void Usart1_Init(unsigned int baud)
{
    GPIO_InitTypeDef gpio_initstruct;
    USART_InitTypeDef usart_initstruct;
    NVIC_InitTypeDef nvic_initstruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_9;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);

    gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_10;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);

    usart_initstruct.USART_BaudRate = baud;
    usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_initstruct.USART_Parity = USART_Parity_No;
    usart_initstruct.USART_StopBits = USART_StopBits_1;
    usart_initstruct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &usart_initstruct);

    USART_Cmd(USART1, ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    nvic_initstruct.NVIC_IRQChannel = USART1_IRQn;
    nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
    nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_initstruct.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&nvic_initstruct);
}

void Usart2_Init(unsigned int baud)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    USART_DeInit(USART2);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void USART3_Init(unsigned int baud)
{
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &USART_InitStructure);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);
}

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len)
{
    unsigned short count = 0;
    for(; count < len; count++)
    {
        USART_SendData(USARTx, *str++);
        while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    }
}

void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...)
{
    unsigned char UsartPrintfBuf[296];
    va_list ap;
    unsigned char *pStr = UsartPrintfBuf;

    va_start(ap, fmt);
    vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);
    va_end(ap);

    while(*pStr != 0)
    {
        USART_SendData(USARTx, *pStr++);
        while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    }
}

void DEBUG_LOG(char *fmt,...)
{
    (void)fmt;
}

u8 ESP8266_GetCommand(u8 *cmd, u8 *value)
{
    u8 ret = 0;

    if(cmd == 0 || value == 0)
    {
        return 0;
    }

    __disable_irq();
    if(esp_cmd_ready)
    {
        *cmd = esp_cmd;
        *value = esp_value;
        esp_cmd_ready = 0;
        ret = 1;
    }
    __enable_irq();

    return ret;
}

void ESP8266_SendFrame(u8 cmd, u8 value)
{
    u8 frame[4];
    frame[0] = 'B';
    frame[1] = cmd;
    frame[2] = value;
    frame[3] = 'E';
    Usart_SendString(USART1, frame, 4);
}

void ESP8266_SendJsonStatus(u8 door_state, u8 light_percent, u8 usb_light)
{
    UsartPrintf(USART1, "{\"door\":%d,\"light\":%d,\"usb\":%d}\r\n", door_state, light_percent, usb_light);
}


static u8 ESP01S_EncryptByte(u8 value, u8 key, u16 idx)
{
    return (u8)(value ^ (u8)(key + (idx * 13u)));
}

void ESP01S_SendLogLine(const char *log)
{
    if(log == 0)
    {
        return;
    }
    UsartPrintf(USART1, "$LOG,%s\r\n", log);
}

void ESP01S_SendBlurEncryptedImage(const u8 *gray, u16 width, u16 height, u8 key)
{
    u16 x;
    u16 y;
    u16 i;
    u16 pixel_count;
    u16 chunk_idx;
    u16 chunk_total;
    u16 chunk_offset;
    u16 chunk_size;
    u8 blurred[256];
    u8 encrypted[32];
    char hex[65];

    if(gray == 0)
    {
        return;
    }
    if(width == 0 || height == 0 || width > 16 || height > 16)
    {
        return;
    }

    pixel_count = (u16)(width * height);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            u16 sum = 0;
            u16 cnt = 0;
            s16 dx;
            s16 dy;

            for(dy = -1; dy <= 1; dy++)
            {
                for(dx = -1; dx <= 1; dx++)
                {
                    s16 nx = (s16)x + dx;
                    s16 ny = (s16)y + dy;
                    if(nx >= 0 && nx < (s16)width && ny >= 0 && ny < (s16)height)
                    {
                        sum += gray[(u16)ny * width + (u16)nx];
                        cnt++;
                    }
                }
            }

            blurred[y * width + x] = (u8)(sum / cnt);
        }
    }

    chunk_total = (u16)((pixel_count + 31u) / 32u);

    for(chunk_idx = 0; chunk_idx < chunk_total; chunk_idx++)
    {
        chunk_offset = (u16)(chunk_idx * 32u);
        chunk_size = (u16)((pixel_count - chunk_offset) > 32u ? 32u : (pixel_count - chunk_offset));

        for(i = 0; i < chunk_size; i++)
        {
            encrypted[i] = ESP01S_EncryptByte(blurred[chunk_offset + i], key, (u16)(chunk_offset + i));
        }

        for(i = 0; i < chunk_size; i++)
        {
            static const char lut[] = "0123456789ABCDEF";
            hex[i * 2] = lut[(encrypted[i] >> 4) & 0x0F];
            hex[i * 2 + 1] = lut[encrypted[i] & 0x0F];
        }
        hex[chunk_size * 2] = '\0';

        UsartPrintf(USART1,
                    "$IMG,%d,%d,%d,%d,%d,%s\r\n",
                    (int)(chunk_idx + 1),
                    (int)chunk_total,
                    (int)width,
                    (int)height,
                    (int)chunk_size,
                    hex);
    }
}
void USART1_IRQHandler(void)
{
    u8 data;

    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        data = (u8)USART_ReceiveData(USART1);

        if(data == 'B')
        {
            esp_in_frame = 1;
            esp_frame_len = 0;
        }
        else if(esp_in_frame)
        {
            if(data == 'E')
            {
                if(esp_frame_len >= 2)
                {
                    esp_cmd = esp_frame[0];
                    esp_value = esp_frame[1];
                    esp_cmd_ready = 1;
                }
                esp_in_frame = 0;
                esp_frame_len = 0;
            }
            else if(esp_frame_len < sizeof(esp_frame))
            {
                esp_frame[esp_frame_len++] = data;
            }
            else
            {
                esp_in_frame = 0;
                esp_frame_len = 0;
            }
        }

        USART_ClearFlag(USART1, USART_FLAG_RXNE);
    }
}

void USART2_IRQHandler(void)
{
    u8 res;
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        res = USART_ReceiveData(USART2);
        if((USART2_RX_STA & (1 << 15)) == 0)
        {
            if(USART2_RX_STA < USART2_MAX_RECV_LEN)
            {
                TIM_SetCounter(TIM4, 0);
                if(USART2_RX_STA == 0)
                {
                    TIM_Cmd(TIM4, ENABLE);
                }
                USART2_RX_BUF[USART2_RX_STA++] = res;
            }
            else
            {
                USART2_RX_STA |= 1 << 15;
            }
        }
    }
}

