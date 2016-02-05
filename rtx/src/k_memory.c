/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Pushpak Kumar
 * @date:   2016/01/23
 */

#include "k_memory.h"
 #include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */

mem_t mem_blocks[30];
LL_DECLARE(g_heap, mem_t*, 30);
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
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
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

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_PROCS * sizeof(PCB *);
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB);
	}

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

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b)
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */

	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);

	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack;
	}
	return sp;
}

void *k_request_memory_block(void)
{
	U8 *p_mem_blk = NULL;

	while(LL_SIZE(g_heap) == 0) {
        // release proc, put cur proc. in ready queue
        // not right because: call k
        // never set current state to blocked
        // if all we do is release processor
        gp_current_process->m_state = BLOCKED_ON_RESOURCE;
		k_release_processor();
        assert(gp_current_process->m_state == RDY);
        assert(LL_SIZE(g_heap) != 0);
	}

	p_mem_blk = (U8 *)LL_POP_FRONT(g_heap);
	return (void *)p_mem_blk;
}

int k_release_memory_block_valid(void *p_mem_blk)
{
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

    //get PCB of the next highest-priority blocked process
    PCB *p_unblocked_pcb;
		PCB *p_blocked_pcb = k_peek_blocked_on_resource_front();

    //enqueue the popped PCB to the g_ready_queue;
    if(p_blocked_pcb != NULL){
      if(p_blocked_pcb->m_priority > gp_current_process){
        p_unblocked_pcb = k_dequeue_blocked_on_resource_process();
      }
      else{
        return RTX_OK;
      }
			p_blocked_pcb->m_state = RDY;  //update the process state to RDY
			k_enqueue_ready_process(p_blocked_pcb);  //enqueue the process in ready queue
      return k_release_processor(); //call release processor
		}
	}
	else{
		return RTX_ERR;
	}
	return RTX_OK;
}
