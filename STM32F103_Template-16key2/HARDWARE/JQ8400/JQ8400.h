#ifndef __JQ8400_H
#define	__JQ8400_H


#include "stm32f10x.h"

/*串口2总线*/
#define UserUSARTx USART2
#define UserPeriph_USARTx RCC_APB1Periph_USART2

/*串口2GPIO*/
// RX
#define USART_TX_PORT    	    GPIOA			              /* GPIO端口 */
#define USART_TX_CLK 	        RCC_APB2Periph_GPIOA		/* GPIO端口时钟 */
#define USART_TX_PIN		      GPIO_Pin_2			        /* 连接到SCL时钟线的GPIO */

// TX
#define USART_RX_PORT    	    GPIOA
#define USART_RX_CLK 	        RCC_APB2Periph_GPIOA		
#define USART_RX_PIN		      GPIO_Pin_3			        

// BUSY
//#define JQ8400_BUSY    	    GPIOA			              /* GPIO端口 */
//#define JQ8400_BUSY_CLK 	  RCC_APB1Periph_GPIOA		/* GPIO端口时钟 */
//#define JQ8400_BUSY_PIN		  GPIO_Pin_5			        /* 连接到SCL时钟线的GPIO */


void Usart_Send8bit( USART_TypeDef * pUSARTx, uint8_t eight);
void USART_Config(void);
void Usart_Send32bit( USART_TypeDef * pUSARTx, uint32_t ch);
void asong_stop(void);
void playsong(u16 i);

#endif /* __JQ8400_H */

