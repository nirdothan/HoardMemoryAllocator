/*
 * superblock.c
 *
 *  Created on: Dec 18, 2014
 *      Author: Nir Dothan 028434009
 */


#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

void *getCore(unsigned int );

/*
 *    |+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|
 *    |																															|
 *    |	<--------------------------------------------SUPERBLOCK_SIZE----------------------------------------------------------> |
 *    |																															|
 *    |	   sblk_metadata_t   |<--------------------------	SUPERBLOCK_SIZE -sizeof(sblk_metadata_t)--------------------------> |
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
superblock_t* makeSuperblock(unsigned int sizeClass) {

	block_header_t *p, *pPrev = NULL;
	unsigned int netSuperblockSize = SUPERBLOCK_SIZE - sizeof(block_header_t);
	unsigned int normSuperblockSize=netSuperblockSize/ sizeof(block_header_t);


	/* the offset between subsequent blocks in units of block_header_t */
	unsigned int blockOffset = (sizeClass + 2 * sizeof(block_header_t) - 1)
			/ sizeof(block_header_t);

	/* the number of blocks that we'll generate in this superblock */
	unsigned int i, numberOfBlocks = normSuperblockSize  / blockOffset;

	superblock_t *pSb = (superblock_t*) getCore(SUPERBLOCK_SIZE);

	pSb->_meta._NoBlks = pSb->_meta._NoFreeBlks = numberOfBlocks;
	pSb->_meta._pNxtSBlk = pSb->_meta._pPrvSblk = NULL;

	/* initialize the working pointer to the address where allocated buffer begins */
	p = (block_header_t*) pSb->_buff;

	/* initialize the stack pointer to the first element in the list
	 * it will later be regarded as "top of the stack"
	 */
	pSb->_meta._pFreeBlkStack = p;
	p->_pOwner = pSb;

	/* create the initial free blocks stack inside the allocated memory buffer */
	for (i = 0; i < numberOfBlocks - 1; i++) {
		pPrev = p;
		p += blockOffset;
		pPrev->_pNextBlk = p;
		p->_pOwner = pSb;
		p->_pNextBlk = NULL;
	}

	return pSb;

}

void *popBlock(superblock_t *pSb){

	if ( !pSb->_meta._NoFreeBlks)
		return NULL; /* no free blocks */

	/* get tail block */
	block_header_t  *pTail=pSb->_meta._pFreeBlkStack;




	/* advance superblock tail */
	pSb->_meta._pFreeBlkStack=	pSb->_meta._pFreeBlkStack->_pNextBlk;
	pSb->_meta._NoFreeBlks--;

	/* disconnect from stack - but leave the owner for when the user wants to free it*/
	pTail->_pNextBlk=NULL;

	return pTail;

}

superblock_t *pushBlock(superblock_t *pSb, block_header_t *pBlk){

	if (pSb->_meta._NoFreeBlks==pSb->_meta._NoBlks)
		return NULL; /* stack full */


	/* new block's next is current tail */
	pBlk->_pNextBlk=pSb->_meta._pFreeBlkStack;

	/* stack tail points to new block */
	pSb->_meta._pFreeBlkStack=pBlk;

	pSb->_meta._NoFreeBlks++;

	return pSb;

}



unsigned short getFullness(superblock_t *pSb){

	float fullness=(pSb->_meta._NoFreeBlks)
			         /(pSb->_meta._NoBlks);

	return ((unsigned short)fullness*100);

}
