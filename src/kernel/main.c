#include "memory.h"
#include "hardware.h"
#include "timer.h"
#include "scheduler.h"

void __attribute__ ( ( noreturn, naked ) ) kernel_main ( )
{
	kernel_memory_init ( );
	kernel_hardware_init ( );
	kernel_timer_init ( );
	kernel_scheduler_init ( );

	kernel_scheduler_set_next_deadline ( );

	kernel_scheduler_yield_noreturn ( );
}
