#include "bc_tick.h"
#include <sys/time.h>

struct timeval start;
struct timeval now;

bc_tick_t bc_tick_get(void)
{
	
	if(start.tv_sec==0)
	{
		gettimeofday(&start,NULL);
	}
	
	gettimeofday(&now,NULL);

	return (bc_tick_t)( (1000 * (now.tv_sec - start.tv_sec) ) + (( now.tv_usec - start.tv_usec )/1000) );
}
