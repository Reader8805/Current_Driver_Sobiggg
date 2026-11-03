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
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ad7606.h"
#include "pwm_control.h"
#include "fsm.h"
#include "control_loop.h"
#include "serial.h"
#include "rs485.h"
#include "LCD.h"
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
volatile int16_t offset_ch1 = 0;
uint8_t current_state = 0;
volatile float current_display = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  // 1. Khởi tạo State Machine và các Module ngoại vi
//    FSM_Init();
//    AD7606_Init();
//    offset_ch1 = AD7606_Calibrate_Offset_CH1(5000);
//    PWM_Init();
//    RS485_Init(&huart1);
//    lcd_init();
//    // 2. Bắt đầu kích hoạt Timer chạy ngầm tạo Pipeline ADC-PWM 25kHz
//    HAL_TIM_Base_Start_IT(&htim1);

  FSM_Init();
  /* Chưa khởi động PWM/timer trong quá trình hiệu chuẩn. */
  PWM_Stop();
  HAL_NVIC_DisableIRQ(EXTI3_IRQn);

  lcd_init();
  lcd_put_cur(0, 0);
  lcd_send_string("STARTING...  ");

  /*
   * Khoảng chờ thử nghiệm cho nguồn cảm biến ổn định.
   * Nếu cảm biến cần lâu hơn, phải điều chỉnh theo phần cứng.
   */
  HAL_Delay(1000);

  AD7606_Init();

  /* Use the runtime sampling path during zero calibration as well.
   * Keep STATE_INIT and zero duty until calibration succeeds.
   */
  PWM_Init();
  PWM_Stop();
  __HAL_GPIO_EXTI_CLEAR_IT(ADC_BUSY_Pin);
  HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
  HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
  HAL_StatusTypeDef timer_status = HAL_TIM_Base_Start_IT(&htim1);

  int16_t calibrated_offset = 0;

  HAL_StatusTypeDef adc_status = timer_status;
  if (adc_status == HAL_OK)
      adc_status = AD7606_Calibrate_Offset_CH1(4096, &calibrated_offset);

  if (adc_status != HAL_OK)
  {
      /* Preserve the cause before aborting SPI changes peripheral status. */
      AD7606_Error_t adc_error = AD7606_GetError();
      HAL_TIM_Base_Stop_IT(&htim1);
      HAL_NVIC_DisableIRQ(EXTI3_IRQn);
      HAL_SPI_Abort(&hspi1);
      AD7606_End_DMA_Read();
      PWM_Stop();
      FSM_SetState(STATE_FAULT);

      lcd_clear();
      lcd_put_cur(0, 0);
      lcd_send_string("ADC INIT ERROR  ");
      lcd_put_cur(1, 0);
      if (timer_status != HAL_OK)
          lcd_send_string("TIMER START ERR ");
      else
      {
          switch (adc_error)
          {
          case ADC_ERROR_SAMPLE_OVERRUN:
              lcd_send_string("ADC OVERRUN     "); break;
          case ADC_ERROR_BUSY_STUCK:
              lcd_send_string("ADC BUSY HIGH   "); break;
          case ADC_ERROR_DMA_START:
              lcd_send_string("DMA START ERROR "); break;
          case ADC_ERROR_SPI:
              lcd_send_string("SPI ERROR       "); break;
          case ADC_ERROR_NO_SAMPLES:
              lcd_send_string("ADC NO SAMPLES  "); break;
          case ADC_ERROR_ZERO_NOISE:
              lcd_send_string("ZERO TOO NOISY  "); break;
          case ADC_ERROR_ZERO_DRIFT:
              lcd_send_string("ZERO DRIFT      "); break;
          default:
              lcd_send_string("CALIBRATION ERR "); break;
          }
      }
      while (1)
      {
          HAL_Delay(100);
      }
  }

  offset_ch1 = calibrated_offset;

  RS485_Init(&huart1);

  lcd_clear();

  /* Sampling is already running; do not restart it after calibration. */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Hàm đóng gói, phân tách bản tin
    APP_RS485_Task(&huart1);

	  // Hàm này check lỗi, chạy nền song song, không làm tắc nghẽn PID
	current_state = FSM_GetCurrentState();
	//UI_System_Update(current_state, current_display);
	static uint32_t last_lcd_tick = 0;
	uint32_t now = HAL_GetTick();

	if ((uint32_t)(now - last_lcd_tick) >= 50U)
	{
	    last_lcd_tick = now;
	    UI_System_Update(current_state, current_display);
	}
	FSM_Update();
  }
  /* USER CODE END 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
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
