/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Pushpak Kumar
 * @date:   2016/01/23
 */

#include "k_memory.h"
 #include "k_process.h"
#include "priority_queue.h"
#include <assert.h>

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

#include "allow_k.h"

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */

mem_t mem_blocks[NUM_MEM_BLOCKS];
LL_DECLARE(g_heap, mem_t*, NUM_MEM_BLOCKS);
U8 *gp_heap_begin_addr;
U8 *gp_heap_end_addr;

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        Padding            |
          |---------------------------|
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;

	/* 4 bytes padding */
	p_end += 4;

	/* prepare for alloc_stack() to allocate memory for stacks */
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack;
	}

	/* allocate memory for heap*/
	gp_heap_begin_addr = p_end;

  	for( i = 0; i < NUM_MEM_BLOCKS; i++) {
	    LL_PUSH_FRONT_(g_heap) = &mem_blocks[i];
	    p_end += MEM_BLOCK_SIZE;
	}

	gp_heap_end_addr = p_end;
}

static char __attribute__((aligned(8))) stack_space[13 * 0x200 + 8];
static int stack_space_begin = 0;

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b)
{
	stack_space_begin += size_b;
	stack_space_begin = (stack_space_begin + 7) / 8 * 8;
	assert(stack_space_begin + 8 <= sizeof(stack_space));
	return (U32 *)(stack_space + stack_space_begin);
}

void *k_request_memory_block(void)
{
	U8 *p_mem_blk = NULL;

	while(LL_SIZE(g_heap) == 0) {
        // release proc, put cur proc. in ready queue
        // not right because: call k
        // never set current state to blocked
        // if all we do is release processor
        k_poll(BLOCKED_ON_RESOURCE);
	}
	//increment the address the address of the node by the header size to get the start address of the block itslef 
	p_mem_blk = (U8 *)LL_POP_FRONT(g_heap);
	return (void *)p_mem_blk;	//this is pointing the content not the header
}

int k_release_memory_block_valid(void *p_mem_blk)
{
  //when the block is returned adjust the pointer so it points to start of envelope i.e. from header
	mem_t *const p_mem = (mem_t *)p_mem_blk;
  if(p_mem == NULL){
    return RTX_ERR;
  }

	const unsigned long offset = (unsigned long)p_mem - (unsigned long)mem_blocks;
  if(offset >= sizeof(mem_blocks)) {
    return RTX_ERR;
  }
  if(offset % MEM_BLOCK_SIZE != 0){
    return RTX_ERR;
  }
	// Check if the memory block is already free i.e. already in the list
	mem_t *blk;
	LL_FOREACH(blk, g_heap) {
		if (blk == p_mem) {
			return RTX_ERR;
		}
	}
  return RTX_OK;
}

int k_release_memory_block(void *p_mem_blk)
{
	//if memory block pointer being released is valid
	if(k_release_memory_block_valid(p_mem_blk) == RTX_OK){
    mem_t *p_mem = (mem_t *)p_mem_blk;
    LL_PUSH_BACK(g_heap, p_mem);

	k_check_preemption();
	}
	else{
		return RTX_ERR;
	}
	return RTX_OK;
}

int k_memory_heap_free_blocks(void) {
	return LL_SIZE(g_heap);
}
