#include "NRF24L01.h"

// Global Variables
extern SPI_HandleTypeDef hspi1;
tPipeSettings RxPipes[6];

uint8_t Address[5];
uint8_t *pBuff = Address;
uint8_t spiData[20];
// Functions

// Read register
uint8_t NRF24_read_reg(uint8_t RegAddr) // Alright
{
	uint8_t spiData[2];
	spiData[0] = (NRF24_CMD_R_REGISTER| (RegAddr & NRF24_REGADDR_MASK));
	// Begin SPI communication
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, spiData, 1, 100); // Transmit SPI data
	HAL_SPI_Receive(&hspi1, &spiData[1], 1, 100); // Receive SPI data
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
	// End SPI communication
	return spiData[1];
}

void NRF_read_multi_bytes(uint8_t RegAddr,uint8_t *pBuff,uint8_t len_data)
{
	uint8_t spiData[2];
	spiData[0] = (NRF24_CMD_R_REGISTER| (RegAddr & NRF24_REGADDR_MASK));
	
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, spiData, 1, 100); // Transmit SPI data
	HAL_SPI_Receive(&hspi1, pBuff, len_data, 100); // Receive SPI data
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
}

// Write register
void NRF24_write_reg(uint8_t RegAddr,uint8_t data) // Alright
{
	uint8_t spiData;
	spiData = (NRF24_CMD_W_REGISTER| (RegAddr & NRF24_REGADDR_MASK));
	// Begin SPI communication
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, &spiData, 1, 100); // Transmit SPI command & adress
	HAL_SPI_Transmit(&hspi1, &data, 1, 100); // Transmit SPI data
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
	// End SPI communication
}

// Read command
uint8_t* NRF24_read_command(uint8_t command,uint8_t len_data)
{
	uint8_t *buffer;
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, &command, 1, 100); // Transmit SPI command & adress
	HAL_SPI_Receive(&hspi1, buffer, len_data, 300); // Transmit SPI data
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
	
	return buffer;
}

// Write command
void NRF24_write_command(uint8_t command,uint8_t *data,uint8_t len_data)
{
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, &command, 1, 100); // Transmit SPI command & adress
	HAL_SPI_Transmit(&hspi1, data, len_data, 300); // Transmit SPI data
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
}
void NRF24_FlushRx(void)
{
	NRF24_write_command(NRF24_CMD_FLUSH_RX,0,1);
}

void NRF24_FlushTx(void)
{
	NRF24_write_command(NRF24_CMD_FLUSH_TX,0,1);
}

// Set Output Power
void NRF24_SetOutputPower(uint8_t PowerLevel) // Alright
{
  NRF24Reg06 RegValue; 
	uint8_t *pBuff = (uint8_t *) &RegValue;
	*pBuff = NRF24_read_reg(NRF24_REG_RF_SETUP);
  RegValue.bRF_PWR = (PowerLevel > 3) ? 3 : PowerLevel; 
  NRF24_write_reg(NRF24_REG_RF_SETUP,*pBuff);
}

// Set channel
void NRF24_SetChannel(uint8_t Channel) // Alright
{
  Channel &= 0x7F;
  NRF24_write_reg(NRF24_REG_RF_CH,Channel);
}

// Set Data Rate
void NRF24_SetDataRate(uint8_t DataRate)
{
  if (DataRate > 2)
    DataRate = NRF24_DataRate_250KBps;
  
  NRF24Reg06 RegValue; 
	uint8_t *pBuff = (uint8_t *) &RegValue;
	*pBuff = NRF24_read_reg(NRF24_REG_RF_SETUP);
  
  // RF_DR_LOW. 00 = 1Mbps; 10 = 2Mbps; 01 = 250Kbps
  switch (DataRate)
  {
			case NRF24_DataRate_1MBps:
				RegValue.bRF_DR_HIGH = 0;
				RegValue.bRF_DR_LOW = 0;
				break;
			case NRF24_DataRate_2MBps:
				RegValue.bRF_DR_HIGH = 1;
				RegValue.bRF_DR_LOW = 0;
				break;
			case NRF24_DataRate_250KBps:
				RegValue.bRF_DR_HIGH = 0;
				RegValue.bRF_DR_LOW = 1;
				break;
  }
	NRF24_write_reg(NRF24_REG_RF_SETUP,*pBuff);
}

// Set CRC length in config register
void NRF24_SetCRCLen(uint8_t CRCLen)
{
  NRF24Reg00 RegValue; 
	uint8_t *pBuff = (uint8_t *) &RegValue;
	*pBuff = NRF24_read_reg(NRF24_REG_CONFIG);

  if (CRCLen)   // CRC должен быть включен
  {
    RegValue.bEN_CRC = 1;
    RegValue.bCRC_O = (CRCLen == 1) ? 0 : 1;
  }
  else          // CRC нужно отключить
    RegValue.bEN_CRC = 0;
  
  NRF24_write_reg(NRF24_REG_CONFIG,*pBuff);
}

void NRF24_Set_PRX_Mode(void)
{
  // (PowerUp)
  NRF24Reg00 RegValue;
	uint8_t *pBuff = (uint8_t *) &RegValue;
  *pBuff = NRF24_read_reg(NRF24_REG_CONFIG);
  RegValue.bPRIM_RX = 1;        // Переключаемся в режим приёмника (PRX)
  RegValue.bPWR_UP = 1;         // Будим nRF24, если он спал
  NRF24_write_reg(NRF24_REG_CONFIG,*pBuff);

  // Сбрасываем флаги прерываний nRF24
  NRF24_ResetStateFlags(NRF24_bMAX_RT_Mask | NRF24_bTX_DS_Mask | NRF24_bRX_DR_Mask);

  // Восстанавливаем адрес Rx для соединения №0, т.к. он мог быть затёрт при передаче
  //NRF24_write_reg(NRF24_REG_RX_ADDR_P0, RxPipes[0].Address, 5);

  // Очищаем буферы FIFO
  NRF24_FlushRx();
  NRF24_FlushTx();

  HAL_GPIO_WritePin(GPIOA, CE_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 3 to High

  // Задержка как минимум 130 мкс
  HAL_Delay(1);
}

void NRF24_Set_PTX_Mode(void)
{
	NRF24Reg00 RegValue;
	uint8_t *pBuff = (uint8_t *) &RegValue;
	
  HAL_GPIO_WritePin(GPIOA, CE_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 3 to High
  // Очищаем буферы FIFO
  NRF24_FlushRx();
  NRF24_FlushTx();
	
	 // (PowerUp)
	*pBuff = NRF24_read_reg(NRF24_REG_CONFIG);
  RegValue.bPRIM_RX = 0;        // Переключаемся в режим передатчика (PTX)
  RegValue.bPWR_UP = 1;         // Будим nRF24, если он спал
  NRF24_write_reg(NRF24_REG_CONFIG,*pBuff);
}

void NRF24_AutoRetrasmission_Setup(uint8_t TryCount, uint8_t TryPeriod)
{
  NRF24Reg04 RegValue;
	uint8_t *pBuff = (uint8_t *) &RegValue;
	*pBuff = NRF24_read_reg(NRF24_REG_SETUP_RETR);
  RegValue.bARC = (TryCount > 15) ? 15 : TryCount;
  RegValue.bARD = (TryPeriod > 15) ? 15 : TryPeriod;
  
	NRF24_write_reg(NRF24_REG_SETUP_RETR,*pBuff);
}

void NRF24_ResetStateFlags(uint8_t Flags)
{
  NRF24_write_reg(NRF24_REG_STATUS,Flags);
}

void NRF24_Receive_RxPayload(uint8_t *pBuff)
{	
	uint8_t spiData[1];
	spiData[0] = NRF24_CMD_R_RX_PAYLOAD;
	// Begin SPI communication
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, spiData, 1, 100); // Transmit SPI command 
	for(unsigned int c=0;c<NRF24_TX_PAYLOAD_WIDTH;c++)
	{
		//HAL_SPI_Transmit(&hspi1,pBuff, 1, 100); // Transmit SPI data
		//pBuff++;
		pBuff[c] = HAL_SPI_Receive(&hspi1,pBuff, 1, 100); // Transmit SPI data
	}	
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
}

uint8_t NRF24_Receive(uint8_t *pPipe, uint8_t *pBuff)
{	
  uint8_t value=0, status = NRF24_read_reg(NRF24_REG_STATUS);
	
  if (status & NRF24_bRX_DR_Mask)       // Был принят пакет и флаг RX_DR ещё не сброшен
  {
		NRF24_Receive_RxPayload(pBuff);
		value = 1;  
    // Сбрасываем флаг RX_DR
    NRF24_ResetStateFlags(NRF24_bRX_DR_Mask);
  }
  
  return value;
}
//==============================================================================

void NRF24_Write_TxPayload(uint8_t *pBuff)
{	
	uint8_t spiData[1];
	spiData[0] = NRF24_CMD_W_TX_PAYLOAD;
	// Begin SPI communication
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	HAL_SPI_Transmit(&hspi1, spiData, 1, 100); // Transmit SPI command 
	for(unsigned int c=0;c<NRF24_TX_PAYLOAD_WIDTH;c++)
	{
		HAL_SPI_Transmit(&hspi1,pBuff, 1, 100); // Transmit SPI data
		pBuff++;
	}	
	HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to High
}

void NRF24_Send(uint8_t *pBuff)
{
  //NRF24_write_regs(NRF24_REG_TX_ADDR, pAddress, 5);
  NRF24_Set_PTX_Mode();
	
	NRF24_Write_TxPayload(pBuff);
  
  HAL_Delay(2); // 1.5ms
	
  // Выдаём положительный импульс на ножку CE
  HAL_GPIO_WritePin(GPIOA, CE_Pin, GPIO_PIN_SET); // Set GPIOA, PIN 4 to Low
  HAL_Delay(1);         // Задержка минимум 10 мкс
  HAL_GPIO_WritePin(GPIOA, CE_Pin, GPIO_PIN_RESET); // Set GPIOA, PIN 4 to Low
	
	NRF24_Set_PRX_Mode();
}
 // NRF24 initialization
void NRF24_init(uint8_t Channel)
{
	HAL_GPIO_WritePin(GPIOA, CE_Pin, GPIO_PIN_RESET); // Set Chip Enable, PIN 3 to Low
  HAL_GPIO_WritePin(GPIOA, CSN_Pin, GPIO_PIN_SET); // Set Chip Selection, PIN 4 to High
	
  NRF24_AutoRetrasmission_Setup(5, 2);          // 5 попыток с периодом 0.75 мс
	spiData[0] = NRF24_read_reg(NRF24_REG_SETUP_RETR);
	spiData[0] = NRF24_read_reg(NRF24_REG_SETUP_RETR);

  NRF24_SetOutputPower(NRF24_OutPower_0dBm);    // 0dBm
	spiData[1] = NRF24_read_reg(NRF24_REG_RF_SETUP);

  NRF24_SetDataRate(NRF24_DataRate_250KBps);//NRF24_DataRate_2MBps);
  spiData[2] = NRF24_read_reg(NRF24_REG_RF_SETUP);
	
  NRF24_SetCRCLen(2);
  spiData[3] = NRF24_read_reg(NRF24_REG_CONFIG);
	
  NRF24_ResetStateFlags(NRF24_bMAX_RT_Mask | NRF24_bTX_DS_Mask | NRF24_bRX_DR_Mask);
  spiData[4] = NRF24_read_reg(NRF24_REG_STATUS);
	
  NRF24_SetChannel(Channel);
	spiData[5] = NRF24_read_reg(NRF24_REG_RF_CH);
  
  NRF24_FlushRx();
  NRF24_FlushTx();

	NRF24_Set_PRX_Mode();
}

