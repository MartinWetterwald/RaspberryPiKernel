#include "kernel_pcb.h"

void kernel_pcb_turnstile_init ( kernel_pcb_turnstile_t * turnstile )
{
	turnstile -> mpFirst = 0;
	turnstile -> mpLast = 0;
}

void kernel_pcb_add_turnstile ( kernel_pcb_t * pcb, kernel_pcb_turnstile_t * turnstile )
{
	pcb -> mpNext = 0;

	if ( ! turnstile -> mpFirst )
	{
		turnstile -> mpFirst = pcb;
		turnstile -> mpLast = pcb;
		return;
	}

	turnstile -> mpLast -> mpNext = pcb;
	turnstile -> mpLast = pcb;
}