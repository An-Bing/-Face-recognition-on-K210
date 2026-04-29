#include "jq8400.h"   

 /*
  * @brief  初始化控制JQ8400的串口2
  */
void USART_Config(void)
{		
		GPIO_InitTypeDef GPIO_InitStructure;
	  USART_InitTypeDef USART_InitStructure;

		RCC_APB2PeriphClockCmd( USART_RX_CLK | USART_TX_CLK , ENABLE);//串口GPIO时钟
	  RCC_APB1PeriphClockCmd( UserPeriph_USARTx ,ENABLE);//串口外设时钟
	
		GPIO_InitStructure.GPIO_Pin = USART_TX_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(USART_TX_PORT, &GPIO_InitStructure);
	
	  GPIO_InitStructure.GPIO_Pin = USART_RX_PIN;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
		GPIO_Init(USART_RX_PORT, &GPIO_InitStructure);	
		
	
	  USART_InitStructure.USART_BaudRate = 9600;
	  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx ;
	  USART_InitStructure.USART_Parity = USART_Parity_No;
	  USART_InitStructure.USART_StopBits = USART_StopBits_1 ;
	  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		
		USART_Init( UserUSARTx,&USART_InitStructure);
		
		USART_Cmd( UserUSARTx,ENABLE);
		
}

/***************** 发送一个 32 位数 **********************/
void Usart_Send32bit( USART_TypeDef * pUSARTx, uint32_t ch)
{
   uint8_t temp_1, temp_2, temp_3, temp_4; //4个8位

   /* 取出第一八位 */
   temp_1 = (ch&0XFF000000)>>24;
   /* 取出第二八位 */
   temp_2 = (ch&0X00FF0000)>>16;
	 /* 取出第三八位 */
   temp_3 = (ch&0X0000FF00)>>8;
   /* 取出第四八位 */
   temp_4 = (ch&0X000000FF);

   /* 发送第一八位 */
   USART_SendData(pUSARTx,temp_1);
   while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
	
   /* 发送第二八位 */
   USART_SendData(pUSARTx,temp_2);
   while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
	
	 /* 发送第三八位 */
   USART_SendData(pUSARTx,temp_3);
   while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
	
   /* 发送第四八位 */
   USART_SendData(pUSARTx,temp_4);
   while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
 }

/***************** 发送一个 8 位数 **********************/ 
void Usart_Send8bit( USART_TypeDef * pUSARTx, uint8_t eight)
{
   /* 发送一个字节数据到 USART */
   USART_SendData(pUSARTx,eight);

   /* 等待发送数据寄存器为空 */
   while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}
 
/***************** 循环模式：单曲停止 **********************/ 
void asong_stop(void)
{
	 Usart_Send32bit( UserUSARTx, 0xAA180102);
	 Usart_Send8bit( UserUSARTx, 0xC5);
}

/***************** 播放模式：指定歌曲（playsong(1)：播放00001曲目） **********************/ 
//曲目的文件名需要按指定格式来保存的，使用说明书上有写
/*
指定曲目(07)
指令：AA 07 02 曲目高 曲目低 SM
返回：无
和检验（SM）：为之前所有字节之和的低8位,即前面的数据相加后取低8位
例如:  AA 07 02 00 08 BB 指定播放当前盘符第8首（AA+07+02+00+08=BB取低8位，即为BB）
*/
void playsong(u16 i)
{
	Usart_Send32bit( UserUSARTx,0xAA070200);
	Usart_Send8bit( UserUSARTx, 0x00+i);
	Usart_Send8bit( UserUSARTx, 0xB3+i);//控制指令要48位，故拆分开，32位+8位+8位
}
