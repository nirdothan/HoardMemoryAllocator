#include <stdio.h>
#include <strings.h>
#include "structs.h"

superblock_t* makeSuperblock(unsigned int );
void insertSuperBlock(size_class_t *sizeClass, superblock_t *superBlock) ;
void *getCore(unsigned int ) ;
void makeSizeClass(size_class_t *sizeClass, unsigned int size);
superblock_t *removeSuperblock(size_class_t *sizeClass,
		superblock_t *superBlock) ;

void relocateSuperBlockAhead(size_class_t *sizeClass, superblock_t *superBlock) ;

void main() {



	hoard_t *mem=getMem();


	size_class_t sc;

	superblock_t *sb1 = makeSuperblock(1);
	superblock_t *sb2 = makeSuperblock(20000);
	superblock_t *sb3 = makeSuperblock(20000);
	//block_header_t *bl2, *bl=popBlock(sb1);



	    malloc(1);
		exit(0);



	makeSizeClass(&(mem->_heaps[1]._sizeClasses[0]),1);
	insertSuperBlock(&(mem->_heaps[1]._sizeClasses[0]), sb1);


//	makeSizeClass(&sc,2);


	//insertSuperBlock(&sc, sb1);

	insertSuperBlock(&sc, sb2);
	insertSuperBlock(&sc, sb3);



	printSizeClass(&sc);

	removeSuperblock(&sc,sb2);

	printf("rm sb2\n\n");

	printSizeClass(&sc);





	removeSuperblock(&sc,sb3);

	printf("rm sb3\n\n");

	printSizeClass(&sc);


	removeSuperblock(&sc,sb1);

	printf("rm sb1\n\n");

	printSizeClass(&sc);


	insertSuperBlock(&sc,sb3);

	printf("ins sb3\n\n");

	printSizeClass(&sc);


	insertSuperBlock(&sc,sb2);

		printf("ins sb2\n\n");

		printSizeClass(&sc);


		insertSuperBlock(&sc,sb1);

			printf("ins sb1\n\n");

			printSizeClass(&sc);




			relocateSuperBlockAhead(&sc,sb2);
			printf("relocate sb2\n\n");

				printSizeClass(&sc);


				relocateSuperBlockAhead(&sc,sb1);
							printf("relocate sb1\n\n");

								printSizeClass(&sc);




	printf("done\n");



}
