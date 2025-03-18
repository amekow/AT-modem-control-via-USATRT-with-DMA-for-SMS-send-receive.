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
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ramka.h"
#include "bufork.h"
#include "stdarg.h"
#include "stdio.h"
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

uint8_t znak; // odbieranie znaków
uint8_t znakTx; //Wysłanie znaków
uint8_t recBuff1[4096];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*KOMUNIKACJA*/
void sendf(char* message,...) { //funkcja do wysyłania przez UART łańcuchów znaków
	char tmp[BUF_LEN]; //tymczasowa tablica o wielkości bufora
	bufork_t tmp_txBuf = txBufor2; //zmienna typu strutury tymczasowo wykorzystywana do zapisu bufora kołowego
	va_list arguments;
	va_start(arguments, message);
	vsprintf(tmp, message, arguments); //do bufora tmp przepisuje komunikat uwzględniając argumenty
	va_end(arguments);
	for (uint16_t i =0; i < strlen(tmp); i++) {
		bufork_zapisz(&tmp_txBuf, tmp[i]); //zapisuje w buforze docelowym, tylko jeszcze nie zmienia zmiennej
	}


	__disable_irq();//zabronienie przerwań, by nic nie przeszkodziło w działaniach na buforze
	if ((txBufor2.start==txBufor2.end)&&(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE)==SET)) { //sprawdzenie statusu buforu kołowego - czy UART jest gotowy do wysłania
		txBufor2.start = tmp_txBuf.start; //przypisanie pierwszej komórki do której można przypisywać
		uint8_t ztmp;
		bufork_odczyt(&txBufor2, &ztmp);
		HAL_UART_Transmit_IT(&huart2, &ztmp, 1); //uruchomione jest przerwanie - rozpocznie się dopiero na enable
	} else {
		txBufor2.start = tmp_txBuf.start;  // Uart nie skończył jeszcze poprzedniej wysyłki, więc poinformuj go, że są następne dane
	}

	__enable_irq();
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, &znak, 1);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, &recBuff1, 4096);

  // sendf("%s\n", "WITAJ, podrozniku");

  initSIM800();
  uint16_t il_zn_rx2 = 0;
  uint8_t ramka[200];
  ramka[0] = 0;
  sendf("I am ready!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	//moduł nie przyjmnie nowego rozkazu puki nie obrobi się z tym co otrzymał - jeśli coś w tym czasie zostanie wysłane to nie zostanie odebrane przez moduł
	if(rxBufor2.start!=rxBufor2.end && dma_transfer_complete == 1){
		  if (dopisz_znak_ramki(&ramka)==0) {
			  uint8_t komenda[200];
			  wybierz(&ramka[6],&komenda ,strlen(ramka)-6);
			  // wysłanie komendy do wykonania
			  obsluga_komend(&komenda);
//			  sendf("%s\r\n", ramka);
			  // Po wykonaniu komendy zerowania tablicy ramka
			  ramka[0] = 0;

		  }
	  }

	  if (rxBufor1.start != rxBufor1.end) { //wyświetla to co przyszło z modułu i przekazuje na terminal
		  uint8_t bufor[1024];
		  odczytSIM(&bufor);
//		    sendf("%s\n", &bufor);
		  if (strncmp(&bufor, "+CMGR:", 6) == 0 || strncmp(&bufor, "+CMGL:", 6) == 0) {
			  // Odczytano zadany nr SMS-a
			  char * w = strtok(&bufor, ",");
			  sendf("%s, ", w+7);
			  if (strncmp(&bufor, "+CMGL:", 6) == 0) {
				  w = strtok(NULL, ",");
				  sendf("%s, ", w);
			  }
			  // nr telefonu nadawcy:
			  w = strtok(NULL, ",");
			  sendf("SMS from nr %s\n", w);
			  strtok(NULL, ","); // tu nazwa nadawcy, ale nie ma książki telefonicznej, więc nie ma i nazwy
			  w = strtok(NULL, "\r\n"); // data i czas
			  sendf("received: %s", w);
			  w = strtok(NULL, "");
			  sendf("%s\r\n---------------------\r\n", w);
		  } else if (bufor[0] == '>') {
			  if (strlen(tresc) > 0) {
				  uint8_t tmp[200];
				  sprintf(&tmp, "%s\x1A", tresc);
//				  dma_transfer_complete = 1;
				  sendSIM(&tmp);
				  tresc[0] = 0;
			  }
		  } else if (strncmp(&bufor, "+CMGS:", 6)==0) {
			  if (strstr(&bufor, "OK") != NULL) {
				  sendf("SMS sent successfully");
			  } else {
				  sendf("Error sent SMS");
			  }

		  }
		  dma_transfer_complete = 1;

		  /*else {
		    sendf("%s\n", &recBuff1);
		  }*/
	  }

	  if (txBufor1.start != txBufor1.end && dma_transfer_complete == 1) {
		  // wysłanie do SIM
		  uint8_t pom[200],i=0;
		  while (bufork_odczyt(&txBufor1, &pom[i++])==0);
		  pom[--i] = 0;
		  dma_transfer_complete = 0; //rozpoczęcie transmisji
		  HAL_UART_Transmit_DMA(&huart1,&pom,strlen(pom));
	  }


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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//obsłua przerwań

void HAL_UART_RxCpltCallback(UART_HandleTypeDef*huart){ //odbieranie
	if(huart->Instance== USART2){
		//dodawanie znaku do bufora odbiorczego
		bufork_zapisz(&rxBufor2, znak);
		HAL_UART_Receive_IT(&huart2, &znak, 1);
	}

}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef*huart){ //nadawanie
	if(huart->Instance== USART2){
//		huart2_transfer_complete = 1;
	// nie ma potrzeby sprawdzania gotowości UART do wysłania znaku, bo przerwaanie wywołuje się dopiero po wysłaniu
		if (bufork_odczyt(&txBufor2, &znakTx) == 0) {
				HAL_UART_Transmit_IT(&huart2, &znakTx, 1);
		}
	}
	//if (huart->Instance == USART1) { //czy dotyczy UART1
//	        dma_transfer_complete = 1; //czy zakończone
	 //}
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART1)
    {
     	for (uint8_t i = 0; i < Size; i++){
    		if (bufork_zapisz(&rxBufor1, recBuff1[i]) == -1) {
    			sendf('Buffer overflow');
    			break;
    		}
    		recBuff1[i] = 0;
    	}
     	dma_transfer_complete == 0; //żeby mógł odebrać kilka SMS, bo każdy odczytany SMS powoduje wywołanie tego przerwania
     	// Start to listening again - IMPORTANT!
     	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, recBuff1, 4096);
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
