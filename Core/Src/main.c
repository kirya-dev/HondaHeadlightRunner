#include "main.h"

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];
uint32_t TxMailbox;
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];

void prepareStaticData(void);

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

	HAL_Delay(100);
	RTT_LOG("Project: ver1.00 by kirchan\r\n");
	MX_GPIO_Init();
	MX_CAN1_Init(CAN_SPEED_125000);
	// MX_CAN2_Init(CAN_SPEED_125000);

	setup();

	RTT_LOG("INFO: Entering loop.");
	for (;;)
		loop();

	return 0;
}

void setup()
{
	// TxHeader.StdId = 0x111;
	TxHeader.ExtId = 0x0AF81111;
	TxHeader.DLC = 6;
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_EXT; // CAN_ID_STD; CAN_ID_EXT;
	TxHeader.TransmitGlobalTime = DISABLE;

	TxData[0] = 0x43;
	TxData[1] = 0x40;
	TxData[2] = 0x00;
	TxData[3] = 0x00;
	TxData[4] = 0x00;
	TxData[5] = 0x00;
	TxData[6] = 0x00;
	TxData[7] = 0x00;
}

void loop()
{
	// Отправляем пакет
	HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);

	// Мигаем диодом
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	HAL_Delay(500);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
	HAL_Delay(500);
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
	hcan1.Init.AutoBusOff = DISABLE;
	hcan1.Init.AutoWakeUp = DISABLE;
	hcan1.Init.AutoRetransmission = DISABLE;
	hcan1.Init.ReceiveFifoLocked = DISABLE;
	hcan1.Init.TransmitFifoPriority = DISABLE;
	Error_Handler(__LINE__, HAL_CAN_Init(&hcan1));

	sFilterConfig.FilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	sFilterConfig.FilterActivation = DISABLE;
	sFilterConfig.SlaveStartFilterBank = 14;

	Error_Handler(__LINE__, HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig));
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
	hcan2.Init.AutoBusOff = DISABLE;
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
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
	__HAL_AFIO_REMAP_SWJ_NOJTAG();

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
	for (;;)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_Delay(200);
	}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
