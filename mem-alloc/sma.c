/*
 * =====================================================================================
 *
 *	Filename:  		sma.c
 *
 *  Description:	code for Assignment 3 for ECSE-427 / COMP-310
 *
 *  Version:  		1.0
 *  Created:  		6/11/2020 9:30:00 AM
 *  Revised:  		-
 *  Compiler:  		gcc
 *
 *  Author:  		Colton Camobell 260777576
 *      
 * =====================================================================================
 */

/* Includes */
#include "sma.h" // Please add any libraries you plan to use inside this file

/* Definitions*/
#define MAX_TOP_FREE (128 * 1024) // Max top free block size = 128 Kbytes
//	TODO: Change the Header size if required
#define FREE_BLOCK_HEADER_SIZE 2*sizeof(struct Tag*)+sizeof(struct FreeNode*) // Size of the Header in a free memory block plus end tag
//	TODO: Add constants here
#define ALLOC_BLOCK_HEADER_SIZE 2*sizeof(struct Tag*)

typedef enum //	Policy type definition
{
	WORST,
	NEXT
} Policy;

char *sma_malloc_error;
void *freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void *freeListTail = NULL;			  //	The pointer to the TAIL of the doubly linked free memory list
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy
//	TODO: Add any global variables here
struct Tag {
	int L; //size
	char F; //tag: 0 if free, 1 if in use
};

struct FreeNode {
	void *prev; //previous block address
	void *next; //next block address
};


void *last_allocated_ptr=NULL; // pointer for last allocated block
void *heap_base = NULL; 

/*
 * =====================================================================================
 *	Public Functions for SMA
 * =====================================================================================
 */

/*
 *	Funcation Name: sma_malloc
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates a memory block of input size from the heap, and returns a 
 * 					pointer pointing to it. Returns NULL if failed and sets a global error.
 */
void *sma_malloc(int size)
{
	void *pMemory = NULL;

	// Checks if the free list is empty
	if (freeListHead == NULL)
	{
		// Allocate memory by increasing the Program Break
		pMemory = allocate_pBrk(size);
	}
	// If free list is not empty
	else
	{
		// Allocate memory from the free memory list
		pMemory = allocate_freeList(size);

		// If a valid memory could NOT be allocated from the free memory list
		if (pMemory == (void *)-2)
		{
			// Allocate memory by increasing the Program Break
			pMemory = allocate_pBrk(size);
		}
	}

	// Validates memory allocation
	if (pMemory < 0 || pMemory == NULL)
	{
		sma_malloc_error = "Error: Memory allocation failed!";
		return NULL;
	}

	// Updates SMA Info
	totalAllocatedSize += size;

	return pMemory;
}

/*
 *	Funcation Name: sma_free
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Deallocates the memory block pointed by the input pointer
 */
void sma_free(void *ptr)
{
	//	Checks if the ptr is NULL
	if (ptr == NULL)
	{
		puts("Error: Attempting to free NULL!");
	}
	//	Checks if the ptr is beyond Program Break
	else if (ptr > sbrk(0))
	{
		puts("Error: Attempting to free unallocated space!");
	}
	else
	{
		//	Adds the block to the free memory list
		add_block_freeList(ptr);
	}
}

/*
 *	Funcation Name: sma_mallopt
 *	Input type:		int
 * 	Output type:	void
 * 	Description:	Specifies the memory allocation policy
 */
void sma_mallopt(int policy)
{
	// Assigns the appropriate Policy
	if (policy == 1)
	{
		currentPolicy = WORST;
	}
	else if (policy == 2)
	{
		currentPolicy = NEXT;
	}
}

/*
 *	Funcation Name: sma_mallinfo
 *	Input type:		void
 * 	Output type:	void
 * 	Description:	Prints statistics about current memory allocation by SMA.
 */
void sma_mallinfo()
{
	//	Finds the largest Contiguous Free Space (should be the largest free block)
	// int largestFreeBlock = get_largest_freeBlock();
	char str[60];

	//	Prints the SMA Stats
	sprintf(str, "Total number of bytes allocated: %lu", totalAllocatedSize);
	puts(str);
	sprintf(str, "Total free space: %lu", totalFreeSize);
	puts(str);
	// sprintf(str, "Size of largest contigious free space (in bytes): %d", largestFreeBlock);
	// puts(str);
}

/*
 *	Funcation Name: sma_realloc
 *	Input type:		void*, int
 * 	Output type:	void*
 * 	Description:	Reallocates memory pointed to by the input pointer by resizing the
 * 					memory block according to the input size.
 */
void *sma_realloc(void *ptr, int size)
{

	if(!ptr || !(size>0)) {
		return (void *)-2; //return invalid address if given a NULL pointer or size of 0
	}
	void *new_block_addr = NULL;

	if((char*)ptr < sbrk(0)){ // then this pointer has previously been allocated (it's in the heap)
		struct Tag *ptr_front_tag = (char *)ptr - sizeof(struct Tag *);
		struct Tag *ptr_end_tag = (char*)ptr + get_blockSize(ptr);

		struct Tag *cur_front_tag = (struct Tag *)heap_base;
		void *cur_addr = (char*)heap_base + sizeof(struct Tag*);
		struct FreeNode *cur_node = (struct FreeNode *)((char *)cur_addr + sizeof(char *));

		// start from the bottom of the heap and move up to see where the memory can be copied to
		while(cur_addr && cur_front_tag->F == (char)0){
			if(ptr_front_tag == cur_front_tag) {
				// here we can move the memory
				if(ptr_front_tag->L < cur_front_tag->L) {
					sma_free(ptr);
					
					//expand
					new_block_addr = sma_malloc(size);
				}else{
					ptr_front_tag->L = size;
					ptr_end_tag->L = size;
					new_block_addr = ptr;
				}
			}
			cur_addr = (char*)cur_node + cur_front_tag->L + sizeof(struct Tag*);
			cur_front_tag = (char*) cur_addr - sizeof (struct Tag*);
		}
		return new_block_addr;
	}else { // not already in heap
		return ptr;
	}
	
}

/*
 * =====================================================================================
 *	Private Functions for SMA
 * =====================================================================================
 */

/*
 *	Funcation Name: allocate_pBrk
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory by increasing the Program Break
 */
void *allocate_pBrk(int size)
{
	void *newBlock = NULL;
	void *pb=NULL; //program break to keep track of top of heap here

	// get base of our heap if hasn't already been initialized
	if(!heap_base) {
		heap_base = sbrk(0);
	}

	// give it 127kB to reduce number of calls to sbrk()
	int excessSize = MAX_TOP_FREE - 1024;

	//	Allocate memory by incrementing the Program Break by calling sbrk() or brk()

	pb = sbrk(0);
	int total_bytes = size + excessSize; 
	if(sbrk(total_bytes + FREE_BLOCK_HEADER_SIZE) == (void *)-1) {
		return NULL;
	}

	newBlock = pb + sizeof(struct Tag *);

	pb = sbrk(0);

	//	Allocates the Memory Block
	allocate_block(newBlock, size, excessSize, 0);

	return newBlock;
}

/*
 *	Funcation Name: allocate_freeList
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory from the free memory list
 */
void *allocate_freeList(int size)
{
	void *pMemory = NULL;

	if (currentPolicy == WORST)
	{
		// Allocates memory using Worst Fit Policy
		pMemory = allocate_worst_fit(size);
	}
	else if (currentPolicy == NEXT)
	{
		// Allocates memory using Next Fit Policy
		pMemory = allocate_next_fit(size);
	}
	else
	{
		pMemory = NULL;
	}

	return pMemory;
}

/*
 *	Funcation Name: allocate_worst_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Worst Fit from the free memory list
 */
void *allocate_worst_fit(int size)
{
	void *worstBlock = NULL;
	int excessSize;
	int blockFound = 0;

	//	Allocate memory by using Worst Fit Policy

	// grab the largest block from get_largest_freeBlock and allocate the requested size in it

	worstBlock = get_largest_freeBlock_address();
	int freeBlock_size = get_largest_freeBlock();
	
	// check if free block exists
	if(worstBlock)
		blockFound = 1;

	//	Checks if appropriate block is found.
	if (blockFound)
	{
		excessSize = freeBlock_size - size;
		//	Allocates the Memory Block
		allocate_block(worstBlock, size, excessSize, 1);
	}
	else
	{
		//	Assigns invalid address if appropriate block not found in free list
		worstBlock = (void *)-2;
	}

	return worstBlock;
}

/*
 *	Funcation Name: allocate_next_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Next Fit from the free memory list
 */
void *allocate_next_fit(int size)
{
	void *nextBlock = NULL;
	int excessSize;
	int blockFound = 0;

	//	Allocate memory by using Next Fit Policy

	void *cur_address = freeListHead;
	void *lowest_address = freeListTail;
	
	// iterate through free list to find next fit
	while(cur_address) {
		struct Tag *cur_front_tag = (struct Tag *) ((char*)cur_address - sizeof(struct Tag*));
		struct FreeNode *cur_node = (struct FreeNode *) ((char*)cur_address + sizeof(char*));
		
		// we want the block that can hold the requested size that is most recently after the last allocated block
		if(size < get_blockSize(cur_address) && (char*)cur_address > (char*)last_allocated_ptr){
			nextBlock = cur_address;
			if((char*)cur_address < (char*)lowest_address) {
				lowest_address = cur_address;
				nextBlock = cur_address;
			}
		}
		cur_address = cur_node->next;
	}

	if((char*)nextBlock > (char*)last_allocated_ptr) {
		blockFound = 1;
	}

	//	Checks if appropriate found is found.
	if (blockFound)
	{
		excessSize = get_blockSize(nextBlock) - size;
		//	Allocates the Memory Block
		allocate_block(nextBlock, size, excessSize, 1);
	}
	else
	{
		//	Assigns invalid address if appropriate block not found in free list
		nextBlock = (void *)-2;
	}

	return nextBlock;
}

/*
 *	Funcation Name: allocate_block
 *	Input type:		void*, int, int, int
 * 	Output type:	void
 * 	Description:	Performs routine operations for allocating a memory block: allocates space and updates linked list of free to add excess size
 */
void allocate_block(void *newBlock, int size, int excessSize, int fromFreeList)
{
	void *excessFreeBlock; //	pointer for any excess free block
	int addFreeBlock;

	// 	Checks if excess free size is big enough to be added to the free memory list
	//	Helps to reduce external fragmentation
	addFreeBlock = excessSize > FREE_BLOCK_HEADER_SIZE+ALLOC_BLOCK_HEADER_SIZE+8;

	//	If excess free size is big enough
	if (addFreeBlock)
	{
		//Create a free block using the excess memory size, then assign it to the Excess Free Block

		// adjust size for newBlock
		struct Tag *alloc_front_tag = (struct Tag *) ((char *)newBlock - sizeof(struct Tag *));
		alloc_front_tag->L = size;
		alloc_front_tag->F = (char)1;

		last_allocated_ptr = newBlock;

		// end tag for new block
		struct Tag *alloc_end_tag = (struct Tag *) ((char *)newBlock + get_blockSize(newBlock));
		alloc_end_tag->L = size;
		alloc_end_tag->F = (char)1;

		// front free block tag
		struct Tag *free_front = (struct Tag *) ((char *)alloc_end_tag + sizeof(struct Tag *));
		free_front->L = excessSize;
		free_front->F = (char)0;

		// block start
		excessFreeBlock = (char *) free_front + sizeof(struct Tag *);

		// free node
		struct FreeNode *new_free = (struct FreeNode *) ((char *) excessFreeBlock + sizeof(char *));
		new_free->prev = NULL;
		new_free->next = NULL;

		// end free block tag (block end)
		struct Tag *free_end = (struct Tag *) ((char *)excessFreeBlock + get_blockSize(excessFreeBlock));
		free_end->L = excessSize;
		free_end->F = (char)0;

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList)
		{
			//	Removes new block and adds the excess free block to the free list
			replace_block_freeList(newBlock, excessFreeBlock);
		}
		else
		{
			//	Adds excess free block to the free list
			add_block_freeList(excessFreeBlock);
		}
	}
	//	Otherwise add the excess memory to the new block
	else
	{
		//Add excessSize to size and assign it to the new Block
		struct Tag *alloc_block_front = (struct Tag *) ((char *)newBlock - sizeof(struct Tag *));
		alloc_block_front->L = size + excessSize;
		alloc_block_front->F = (char)1;
		newBlock = (char *)alloc_block_front + sizeof(struct Tag *);
		struct Tag *alloc_block_end = (struct Tag * ) ((char *)newBlock + get_blockSize(newBlock));
		alloc_block_end->L = size + excessSize;
		alloc_block_end->F = (char)1;



		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList)
		{
			//	Removes the new block from the free list
			remove_block_freeList(newBlock);
		}
	}
}

/*
 *	Funcation Name: replace_block_freeList
 *	Input type:		void*, void*
 * 	Output type:	void
 * 	Description:	Replaces old block with the new block in the free list
 */
void replace_block_freeList(void *oldBlock, void *newBlock)
{
	remove_block_freeList(oldBlock);
	add_block_freeList(newBlock);
}

/*
 *	Funcation Name: add_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Adds a memory block to the the free memory list
 */
void add_block_freeList(void *block)
{
	//	TODO: 	Add the block to the free list
	//	Hint: 	You could add the free block at the end of the list, but need to check if there
	//			exits a list. You need to add the TAG to the list.
	//			Also, you would need to check if merging with the "adjacent" blocks is possible or not.
	//			Merging would be tideous. Check adjacent blocks, then also check if the merged
	//			block is at the top and is bigger than the largest free block allowed (128kB).

	//	Updates SMA info

	// totalAllocatedSize -= get_blockSize(block);
	// totalFreeSize += get_blockSize(block);

	// // if there is no free list, make this the first node
	// if(!freeListHead) {
	// 	struct FreeNode *head = (struct FreeNode *) ((char *)block + sizeof(char *));
	// 	head->prev = NULL;
	// 	head->next = NULL;
	// 	freeListHead = block;
	// 	freeListTail = block;
	// }else{ 
	// 	// add this block to the tail of the free list and check if it can merge with adjacent free blocks
	// 	struct FreeNode *tail = (struct FreeNode *)((char *)freeListTail + sizeof(char *));
	// 	tail->next = block;
	// 	struct FreeNode *this_node = (struct FreeNode*) ((char*)block + sizeof(char*));
	// 	this_node->prev = freeListTail;
	// 	this_node->next = NULL;
	// 	freeListTail = block;

	// 	// boundary tags for this block
	// 	struct Tag *this_front_tag = (struct Tag *) ((char *)block - sizeof(struct Tag *));
	// 	struct Tag *this_end_tag = (struct Tag *) ((char *)block + get_blockSize(block));
	
	// 	//end tag of the block before this block
	// 	struct Tag *end_tag_before = (struct Tag*) ((char*)this_front_tag - sizeof(struct Tag*));
	
	// 	//front tag of the block after this block
	// 	struct Tag *front_tag_after = (struct Tag *)((char *)this_end_tag + sizeof(struct Tag*));
		
	// 	//check if block before is free and merge if so
	// 	if(end_tag_before && end_tag_before->F == (char)0){
	// 		// change end tag of this block
	// 		this_end_tag->L += end_tag_before->L;
	// 		//front tag of the block before
	// 		struct Tag *front_tag_before = (struct Tag*) ((char*)end_tag_before - end_tag_before->L - sizeof(struct Tag*));
	// 		//change front tag of block before
	// 		front_tag_before->L += this_front_tag->L;

	// 		// point this block at block before it to merge
	// 		block = (char*)front_tag_before + sizeof(struct Tag*);
	// 		this_front_tag = front_tag_before;
	// 		end_tag_before = this_end_tag;
	// 	}

	// 	//check if block after is free and merge if so
	// 	if(front_tag_after && front_tag_after->F == (char)0) {

	// 		//change front tag of this block
	// 		this_front_tag->L += front_tag_after->L;

	// 		// free block after this one in the heap
	// 		void *block_after = (char *)front_tag_after + sizeof(struct Tag *);
	// 		//end tag of block after
	// 		struct Tag *end_tag_after = (struct Tag*) ((char*)block_after + get_blockSize(block_after));
	// 		//change end tag of block after
	// 		end_tag_after->L += this_end_tag->L;

	// 		// manipulate free list: point previous node's "next" at this new merged block
	// 		struct FreeNode *block_node = (struct FreeNode*) ((char*)block_after + sizeof(char*));
			
	// 		// check if the next block node has a previous block 
	// 		if(block_node->prev){
	// 			// update the next block's previous block to point at merged block
	// 			struct FreeNode *prev_node = (struct FreeNode*) ((char*)block_node->prev + sizeof(char*));
	// 			prev_node->next = block;
	// 		}
	// 		// check if the next block node has a next block
	// 		if(block_node->next) {
	// 			// update the next block's next block to point at merged block
	// 			struct FreeNode *next_node = (struct FreeNode*) ((char*)block_node->next + sizeof(char*));
	// 			next_node->prev = block;
	// 		}

	// 		// point merged block's previous and next to the merging block's previous and next
	// 		this_node->prev = block_node->prev;
	// 		this_node->next = block_node->next;
	// 		//point block to be merged at this block
	// 		block_after = block;
	// 		block_node = NULL;
	// 		// update tags on both sides
	// 		front_tag_after = this_front_tag;
	// 		this_end_tag = end_tag_after;
	// 		tail->next = NULL;
	// 	}
		
	// } 
}

/*
 *	Funcation Name: remove_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Removes a memory block from the the free memory list
 */
void remove_block_freeList(void *block)
{
	

	//	Updates SMA info
	totalAllocatedSize += get_blockSize(block);
	totalFreeSize -= get_blockSize(block);

	//	Remove the block from the free list

	// free node to be removed
	struct FreeNode *old_node = (struct FreeNode *) ((char *)block + sizeof(char *));

	// addresses of the old block's previous and next blocks in free list
	void *prev_addr = old_node->prev;
	void *next_addr = old_node->next;

	// check if it has a previous block and update it's next block to the old block's next block
	if(prev_addr) {
		struct FreeNode *prev_node = (struct FreeNode *) ((char *)prev_addr + sizeof(char *));
		prev_node->next = next_addr;
	}
	// check if it has a next block and update it's previous block to the old block's previous block
	if(next_addr) {
		struct FreeNode *next_node = (struct FreeNode *) ((char *)next_addr + sizeof(char *));
		next_node->prev = prev_addr;
	}

	// udpate boundary tags to show this block is no longer free
	struct Tag *front_tag = (struct Tag *) ((char *)block - sizeof(struct Tag *));
	front_tag->F = (char)1;
	struct Tag *end_tag = (struct Tag *) ((char *)block + get_blockSize(block));
	end_tag->F = (char)1;
}

/*
 *	Funcation Name: get_blockSize
 *	Input type:		void*
 * 	Output type:	int
 * 	Description:	Extracts the Block Size
 */
int get_blockSize(void *ptr)
{
	struct Tag *block_front_tag = (struct Tag *) ((char *)ptr - sizeof(struct Tag *));
	return block_front_tag->L;
}

/*
 *	Funcation Name: get_largest_freeBlock
 *	Input type:		void
 * 	Output type:	int
 * 	Description:	Extracts the largest Block Size
 */
int get_largest_freeBlock()
{
	int largestBlockSize = 0;
	largestBlockSize = get_blockSize(get_largest_freeBlock_address());

	return largestBlockSize;
}

void *get_largest_freeBlock_address() {
	int largestBlockSize = 0;
	void *largestBlock_address = NULL;

	// Iterate through the Free Block List to find the largest free block and return its address

	// start at FreeList head
	void *cur_address = freeListHead;
	while(cur_address) {
		//front tag of current block
		struct Tag *cur_tag_front = (struct Tag *) ((char *)cur_address - sizeof(struct Tag *));
		// current block's free node
		struct FreeNode *cur_node = (struct FreeNode *) ((char*)cur_address + sizeof(char *));
		//update largest block size found and address of largest block
		if(cur_tag_front->L > largestBlockSize) {
			largestBlockSize = cur_tag_front->L;
			largestBlock_address = cur_address;
		}
		cur_address = cur_node->next;
	}

	return largestBlock_address;
}
