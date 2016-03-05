/** 
 * @file:   k_rtx_init.h
 * @brief:  Kernel initialization header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_INIT_H_
#define K_RTX_INIT_H_

#include "k_rtx.h"

#include "allow_k.h"

/* Functions */

void k_rtx_init(void);

extern int k_release_processor(void);
extern void *k_request_memory_block(void);
extern int k_release_memory_block(void *);

#include "disallow_k.h"

#endif /* ! K_RTX_INIT_H_ */
