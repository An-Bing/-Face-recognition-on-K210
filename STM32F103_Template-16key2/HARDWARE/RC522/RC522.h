#ifndef __RC522_H
#define __RC522_H

#define MF522_RST_PIN                    GPIO_Pin_3
#define MF522_RST_PORT                   GPIOB
#define MF522_RST_CLK                    RCC_APB2Periph_GPIOB

#define MF522_MISO_PIN                   GPIO_Pin_4
#define MF522_MISO_PORT                  GPIOB
#define MF522_MISO_CLK                   RCC_APB2Periph_GPIOB

#define MF522_MOSI_PIN                   GPIO_Pin_5
#define MF522_MOSI_PORT                  GPIOB
#define MF522_MOSI_CLK                   RCC_APB2Periph_GPIOB

#define MF522_SCK_PIN                    GPIO_Pin_6
#define MF522_SCK_PORT                   GPIOB
#define MF522_SCK_CLK                    RCC_APB2Periph_GPIOB

#define MF522_NSS_PIN                    GPIO_Pin_7
#define MF522_NSS_PORT                   GPIOB
#define MF522_NSS_CLK                    RCC_APB2Periph_GPIOB

#define RST_H                            GPIO_SetBits(MF522_RST_PORT, MF522_RST_PIN)
#define RST_L                            GPIO_ResetBits(MF522_RST_PORT, MF522_RST_PIN)
#define MOSI_H                           GPIO_SetBits(MF522_MOSI_PORT, MF522_MOSI_PIN)
#define MOSI_L                           GPIO_ResetBits(MF522_MOSI_PORT, MF522_MOSI_PIN)
#define SCK_H                            GPIO_SetBits(MF522_SCK_PORT, MF522_SCK_PIN)
#define SCK_L                            GPIO_ResetBits(MF522_SCK_PORT, MF522_SCK_PIN)
#define NSS_H                            GPIO_SetBits(MF522_NSS_PORT, MF522_NSS_PIN)
#define NSS_L                            GPIO_ResetBits(MF522_NSS_PORT, MF522_NSS_PIN)
#define READ_MISO                        GPIO_ReadInputDataBit(MF522_MISO_PORT, MF522_MISO_PIN)

// 初始化RC522模块
void PcdInit(void);

// 复位RC522模块
char PcdReset(void);

// 打开天线，使能射频场
void PcdAntennaOn(void);

// 关闭天线，禁用射频场
void PcdAntennaOff(void);

// 请求寻卡，获取卡片类型
char PcdRequest(unsigned char req_code,unsigned char *pTagType);

// 防碰撞，获取卡片序列号
char PcdAnticoll(unsigned char *pSnr);

// 选择卡片，锁定特定卡片进行操作
char PcdSelect(unsigned char *pSnr);

// 验证卡片密码
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr);

// 读取卡片数据块
char PcdRead(unsigned char addr,unsigned char *pData);

// 写入卡片数据块
char PcdWrite(unsigned char addr,unsigned char *pData);

// 初始化值操作
char PcdValue(unsigned char dd_mode,unsigned char addr,unsigned char *pValue);

// 将数据块备份到另一个数据块
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr);

// 使卡片进入休眠状态
char PcdHalt(void);

// 与卡片通信，传输数据
char PcdComMF522(unsigned char Command,
                 unsigned char *pInData, 
                 unsigned char InLenByte,
                 unsigned char *pOutData, 
                 unsigned int  *pOutLenBit);

// 计算CRC16校验和
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData);

// 向RC522寄存器写入数据
void WriteRawRC(unsigned char Address,unsigned char value);

// 从RC522寄存器读取数据
unsigned char ReadRawRC(unsigned char Address);

// 设置寄存器位掩码
void SetBitMask(unsigned char reg,unsigned char mask);

// 清除寄存器位掩码
void ClearBitMask(unsigned char reg,unsigned char mask);

// 配置RC522的工作模式，例如ISO14443A或Mifare卡片
char M500PcdConfigISOType(unsigned char type);

// 延时函数，单位为10ms
void delay_10ms(unsigned int _10ms);

// 等待卡片离开感应区域
void WaitCardOff(void);

// MF522???
#define PCD_IDLE              0x00               //??????
#define PCD_AUTHENT           0x0E               //????
#define PCD_RECEIVE           0x08               //????
#define PCD_TRANSMIT          0x04               //????
#define PCD_TRANSCEIVE        0x0C               //???????
#define PCD_RESETPHASE        0x0F               //??
#define PCD_CALCCRC           0x03               //CRC??

// Mifare_One?????
#define PICC_REQIDL           0x26               //????????????
#define PICC_REQALL           0x52               //????????
#define PICC_ANTICOLL1        0x93               //???
#define PICC_ANTICOLL2        0x95               //???
#define PICC_AUTHENT1A        0x60               //??A??
#define PICC_AUTHENT1B        0x61               //??B??
#define PICC_READ             0x30               //??
#define PICC_WRITE            0xA0               //??
#define PICC_DECREMENT        0xC0               //??
#define PICC_INCREMENT        0xC1               //??
#define PICC_RESTORE          0xC2               //????????
#define PICC_TRANSFER         0xB0               //????????
#define PICC_HALT             0x50               //??

// MF522 FIFO????
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte

// MF522?????
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01    
#define     ComIEnReg             0x02    
#define     DivlEnReg             0x03    
#define     ComIrqReg             0x04    
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    
#define     Status1Reg            0x07    
#define     Status2Reg            0x08    
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     RFU3F                 0x3F

#define     REQ_ALL               0x52
#define     KEYA                  0x60

// ?MF522??????????
#define MI_OK                          (char)0
#define MI_NOTAGERR                    (char)(-1)
#define MI_ERR                         (char)(-2)

#endif


