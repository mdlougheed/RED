/*
	Header for global buffer constants, structures.
	Source:  redbuf.h
	Version: November 15, 1983

	Copyright (C) 1983 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/


/*
You may tune these constants for better disk performance.

DATA_SIZE:  The size of that part of struct BLOCK which is
	    written to the disk.  Make sure that DATA_SIZE
	    is a multiple of the size of your disk sectors.

MAX_RES:    The maximum number of BLOCKS resident in memory.
	    The code assumes this number is AT LEAST 3.
	    The actual number of resident blocks is DATA_RES.
	    DATA_RES is set by calling alloc(DATA_SIZE) until
	    MAX_RES blocks are allocated or until alloc() fails.
*/

#define DATA_SIZE 1024
#define MAX_RES	  100


/* Do not touch these constants. */

#define CPM_SIZE 128
#define READ_SIZE  (DATA_SIZE/CPM_SIZE)

/*
	Partially define the format of a block.  The end of
	the data area contains an "index table" of indices
	into the front of the data area.  Each index is the
	index of the last byte of a line.

	Thus, it is possible to calculate the starting address
	and length of each line in the block WITHOUT searching
	through either the data field or the index table.

	The d_back and d_next fields in the header are used
	to doubly-link the disk blocks so that stepping
	through the blocks either forward or backwards is
	efficient.  -1 denotes the end of each list.

	When blocks become totally empty they are entered
	on a list of free blocks.  The links of this list
	are kept in the blocks themselves in the d_next field.
	The b_free variable is the head of this list.
*/

/*
	Warning!!  RED will crash if the following two
		   defines are not right.

		   They should be equal to 3*sizeof(int)
*/

#define HEADER_SIZE 6		/* size of the block header	*/
#define STATUS_SIZE 6		/* size of the status table	*/

#define BUFF_SIZE  (DATA_SIZE - HEADER_SIZE)
#define BLOCK_SIZE (DATA_SIZE + STATUS_SIZE)

struct BLOCK {

	/*	The block header.				*/

	int	d_back;		/* # of previous block		*/
	int	d_next;		/* # of next block		*/
	int	d_lines;	/* # of lines on block		*/

	/*	The data area and index table.			*/

	char	d_data [BUFF_SIZE];

	/*	The status table -- not written to disk.	*/

	int	d_lru;		/* lru count		*/
	int	d_status;	/* FULL, FREE or DIRTY	*/
	int	d_diskp;	/* disk pointer		*/
};


/*
	Define the entries in the d_status field.
*/

#define FREE	1	/* status:  block is available	*/
#define FULL	2	/* status:  block is allocated	*/
#define DIRTY	3	/* status:  must swap out	*/
tatus:  block is available	*/
#define FULL	2	/* 