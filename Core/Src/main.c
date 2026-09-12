/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "usb.h"
#include "gd_usb.h"

#include "gameboy_rom.h"
#include "gameboy_bridge.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t rx_buffer[64];

static uint8_t gameboy_framebuffer[160 * 144];



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void App_Test(void)
{
    USB_SendString("Hello from STM32\r\n");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  uint32_t reset_flags = RCC->CSR;

  /* Clear the reset flags so the next reset gives us fresh information */
  RCC->CSR |= RCC_CSR_RMVF;



  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  USB_Init();

  HAL_Delay(5000);

  if (reset_flags & RCC_CSR_SFTRSTF)
  {
      USB_SendString("SOFTWARE RESET\r\n");
  }
  else if (reset_flags & RCC_CSR_WWDGRSTF)
  {
      USB_SendString("WINDOW WATCHDOG RESET\r\n");
  }
  else if (reset_flags & RCC_CSR_IWDGRSTF)
  {
      USB_SendString("INDEPENDENT WATCHDOG RESET\r\n");
  }
  else if (reset_flags & RCC_CSR_PINRSTF)
  {
      USB_SendString("PIN RESET\r\n");
  }
  else if (reset_flags & RCC_CSR_BORRSTF)
  {
      USB_SendString("BROWNOUT RESET\r\n");
  }
  else if (reset_flags & RCC_CSR_PORRSTF)
  {
      USB_SendString("POWER RESET\r\n");
  }
  else
  {
      USB_SendString("NO KNOWN RESET FLAG\r\n");
  }

  HAL_Delay(1000);

  USB_SendString("1\r\n");
  HAL_Delay(1000);

  USB_SendString("2\r\n");
  HAL_Delay(1000);





  //==============================================================//
  char msges[64];

  snprintf(msges, sizeof(msges),
           "ROM SIZE: %lu\r\n",
           (unsigned long)gameboy_rom_size);

  USB_SendString(msges);

  //===============================================================//




  GameBoy_TestCartInit(gameboy_rom, gameboy_rom_size);
  HAL_Delay(1000);
//  char msg[64];
//
//  snprintf(msg, sizeof(msg),
//           "CART STAGE: %lu\r\n",
//           (unsigned long)GameBoy_GetDebugStage());
HAL_Delay(1000);
//  USB_SendString(msg);
  USB_SendString("3\r\n");
  HAL_Delay(1000);



  GameBoy_TestGBInit();

  USB_SendString("4\r\n");
  HAL_Delay(1000);


  uint32_t white;
  uint32_t light_gray;
  uint32_t dark_gray;
  uint32_t black;

  char palette_msg[128];

  char msg[128];


  while (1)
  {
      GameBoy_RunFrame();

      GameBoy_GetFrame(gameboy_framebuffer);

      GD_USB_SendFrame(gameboy_framebuffer);

      HAL_Delay(100);
  }

  while (1)
  {
      GameBoy_RunFrame();

      GameBoy_GetPaletteStats(&white,
                              &light_gray,
                              &dark_gray,
                              &black);

      snprintf(palette_msg, sizeof(palette_msg),
               "W=%lu LG=%lu DG=%lu B=%lu\r\n",
               (unsigned long)white,
               (unsigned long)light_gray,
               (unsigned long)dark_gray,
               (unsigned long)black);

      USB_SendString(palette_msg);

      HAL_Delay(500);
  }

//  GD_USB_Init();
//  /* USER CODE END 2 */
//
//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */
//  while (1)
//  {
//	    USB_SendString("A\r\n");
//
//	    GameBoy_RunFrame();
//
//	    USB_SendString("B\r\n");
//
//	    GameBoy_GetFrame(gameboy_framebuffer);
//
//	    USB_SendString("C\r\n");
//
//	    HAL_Delay(100);
//  }
//  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
