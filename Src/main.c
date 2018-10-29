
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */
#include <stdarg.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

const size_t PRINT_DEBUG_BUFFER_SIZE = 512;
const uint8_t I2C_ADDR_ACC = 0b00111100; //0x1e << 1;
const int16_t ACC_CRASH_THRESHOLD = 30000;
uint8_t bluetooth_command = 0;
int16_t last_acc_x = 0, last_acc_y = 0, last_acc_z = 0, last_acc_abs = 0;
int16_t max_acc_x = 0, max_acc_y = 0, max_acc_z = 0, max_acc_abs = 0;

typedef enum {
	INIT_ACC, WAIT_FOR_CRASH, CRASHING, SEND_GPS
} state_t;

state_t state = INIT_ACC;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_UART4_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

//void write_debug(char *str, uint16_t len);
static void write_debug(char *str, ...);
static uint8_t read_byte(uint8_t addr);
static int16_t read_short(uint8_t addr);
static int16_t absolute(int16_t val);
static HAL_StatusTypeDef read_gps_bytes(uint8_t buffer[], uint8_t len, uint32_t timeout);
static void get_gps_coordinate(int32_t *lat, int32_t *lon);
static void read_acc_data();
static uint8_t read_gps_byte();
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

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
  MX_USART1_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  HAL_UART_Receive_IT(&huart1, &bluetooth_command, 1);

  while (1)
  {

	  /*HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(1000);

	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_Delay(1000);*/

	  /*if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0) {
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

		  if (!written) {
			  write_debug("Ahoj %d!\n", 42);
			  written = 1;
		  }

	  } else {
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		  written = 0;
	  }*/

	  // OUT_X_L_A (28h), OUT_X_H_A (29h)
	  // OUT_Y_L_A (2Ah), OUT_X_H_A (2Bh)
	  // OUT_X_L_A (2Ch), OUT_X_H_A (2Dh)

	  //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

	  uint8_t alive;

	  switch (state) {
	  case INIT_ACC:
		  alive = read_byte(0x0f);
		  if (alive == 0x49) {
			  uint8_t data[2] = {0x20, 0b01000111};
			  if (HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR_ACC, data, 2, 0xffff) == HAL_OK) {
				  //state = WAIT_FOR_CRASH;
				  state = SEND_GPS;
			  } else {
				  write_debug("Error while configuring acc.\r\n");
				  HAL_Delay(50);
			  }
		  } else {
			  write_debug("Acc is not connected yet.\r\n");
			  HAL_Delay(1000);
		  }
		  break;
	  case WAIT_FOR_CRASH:
		  // Read data from accelerometer
		  read_acc_data();
		  if (last_acc_abs > ACC_CRASH_THRESHOLD) {
			  state = CRASHING;
			  max_acc_x = last_acc_x;
			  max_acc_y = last_acc_y;
			  max_acc_z = last_acc_z;
			  max_acc_abs = last_acc_abs;
		  }

		  write_debug("Accel: %6d %6d %6d %6d\n", last_acc_x, last_acc_y, last_acc_z,
				  last_acc_abs);
		  HAL_Delay(25);
		  break;
	  case CRASHING:
		  write_debug("Crashing\n");

		  read_acc_data();
		  if (last_acc_abs < ACC_CRASH_THRESHOLD) {
			  state = SEND_GPS;
		  } else if (last_acc_abs > max_acc_abs) {
			  max_acc_x = last_acc_x;
			  max_acc_y = last_acc_y;
			  max_acc_z = last_acc_z;
			  max_acc_abs = last_acc_abs;
		  }
		  break;
	  case SEND_GPS:
		  write_debug("Sending GPS data\n");

		  int32_t lat = 0, lon = 0;
		  get_gps_coordinate(&lat, &lon);
		  write_debug("Found GPS: lat=%d lon=%d\n", lat, lon);

		  // state = WAIT_FOR_CRASH;
		  HAL_Delay(3000);
		  break;
	  }



	  /**uint8_t addr_arr[1] = {0x2a};
	  HAL_I2C_Master_Transmit(&hi2c1, 0b00111100, addr_arr, 1, 0xffff);
	  uint8_t val[2];
	  HAL_I2C_Master_Receive(&hi2c1, 0b00111100, val, 2, 0xffff);
	  write_debug("Read bytes: %02X %02X\n", val[0], val[1]);**/


	  //int32_t lat, lon;
	  //get_gps_coordinate(&lat, &lon);
	  //write_debug("Lat: %d, lon: %d", lat, lon);

	  //uint8_t data[] = "AT";
	  //HAL_UART_Transmit(&huart1, data, 2, 1000);

	  /*uint8_t buffer[1];
	  while (HAL_UART_Receive(&huart1, buffer, 1, 1000) == HAL_OK) {
		  write_debug("%c", buffer[0]);
	  }*/


	  //write_debug("*");



	  // HAL_UART_Transmit(USART2, "ABC", 3, 5000);
	  //HAL_UART_Transmit(UART5, "ABC", 3, 5000);


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

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{

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
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* UART4 init function */
static void MX_UART4_Init(void)
{

  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

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
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

//void write_debug(char *str, uint16_t len) {
//	HAL_UART_Transmit(&huart2, (uint8_t *) str, len, 0xFFFF);
//}

void write_debug(char *str, ...) {
	char output_str[PRINT_DEBUG_BUFFER_SIZE];
	va_list args;
	va_start(args, str);
	int len = vsnprintf(output_str, PRINT_DEBUG_BUFFER_SIZE, str, args);
	va_end (args);

	if (len >= PRINT_DEBUG_BUFFER_SIZE) {
		len = PRINT_DEBUG_BUFFER_SIZE - 1;
	}

	HAL_UART_Transmit(&huart2, (uint8_t *) output_str, len, 0xffff);
}


uint8_t read_byte(uint8_t addr) {
	uint8_t addr_arr[1] = {addr};
	HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR_ACC, addr_arr, 1, 0xffff);
	uint8_t val[1];
	HAL_I2C_Master_Receive(&hi2c1, I2C_ADDR_ACC, val, 1, 0xffff);
	return val[0];
}

int16_t read_short(uint8_t addr) {
	uint8_t b1 = read_byte(addr + 1);
	uint8_t b0 = read_byte(addr);
	int16_t out = b1 << 8 | b0;
	return out;
}

uint8_t read_gps_byte() {
	uint8_t buffer;
	if (HAL_UART_Receive(&huart4, &buffer, 1, 2000) == HAL_OK) {
		return buffer;
	} else {
		return 0x00;
	}
}

HAL_StatusTypeDef read_gps_bytes(uint8_t buffer[], uint8_t len, uint32_t timeout) {
	return HAL_UART_Receive(&huart4, buffer, len, timeout);
}

typedef enum {
	DOLLAR, G, GP, GPG, GPGL, GPGLL, FIRST_COMMA, LAT_NUM, LAT_DECIMALS,
	N_OR_S, COMMA_BEFORE_LON, LON_NUM, LON_POINT, LON_DECIMALS,
	DONE
} gps_state_t;

// $GPGLL,4913.94625,N,01635.23247,E,102412.00,A,A*6F

void get_gps_coordinate(int32_t *lat, int32_t *lon) {
	uint8_t ch;
	gps_state_t s = DOLLAR;
	uint8_t decimals;
	while (s != DONE) {
		ch = read_gps_byte();
		switch (s) {
		case DOLLAR:
			s = ch == '$' ? G : DOLLAR; break;
		case G:
			s = ch == 'G' ? GP : DOLLAR; break;
		case GP:
			s = ch == 'P' ? GPG : DOLLAR; break;
		case GPG:
			s = ch == 'G' ? GPGL : DOLLAR; break;
		case GPGL:
			s = ch == 'L' ? GPGLL : DOLLAR; break;
		case GPGLL:
			s = ch == 'L' ? FIRST_COMMA : DOLLAR; break;
		case FIRST_COMMA:
			s = ch == ',' ? LAT_NUM : DOLLAR;
			*lat = 0;
			*lon = 0;
			break;
		case LAT_NUM:
			if (ch >= '0' && ch < '9') {
				*lat = *lat * 10 + (ch - '0');
			} else if (ch == ',') {
				write_debug("No GPS signal\n");
				// return;
			} else if (ch == '.') {
				s = LAT_DECIMALS;
				decimals = 0;
			} else {
				s = DOLLAR;
			}
			break;
		case LAT_DECIMALS:
			write_debug("--> LAT: %d\n", *lat);
			if (ch >= '0' && ch < '9') {
				if (decimals < 5) {
					*lat = *lat * 10 + (ch - '0');
					decimals++;
				}
			} else if (ch == ',') {
				while (decimals < 5) {
					*lat *= 10;
					decimals++;
				}
				s = N_OR_S;
			} else {
				s = DOLLAR;
			}
			break;
		default:
			s = DOLLAR;
			write_debug("\n", ch);
			break;
		}
		if (s != DOLLAR) {
			write_debug("%c", ch);
		}
	}
	//write_debug("\n\n---\n\n");
	*lat = 491394222;  // 49° 13.94222' N
	*lon = 163523552;  // 16° 35.23552' E
}

int16_t absolute(int16_t val) {
	if (val >= 0) {
		return val;
	} else if (val == -32768) {
		return 32767;
	} else {
		return -val;
	}
}

void read_acc_data() {
	last_acc_x = read_short(0x28);
	last_acc_y = read_short(0x2a);
	last_acc_z = read_short(0x2c);

	int16_t acc_abs = absolute(last_acc_x);
	int16_t tmp = absolute(last_acc_y);
	if (tmp > acc_abs) {
		acc_abs = tmp;
	}
	// Uncomment to care about the z-axis:
	// tmp = absolute(last_acc_z);
	// if (tmp > acc_abs) {
	//	 acc_abs = tmp;
	// }
	last_acc_abs = acc_abs;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    write_debug("Data received: %c\r\n", bluetooth_command);
    HAL_UART_Transmit(&huart1, (uint8_t*) "Ahojky\r\n", 8, 1000);
    HAL_UART_Receive_IT(&huart1, &bluetooth_command, 1);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
