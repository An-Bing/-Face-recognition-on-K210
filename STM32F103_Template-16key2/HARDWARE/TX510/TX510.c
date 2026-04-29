#include "TX510.h"
#include "stm32f10x_usart.h"

#define K210_FRAME_HEAD1    0xAA
#define K210_FRAME_HEAD2    0x55

static volatile u8 rx_state = 0;
static volatile u8 rx_cmd = 0;
static volatile u8 rx_len = 0;
static volatile u8 rx_index = 0;
static volatile u8 rx_buf[16];

static volatile u8 face_state = K210_FACE_NONE;
static volatile u8 face_id = 0xFF;
static volatile u8 face_total = 0;
static volatile u8 face_update = 0;

static volatile u8 op_cmd = 0;
static volatile u8 op_result = 0;
static volatile u8 op_slot = 0xFF;
static volatile u8 op_total = 0;
static volatile u8 op_update = 0;

static u8 K210_Checksum(u8 cmd, u8 len, const u8 *payload)
{
    u8 i;
    u8 sum = cmd + len;

    for(i = 0; i < len; i++)
    {
        sum = (u8)(sum + payload[i]);
    }

    return sum;
}

static void K210_HandleFrame(u8 cmd, u8 len, const u8 *payload)
{
    if(cmd == 0x81 && len >= 3)
    {
        face_state = payload[0];
        face_id = payload[1];
        face_total = payload[2];
        face_update = 1;
    }
    else if(cmd == 0x82 && len >= 4)
    {
        op_cmd = payload[0];
        op_result = payload[1];
        op_slot = payload[2];
        op_total = payload[3];
        face_total = payload[3];
        op_update = 1;
    }
}

static void K210_SendFrame(u8 cmd, const u8 *payload, u8 len)
{
    u8 i;
    u8 sum = K210_Checksum(cmd, len, payload);

    USART_SendData(USART3, K210_FRAME_HEAD1);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    USART_SendData(USART3, K210_FRAME_HEAD2);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    USART_SendData(USART3, cmd);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    USART_SendData(USART3, len);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

    for(i = 0; i < len; i++)
    {
        USART_SendData(USART3, payload[i]);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    }

    USART_SendData(USART3, sum);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

void USART3_IRQHandler(void)
{
    u8 data;
    u8 sum;

    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        data = (u8)USART_ReceiveData(USART3);

        if(rx_state == 0)
        {
            if(data == K210_FRAME_HEAD1)
            {
                rx_state = 1;
            }
        }
        else if(rx_state == 1)
        {
            if(data == K210_FRAME_HEAD2)
            {
                rx_state = 2;
            }
            else
            {
                rx_state = 0;
            }
        }
        else if(rx_state == 2)
        {
            rx_cmd = data;
            rx_state = 3;
        }
        else if(rx_state == 3)
        {
            rx_len = data;
            rx_index = 0;
            if(rx_len > sizeof(rx_buf))
            {
                rx_state = 0;
            }
            else if(rx_len == 0)
            {
                rx_state = 5;
            }
            else
            {
                rx_state = 4;
            }
        }
        else if(rx_state == 4)
        {
            rx_buf[rx_index++] = data;
            if(rx_index >= rx_len)
            {
                rx_state = 5;
            }
        }
        else if(rx_state == 5)
        {
            sum = K210_Checksum(rx_cmd, rx_len, (const u8 *)rx_buf);
            if(sum == data)
            {
                K210_HandleFrame(rx_cmd, rx_len, (const u8 *)rx_buf);
            }
            rx_state = 0;
        }
        else
        {
            rx_state = 0;
        }

        USART_ClearFlag(USART3, USART_FLAG_RXNE);
    }
}

void K210_SendRegister(u8 slot)
{
    u8 payload[1];
    payload[0] = slot;
    K210_SendFrame(0x01, payload, 1);
}

void K210_SendDelete(u8 slot)
{
    u8 payload[1];
    payload[0] = slot;
    K210_SendFrame(0x02, payload, 1);
}

void K210_SendClearAll(void)
{
    K210_SendFrame(0x03, 0, 0);
}

u8 K210_GetFaceState(u8 *state, u8 *id, u8 *total)
{
    if(state == 0 || id == 0 || total == 0)
    {
        return 0;
    }

    __disable_irq();
    *state = face_state;
    *id = face_id;
    *total = face_total;
    __enable_irq();
    return 1;
}

u8 K210_FetchFaceUpdate(u8 *state, u8 *id, u8 *total)
{
    u8 ret = 0;

    if(state == 0 || id == 0 || total == 0)
    {
        return 0;
    }

    __disable_irq();
    if(face_update)
    {
        *state = face_state;
        *id = face_id;
        *total = face_total;
        face_update = 0;
        ret = 1;
    }
    __enable_irq();

    return ret;
}

u8 K210_FetchOpResult(u8 *op, u8 *result, u8 *slot, u8 *total)
{
    u8 ret = 0;

    if(op == 0 || result == 0 || slot == 0 || total == 0)
    {
        return 0;
    }

    __disable_irq();
    if(op_update)
    {
        *op = op_cmd;
        *result = op_result;
        *slot = op_slot;
        *total = op_total;
        op_update = 0;
        ret = 1;
    }
    __enable_irq();

    return ret;
}

void K210_ClearFaceUpdate(void)
{
    __disable_irq();
    face_update = 0;
    __enable_irq();
}

void K210_ClearOpResult(void)
{
    __disable_irq();
    op_update = 0;
    __enable_irq();
}
