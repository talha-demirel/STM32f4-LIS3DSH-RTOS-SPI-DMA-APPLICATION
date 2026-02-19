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
static void LIS3DSH_Get_Accel_Task(void *pvParameters);
static void LIS3DSH_Write_Accel_Task(void *pvParameters);


uint8_t LIS3DSH_App_Init(LIS3DSH_HandleTypeDef *hlis){

	/* 1. Create Data Queue */
	DataQueue_Handle = xQueueCreate(30,sizeof(LIS3DSH_DataBatch_t));
	if (DataQueue_Handle == NULL)			return	SENSORMNG_Init_ERR;
	/* 2. Create Binary Semaphores */
	USART_DMA_Cmplt_Semphr_h = xSemaphoreCreateBinary();
	if(USART_DMA_Cmplt_Semphr_h == NULL)	return	SENSORMNG_Init_ERR;

	SPI_DMA_Cmplt_Semphr_h = xSemaphoreCreateBinary();
	if(SPI_DMA_Cmplt_Semphr_h == NULL)		return	SENSORMNG_Init_ERR;

	/* 3. Create Tasks */
	// Producer Task: Reads data from sensor (Stack: 300 Words, Prio: 4)
	if(xTaskCreate(LIS3DSH_Get_Accel_Task, "LIS3DSH_Get_Accel_Task", 300, (void *)hlis, 4, &LIS3DSH_Get_Accel_Task_Handle) != pdPASS) 	return SENSORMNG_TASK_ERR;
	// Consumer Task: Sends data via UART (Stack: 350 Words, Prio: 2)
	if(xTaskCreate(LIS3DSH_Write_Accel_Task, "LIS3DSH_Write_Accel_Task", 350, NULL, 2, &LIS3DSH_Write_Accel_Task_Handle) != pdPASS) 	return SENSORMNG_TASK_ERR;

	return SENSORMNG_OK;
}

static void LIS3DSH_Get_Accel_Task(void *pvParameters){


	LIS3DSH_HandleTypeDef *sensorHandle = (LIS3DSH_HandleTypeDef *)pvParameters;

	uint8_t fifo_wtm = sensorHandle->Init.FIFO_WTM;

	LIS3DSH_DataBatch_t sampleBatch;
	uint8_t sampleCount;

	//	Reset the FIFO to avoid missing a rising edge.
	LIS3DSH_FIFO_Reset(sensorHandle,fifo_wtm);

	for(;;){

		/* 1. Wait for Notification from EXTI Interrupt (Watermark reached) */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		while(1){

			LIS3DSH_FIFO_Get_SampleCount(sensorHandle, &sampleCount);
			if(sampleCount < fifo_wtm)	break;
			sampleBatch.sampleCount = sampleCount;

			LIS3DSH_Get_Accel_FIFO_DMA_Start(sensorHandle, &sampleCount);

			xSemaphoreTake(SPI_DMA_Cmplt_Semphr_h, portMAX_DELAY);

			LIS3DSH_Get_Accel_FIFO_DMA_Cmplt_Handler(sensorHandle, &sampleCount, sampleBatch.samples);


			if(xQueueSend(DataQueue_Handle,&sampleBatch,0)!=pdPASS){
				// Queue is full
			}


		}
	}
}



static void LIS3DSH_Write_Accel_Task(void *pvParameters){

	LIS3DSH_DataBatch_t rxsampleBatch;
	// Static buffer to ensure memory validity during DMA transfer
	static char txBuffer[100];

	for(;;){

		/* 1. Wait for data from the Queue */
		if(xQueueReceive(DataQueue_Handle,&rxsampleBatch , portMAX_DELAY) == pdPASS){

			for(uint8_t i = 0; i < rxsampleBatch.sampleCount; i++){

				/* 2. Format the string */
				int len = snprintf(txBuffer, sizeof(txBuffer),"x-> %.2f\r\ny-> %.2f\r\nz-> %.2f\r\n\n",rxsampleBatch.samples[i].x, rxsampleBatch.samples[i].y, rxsampleBatch.samples[i].z);

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
}

/* --- Interrupt Callbacks --- */

void LIS3DSH_SPI_DMA_Complete_Callback(void){

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR(SPI_DMA_Cmplt_Semphr_h,&xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void LIS3DSH_UART_DMA_Complete_Callback(void){

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR(USART_DMA_Cmplt_Semphr_h,&xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}
