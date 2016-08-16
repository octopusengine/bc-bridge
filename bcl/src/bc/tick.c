#include <bc/tick.h>
#include <stm32l0xx_hal.h>

bc_tick_t bc_tick_get(void)
{
	return (bc_tick_t) HAL_GetTick();
}
