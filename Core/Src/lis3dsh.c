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
void CS_LOW(LIS3DSH_HandleTypeDef *hlis);
void CS_HIGH(LIS3DSH_HandleTypeDef *hlis);

uint8_t LIS3DSH_Write(LIS3DSH_HandleTypeDef *hlis,uint8_t regAddr,uint8_t data){

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

uint8_t LIS3DSH_Read(LIS3DSH_HandleTypeDef *hlis, uint8_t regAddr, uint8_t *rxData){

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

	fifo_ctrl |= hlis->Init.FIFO_Mode;

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



/* --- Low Level Helper Functions --- */
void CS_LOW(LIS3DSH_HandleTypeDef *hlis){
	HAL_GPIO_WritePin(hlis->CS_Port, hlis->CS_Pin, GPIO_PIN_RESET);
}

void CS_HIGH(LIS3DSH_HandleTypeDef *hlis){
	HAL_GPIO_WritePin(hlis->CS_Port, hlis->CS_Pin, GPIO_PIN_SET);
}
