#include <bc/tick.h>
#include <time.h>

clock_t start;

void bc_tick_init(){
	start=clock();
}

bc_tick_t bc_tick_get(void)
{
	return (bc_tick_t)(clock()-start);
}
