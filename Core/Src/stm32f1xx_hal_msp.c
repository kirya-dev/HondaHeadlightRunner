#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);

  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

static uint32_t HAL_RCC_CAN1_CLK_ENABLED = 0;

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (hcan->Instance == CAN1)
  {
    HAL_RCC_CAN1_CLK_ENABLED++;
    if (HAL_RCC_CAN1_CLK_ENABLED == 1)
    {
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (hcan->Instance == CAN2)
  {
    __HAL_RCC_CAN2_CLK_ENABLE();
    HAL_RCC_CAN1_CLK_ENABLED++;
    if (HAL_RCC_CAN1_CLK_ENABLED == 1)
      __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_AFIO_REMAP_CAN2_ENABLE();
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance == CAN1)
  {
    HAL_RCC_CAN1_CLK_ENABLED--;
    if (HAL_RCC_CAN1_CLK_ENABLED == 0)
      __HAL_RCC_CAN1_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
  }
  else if (hcan->Instance == CAN2)
  {
    __HAL_RCC_CAN2_CLK_DISABLE();
    HAL_RCC_CAN1_CLK_ENABLED--;
    if (HAL_RCC_CAN1_CLK_ENABLED == 0)
    {
      __HAL_RCC_CAN1_CLK_DISABLE();
    }

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5 | GPIO_PIN_6);
  }
}
