/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "RoverArmMotor.h"
#include "AMT22.h"
// Standard includes
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <bitset>
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
void print(const char *s)
{
  //	#ifdef PRINT
  HAL_StatusTypeDef code = HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
  //	#endif
}
int printf(const char *s, ...)
{
  char buffer[256];
  //	#ifdef PRINT
  va_list args;
  va_start(args, s);
  vsprintf(buffer, s, args);
  perror(buffer);
  print(buffer);
  va_end(args);
  //	#endif
  return strlen(buffer);
}
void delay_us(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0); // set the counter value a 0
  while (__HAL_TIM_GET_COUNTER(&htim1) < us)
    ; // wait for the counter to reach the us input in the parameter
}

/*---------------------CYTRON DECLARATIONS---------------------*/

double setpoint = 0;
int turn = 0;
int brakeSet = 0;
Pin CYTRON_DIR_1(CYTRON_DIR_1_GPIO_Port, CYTRON_DIR_1_Pin);
Pin CYTRON_PWM_1(CYTRON_PWM_1_GPIO_Port, CYTRON_PWM_1_Pin, &htim2, TIM_CHANNEL_2);
Pin AMT22_1(GPIOB, GPIO_PIN_11);
RoverArmMotor Wrist_Roll(&hspi1, CYTRON_PWM_1, CYTRON_DIR_1, AMT22_1, CYTRON, 0, 359.99f);
int button_counter = 0;

/*---------------------SERVO DECLARATIONS---------------------*/
Pin dummy_pin;
Pin SERVO_PWM_1(SERVO_PWM_1_GPIO_Port, SERVO_PWM_1_Pin, &htim1, TIM_CHANNEL_2);
RoverArmMotor Waist(&hspi1, SERVO_PWM_1, dummy_pin, AMT22_1, BLUE_ROBOTICS, 0, 359.99f);
int is_turning = 0;

/*---------------------HELPER---------------------*/
void print_MOTOR(char *msg, RoverArmMotor *pMotor)
{
  double current_angle = pMotor->get_current_angle();
  double current_angle_multi = pMotor->get_current_angle_multi();
  double current_angle_sw = pMotor->get_current_angle_sw();
  int turn_count = pMotor->get_turn_count();
  printf("%s turn_count %d, setpoint %.2f, angle_sw %.2f, zero_sw %.2f, angle_raw_multi %.2f, angle_raw %.2f, _outputSum %.2f, output %.2f\r\n",
         msg,
         turn_count,
         pMotor->setpoint,
         current_angle_sw,
         pMotor->zero_angle_sw,
         current_angle_multi,
         current_angle,
         pMotor->internalPIDInstance._outputSum,
         (*(pMotor->internalPIDInstance._myOutput)) + 1500.0 - 1.0);
}

/*---------------------UART---------------------*/
const int RX_BUFFER_SIZE = 30;
uint8_t rx_data[5]; // 1 byte
char rx_buffer[RX_BUFFER_SIZE];
uint32_t rx_index = 0;
char command_buffer[20];
double param1, param2, param3;
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
  uint16_t encoderData_1 = 99;
  uint16_t encoderData_2 = 99;
  uint16_t encoderData_3 = 99;
  uint16_t encoder_max = 0;
  uint16_t encoder_min = 4100;
  HAL_TIM_Base_Start(&htim1);

  /*---AMT22 setup---*/
  // resetAMT22(&hspi1, GPIOC, GPIO_PIN_7, &htim1);

  /*---SERVO setup---*/
  int32_t CH2_ESC = 1500 - 1;
  HAL_Delay(500);
  Waist.wrist_waist = 1;
  Waist.begin(aggKp, aggKi, aggKd, regKp, regKi, regKd);
  Waist.setAngleLimits(-359.99, 359.99f); // TODO check good angle limits

  /*---CYTRON setup---*/
  int32_t CH2_DC = 0;
  Wrist_Roll.wrist_waist = 1;
  Wrist_Roll.begin(aggKp, aggKi, aggKd, regKp, regKi, regKd);
  Wrist_Roll.setGearRatio(2.672222f);
  Wrist_Roll.setAngleLimits(-359.99, 359.99f); // TODO check good angle limits



  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 30);
  while (!brakeSet)
  {
    print_MOTOR("BRAKE", &Wrist_Roll);
    // printf("waiting for brake set\r\n");
  }
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);

  /*---UART setup---*/
  HAL_UART_Receive_IT(&huart1, rx_data, 1);

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  // if(huart->Instance == USART2)
  // {
  if (rx_index < RX_BUFFER_SIZE - 1) // check if buffer is not full
  {
    rx_buffer[rx_index++] = (uint8_t)rx_data[0];  // add received byte to buffer
    if (rx_data[0] == '\n' || rx_data[0] == '\r') // check for Enter key
    {
      rx_buffer[rx_index] = '\0'; // add null terminator to make it a string
      rx_index = 0;               // reset buffer index
      // do something with the received data
      sscanf(rx_buffer, "%s %lf %lf %lf", command_buffer, &param1, &param2, &param3);
      // check if commmand_buffer is "pid"
      if (strcmp(command_buffer, "pid") == 0)
      {
        Wrist_Roll.set_PID_params(param1, param2, param3, param1, param2, param3);
        printf("set to Kp: %lf, Ki: %lf, Kd: %lf\r\n", param1, param2, param3);
      }
      else if (strcmp(command_buffer, "sp") == 0)
      {
        Wrist_Roll.newSetpoint(param1);
        Waist.newSetpoint(param1); // TODO check this?
        printf("new Setpoint at %lf\r\n", param1);
      }
      else if (strcmp(command_buffer, "s") == 0)
      {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (int) param1); // set servo output
      }
    }
    else
    {
      printf("invalid command %s\r\n", command_buffer);
    }
  }
  else if (rx_index == RX_BUFFER_SIZE - 1) // buffer is full
  {
    rx_buffer[rx_index] = '\0'; // add null terminator to make it a string
    rx_index = 0;               // reset buffer index
    // do something with the received data
    sscanf(rx_buffer, "%s %lf %lf %lf", command_buffer, &param1, &param2, &param3);
    printf("set to Kp: %lf, Ki: %lf, Kd: %lf\r\n", param1, param2, param3);
  }
  // }
  HAL_UART_Receive_IT(&huart2, rx_data, 1); // start listening for next byte
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
