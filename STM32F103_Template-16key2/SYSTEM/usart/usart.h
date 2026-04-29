#ifndef _USART_H_
#define _USART_H_

#include "stm32f10x.h"

#define USART_DEBUG     USART1

#define USART2_MAX_RECV_LEN     400
#define USART2_MAX_SEND_LEN     400
#define USART2_RX_EN            1

extern u8 USART2_RX_BUF[USART2_MAX_RECV_LEN];
extern u8 USART2_TX_BUF[USART2_MAX_SEND_LEN];
extern vu16 USART2_RX_STA;

void Usart1_Init(unsigned int baud);
void Usart2_Init(unsigned int baud);
void USART3_Init(unsigned int baud);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);
void DEBUG_LOG(char *fmt,...);

/* ESP8266: ´®¿ÚÖ¡Ð­Òé B + cmd + value + E */
u8 ESP8266_GetCommand(u8 *cmd, u8 *value);
void ESP8266_SendFrame(u8 cmd, u8 value);
void ESP8266_SendJsonStatus(u8 door_state, u8 light_percent, u8 usb_light);

#endif
