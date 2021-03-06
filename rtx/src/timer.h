/**
 * @brief timer.h - Timer header file
 * @author Y. Huang
 * @date 2013/02/12
 */
 
#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>

extern uint32_t timer_init ( uint8_t n_timer );  /* initialize timer n_timer */
extern volatile uint32_t g_timer_count;

void proc_timer_i(void);

#endif /* ! _TIMER_H_ */
