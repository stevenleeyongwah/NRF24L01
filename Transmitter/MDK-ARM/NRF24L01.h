#include "stm32f1xx_hal.h"

// Masks and some constants
#define NRF24_REGADDR_MASK      0x1F
#define TX_ADDR_WIDTH    				5      
#define RX_ADDR_WIDTH    				5 
#define NRF24_TX_PAYLOAD_WIDTH  32    
#define NRF24_RX_PAYLOAD_WIDTH  32  

// NRF24L01 data rates
#define NRF24_DataRate_1MBps    0 // 00 - 1Mbps
#define NRF24_DataRate_2MBps    1 // 10 - 2Mbps
#define NRF24_DataRate_250KBps  2 // 01 - 250kbps

// NRF Output power in TX mode
#define NRF24_OutPower_n18dBm   0 // -18dBm
#define NRF24_OutPower_n12dBm   1 // -12dBm
#define NRF24_OutPower_n6dBm    2 // -6dBm
#define NRF24_OutPower_0dBm     3 // 0dBm

// Маски в регистре STATUS
#define NRF24_bMAX_RT_Mask      (1 << 4)        // Флаг прерывания истечения попыток отправки.
#define NRF24_bTX_DS_Mask       (1 << 5)        // Флаг прерывания окончания отправки из TX FIFO. Устанавливается в 1 когда получен ACK
#define NRF24_bRX_DR_Mask       (1 << 6)        // Флаг прерывания получения пакета. Устанавливается в 1 когда в RX FIFO был получен пакет
// Маски в регистре FIFO_STATUS
#define NRF24_bRX_EMPTY_Mask    (1 << 0)        // Флаг опустошения RX FIFO
#define NRF24_bRX_FULL_Mask     (1 << 1)        // Флаг заполнения RX FIFO
#define NRF24_bTX_EMPTY_Mask    (1 << 4)        // Флаг опустошения TX FIFO
#define NRF24_bTX_FULL_Mask     (1 << 5)        // Флаг заполнения TX FIFO
#define NRF24_bTX_REUSE_Mask    (1 << 6)        // Reuse last transmitted data packet if set high. 
                                                // The packet is repeatedly retransmitted as long as CE is high.
                                                // TX_REUSE is set by the SPI command REUSE_TX_PL, and is reset by
																								
// NRF24L01 SPI commands name
#define NRF24_CMD_R_REGISTER    0x00   // 000A AAAA 
#define NRF24_CMD_W_REGISTER    0x20	 // 001A AAAA
#define NRF24_CMD_R_RX_PAYLOAD  0x61   // 0110 0001
#define NRF24_CMD_W_TX_PAYLOAD  0xA0	 // 1010 0000
#define NRF24_CMD_FLUSH_TX      0xE1	 // 1110 0001
#define NRF24_CMD_FLUSH_RX      0xE2	 // 1110 0010
#define NRF24_CMD_REUSE_TX_PL   0xE3	 // 1110 0011
#define NRF24_CMD_R_RX_PL_WID   0x60	 // 0110 0000
#define NRF24_CMD_W_ACK_PAYLOAD 0xA8	 // 1010 1PPP
#define NRF24_CMD_W_TX				  0xB0	 // 1011 0000
#define NRF24_CMD_NOP           0xFF   // 1111 1111

// Register map table
#define NRF24_REG_CONFIG        0x00
#define NRF24_REG_EN_AA         0x01
#define NRF24_REG_EN_RXADDR     0x02
#define NRF24_REG_SETUP_AW      0x03
#define NRF24_REG_SETUP_RETR    0x04
#define NRF24_REG_RF_CH         0x05
#define NRF24_REG_RF_SETUP      0x06
#define NRF24_REG_STATUS        0x07
#define NRF24_REG_OBSERVE_TX    0x08
#define NRF24_REG_RPD           0x09
#define NRF24_REG_RX_ADDR_P0    0x0A
#define NRF24_REG_RX_ADDR_P1    0x0B
#define NRF24_REG_RX_ADDR_P2    0x0C
#define NRF24_REG_RX_ADDR_P3    0x0D
#define NRF24_REG_RX_ADDR_P4    0x0E
#define NRF24_REG_RX_ADDR_P5    0x0F
#define NRF24_REG_TX_ADDR       0x10
#define NRF24_REG_RX_PW_P0      0x11
#define NRF24_REG_RX_PW_P1      0x12
#define NRF24_REG_RX_PW_P2      0x13
#define NRF24_REG_RX_PW_P3      0x14
#define NRF24_REG_RX_PW_P4      0x15
#define NRF24_REG_RX_PW_P5      0x16
#define NRF24_REG_FIFO_STATUS   0x17
#define NRF24_REG_DYNPD	        0x1C
#define NRF24_REG_FEATURE				0x1D

// Bit definition
typedef struct 
{
  uint8_t bPRIM_RX      :1;     // 00 (0 = PTX; 1 = PRX)   
  uint8_t bPWR_UP       :1;     // 01 (1 = Power Up, 0 = Power Down)
  uint8_t bCRC_O        :1;     // 02 CRC encoding scheme (0 = 1 byte, 1 = 2 byte)  
  uint8_t bEN_CRC       :1;     // 03 Enable CRC    
  uint8_t bMASK_MAX_RT  :1;     // 04 IRQ 
  uint8_t bMASK_TX_DS   :1;     // 05 IRQ    
  uint8_t bMASK_RX_DR   :1;     // 06 IRQ  
	uint8_t bRes          :1;     // 07 Reserved	
	} NRF24Reg00; // Adress: 0x00 Configuration Register

typedef struct 
{
  uint8_t bARC          :4;     // 00-03 Кол-во попыток передачи пакета при отсутствии ACK от приёмника
  uint8_t bARD          :4;     // 04-07 Пауза между попытками ((N + 1) * 250мкс)
} NRF24Reg04; // SETUP_RETR   Setup of Automatic Retransmission

typedef struct 
{
  uint8_t bLNA_HCURR    :1;     // 00 Коэффициент усиления приёмника
  uint8_t bRF_PWR       :2;     // 01-02 Мощность передатчика (0 = -18dBm; 1 = -12dBm; 2 = -6dBm; 3 = 0dBm)
  uint8_t bRF_DR_HIGH   :1;     // 03 Частота передачи (старший бит). 00 = 1Mbps; 10 = 2Mbps; 01 = 250Kbps
  uint8_t bPLL_LOCK     :1;     // 04 Force PLL lock signal. Only used in test
  uint8_t bRF_DR_LOW    :1;     // 05 Частота передачи (младший бит). 00 = 1Mbps; 10 = 2Mbps; 01 = 250Kbps
  uint8_t bRes          :1;     // 06 Reserved
  uint8_t bCONT_WAVE    :1;     // 07 Enable continuous carrier transmit
} NRF24Reg06; // RF_SETUP     RF Setup Register

typedef struct 
{
  uint8_t bARC_CNT      :4;     // 00-03 Счётчик повторов отправки пакета. Счётчик сбрасывается когда начинается отправка нового пакета.
  uint8_t bPLOS_CNT     :4;     // 04-07 Счётчик потерянных пакетов (не превышает 15). Счётчик сбрасывается записью в RF_CH.
} NRF24Reg08; // OBSERVE_TX   Transmit observe register

typedef struct 
{
  uint8_t bEN_DYN_ACK   :1;     // 00 Enables the W_TX_PAYLOAD_NOACK command
  uint8_t bEN_ACK_PAY   :1;     // 01 Enables Payload with ACK
  uint8_t bEN_DPL       :1;     // 02 Enables Dynamic Payload Length
  uint8_t bRes          :5;     // 03-07 Reserved
} NRF24Reg1D; // FEATURE      R/W Feature Register

typedef struct
{
  uint8_t bDynPayLoad   :1;
  uint8_t PayLoadLen    :6;
  uint8_t Address[5];
} tPipeSettings;

uint8_t NRF24_read_reg(uint8_t RegAddr);
void NRF24_write_reg(uint8_t RegAddr,uint8_t data);
void NRF_read_multi_bytes(uint8_t RegAddr,uint8_t *pBuff,uint8_t len_data);
void NRF24_SetOutputPower(uint8_t PowerLevel);
void NRF24_SetChannel(uint8_t Channel);
void NRF24_init(uint8_t Channel);
void NRF24_ResetStateFlags(uint8_t Flags);
