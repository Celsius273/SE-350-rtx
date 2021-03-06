/**
 * @file:   k_memory.h
 * @brief:  kernel memory managment header file
 * @author: Pushpak Kumar
 * @date:   2014/01/17
 */

#ifndef K_MEM_H_
#define K_MEM_H_

#include "k_rtx.h"
#include "k_process.h"
#include "list.h"

#include "allow_k.h"

typedef struct mem_t{
	// Guarantee 8-byte alignment, the largest common alignment
	U32 m_val[(MEM_BLOCK_SIZE + 3) / 4] __attribute__ ((aligned (8)));
} mem_t;

/* ----- Definitions ----- */
#define RAM_END_ADDR 0x10008000

/* ----- Variables ----- */
/* This symbol is defined in the scatter file (see RVCT Linker User Guide) */
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;

/* ----- Functions ------ */
void memory_init(void);

U32 *alloc_stack(U32 size_b);

void *k_request_memory_block(void);

int k_release_memory_block(void *);

int k_memory_heap_free_blocks(void);

#include "disallow_k.h"
#endif /* ! K_MEM_H_ */
