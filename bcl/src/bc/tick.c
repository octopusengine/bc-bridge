#include <bc/tick.h>
#include <time.h>

struct timeval start;
struct timeval now;

void bc_tick_init(){
	gettimeofday(&start,NULL);
}

bc_tick_t bc_tick_get(void)
{
	gettimeofday(&now,NULL);
	return (bc_tick_t)( (1000000 * (now.tv_sec - start.tv_sec) )+ ( now.tv_usec - start.tv_usec ) );
}
