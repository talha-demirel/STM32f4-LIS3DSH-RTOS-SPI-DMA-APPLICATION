/*
 * lis3dsh.c
 *
 *  Created on: 3 Şub 2026
 *      Author: Talha
 */
#include "lis3dsh.h"
#include "main.h"
#include "stm32f4xx.h"


/* --- Private Function Prototypes --- */
static void CS_LOW(LIS3DSH_HandleTypeDef *hlis);
static void CS_HIGH(LIS3DSH_HandleTypeDef *hlis);





/**
 * @brief  Write single byte to LIS3DSH register over SPI
 * @param  hlis     Pointer to sensor handle
 * @param  regAddr  Register address
 * @param  data     Data to write
 * @retval Status
 */
static uint8_t LIS3DSH_Write(LIS3DSH_HandleTypeDef *hlis,uint8_t regAddr,uint8_t data){

	if(hlis == NULL)															return	LIS3DSH_ERR;

	uint8_t txBuf[2];
	txBuf[0] = regAddr;
	txBuf[1] = data;

	CS_LOW(hlis);

	if(HAL_SPI_Transmit(hlis->hspi, txBuf, sizeof(txBuf), LIS3DSH_SPI_TIMEOUT) == HAL_TIMEOUT){

		CS_HIGH(hlis);
		return LIS3DSH_ERR_SPI;
	}

	CS_HIGH(hlis);

	return LIS3DSH_OK;

}


/**
 * @brief  Read single byte from LIS3DSH register
 * @note   First byte is command, second byte is dummy to clock data out
 */
static uint8_t LIS3DSH_Read(LIS3DSH_HandleTypeDef *hlis, uint8_t regAddr, uint8_t *rxData){

	if(hlis == NULL)															return	LIS3DSH_ERR;
	if(rxData == NULL)															return	LIS3DSH_ERR;

	uint8_t txBuf[2];
	uint8_t rxBuf[2];

    // Adding Read Bit
	txBuf[0] = regAddr | LIS3DSH_SPI_READ_BIT;
    txBuf[1] = 0x00;

    CS_LOW(hlis);

    if(HAL_SPI_TransmitReceive(hlis->hspi, txBuf, rxBuf, 2, LIS3DSH_SPI_TIMEOUT) != HAL_OK){

    	CS_HIGH(hlis);

    	return LIS3DSH_ERR_SPI;
    }

    CS_HIGH(hlis);

    *rxData = rxBuf[1];

    return LIS3DSH_OK;

}


uint8_t LIS3DSH_Init(LIS3DSH_HandleTypeDef *hlis){

	if(hlis == NULL)															return	LIS3DSH_ERR;
	if(hlis->hspi == NULL) 														return	LIS3DSH_ERR_SPI;
	if(hlis->CS_Port == NULL)													return	LIS3DSH_ERR;

	/* 1. Check Device ID (WHO_AM_I) */
	uint8_t chipID = 0;
	if(LIS3DSH_Read(hlis, LIS3DSH_WHO_AM_I_ADDR, &chipID) != LIS3DSH_OK)		return	LIS3DSH_ERR_SPI;

	if(chipID != LIS3DSH_WHO_AM_I_ID) 											return	LIS3DSH_ERR_ID;

	/* 2. Configure CTRL_REG4 (ODR, BDU, Axes) */
	uint8_t ctrl_reg4 = 0x0;

	ctrl_reg4 |= hlis->Init.ODR;
	ctrl_reg4 |= hlis->Init.BlockDataUpdate;
	ctrl_reg4 |= hlis->Init.Axes_Enable;

	if(LIS3DSH_Write(hlis, LIS3DSH_CTRL_REG4_ADDR, ctrl_reg4) != LIS3DSH_OK)	return LIS3DSH_ERR_SPI;


	/* 3. Configure CTRL_REG3 (Interrupts) */
	uint8_t ctrl_reg3 = 0x0;

	ctrl_reg3 |= hlis->Init.Interrupt_Config;

	if(LIS3DSH_Write(hlis, LIS3DSH_CTRL_REG3_ADDR, ctrl_reg3) != LIS3DSH_OK)	return LIS3DSH_ERR_SPI;

	/* 4. Configure CTRL_REG5 (Bandwidth, Full Scale) */
	uint8_t ctrl_reg5 = 0x0;

	ctrl_reg5 |= hlis->Init.AntiAliasingBW;
	ctrl_reg5 |= hlis->Init.Full_Scale;

	if(LIS3DSH_Write(hlis, LIS3DSH_CTRL_REG5_ADDR, ctrl_reg5) != LIS3DSH_OK)	return LIS3DSH_ERR_SPI;

	/* 5. Set Sensitivity Multiplier based on Full Scale */
	uint8_t selected_fs = (hlis->Init.Full_Scale != 0x0) ? hlis->Init.Full_Scale : LIS3DSH_FULLSCALE_2G;

	switch(selected_fs) {

	    case LIS3DSH_FULLSCALE_2G:  	hlis->sensitivity_mult = 0.06f; 	break;
	    case LIS3DSH_FULLSCALE_4G:  	hlis->sensitivity_mult = 0.12f; 	break;
	    case LIS3DSH_FULLSCALE_6G:  	hlis->sensitivity_mult = 0.18f; 	break;
	    case LIS3DSH_FULLSCALE_8G:  	hlis->sensitivity_mult = 0.24f; 	break;
	    case LIS3DSH_FULLSCALE_16G: 	hlis->sensitivity_mult = 0.73f; 	break;
	    default:                    	hlis->sensitivity_mult = 0.06f; 	break;
	}

	/* 6. Configure CTRL_REG6 (FIFO Enable, ADD_INC) */
	uint8_t ctrl_reg6 = 0;

	ctrl_reg6 |= hlis->Init.FIFO_Enable | hlis->Init.ADD_INC_Enable;

	if(LIS3DSH_Write(hlis, LIS3DSH_CTRL_REG6_ADDR, ctrl_reg6) != LIS3DSH_OK)	return LIS3DSH_ERR_SPI;


	/* 7. Configure FIFO_CTRL (FIFO Mode, Threshold) */
	uint8_t fifo_ctrl = 0x0;

	fifo_ctrl |= hlis->Init.FIFO_Mode | hlis->Init.FIFO_WTM;

	if(LIS3DSH_Write(hlis, LIS3DSH_FIFO_CTRL_ADDR, fifo_ctrl) != LIS3DSH_OK)	return LIS3DSH_ERR_SPI;


	return LIS3DSH_OK;
}

uint8_t LIS3DSH_SetDefaults(LIS3DSH_HandleTypeDef *hlis){

	if(hlis == NULL)															return	LIS3DSH_ERR;

	hlis->Init.ODR = LIS3DSH_ODR_100HZ;
	hlis->Init.Full_Scale = LIS3DSH_FULLSCALE_2G;
	hlis->Init.Axes_Enable = LIS3DSHX_AXES_XYZ_ENABLE;
	hlis->Init.AntiAliasingBW = LIS3DSH_BW_FILTER_50HZ;
	hlis->Init.BlockDataUpdate = LIS3DSH_BDU_BLOCKED;
	hlis->Init.FIFO_Enable = LIS3DSH_FIFO_DISABLE;
	hlis->Init.ADD_INC_Enable = LIS3DSH_ADD_INC_ENABLE;
	hlis->Init.FIFO_Mode = LIS3DSH_FIFO_BYPASS_MODE;
	hlis->Init.FIFO_WTM = 0;
	hlis->Init.Interrupt_Config = LIS3DSH_INT_DISABLE;

	return LIS3DSH_OK;

}


uint8_t LIS3DSH_Get_Accel(LIS3DSH_HandleTypeDef *hlis,LIS3DSH_Data_t *pData){

	if(hlis == NULL)															return	LIS3DSH_ERR;
	if(pData == NULL)															return	LIS3DSH_ERR;

	// Address of OUT_X_L with Read Bit set
	uint8_t regAddr = LIS3DSH_OUT_X_L_ADDR | LIS3DSH_SPI_READ_BIT;		//Adding Read Bit
	uint8_t txBuf[7] = {regAddr,0,0,0,0,0,0};
	uint8_t rxBuf[7];

	CS_LOW(hlis);

	// Read 7 bytes: 1 dummy (while sending addr) + 6 data (X_L, X_H, Y_L, Y_H, Z_L, Z_H)
	if(HAL_SPI_TransmitReceive(hlis->hspi, txBuf, rxBuf, 7 ,LIS3DSH_SPI_TIMEOUT) != HAL_OK){

		CS_HIGH(hlis);
		return LIS3DSH_ERR_SPI;
	}

	CS_HIGH(hlis);

	// Combine High and Low bytes
	int16_t xFull = (int16_t)(rxBuf[2] << 8 | rxBuf[1]);
	int16_t yFull = (int16_t)(rxBuf[4] << 8 | rxBuf[3]);
	int16_t zFull = (int16_t)(rxBuf[6] << 8 | rxBuf[5]);

	// Convert raw values to physical units (mg -> g) using sensitivity
	pData->x = (float)xFull * hlis->sensitivity_mult / 1000.;
	pData->y = (float)yFull * hlis->sensitivity_mult / 1000.;
	pData->z = (float)zFull * hlis->sensitivity_mult / 1000.;

	return LIS3DSH_OK;

}



static uint8_t txBuf[MAX_BUFFER_SIZE];
static uint8_t rxBuf[MAX_BUFFER_SIZE];
/**
 * @brief  Start SPI DMA transfer to read FIFO samples
 * @note   Reads N samples (6 bytes each) + 1 dummy command byte
 */
uint8_t LIS3DSH_Get_Accel_FIFO_DMA_Start(LIS3DSH_HandleTypeDef *hlis, uint8_t *sampleCount){

	if(hlis == NULL)															return	LIS3DSH_ERR;
	if(sampleCount == NULL)														return	LIS3DSH_ERR;

	// Address of OUT_X_L with Read Bit set
	const uint8_t X_L_regAddr = LIS3DSH_OUT_X_L_ADDR | LIS3DSH_SPI_READ_BIT;

	/* Static Buffers*/
	uint8_t bufferSize = *sampleCount * LIS3DSH_FIFO_BYTES_PER_SAMPLE + 1;

	txBuf[0] = X_L_regAddr;

	CS_LOW(hlis);

	/* Start DMA Transfer (Transmit Command + Receive 61 Bytes) */
	if(HAL_SPI_TransmitReceive_DMA(hlis->hspi, txBuf, rxBuf, bufferSize) != HAL_OK){

		CS_HIGH(hlis);
		return LIS3DSH_ERR_SPI;

	}

	return LIS3DSH_OK;
}


/**
 * @brief  Process FIFO data after DMA completes
 * @note   Converts raw data to floating-point acceleration values
 */
uint8_t LIS3DSH_Get_Accel_FIFO_DMA_Cmplt_Handler(LIS3DSH_HandleTypeDef *hlis, uint8_t *sampleCount, LIS3DSH_Data_t *pXYZ_valArr){

	if(hlis == NULL)															return	LIS3DSH_ERR;

	CS_HIGH(hlis);		// Release Chip Select immediately after transfer

	LIS3DSH_Data_t XYZ_val;

	for(uint8_t i = 0; i<*sampleCount; i++){

		// Calculate index: 1st byte is dummy, so data starts at index 1.
		// Each sample is 6 bytes.
		uint8_t byte = i * 6 + 1;

		// Combine Low and High bytes
		int16_t xFull = (int16_t)(rxBuf[byte+1] << 8 | rxBuf[byte]);
		int16_t yFull = (int16_t)(rxBuf[byte+3] << 8 | rxBuf[byte+2]);
		int16_t zFull = (int16_t)(rxBuf[byte+5] << 8 | rxBuf[byte+4]);

		// Convert to physical units
		XYZ_val.x = (float)xFull * hlis->sensitivity_mult / 1000.;
		XYZ_val.y = (float)yFull * hlis->sensitivity_mult / 1000.;
		XYZ_val.z = (float)zFull * hlis->sensitivity_mult / 1000.;

		pXYZ_valArr[i] = XYZ_val;

	}

	return	LIS3DSH_OK;
}


/**
 * @brief  Reset FIFO by switching to bypass then stream mode
 * @note   Required to avoid stale watermark interrupt condition
 */
uint8_t LIS3DSH_FIFO_Reset(LIS3DSH_HandleTypeDef *hlis, uint8_t waterMark){

	if(LIS3DSH_Write(hlis, LIS3DSH_FIFO_CTRL_ADDR, LIS3DSH_FIFO_BYPASS_MODE)!=LIS3DSH_OK) 						return LIS3DSH_ERR;
	if(LIS3DSH_Write(hlis, LIS3DSH_FIFO_CTRL_ADDR,LIS3DSH_FIFO_STREAM_MODE | waterMark)!=LIS3DSH_OK) 			return LIS3DSH_ERR;

	return LIS3DSH_OK;
}

uint8_t LIS3DSH_FIFO_Get_SampleCount(LIS3DSH_HandleTypeDef *hlis, uint8_t *sampleCount){

	uint8_t src;
	if(LIS3DSH_Read(hlis, LIS3DSH_FIFO_SRC_ADDR, &src) != LIS3DSH_OK)		return	LIS3DSH_ERR;
	*sampleCount = src & 0x1F;

	return LIS3DSH_OK;

}



/* --- Low Level Helper Functions --- */
static void CS_LOW(LIS3DSH_HandleTypeDef *hlis){
	HAL_GPIO_WritePin(hlis->CS_Port, hlis->CS_Pin, GPIO_PIN_RESET);
}

static void CS_HIGH(LIS3DSH_HandleTypeDef *hlis){
	HAL_GPIO_WritePin(hlis->CS_Port, hlis->CS_Pin, GPIO_PIN_SET);
}



/* Weak default implementation.
 * User can override this function in another file.
 */
__weak void LIS3DSH_SPI_DMA_Complete_Callback(void);
__weak void LIS3DSH_UART_DMA_Complete_Callback(void);
