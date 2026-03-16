/*
 * lis3dsh.h
 *
 *  Created on: 3 Şub 2026
 *      Author: Talha
 */

#ifndef INC_LIS3DSH_H_
#define INC_LIS3DSH_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "gpio.h"
#include "spi.h"

/* Common Definitions --------------------------------------------------------*/
#define LIS3DSH_SPI_TIMEOUT				100U

					//REGISTER ADDRESSES

#define LIS3DSH_WHO_AM_I_ADDR       	(uint8_t)0x0F
#define LIS3DSH_CTRL_REG4_ADDR      	(uint8_t)0x20
#define LIS3DSH_CTRL_REG1_ADDR      	(uint8_t)0x21
#define LIS3DSH_CTRL_REG2_ADDR      	(uint8_t)0x22
#define LIS3DSH_CTRL_REG3_ADDR      	(uint8_t)0x23 	// Interrupt Configuration
#define LIS3DSH_CTRL_REG5_ADDR      	(uint8_t)0x24	// Bandwidth / Scale
#define LIS3DSH_CTRL_REG6_ADDR      	(uint8_t)0x25 	// FIFO Enable / Addr Inc
#define LIS3DSH_FIFO_CTRL_ADDR      	(uint8_t)0x2E	// FIFO Configuration
#define LIS3DSH_STATUS_ADDR         	(uint8_t)0x27
#define LIS3DSH_OUT_X_L_ADDR       		(uint8_t)0x28
#define LIS3DSH_FIFO_SRC_ADDR			(uint8_t)0x2F


#define LIS3DSH_WHO_AM_I_ID				(uint8_t)0x3F
#define LIS3DSH_SPI_READ_BIT   			(uint8_t)0x80	// Read operation mask
#define LIS3DSH_SPI_WRITE_BIT   		(uint8_t)0x7F	// Write operation mask
#define LIS3DSH_SPI_MULTI_BIT   		(uint8_t)0x40

					//INIT MACROS

/* --- 1. Output Data Rate (ODR) Selection [CTRL_REG4] --- */
#define LIS3DSH_ODR_POWER_DOWN      	(uint8_t)0x00
#define LIS3DSH_ODR_3_125HZ         	(uint8_t)0x10
#define LIS3DSH_ODR_6_25HZ          	(uint8_t)0x20
#define LIS3DSH_ODR_12_5HZ          	(uint8_t)0x30
#define LIS3DSH_ODR_25HZ            	(uint8_t)0x40
#define LIS3DSH_ODR_50HZ            	(uint8_t)0x50
#define LIS3DSH_ODR_100HZ           	(uint8_t)0x60 	// Default
#define LIS3DSH_ODR_400HZ           	(uint8_t)0x70
#define LIS3DSH_ODR_800HZ           	(uint8_t)0x80
#define LIS3DSH_ODR_1600HZ          	(uint8_t)0x90

/* --- 2. Full Scale (Range) Selection [CTRL_REG5] --- */
#define LIS3DSH_FULLSCALE_2G        	(uint8_t)0x00 	// Default
#define LIS3DSH_FULLSCALE_4G        	(uint8_t)0x08
#define LIS3DSH_FULLSCALE_6G        	(uint8_t)0x10
#define LIS3DSH_FULLSCALE_8G        	(uint8_t)0x18
#define LIS3DSH_FULLSCALE_16G       	(uint8_t)0x20

/* --- 3. Anti-Aliasing Filter Bandwidth [CTRL_REG5] --- */
#define LIS3DSH_BW_FILTER_800HZ     	(uint8_t)0x00
#define LIS3DSH_BW_FILTER_400HZ     	(uint8_t)0x40
#define LIS3DSH_BW_FILTER_200HZ     	(uint8_t)0x80
#define LIS3DSH_BW_FILTER_50HZ      	(uint8_t)0xC0	// Default

/* --- 4. Axes Selection [CTRL_REG4] --- */
#define LIS3DSHX_AXES_XYZ_ENABLE      	(uint8_t)0x07	// Default
#define LIS3DSH_AXES_X_ENABLE        	(uint8_t)0x01
#define LIS3DSH_AXES_Y_ENABLE        	(uint8_t)0x02
#define LIS3DSH_AXES_Z_ENABLE        	(uint8_t)0x04

/* --- 5. Block Data Update (BDU) [CTRL_REG4] --- */
#define LIS3DSH_BDU_CONTINUOUS      	(uint8_t)0x00
#define LIS3DSH_BDU_BLOCKED         	(uint8_t)0x08	// Default

/* --- 6. Interrupt Configuration [CTRL_REG3] --- */
#define LIS3DSH_INT_DISABLE         	(uint8_t)0x00	// Default
#define LIS3DSH_INT1_DRDY_ENABLE     	(uint8_t)0x88 	// DR_EN | INT1_EN
#define LIS3DSH_INT1_ENABLE 			(uint8_t)0x08	// INT1_EN(1)
#define LIS3DSH_INT_ACTIVE_HIGH     	(uint8_t)0x40 	// IEA (1)
#define LIS3DSH_INT_ACTIVE_LOW      	(uint8_t)0x00	// IEA (0)
#define LIS3DSH_INT_LATCHED          	(uint8_t)0x00
#define LIS3DSH_INT_PULSED         		(uint8_t)0x20 	// IEL

/* --- 7. FIFO Configuration [CTRL_REG6 & FIFO_CTRL] --- */
// FIFO_CTRL Register Modes
#define LIS3DSH_FIFO_BYPASS_MODE    	(uint8_t)0x00	// Default
#define LIS3DSH_FIFO_STREAM_MODE    	(uint8_t)0x40
#define LIS3DSH_FIFO_MODE           	(uint8_t)0x20
// FIFO_CTRL Watermark Macros
#define LIS3DSH_FIFO_WTM_5				(uint8_t)5
#define LIS3DSH_FIFO_WTM_10				(uint8_t)10
#define LIS3DSH_FIFO_WTM_15				(uint8_t)15
#define LIS3DSH_FIFO_WTM_20				(uint8_t)20
#define LIS3DSH_FIFO_WTM_25				(uint8_t)25
#define LIS3DSH_FIFO_WTM_30				(uint8_t)30
#define LIS3DSH_FIFO_WTM_MAX			(uint8_t)32

#define LIS3DSH_FIFO_BYTES_PER_SAMPLE   (uint8_t)6		/* X_L, X_H, Y_L, Y_H, Z_L, Z_H */
#define LIS3DSH_FIFO_MAX_SIZE			(uint8_t)32
#define MAX_BUFFER_SIZE					(uint8_t)32 * LIS3DSH_FIFO_BYTES_PER_SAMPLE + 1



// CTRL_REG6 Enable Bits
#define LIS3DSH_FIFO_DISABLE     		(uint8_t)0x00	// Default
#define LIS3DSH_FIFO_ENABLE     		(uint8_t)0x40	// FIFO_EN
#define LIS3DSH_FIFO_WTM_ENABLE     	(uint8_t)0x20	// WTM_EN (Watermark Enable)
#define LIS3DSH_ADD_INC_ENABLE 			(uint8_t)0x10	// Address Auto-Increment (Default)
#define LIS3DSH_FIFO_WTM_INT1_ENABLE	(uint8_t)0x04	// Watermark Interrupt Enable on Int1
#define LIS3DSH_FIFO_OVRRN_INT1_ENABLE	(uint8_t)0x04	// Overrun Interrupt Enable on Int1


typedef struct {

	uint8_t Interrupt_Config;
	uint8_t ODR;
	uint8_t BlockDataUpdate;
	uint8_t Axes_Enable;
	uint8_t Full_Scale;
	uint8_t AntiAliasingBW;
	uint8_t FIFO_Enable;
	uint8_t FIFO_Mode;
	uint8_t FIFO_WTM;
	uint8_t ADD_INC_Enable;

} LIS3DSH_InitTypeDef;



typedef struct{

	SPI_HandleTypeDef 		*hspi;

	GPIO_TypeDef			*CS_Port;
	uint16_t 				CS_Pin;

	GPIO_TypeDef            *INT_Port;
	uint16_t                INT_Pin;

	LIS3DSH_InitTypeDef 	Init;

	float 					sensitivity_mult;

}LIS3DSH_HandleTypeDef;



typedef struct{

	float x;
	float y;
	float z;

}LIS3DSH_Data_t;

typedef struct {

    uint8_t sampleCount;
    LIS3DSH_Data_t samples[LIS3DSH_FIFO_MAX_SIZE];

} LIS3DSH_DataBatch_t;


typedef enum {
    LIS3DSH_OK,
	LIS3DSH_ERR,
    LIS3DSH_ERR_SPI,
    LIS3DSH_ERR_ID
} LIS3DSH_Status_t;


/* Function Prototypes -------------------------------------------------------*/
uint8_t LIS3DSH_Init(LIS3DSH_HandleTypeDef *hlis);
uint8_t LIS3DSH_SetDefaults(LIS3DSH_HandleTypeDef *hlis);
uint8_t LIS3DSH_Get_Accel(LIS3DSH_HandleTypeDef *hlis,LIS3DSH_Data_t *pData);
uint8_t LIS3DSH_Get_Accel_FIFO_DMA_Start(LIS3DSH_HandleTypeDef *hlis, uint8_t *sampleCount);
uint8_t LIS3DSH_Get_Accel_FIFO_DMA_Cmplt_Handler(LIS3DSH_HandleTypeDef *hlis, uint8_t *sampleCount, LIS3DSH_Data_t *pXYZ_valArr);
uint8_t LIS3DSH_FIFO_Reset(LIS3DSH_HandleTypeDef *hlis, uint8_t waterMark);
uint8_t LIS3DSH_FIFO_Get_SampleCount(LIS3DSH_HandleTypeDef *hlis, uint8_t *sampleCount);


#endif /* INC_LIS3DSH_H_ */
