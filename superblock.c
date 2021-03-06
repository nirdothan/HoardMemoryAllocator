/*
 * superblock.c
 *
 *  Created on: Dec 18, 2014
 *      Author: Nir Dothan 028434009
 *
 *      This module creates and handles everything that occurs at the superblock level or inside it i.e. the blocks
 */


#include "memory_allocator.h"

/*
 *    |+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|
 *    |																															|
 *    |	<--------------------------------------------SUPERBLOCK_SIZE + sizeof(sblk_metadata_t)--------------------------------> |
 *    |																															|
 *    |	   sblk_metadata_t   |<--------------------------	SUPERBLOCK_SIZE -----------------------)--------------------------> |
 *    |				         |                                                                                                  |
 *    |				         |                                                                                                  |
 *    |				         |__________________________________________                                                        |
 *    |				         |                |                         |                                                       |
 *    |				         | block_header_t |        sizeClass        |                                                       |
 *    |				         |                |                         |                                                       |
 *    |				         |                |                         |                                                       |
 *    |				         |__________________________________________|                                                       |
 *    |				         |<--pointer sz-->|<--pointer sz--> <--pointer sz-->|                                               |
 *    |				         |                                                  |                                               |
 *    |				         |  block size (in pointer units)                   |                                               |
 *    |                      |        = (2*header+sizeClass-1)/header           |                                               |
 *    |				         |                                                  |                                               |
 *    |+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|
 *
 *
 */

superblock_t* makeSuperblock(size_t sizeClassBytes) {

	block_header_t *p, *pPrev = NULL;
	size_t netSuperblockSize = SUPERBLOCK_SIZE;
	size_t normSuperblockSize = netSuperblockSize / sizeof(block_header_t);

	/* the offset between subsequent blocks in units of block_header_t */
	size_t blockOffset = getBlockActualSizeInHeaders(sizeClassBytes);

	/* the number of blocks that we'll generate in this superblock */
	size_t numberOfBlocks = normSuperblockSize / blockOffset;
	int i;

	/* call system to allocate memory */
	superblock_t *pSb = (superblock_t*) getCore(SUPERBLOCK_SIZE + sizeof(sblk_metadata_t));

	pSb->_meta._sizeClassBytes = sizeClassBytes;
	pSb->_meta._NoBlks = pSb->_meta._NoFreeBlks = numberOfBlocks;
	pSb->_meta._pNxtSBlk = pSb->_meta._pPrvSblk = NULL;

	/* initialize the working pointer to the address where allocated buffer begins */
	p = (block_header_t*) pSb->_buff;

	/* initialize the stack pointer to the first element in the list
	 * it will later be regarded as "top of the stack"
	 */
	pSb->_meta._pFreeBlkStack = p;
	p->_pOwner = pSb;
	p->size = sizeClassBytes;

	/* create the initial free blocks stack inside the allocated memory buffer */
	for (i = 0; i < numberOfBlocks - 1; i++) {

		pPrev = p;
		p += blockOffset;
		pPrev->_pNextBlk = p;
		p->_pOwner = pSb;
		p->size = sizeClassBytes;
		p->_pNextBlk = NULL;
	}

	pthread_mutex_init(&(pSb->_meta._sbLock),NULL);

	return pSb;

}

/**
 * pop a block from the top of the stack.
 * caller must call the relocateSuperBlockBack on the owning heap to update the superblock's position
 */
block_header_t *popBlock(superblock_t *pSb) {

	if (!pSb->_meta._NoFreeBlks)
		return NULL; /* no free blocks */

	/* get tail block */
	block_header_t *pTail = pSb->_meta._pFreeBlkStack;

	/* advance superblock tail */
	pSb->_meta._pFreeBlkStack = pSb->_meta._pFreeBlkStack->_pNextBlk;
	pSb->_meta._NoFreeBlks--;

	/* disconnect from stack - but leave the owner for when the user wants to free it*/
	pTail->_pNextBlk = NULL;

	return pTail;

}

void *allocateFromSuperblock(superblock_t *pSb) {
	block_header_t *block = popBlock(pSb);

	relocateSuperBlockBack(
			&pSb->_meta._pOwnerHeap->_sizeClasses[getSizeClassIndex(
					pSb->_meta._sizeClassBytes)], pSb);
	return (void*) (block + 1);

}
/**
 * push a block to the top of the stack.
 * caller must call the relocateSuperBlockAhead on the owning heap to update the superblock's position
 */
superblock_t *pushBlock(superblock_t *pSb, block_header_t *pBlk) {

	if (pSb->_meta._NoFreeBlks == pSb->_meta._NoBlks)
		return NULL; /* stack full */

	/* new block's next is current tail */
	pBlk->_pNextBlk = pSb->_meta._pFreeBlkStack;

	/* stack tail points to new block */
	pSb->_meta._pFreeBlkStack = pBlk;

	pSb->_meta._NoFreeBlks++;

	return pSb;

}

/* reruns the percentage (0-100) of used blocks out of total blocks */
unsigned short getFullness(superblock_t *pSb) {

	double freeness = ((double) (pSb->_meta._NoFreeBlks))
			/ ((double) (pSb->_meta._NoBlks));

	double fullness = 1 - freeness;

	return ((unsigned short) (fullness * 100));

}

/**
 * utility function for printing a superblock
 */
void printSuperblock(superblock_t *pSb) {
	unsigned int i;
	block_header_t *p = pSb->_meta._pFreeBlkStack;
	printf("  Superblock: [%p] blocks: [%u] free [%u] used bytes [%u]\n", pSb,
			pSb->_meta._NoBlks, pSb->_meta._NoFreeBlks, getBytesUsed(pSb));
	printf("	[%p]<----prev    next---->[%p]\n", pSb->_meta._pPrvSblk,
			pSb->_meta._pNxtSBlk);
	printf("	====================================\n");

	for (i = 0; i < pSb->_meta._NoFreeBlks; i++, p = p->_pNextBlk) {
		printf("		free block %u) [%p]\n", i, p);
	}

}

/* returns the size in bytes of blocks used in superblock */
size_t getBytesUsed(const superblock_t *pSb) {
	size_t usedBlocks = pSb->_meta._NoBlks - pSb->_meta._NoFreeBlks;
	return usedBlocks * ( getBlockActualSizeInBytes(pSb->_meta._sizeClassBytes));
}

block_header_t *getBlockHeaderForPtr(void *ptr) {
	block_header_t *pBlock = (block_header_t *)( ptr - sizeof(block_header_t));
	return pBlock;
}

superblock_t *getSuperblockForPtr(void *ptr) {
	block_header_t *pHeader = getBlockHeaderForPtr(ptr);
	return pHeader->_pOwner;
}

/* for use with large allocations that do not use Hoard */
superblock_t* makeDummySuperblock(superblock_t *pSb, size_t sizeClassBytes) {
	pSb->_meta._sizeClassBytes = sizeClassBytes;
	return pSb;
}

void freeBlockFromSuperBlock(superblock_t *pSb, block_header_t *pBlock) {

	if (!pushBlock(pSb, pBlock))
		printf("Error freeing memory!\n");

}
/* the offset between subsequent blocks in units of block_header_t */
size_t getBlockActualSizeInHeaders(size_t sizeClassBytes){
	return (sizeClassBytes + 2 * sizeof(block_header_t) - 1)
			/ sizeof(block_header_t);

}

size_t getBlockActualSizeInBytes(size_t sizeClassBytes){
	return getBlockActualSizeInHeaders(sizeClassBytes)*sizeof(block_header_t);
}

