#include "main.h"

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

void setup();
void loop();
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(int);
static void MX_CAN2_Init(int);

int main(void)
{
	HAL_Init();

	SystemClock_Config();
	SEGGER_RTT_Init();

	RTT_LOG("Project: ver1.00 by kirchan\r\n");

	MX_GPIO_Init();
	MX_CAN1_Init(CAN_SPEED_125000);
	MX_CAN2_Init(CAN_SPEED_125000);

	setup();

	RTT_LOG("INFO: Entering loop.");
	for (;;)
		loop();

	return 0;
}

uint16_t convert_KA_to_KH_headlight(uint16_t ka)
{
	uint16_t kh = ka;
	// todo - convert logic
	return kh;
}

void set_headlight(uint16_t value)
{
	CAN_TxHeaderTypeDef TxHeader;
	uint8_t TxData[8] = {0};
	uint32_t TxMailbox;

	TxHeader.ExtId = 0x0AF81111;
	TxHeader.DLC = 6;
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_EXT;
	TxHeader.TransmitGlobalTime = DISABLE;

	TxData[0] = value >> 8;
	TxData[1] = value;

	HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox);
}

void TJA_standby(int stb)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, stb ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hardware_sleep()
{
	HAL_CAN_RequestSleep(&hcan1);
	HAL_CAN_RequestSleep(&hcan2);

	// Ждем, пока CAN1 и CAN2 подтвердят переход в Sleep (не обязательно, но надежно)
	while (HAL_CAN_IsSleepActive(&hcan1) == 0) {}
	while (HAL_CAN_IsSleepActive(&hcan2) == 0) {}

	TJA_standby(1);
	HAL_Delay(5); // Даем трансиверу успокоиться после пакетов

	// ВАЖНО: Деинициализируем CAN1, чтобы освободить пин PA11 для EXTI.
	HAL_CAN_MspDeInit(&hcan1);

	// Перенастраиваем PA11 как обычный EXTI вход по спаду
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Очищаем старые флаги и включаем прерывание EXTI11 (оно находится в векторе 15_10)
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
	HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	// Засыпаем.
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	// ===================================================================
	// ТОЧКА ПРОСЫПАНИЯ (Сюда мы возвращаемся ПОСЛЕ прерывания EXTI)
	// ===================================================================
	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
	SystemClock_Config();
	HAL_ResumeTick();
	HAL_CAN_MspInit(&hcan1);
	TJA_standby(0);
	HAL_Delay(1);
	HAL_CAN_WakeUp(&hcan1);
	HAL_CAN_WakeUp(&hcan2);
}

volatile int count_of_blinks = 1;
volatile uint32_t last_can1_activity = 0; // Время последнего пакета от машины

void setup()
{
	set_headlight(0x0000);
	TJA_standby(0);
	last_can1_activity = HAL_GetTick();
}

void loop()
{
	// Елси прошло 8сек, то всё выключаем и засыпаем.
	if (HAL_GetTick() - last_can1_activity > 8 * 1000)
	{
		set_headlight(0x0000);								   // Выключаем фары!
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // Гасим диод

		hardware_sleep();

		last_can1_activity = HAL_GetTick();
	}

	for (int j = 0; j < count_of_blinks; ++j)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
		HAL_Delay(250);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // Гасим после медленной вспышки
		HAL_Delay(150);
	}
	HAL_Delay(1000);
}

// ПРЕРЫВАНИЕ: Выход из сна
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_11)
	{
		// Здесь проснулись по прерыванию на CAN_RX, но 1 пакет потеряли(такова архитетукра CAN при пробуждении).
		// Сбрасываем таймаут сна, чтобы loop() не отправил МК обратно в STOP на следующей итерации
		last_can1_activity = HAL_GetTick();
		count_of_blinks = 2;
	}
}

// ПРЕРЫВАНИЕ: Когда в CAN1 прилетает новое сообщение
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if (hcan->Instance == CAN1)
	{
		CAN_RxHeaderTypeDef tmpRxHeader;
		uint8_t tmpRxData[8] = {0};

		if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &tmpRxHeader, tmpRxData) == HAL_OK)
		{
			// Cбрасываем таймаут сна
			last_can1_activity = HAL_GetTick();
			count_of_blinks = 3;

			set_headlight(convert_KA_to_KH_headlight((tmpRxData[0] << 8) | tmpRxData[1]));
		}
	}
}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV5;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_PLL2;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL2_ON;
	RCC_OscInitStruct.PLL2.PLL2MUL = RCC_PLL2_MUL8;
	RCC_OscInitStruct.PLL2.HSEPrediv2Value = RCC_HSE_PREDIV2_DIV5;
	Error_Handler(__LINE__, HAL_RCC_OscConfig(&RCC_OscInitStruct));

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	Error_Handler(__LINE__, HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2));
	__HAL_RCC_PLLI2S_ENABLE();
}

static void MX_CAN1_Init(int speedPrescaler)
{
	CAN_FilterTypeDef sFilterConfig;
	hcan1.Instance = CAN1;
	hcan1.Init.Prescaler = speedPrescaler;
	hcan1.Init.Mode = CAN_MODE_NORMAL; // CAN_MODE_NORMAL; CAN_MODE_LOOPBACK;
	hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan1.Init.TimeSeg1 = CAN_BS1_8TQ;
	hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
	hcan1.Init.TimeTriggeredMode = DISABLE;
	hcan1.Init.AutoBusOff = ENABLE;
	hcan1.Init.AutoWakeUp = ENABLE;
	hcan1.Init.AutoRetransmission = DISABLE;
	hcan1.Init.ReceiveFifoLocked = DISABLE;
	hcan1.Init.TransmitFifoPriority = DISABLE;
	Error_Handler(__LINE__, HAL_CAN_Init(&hcan1));

	uint32_t target_id = 0x0AF81111;
	uint16_t id_high = (target_id << 3) >> 16;
	uint16_t id_low = (target_id << 3) | CAN_ID_EXT;

	sFilterConfig.FilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = id_high;
	sFilterConfig.FilterIdLow = id_low;
	sFilterConfig.FilterMaskIdHigh = id_high;
	sFilterConfig.FilterMaskIdLow = id_low;
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.SlaveStartFilterBank = 14;

	Error_Handler(__LINE__, HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig));
	Error_Handler(__LINE__, HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING));
	Error_Handler(__LINE__, HAL_CAN_Start(&hcan1));
}

static void MX_CAN2_Init(int speedPrescaler)
{
	CAN_FilterTypeDef sFilterConfig;

	hcan2.Instance = CAN2;
	hcan2.Init.Prescaler = speedPrescaler;
	hcan2.Init.Mode = CAN_MODE_NORMAL;
	hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan2.Init.TimeSeg1 = CAN_BS1_8TQ;
	hcan2.Init.TimeSeg2 = CAN_BS2_3TQ;
	hcan2.Init.TimeTriggeredMode = DISABLE;
	hcan2.Init.AutoBusOff = ENABLE;
	hcan2.Init.AutoWakeUp = DISABLE;
	hcan2.Init.AutoRetransmission = DISABLE;
	hcan2.Init.ReceiveFifoLocked = DISABLE;
	hcan2.Init.TransmitFifoPriority = DISABLE;
	Error_Handler(__LINE__, HAL_CAN_Init(&hcan2));

	sFilterConfig.FilterBank = 15;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO1;
	sFilterConfig.FilterActivation = DISABLE;
	sFilterConfig.SlaveStartFilterBank = 15;

	Error_Handler(__LINE__, HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig));
	Error_Handler(__LINE__, HAL_CAN_Start(&hcan2));
}

void init_GPIO_for_TJA_standby()
{
	TJA_standby(1);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void init_GPIO_for_TURN_SIGNAL()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_GPIO_Init(void)
{
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
	__HAL_AFIO_REMAP_SWJ_NOJTAG();

	init_GPIO_for_TJA_standby();
	init_GPIO_for_TURN_SIGNAL();
}

void Error_Handler(int line, int err)
{
	if (err == HAL_OK)
	{
		RTT_LOG("INFO: HAL_OK on line %d.", line);
		return;
	}
	RTT_LOG("ERROR: CRITIAL %d was on line %d.", err, line);
	for (int i=0; i < 5; ++i)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_Delay(100);
	}
	__HAL_RCC_CAN1_FORCE_RESET();
	HAL_Delay(5);
	__HAL_RCC_CAN1_RELEASE_RESET();
	NVIC_SystemReset();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
