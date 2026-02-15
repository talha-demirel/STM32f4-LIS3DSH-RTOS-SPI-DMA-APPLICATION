/*
 * freeRTOS.h
 *
 *  Created on: 6 Şub 2026
 *      Author: Talha
 */

#ifndef INC_SENSOR_MANAGER_H_
#define INC_SENSOR_MANAGER_H_

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lis3dsh.h"
#include "semphr.h"

/* Configuration Macros ------------------------------------------------------*/
#define FIFO_WATERMARK      		10U

typedef enum{
	SENSORMNG_OK,
	SENSORMNG_Init_ERR,
	SENSORMNG_TASK_ERR,
	SENSORMNG_ERR
}sensorMng_Status_t;

/* External Handles ----------------------------------------------------------*/
extern TaskHandle_t LIS3DSH_Get_Accel_Task_Handle;

/* Function Prototypes -------------------------------------------------------*/
uint8_t LIS3DSH_App_Init(LIS3DSH_HandleTypeDef *hlis);

#endif /* INC_SENSOR_MANAGER_H_ */
