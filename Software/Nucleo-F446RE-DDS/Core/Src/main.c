/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *
  * Very Straight forward 3 Phase 50Hz DDS Signal Generator
  * PWM Sine Pattern on TIM1 Ch1, TIM1 Ch2, TIM1 Ch3
  * - Ch1 = PA8  ( CN10-23 )
  * - Ch2 = PA9  ( CN10-21 )
  * - Ch3 = PA10 ( CN10-33 )
  *
  * TIM6 Interrupt for Look Up table increment
  * Nucleo LED blink 5Hz
  *
  *
  *
  *
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#define DDS_START_CLK_TIMER()	HAL_TIM_Base_Start_IT(&htim6) 				// heart beat timer = dds clock timer

#define DDS_START_PWM_TIMER()		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1)			// PWM driver
#define DDS_SET_DUTY_CYCLE_CH1(DC)	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, DC)	// set duty cycle
#define DDS_SET_DUTY_CYCLE_CH2(DC)	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, DC)
#define DDS_SET_DUTY_CYCLE_CH3(DC)	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, DC)

#define N_LUT		512UL
#define REF_CLK		503UL			// choose prime number to not have whole multiple of f_out
#define FULL_RES 	0xFFFFFFFFUL

/* log2(LUT_Lenght) = n = Bit Truncation, 2^n = LUT_Length, 4096 => 12, 2048 => 11, ...*/
#if N_LUT== 128
	#define DDS_TRUNCTION_BIT 7
#elif N_LUT == 256
	#define DDS_TRUNCTION_BIT 8
#elif N_LUT == 512
	#define DDS_TRUNCTION_BIT 9
#elif N_LUT == 1024
	#define DDS_TRUNCTION_BIT 10
#elif N_LUT == 2048
	#define DDS_TRUNCTION_BIT 11
#elif N_LUT == 4096
	#define DDS_TRUNCTION_BIT 12
#else
	#define DDS_TRUNCTION_BIT 9
#endif

static uint32_t DDS_LUT[N_LUT] = {
500, 506, 512, 518, 524, 530, 536, 542, 549, 555, 561, 567, 573, 579, 585, 591, 597, 603, 609, 615,
621, 627, 633, 639, 645, 651, 657, 662, 668, 674, 680, 686, 691, 697, 702, 708, 714, 719, 725, 730,
736, 741, 746, 752, 757, 762, 767, 773, 778, 783, 788, 793, 798, 803, 808, 812, 817, 822, 827, 831,
836, 840, 845, 849, 854, 858, 862, 866, 871, 875, 879, 883, 887, 890, 894, 898, 902, 905, 909, 912,
916, 919, 922, 926, 929, 932, 935, 938, 941, 944, 947, 949, 952, 955, 957, 960, 962, 964, 966, 969,
971, 973, 975, 977, 978, 980, 982, 983, 985, 986, 988, 989, 990, 991, 992, 993, 994, 995, 996, 997,
997, 998, 998, 999, 999, 999, 999, 999, 999, 999, 999, 999, 999, 998, 998, 998, 997, 996, 996, 995,
994, 993, 992, 991, 990, 988, 987, 986, 984, 983, 981, 979, 977, 976, 974, 972, 970, 968, 965, 963,
961, 958, 956, 953, 951, 948, 945, 942, 940, 937, 934, 930, 927, 924, 921, 917, 914, 911, 907, 903,
900, 896, 892, 888, 885, 881, 877, 873, 868, 864, 860, 856, 851, 847, 843, 838, 834, 829, 824, 820,
815, 810, 805, 800, 795, 790, 785, 780, 775, 770, 765, 760, 754, 749, 744, 738, 733, 727, 722, 716,
711, 705, 700, 694, 688, 683, 677, 671, 665, 660, 654, 648, 642, 636, 630, 624, 618, 612, 606, 600,
594, 588, 582, 576, 570, 564, 558, 552, 546, 539, 533, 527, 521, 515, 509, 503, 496, 490, 484, 478,
472, 466, 460, 453, 447, 441, 435, 429, 423, 417, 411, 405, 399, 393, 387, 381, 375, 369, 363, 357,
351, 345, 339, 334, 328, 322, 316, 311, 305, 299, 294, 288, 283, 277, 272, 266, 261, 255, 250, 245,
239, 234, 229, 224, 219, 214, 209, 204, 199, 194, 189, 184, 179, 175, 170, 165, 161, 156, 152, 148,
143, 139, 135, 131, 126, 122, 118, 114, 111, 107, 103, 99, 96, 92, 88, 85, 82, 78, 75, 72,
69, 65, 62, 59, 57, 54, 51, 48, 46, 43, 41, 38, 36, 34, 31, 29, 27, 25, 23, 22,
20, 18, 16, 15, 13, 12, 11, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9,
10, 11, 13, 14, 16, 17, 19, 21, 22, 24, 26, 28, 30, 33, 35, 37, 39, 42, 44, 47,
50, 52, 55, 58, 61, 64, 67, 70, 73, 77, 80, 83, 87, 90, 94, 97, 101, 105, 109, 112,
116, 120, 124, 128, 133, 137, 141, 145, 150, 154, 159, 163, 168, 172, 177, 182, 187, 191, 196, 201,
206, 211, 216, 221, 226, 232, 237, 242, 247, 253, 258, 263, 269, 274, 280, 285, 291, 297, 302, 308,
313, 319, 325, 331, 337, 342, 348, 354, 360, 366, 372, 378, 384, 390, 396, 402, 408, 414, 420, 426,
432, 438, 444, 450, 457, 463, 469, 475, 481, 487, 493, 499,
};

static uint32_t dds_f_out = 50; 		// Hz
static uint32_t M = 0;					//
static uint32_t phase_accu = 0;			//
static uint32_t sin_lut_pointer = 0;	// position in Look Up table ( DDS_LUT[] )

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
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  M = (FULL_RES / REF_CLK * dds_f_out);
  DDS_START_CLK_TIMER();
  DDS_START_PWM_TIMER();		// f(PWM) is set to 1kHz in cubeMX, Duty Cycle max = 1000
  DDS_SET_DUTY_CYCLE_CH1(300);	// just for testing

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

	static uint32_t cnt = 0;
	/* 500Hz IRQ for DDS Tuning Wheel Phase Output */
	if (htim->Instance == TIM6) {
		M = (FULL_RES / REF_CLK * dds_f_out);
		phase_accu += M;
	 	sin_lut_pointer = phase_accu >> (32 - DDS_TRUNCTION_BIT);
	 	DDS_SET_DUTY_CYCLE_CH1( DDS_LUT[sin_lut_pointer] );

		cnt++;
		if (cnt == 100-1){
			HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
			cnt = 0;
		}
	}
}

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

#ifdef  USE_FULL_ASSERT
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
