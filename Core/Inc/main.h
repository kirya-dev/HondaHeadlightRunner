#ifndef __MAIN_H
#define __MAIN_H

#define CAN_SPEED_500000 6
#define CAN_SPEED_125000 6 * 4

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"

void Error_Handler(int, int);

#include "segger/SEGGER_RTT.h"
#define RTT_LOG(msg, ...)  { SEGGER_RTT_printf(0, msg, ##__VA_ARGS__); SEGGER_RTT_TerminalOut(0, ""); }

#ifdef __cplusplus
}
#endif

#endif
