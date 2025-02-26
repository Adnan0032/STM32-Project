/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "i2c-lcd.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SECRET_CODE_LENGTH 3
#define INPUT_TIMEOUT 5000  // 5 seconds in milliseconds

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void start_signal(void);
void delay_us(uint16_t us);
uint8_t check_response(void);
uint8_t read_byte(void);
void process_sensor_data(void);
void Security_System_Handler(void);
/* USER CODE END PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char msg[50];
uint8_t TOUT = 0, CheckSum, i;
uint8_t T_Byte1, T_Byte2, RH_Byte1, RH_Byte2;

// Security system variables
const uint8_t SECRET_CODE[SECRET_CODE_LENGTH] = {0, 1, 2};  // Red=0, Green=1, White=2, Yellow=3
uint8_t button_history[SECRET_CODE_LENGTH] = {0};
uint8_t input_index = 0;
uint32_t last_press_time = 0;
uint8_t door_open = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define DHT11_PORT GPIOC
#define DHT11_PIN GPIO_PIN_2
#define BUZZER_PORT GPIOB
#define BUZZER_PIN GPIO_PIN_15
#define BUTTON_RED_PIN GPIO_PIN_1
#define BUTTON_GREEN_PIN GPIO_PIN_2
#define BUTTON_BLUE_PIN GPIO_PIN_13

#define LED_PIN2 GPIO_PIN_7
#define BUTTON_PORT GPIOB
uint8_t RHI, RHD, TCI, TCD, SUM;
uint32_t pMillis, cMillis;
float tCelsius = 0;
float tFahrenheit = 0;
float RH = 0;

void microDelay (uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  while (__HAL_TIM_GET_COUNTER(&htim1) < delay);
}

uint8_t DHT11_Start (void)
{
  uint8_t Response = 0;
  GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
  GPIO_InitStructPrivate.Pin = DHT11_PIN;
  GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as output
  HAL_GPIO_WritePin (DHT11_PORT, DHT11_PIN, 0);   // pull the pin low
  HAL_Delay(20);   // wait for 20ms
  HAL_GPIO_WritePin (DHT11_PORT, DHT11_PIN, 1);   // pull the pin high
  microDelay (30);   // wait for 30us
  GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as input
  microDelay (40);
  if (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))
  {
    microDelay (80);
    if ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN))) Response = 1;
  }
  pMillis = HAL_GetTick();
  cMillis = HAL_GetTick();
  while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
  {
    cMillis = HAL_GetTick();
  }
  return Response;
}

uint8_t DHT11_Read (void)
{
  uint8_t a,b;
  for (a=0;a<8;a++)
  {
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go high
      cMillis = HAL_GetTick();
    }
    microDelay (40);   // wait for 40 us
    if (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))   // if the pin is low
      b&= ~(1<<(7-a));
    else
      b|= (1<<(7-a));
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go low
      cMillis = HAL_GetTick();
    }
  }
  return b;
}

/* Gestionnaire du système de sécurité */

void Security_System_Handler(void) {
    // Lire l'état des boutons (actif bas)
    uint8_t rouge = !HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_RED_PIN);
    uint8_t vert = !HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_GREEN_PIN);
    uint8_t bleu = !HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_BLUE_PIN);



    if (input_index < SECRET_CODE_LENGTH) {
        if (rouge) {
            button_history[input_index++] = 0; // Enregistre l'appui sur le bouton rouge
            last_press_time = HAL_GetTick();
            lcd_clear();
            lcd_init();
            lcd_send_string("R");
            HAL_Delay(1000); // Délai pour éviter les appuis multiples
        }
        else if (vert) {
            button_history[input_index++] = 1; // Enregistre l'appui sur le bouton vert
            last_press_time = HAL_GetTick();
            lcd_clear();
            lcd_init();
            lcd_send_string("G");
            HAL_Delay(1000);
        }
        else if (bleu) {
            button_history[input_index++] = 2; // Enregistre l'appui sur le bouton bleu
            last_press_time = HAL_GetTick();
            lcd_clear();
            lcd_init();
            lcd_send_string("B");
            HAL_Delay(1000);
        }


    }

    // Vérifier le code après 6 secondes
    if((HAL_GetTick() - last_press_time > 6000) && (input_index > 0)) {
        if(memcmp(button_history, SECRET_CODE, SECRET_CODE_LENGTH) == 0) {
            door_open = 1;
            char buff[64];
            sprintf(buff, "\033[2J\033[H"); // ANSI escape codes to clear screen and move cursor
            HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);

            sprintf(buff,"Correct! Welcome to your house");
            HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);

            lcd_clear();
            lcd_init();
            lcd_send_string("OPEN");

            HAL_Delay(2000);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
            HAL_Delay(500);
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

        } else {
        	 char buff[64];
        	 sprintf(buff, "\033[2J\033[H"); // ANSI escape codes to clear screen and move cursor
        	 HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);

        	 sprintf(buff,"Warniiiing !! Access to your house !");
        	 HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);

        	lcd_clear();
        	lcd_init();
            lcd_send_string("ERROR!");

            HAL_Delay(2000);

            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
            HAL_Delay(2000);
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

        }
        input_index = 0;
        memset(button_history, 0, sizeof(button_history));
    }
}
void display_values() {
    if (DHT11_Start()) {
        RHI = DHT11_Read(); // Relative humidity integral
        RHD = DHT11_Read(); // Relative humidity decimal
        TCI = DHT11_Read(); // Celsius integral
        TCD = DHT11_Read(); // Celsius decimal
        SUM = DHT11_Read(); // Check sum

        if (RHI + RHD + TCI + TCD == SUM) {

            tCelsius = (float)TCI + (float)(TCD / 10.0);
            RH = (float)RHI + (float)(RHD / 10.0);

            lcd_clear();


            lcd_put_cur(0, 0);
            lcd_send_string("Temp: ");

            int temp_int_part = (int)tCelsius; // Integer part of temperature
            int temp_frac_part = (int)((tCelsius - temp_int_part) * 10); // Fractional part of temperature

            lcd_send_char(temp_int_part / 10 + '0'); // Tens place
            lcd_send_char(temp_int_part % 10 + '0'); // Units place
            lcd_send_char('.');
            lcd_send_char(temp_frac_part + '0');
            lcd_send_char(223);
            lcd_send_string("C");


            static uint8_t previous_state = 0;

            if (tCelsius > 25) {
                lcd_put_cur(1, 0);
                lcd_send_string("HIGH temp");

                if (previous_state != 1) {
                    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
                    char buff[64];
                                sprintf(buff, "\033[2J\033[H"); // ANSI escape codes to clear screen and move cursor
                                HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);

                                sprintf(buff,"OPEN THE WINDOOOW QUICKLY");
                                HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);
                    HAL_Delay(1000); // Buzzer court

                    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
                    previous_state = 1;
                }
            } else if (tCelsius < 15) {
                lcd_put_cur(1, 0);
                lcd_send_string("LOW temp");

                if (previous_state != 2) {
                    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
                    HAL_Delay(1000); // Buzzer court
                    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
                    previous_state = 2;
                }
            }


            lcd_put_cur(1, 0);
            lcd_send_string("RH: ");

            int rh_int_part = (int)RH;
            int rh_frac_part = (int)((RH - rh_int_part) * 10);

            lcd_send_char(rh_int_part / 10 + '0');
            lcd_send_char(rh_int_part % 10 + '0');
            lcd_send_char('.');
            lcd_send_char(rh_frac_part + '0');
            lcd_send_string(" %");
            static uint8_t previous_state2 = 0;
            if (RH > 60) {
                                       lcd_put_cur(0, 0);
                                       lcd_send_string("HIGH humidity");

                                       if (previous_state2 != 1) {
                                    	   char buff[64];
                                           HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
                                           sprintf(buff, "\033[2J\033[H"); // ANSI escape codes to clear screen and move cursor
                                            HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);

                                            sprintf(buff,"Activer l'aspirateur !!! ");
                                            HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);
                                           HAL_Delay(2000);
                                           HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
                                           previous_state2 = 1;
                                       }
                                   } else if (RH < 20) {
                                       lcd_put_cur(0, 0);
                                       lcd_send_string("LOW humidity");

                                       if (previous_state2 != 2) {
                                           HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
                                           HAL_Delay(1000);
                                           HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
                                           previous_state2 = 2;
                                       }
                                   }
        } else {
            // En cas d'erreur de checksum
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Checksum Err!");
        }
    }
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  lcd_init();
  lcd_clear();
  lcd_put_cur(0, 0);
  lcd_send_string("Starting...");
  HAL_Delay(500);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {

	  if(door_open) {
		  display_values();

	      }
	  else{

		  Security_System_Handler();
		  display_values();

	  }
	  display_values();

	  HAL_Delay(400);
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
  RCC_OscInitStruct.PLL.PLLN = 72;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);


  //*Configure GPIO pin : PC2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


      GPIO_InitStruct.Pin = BUTTON_RED_PIN | BUTTON_GREEN_PIN | BUTTON_BLUE_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP; // Boutons connectés à GND
      HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);

      GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_0;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
      // Configuration buzzer
      GPIO_InitStruct.Pin = BUZZER_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

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
