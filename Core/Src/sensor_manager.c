/*
 * freeRTOS.c
 *
 *  Created on: 6 Şub 2026
 *      Author: Talha
 */

#include <sensor_manager.h>
#include "queue.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* --- Configuration Macros --- */
#define FIFO_BYTES_PER_SAMPLE    	6				/* X_L, X_H, Y_L, Y_H, Z_L, Z_H */
#define MAX_BUFFER_SIZE				32 * FIFO_BYTES_PER_SAMPLE + 1


/* --- FreeRTOS Handles --- */
TaskHandle_t LIS3DSH_Get_Accel_Task_Handle;
TaskHandle_t LIS3DSH_Write_Accel_Task_Handle;

QueueHandle_t DataQueue_Handle;
SemaphoreHandle_t USART_DMA_Cmplt_Semphr_h;
SemaphoreHandle_t SPI_DMA_Cmplt_Semphr_h;

/* --- External Low-Level Functions --- */
extern void CS_LOW(LIS3DSH_HandleTypeDef *hlis);
extern void CS_HIGH(LIS3DSH_HandleTypeDef *hlis);

/* --- Private Function Prototypes --- */
static uint8_t LIS3DSH_Get_Accel_FIFO_DMA(LIS3DSH_HandleTypeDef *hlis,LIS3DSH_Data_t *pData,uint8_t sampleCount);
static void LIS3DSH_Get_Accel_Task(void *pvParameters);
static void LIS3DSH_Write_Accel_Task(void *pvParameters);


uint8_t LIS3DSH_App_Init(LIS3DSH_HandleTypeDef *hlis){

	/* 1. Create Data Queue */
	DataQueue_Handle = xQueueCreate(40,sizeof(LIS3DSH_Data_t));
	if (DataQueue_Handle == NULL)			return	SENSORMNG_Init_ERR;
	/* 2. Create Binary Semaphores */
	USART_DMA_Cmplt_Semphr_h = xSemaphoreCreateBinary();
	if(USART_DMA_Cmplt_Semphr_h == NULL)	return	SENSORMNG_Init_ERR;

	SPI_DMA_Cmplt_Semphr_h = xSemaphoreCreateBinary();
	if(SPI_DMA_Cmplt_Semphr_h == NULL)		return	SENSORMNG_Init_ERR;

	/* 3. Create Tasks */
	// Producer Task: Reads data from sensor (Stack: 150 Words(~84 Measured), Prio: 4)
	if(xTaskCreate(LIS3DSH_Get_Accel_Task, "LIS3DSH_Get_Accel_Task", 150, (void *)hlis, 4, &LIS3DSH_Get_Accel_Task_Handle) != pdPASS) 	return SENSORMNG_TASK_ERR;
	// Consumer Task: Sends data via UART (Stack: 350 Words(~225 Measured), Prio: 2)
	if(xTaskCreate(LIS3DSH_Write_Accel_Task, "LIS3DSH_Write_Accel_Task", 350, NULL, 2, &LIS3DSH_Write_Accel_Task_Handle) != pdPASS) 	return SENSORMNG_TASK_ERR;

	return SENSORMNG_OK;
}

static void LIS3DSH_Get_Accel_Task(void *pvParameters){


	LIS3DSH_HandleTypeDef *sensorHandle = (LIS3DSH_HandleTypeDef *)pvParameters;

	LIS3DSH_Data_t XYZ_values;

	//	Reset the FIFO to avoid missing a rising edge.
	LIS3DSH_Write(sensorHandle, LIS3DSH_FIFO_CTRL_ADDR, LIS3DSH_FIFO_BYPASS_MODE);
	LIS3DSH_Write(sensorHandle, LIS3DSH_FIFO_CTRL_ADDR,LIS3DSH_FIFO_STREAM_MODE | FIFO_WATERMARK);

	for(;;){

		/* 1. Wait for Notification from EXTI Interrupt (Watermark reached) */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		while(1){

			 uint8_t src;
			 LIS3DSH_Read(sensorHandle, LIS3DSH_FIFO_SRC_ADDR, &src);
			 uint8_t sampleCount = src & 0x1F;

			 if(sampleCount < FIFO_WATERMARK)	break;

			 if(LIS3DSH_Get_Accel_FIFO_DMA(sensorHandle, &XYZ_values,sampleCount) != LIS3DSH_OK)	break;


		}
	}
}


static void LIS3DSH_Write_Accel_Task(void *pvParameters){

	LIS3DSH_Data_t receivedXYZvalues;
	// Static buffer to ensure memory validity during DMA transfer
	static char txBuffer[100];

	for(;;){

		/* 1. Wait for data from the Queue */
		if(xQueueReceive(DataQueue_Handle, &receivedXYZvalues, portMAX_DELAY) == pdPASS){

			/* 2. Format the string */
			int len = snprintf(txBuffer, sizeof(txBuffer),"x-> %.2f\r\ny-> %.2f\r\nz-> %.2f\r\n\n",receivedXYZvalues.x, receivedXYZvalues.y, receivedXYZvalues.z);

			/* 3. Transmit via DMA */
			if(HAL_UART_Transmit_DMA(&huart2, (uint8_t*)txBuffer, len) == HAL_OK){

				/* 4. Wait for Transmission Complete (Semaphore from ISR) */
				xSemaphoreTake(USART_DMA_Cmplt_Semphr_h,portMAX_DELAY);

			}
			else{

				// DMA Busy or Error
			}


		}
	}
}


static uint8_t LIS3DSH_Get_Accel_FIFO_DMA(LIS3DSH_HandleTypeDef *hlis,LIS3DSH_Data_t *pData,uint8_t sampleCount){

	if(hlis == NULL)															return	LIS3DSH_ERR;
	if(pData == NULL)															return	LIS3DSH_ERR;

	// Address of OUT_X_L with Read Bit set
	const uint8_t X_L_regAddr = LIS3DSH_OUT_X_L_ADDR | LIS3DSH_SPI_READ_BIT;
	/* Static Buffers*/

	uint8_t bufferSize = sampleCount * FIFO_BYTES_PER_SAMPLE + 1;

	static uint8_t txBuf[MAX_BUFFER_SIZE];
	static uint8_t rxBuf[MAX_BUFFER_SIZE];

	txBuf[0] = X_L_regAddr;


	CS_LOW(hlis);

	/* Start DMA Transfer (Transmit Command + Receive 61 Bytes) */
	if(HAL_SPI_TransmitReceive_DMA(hlis->hspi, txBuf, rxBuf, bufferSize) == HAL_OK){

		/* Wait for SPI Transfer Complete (Semaphore given by HAL_SPI_TxRxCpltCallback) */
		xSemaphoreTake(SPI_DMA_Cmplt_Semphr_h,portMAX_DELAY);

		CS_HIGH(hlis);		// Release Chip Select immediately after transfer


		for(uint8_t i = 0; i<sampleCount; i++){

			// Calculate index: 1st byte is dummy, so data starts at index 1.
			// Each sample is 6 bytes.
			uint8_t byte = i * 6 + 1;

			// Combine Low and High bytes
			int16_t xFull = (int16_t)(rxBuf[byte+1] << 8 | rxBuf[byte]);
			int16_t yFull = (int16_t)(rxBuf[byte+3] << 8 | rxBuf[byte+2]);
			int16_t zFull = (int16_t)(rxBuf[byte+5] << 8 | rxBuf[byte+4]);

			// Convert to physical units
			pData->x = (float)xFull * hlis->sensitivity_mult / 1000.;
			pData->y = (float)yFull * hlis->sensitivity_mult / 1000.;
			pData->z = (float)zFull * hlis->sensitivity_mult / 1000.;

			/* Send individual sample to the Queue */
			if(xQueueSend(DataQueue_Handle, pData, 0) != pdPASS){
				// Queue full
			}
		}
	}
	else{

		CS_HIGH(hlis);
		return LIS3DSH_ERR_SPI;

	}

	return LIS3DSH_OK;
}



/* --- Interrupt Callbacks --- */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){

	if(huart->Instance == USART2){

		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		xSemaphoreGiveFromISR(USART_DMA_Cmplt_Semphr_h,&xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	}

}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){

	if(hspi->Instance == SPI1){

		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		xSemaphoreGiveFromISR(SPI_DMA_Cmplt_Semphr_h,&xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	}


}
