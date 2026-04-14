	/* USER CODE BEGIN Header */
	/**
	  ******************************************************************************
	  * @file           : main.c
	  * @brief          : Tích hợp Ghi/Đọc EEPROM qua UART và Hiển thị cuộn trên LCD ST7789
	  ******************************************************************************
	  */
	/* USER CODE END Header */
	/* Includes ------------------------------------------------------------------*/
	#include "main.h"

	/* Private includes ----------------------------------------------------------*/
	/* USER CODE BEGIN Includes */
	#include "fonts.h"
	#include <string.h>
	#include <stdio.h>
	/* USER CODE END Includes */

	/* Private typedef -----------------------------------------------------------*/
	/* USER CODE BEGIN PTD */

	/* USER CODE END PTD */

	/* Private define ------------------------------------------------------------*/
	/* USER CODE BEGIN PD */
	#define EEPROM_ADDR 0xA0

	// Chân cấp nguồn EEPROM
	#define EEPROM_PWR_Pin GPIO_PIN_1
	#define EEPROM_PWR_GPIO_Port GPIOA

	// Chân LCD ST7789 (SPI2)
	#define TFT_CS_PORT  GPIOA
	#define TFT_CS_PIN   GPIO_PIN_2
	#define TFT_DC_PORT  GPIOA
	#define TFT_DC_PIN   GPIO_PIN_3
	#define TFT_RES_PORT GPIOA
	#define TFT_RES_PIN  GPIO_PIN_4
	/* USER CODE END PD */

	/* Private macro -------------------------------------------------------------*/
	/* USER CODE BEGIN PM */

	/* USER CODE END PM */

	/* Private variables ---------------------------------------------------------*/
	I2C_HandleTypeDef hi2c1;
	SPI_HandleTypeDef hspi2;
	UART_HandleTypeDef huart1;
	DMA_HandleTypeDef hdma_usart1_rx;
	DMA_HandleTypeDef hdma_usart1_tx;

	/* USER CODE BEGIN PV */
	uint8_t rx_buffer[3];
	uint8_t tx_buffer[26];

	uint8_t double_alphabet[52];
	const uint8_t my_alphabet[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	uint8_t ack_msg[2] = {'O', 'K'};
	uint8_t newline[2] = {'\r', '\n'};

	volatile uint8_t cmd_flag = 0;

	// Biến quản lý cuộn LCD
	uint16_t scroll_y = 0;
	uint16_t physical_y = 0;
	uint32_t line_count = 0;
	/* USER CODE END PV */

	/* Private function prototypes -----------------------------------------------*/
	void SystemClock_Config(void);
	static void MX_GPIO_Init(void);
	static void MX_DMA_Init(void);
	static void MX_I2C1_Init(void);
	static void MX_SPI2_Init(void);
	static void MX_USART1_UART_Init(void);

	/* USER CODE BEGIN PFP */
	void ST7789_WriteCommand(uint8_t cmd);
	void ST7789_WriteData(uint8_t data);
	void ST7789_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	void ST7789_FillScreen(uint16_t color);
	void ST7789_DrawChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
	void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);
	void ST7789_HardwareScroll(uint16_t y_offset);

	void EEPROM_Write_Buffer(uint16_t MemAddress, uint8_t *pData, uint16_t len);
	void EEPROM_Read_Buffer(uint16_t MemAddress, uint8_t *pData, uint16_t len);
	/* USER CODE END PFP */

	/* Private user code ---------------------------------------------------------*/
	/* USER CODE BEGIN 0 */

	// =================== HÀM GIAO TIẾP LCD ST7789 ===================
	void ST7789_WriteCommand(uint8_t cmd) {
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_RESET);
		HAL_SPI_Transmit(&hspi2, &cmd, 1, 10);
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
	}

	void ST7789_WriteData(uint8_t data) {
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
		HAL_SPI_Transmit(&hspi2, &data, 1, 10);
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
	}

	void ST7789_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
		ST7789_WriteCommand(0x2A);
		ST7789_WriteData(x0 >> 8); ST7789_WriteData(x0 & 0xFF);
		ST7789_WriteData(x1 >> 8); ST7789_WriteData(x1 & 0xFF);

		ST7789_WriteCommand(0x2B);
		ST7789_WriteData(y0 >> 8); ST7789_WriteData(y0 & 0xFF);
		ST7789_WriteData(y1 >> 8); ST7789_WriteData(y1 & 0xFF);
		ST7789_WriteCommand(0x2C);
	}

	void ST7789_FillScreen(uint16_t color) {
		uint8_t line_buffer[480];
		uint8_t high_byte = color >> 8;
		uint8_t low_byte = color & 0xFF;
		for (int i = 0; i < 480; i += 2) {
			line_buffer[i] = high_byte;
			line_buffer[i+1] = low_byte;
		}
		ST7789_SetWindow(0, 0, 239, 239);
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
		for (int y = 0; y < 240; y++) {
			HAL_SPI_Transmit(&hspi2, line_buffer, 480, 10);
		}
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
	}

	void ST7789_DrawChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
		uint32_t char_size = font.width * font.height;
		uint8_t char_buffer[char_size * 2];
		uint16_t buffer_index = 0;
		uint8_t color_high = color >> 8, color_low = color & 0xFF;
		uint8_t bg_high = bgcolor >> 8, bg_low = bgcolor & 0xFF;

		for(uint32_t i = 0; i < font.height; i++) {
			uint32_t b = font.data[(ch - 32) * font.height + i];
			for(uint32_t j = 0; j < font.width; j++) {
				if((b << j) & 0x8000) {
					char_buffer[buffer_index++] = color_high; char_buffer[buffer_index++] = color_low;
				} else {
					char_buffer[buffer_index++] = bg_high; char_buffer[buffer_index++] = bg_low;
				}
			}
		}
		ST7789_SetWindow(x, y, x + font.width - 1, y + font.height - 1);
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
		HAL_SPI_Transmit(&hspi2, char_buffer, char_size * 2, 10);
		HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
	}

	void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor) {
		while (*str) {
			if (x + font.width >= 240) {
				x = 0;
				y += font.height;
				if (y + font.height >= 240) break;
			}
			ST7789_DrawChar(x, y, *str, font, color, bgcolor);
			x += font.width;
			str++;
		}
	}

	void ST7789_HardwareScroll(uint16_t y_offset) {
		ST7789_WriteCommand(0x37);
		ST7789_WriteData(y_offset >> 8);
		ST7789_WriteData(y_offset & 0xFF);
	}

	// =================== HÀM GIAO TIẾP EEPROM ===================
	void EEPROM_Write_Buffer(uint16_t MemAddress, uint8_t *pData, uint16_t len) {
		if (HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, MemAddress, I2C_MEMADD_SIZE_16BIT, pData, len, 100) != HAL_OK) return;
		while (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDR, 5, 10) != HAL_OK) {}
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	}

	void EEPROM_Read_Buffer(uint16_t MemAddress, uint8_t *pData, uint16_t len) {
		for (uint16_t i = 0; i < len; i++) {
			if (HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, MemAddress + i, I2C_MEMADD_SIZE_16BIT, &pData[i], 1, 100) != HAL_OK) {
				pData[i] = '?';
			}
		}
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
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
	  HAL_Init();

	  /* Configure the system clock */
	  SystemClock_Config();

	  /* Initialize all configured peripherals */
	  MX_GPIO_Init();
	  MX_DMA_Init();
	  MX_I2C1_Init();
	  MX_SPI2_Init();
	  MX_USART1_UART_Init();

	  /* USER CODE BEGIN 2 */

	  // --- 1. KHỞI TẠO MÀN HÌNH ST7789 ---
	  HAL_GPIO_WritePin(TFT_RES_PORT, TFT_RES_PIN, GPIO_PIN_RESET);
	  HAL_Delay(100);
	  HAL_GPIO_WritePin(TFT_RES_PORT, TFT_RES_PIN, GPIO_PIN_SET);
	  HAL_Delay(100);

	  ST7789_WriteCommand(0x11); HAL_Delay(120);
	  ST7789_WriteCommand(0x3A); ST7789_WriteData(0x55);
	  ST7789_WriteCommand(0x36); ST7789_WriteData(0x00);
	  ST7789_WriteCommand(0x21);
	  ST7789_WriteCommand(0x29); HAL_Delay(20);

	  // Cấu hình vùng cuộn
	  ST7789_WriteCommand(0x33);
	  ST7789_WriteData(0x00); ST7789_WriteData(0x00);
	  ST7789_WriteData(0x00); ST7789_WriteData(0xF0);
	  ST7789_WriteData(0x00); ST7789_WriteData(0x00); // Ở đây lề dưới = 0 cho cuộn toàn màn

	  ST7789_FillScreen(0x0000); // Xóa màn hình đen

	  // --- 2. KHỞI TẠO EEPROM & UART ---
	  memcpy(double_alphabet, my_alphabet, 26);
	  memcpy(double_alphabet + 26, my_alphabet, 26);

	  // Chờ lệnh 3 byte từ PC
	  HAL_UART_Receive_DMA(&huart1, rx_buffer, 3);

	  // Mặc định ngắt nguồn EEPROM
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	  HAL_GPIO_WritePin(EEPROM_PWR_GPIO_Port, EEPROM_PWR_Pin, GPIO_PIN_SET);
	  HAL_Delay(50);

	  char msg[50];
		sprintf(msg, "\r\nDang quet I2C...\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

		for(uint8_t i = 1; i < 128; i++) {
			// Dịch trái 1 bit vì STM32 yêu cầu địa chỉ 8-bit (có bit R/W)
			if(HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 50) == HAL_OK) {
				sprintf(msg, "-> TIM THAY THIET BI TAI DIA CHI: 0x%X \r\n", (i << 1));
				HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
			}
		}
		sprintf(msg, "Hoan thanh quet.\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
	  /* USER CODE END 2 */

		/* Infinite loop */
		  /* USER CODE BEGIN WHILE */
		  while (1)
		  {
		    /* USER CODE END WHILE */

		    /* USER CODE BEGIN 3 */

		    // --------------------------------------------------------
		    // TRƯỜNG HỢP 1: GHI 100 BẢNG CHỮ CÁI (0xBB)
		    // --------------------------------------------------------
		    if (cmd_flag == 0xBB) {
		        cmd_flag = 0;

		        HAL_GPIO_WritePin(EEPROM_PWR_GPIO_Port, EEPROM_PWR_Pin, GPIO_PIN_SET);
		        HAL_Delay(20);

		        HAL_I2C_DeInit(&hi2c1);
		        HAL_I2C_Init(&hi2c1);

		        for (uint16_t i = 0; i < 50; i++) {
		            EEPROM_Write_Buffer(i * 64, (uint8_t*)double_alphabet, 26);
		        }

		        HAL_UART_Transmit(&huart1, ack_msg, 2, 100);
		        HAL_UART_Transmit(&huart1, newline, 2, 100);

		        HAL_GPIO_WritePin(EEPROM_PWR_GPIO_Port, EEPROM_PWR_Pin, GPIO_PIN_RESET);
		    }
		    		// TRƯỜNG HỢP 2: ĐỌC 100 BẢNG CHỮ CÁI VÀ HIỂN THỊ (0xCC)

		    		else if (cmd_flag == 0xCC) {
		    			cmd_flag = 0;

		    			HAL_GPIO_WritePin(EEPROM_PWR_GPIO_Port, EEPROM_PWR_Pin, GPIO_PIN_SET);
		    			HAL_Delay(20);

		    			HAL_I2C_DeInit(&hi2c1);
		    			HAL_I2C_Init(&hi2c1);

		    			char display_str[30];

		    			for (uint16_t i = 0; i < 100; i++) {

		    				EEPROM_Read_Buffer(i * 32, tx_buffer, 26);


		    				memcpy(display_str, tx_buffer, 26);
		    				display_str[26] = '\0';

		    				if (line_count < 23) {
		    					physical_y = line_count * 10;
		    					line_count++;
		    				} else {
		    					scroll_y = (scroll_y + 10) % 240;
		    					ST7789_HardwareScroll(scroll_y);

		    					physical_y = (scroll_y + 240 - 10) % 240;

		    					ST7789_DrawString(0, physical_y, "                          ", Font_7x10, 0x0000, 0x0000);
		    				}


		    				uint16_t text_color = (i % 2 == 0) ? 0xF800 : 0x07E0;

		    				ST7789_DrawString(0, physical_y, display_str, Font_7x10, text_color, 0x0000);


		    				HAL_UART_Transmit(&huart1, tx_buffer, 26, 500);
		    				HAL_UART_Transmit(&huart1, newline, 2, 100);


		    				HAL_Delay(15);
		    			}

		    			HAL_GPIO_WritePin(EEPROM_PWR_GPIO_Port, EEPROM_PWR_Pin, GPIO_PIN_RESET);
		    		}
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

	  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	  {
		Error_Handler();
	  }

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
	  */
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
		Error_Handler();
	  }
	}

	/**
	  * @brief SPI2 Initialization Function
	  */
	static void MX_SPI2_Init(void)
	{
	  hspi2.Instance = SPI2;
	  hspi2.Init.Mode = SPI_MODE_MASTER;
	  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
	  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
	  hspi2.Init.NSS = SPI_NSS_SOFT;
	  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	  hspi2.Init.CRCPolynomial = 10;
	  if (HAL_SPI_Init(&hspi2) != HAL_OK)
	  {
		Error_Handler();
	  }
	}

	/**
	  * @brief USART1 Initialization Function
	  */
	static void MX_USART1_UART_Init(void)
	{
	  huart1.Instance = USART1;
	  huart1.Init.BaudRate = 115200;
	  huart1.Init.WordLength = UART_WORDLENGTH_8B;
	  huart1.Init.StopBits = UART_STOPBITS_1;
	  huart1.Init.Parity = UART_PARITY_NONE;
	  huart1.Init.Mode = UART_MODE_TX_RX;
	  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	  if (HAL_UART_Init(&huart1) != HAL_OK)
	  {
		Error_Handler();
	  }
	}

	/**
	  * Enable DMA controller clock
	  */
	static void MX_DMA_Init(void)
	{
	  __HAL_RCC_DMA1_CLK_ENABLE();
	  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	}

	/**
	  * @brief GPIO Initialization Function
	  */
	static void MX_GPIO_Init(void)
	{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  __HAL_RCC_GPIOC_CLK_ENABLE();
	  __HAL_RCC_GPIOD_CLK_ENABLE();
	  __HAL_RCC_GPIOA_CLK_ENABLE();
	  __HAL_RCC_GPIOB_CLK_ENABLE();

	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIOA, EEPROM_PWR_Pin|TFT_CS_PIN|TFT_DC_PIN|TFT_RES_PIN, GPIO_PIN_RESET);

	  GPIO_InitStruct.Pin = GPIO_PIN_13;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = EEPROM_PWR_Pin|TFT_CS_PIN|TFT_DC_PIN|TFT_RES_PIN;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}

	/* USER CODE BEGIN 4 */
	// Hàm Callback nhận lệnh DMA UART
	void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
		if(huart->Instance == USART1) {
			if(rx_buffer[0] == 0xAA && rx_buffer[2] == 0x55) {
				cmd_flag = rx_buffer[1];
			}
			HAL_UART_Receive_DMA(&huart1, rx_buffer, 3);
		}
	}
	/* USER CODE END 4 */

	/**
	  * @brief  This function is executed in case of error occurrence.
	  * @retval None
	  */

	void Error_Handler(void)
	{
	  __disable_irq();
	  while (1)
	  {
	  }
	}

	#ifdef USE_FULL_ASSERT
	void assert_failed(uint8_t *file, uint32_t line)
	{
	}
	#endif /* USE_FULL_ASSERT */
