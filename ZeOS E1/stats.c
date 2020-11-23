#include <stats.h>
#include <utils.h>

void update_ticks_struct(unsigned long *_ticks, unsigned long *elapsed_total_ticks)
{
	unsigned long ticks = get_ticks();
	*_ticks += ticks - *elapsed_total_ticks;
	*elapsed_total_ticks = ticks;
}