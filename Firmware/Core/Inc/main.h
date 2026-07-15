/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define EF1_Pin GPIO_PIN_13
#define EF1_GPIO_Port GPIOC
#define OCS32_A_Pin GPIO_PIN_14
#define OCS32_A_GPIO_Port GPIOC
#define OSC32_B_Pin GPIO_PIN_15
#define OSC32_B_GPIO_Port GPIOC
#define OSC_IN_Pin GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOH
#define EF2_Pin GPIO_PIN_1
#define EF2_GPIO_Port GPIOH
#define PWM1_Pin GPIO_PIN_0
#define PWM1_GPIO_Port GPIOA
#define LCD_SCK_Pin GPIO_PIN_1
#define LCD_SCK_GPIO_Port GPIOA
#define PWM2_Pin GPIO_PIN_2
#define PWM2_GPIO_Port GPIOA
#define ADC0_Pin GPIO_PIN_3
#define ADC0_GPIO_Port GPIOA
#define DAC1_Pin GPIO_PIN_4
#define DAC1_GPIO_Port GPIOA
#define DAC2_Pin GPIO_PIN_5
#define DAC2_GPIO_Port GPIOA
#define LCD_PWM_Pin GPIO_PIN_6
#define LCD_PWM_GPIO_Port GPIOA
#define LCD_MOSI_Pin GPIO_PIN_7
#define LCD_MOSI_GPIO_Port GPIOA
#define LCD_DC_Pin GPIO_PIN_0
#define LCD_DC_GPIO_Port GPIOB
#define STATUS_LED_Pin GPIO_PIN_1
#define STATUS_LED_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_2
#define LCD_CS_GPIO_Port GPIOB
#define STLINK_TX_Pin GPIO_PIN_10
#define STLINK_TX_GPIO_Port GPIOB
#define BTN1_Pin GPIO_PIN_12
#define BTN1_GPIO_Port GPIOB
#define BTN2_Pin GPIO_PIN_15
#define BTN2_GPIO_Port GPIOB
#define PWM0_Pin GPIO_PIN_8
#define PWM0_GPIO_Port GPIOA
#define UART_TX_Pin GPIO_PIN_9
#define UART_TX_GPIO_Port GPIOA
#define UART_RX_Pin GPIO_PIN_10
#define UART_RX_GPIO_Port GPIOA
#define USB_DN_Pin GPIO_PIN_11
#define USB_DN_GPIO_Port GPIOA
#define USB_DP_Pin GPIO_PIN_12
#define USB_DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define SPI_CS2_Pin GPIO_PIN_15
#define SPI_CS2_GPIO_Port GPIOA
#define SPI_SCK_Pin GPIO_PIN_3
#define SPI_SCK_GPIO_Port GPIOB
#define SPI_MISO_Pin GPIO_PIN_4
#define SPI_MISO_GPIO_Port GPIOB
#define SPI_MOSI_Pin GPIO_PIN_5
#define SPI_MOSI_GPIO_Port GPIOB
#define ENC_A_Pin GPIO_PIN_6
#define ENC_A_GPIO_Port GPIOB
#define ENC_B_Pin GPIO_PIN_7
#define ENC_B_GPIO_Port GPIOB
#define SPI_CS1_Pin GPIO_PIN_3
#define SPI_CS1_GPIO_Port GPIOH
#define FDCAN_RX_Pin GPIO_PIN_8
#define FDCAN_RX_GPIO_Port GPIOB
#define FDCAN_TX_Pin GPIO_PIN_9
#define FDCAN_TX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
