/*
	RED buffer routines -- Full C version
	Part 3 -- file routines

	Source:  red12.c
	Version: April 2, 1984;  April 20, 1984; March 8, 1985

	Copyright (C) 1983, 1984, 1985 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#include "red.h"


/*
	## data_open()
	Open the data file.
*/

int
data_open()
{
	/* Erase the data file if it exists. */
	sysunlink(DATA_FILE);

	/* Create the data file. */
	b_data_fd = syscreat(DATA_FILE);
	if (b_data_fd == ERROR) {
		disk_error("Can not open swap file.");
	}

	/* Close the file, reopen it for read/write access. */
	sysclose(b_data_fd);
	b_data_fd = sysopen(DATA_FILE, 2);
	return b_data_fd;
}


/*
	## do_lru(bp)
	Make the slot the MOST recently used slot.
*/

do_lru(bp)
struct BLOCK *bp;
{
	struct BLOCK *bp1;
	int i, lru;
	
	/*
		Change the relative ordering of all slots
	 	which have changed more recently than slot.
	 */
	lru = bp -> d_lru;
	for (i = 0; i < DATA_RES; i++) {
		bp1 = b_bpp [i];
		if (bp1 -> d_lru < lru) {
			bp1 -> d_lru++;
		}
	}

	/* The slot is the most recently used. */
	bp -> d_lru = 0;
}


/*
	## disk_error(message)
	## disk_full()
	Disk error routines
*/

disk_error(message)
char	*message;
{
	error(message);

	/* Clear the buffer if no recovery is possible. */
	if (b_fatal == YES) {
		bufnew();
	}

	/* Abort the operation that caused the error. */
	longjmp(DISK_ERR, ERROR);
}

disk_full()
{
	disk_error("Disk or directory full?");
}

disk_rdy()
{
	disk_error("Drive not ready?");
}


/*
	## is_dirty(slot)
	Indicate that a slot must be saved on the disk.
*/

is_dirty(bp)
struct BLOCK *bp;
{
	bp -> d_status = DIRTY;
}


/*
	## put_block(blockp, diskp)
	Put out the block-sized buffer to the disk sector.
*/

put_block(bp, diskp)
struct	BLOCK *bp;
int	diskp;
{
	int s;

	/* Make sure blocks are written in order. */
	if (diskp > b_max_put + 1) {
		swap_sync(b_max_put + 1, diskp - 1);
	}
	b_max_put = max(b_max_put, diskp);
	
	/* Seek to the correct sector of the data file. */
	s = sysseek(b_data_fd, diskp);
	if (s == -1) {
		disk_full();
	}

	/* Write the block to the data file. */

#ifdef CAST
	if (syswrite(b_data_fd, (char *) bp,
	    READ_SIZE) != READ_SIZE) {
		disk_full();
	}
#else
	if (syswrite(b_data_fd, bp,
	    READ_SIZE) != READ_SIZE) {
		disk_full();
	}
#endif

}


/*
	## put_buf(avail)
	Fill in the header fields of the output buffer and
	write it to the disk.
	avail is the number of free characters in the buffer.
*/

char *
put_buf(avail)
int avail;
{
	struct BLOCK *bp;

	/*
		Fill in the back and next links immediately.
		This can be done because we are not waiting
		for the LRU algorithm to allocated disk blocks.
		The last block that put_buf() writes will have
		an incorrect next link.  Read_file() will make
		the correction.
	*/

#ifdef CAST
	bp = (struct BLOCK *) b_buff;
#else
	bp = b_buff;
#endif

	bp -> d_back  = b_max_diskp - 1;
	bp -> d_next  = b_max_diskp + 1;
	bp -> d_lines = b_line - b_start;

	if (avail < 0) {
		cant_happen("put_buf");
	}

	/* Update block and line counts. */
	b_max_diskp++;
	b_start = b_line;

	/* Write the block. */
	put_block(b_buff, b_max_diskp - 1);
}


/*
	## put_slot(bp)
	Write out the slot to the data file.
*/

put_slot(bp)
struct BLOCK *bp;
{
	if (bp -> d_diskp == ERROR) {
		cant_happen("put_slot");
	}
	put_block(bp, bp -> d_diskp);
}


/*
	## read_file(file_name)
	Read a file into the buffer.

	This version of read_file puts an index table at
	the end of each block.  The index table's entry
	for each line tells the distance of the LAST character
	of the line from the start of the data buffer.

	The global variables br_count, br_bufp, and br_bufc
	are used to communicate with read1().  Using these
	variables speeds the code by a factor of 3!

	The "global" variables br_avail and br_out are used
	only by read_file() -- again, purely to speed the code.
*/

read_file(file_name)
char file_name [];
{
	struct BLOCK *bp, *swap_in();

	/* global:  char * br_bufp   pointer to buffer	*/
	/* global:  int    br_bufc   index into buffer	*/
	/* global:  int    br_count  number of buffer	*/

	/* global:  int    br_avail  available chars	*/
	/* global:  int    br_out    index into outbuf	*/

	char	*outbuf;	/* the output buffer	*/
	int	out_save;	/* line starts here	*/
	int	c, i, j;

	/* Clear the swapping buffers and the files. */
	bufnew();
	b_bp -> d_status = FREE;

	/* Open the user file for reading only. */
	b_user_fd = sysopen(file_name, 0);
	if (b_user_fd == ERROR) {
		disk_error("File not found.");
	}

	/* Clear the buffer on any disk error. */
	b_fatal = YES;

	/* Open the data file. */
	data_open();

	/* The file starts with line 1. */
	b_line = 1;
	b_start = 1;

	/* There are no blocks in the file yet. */
	b_head = b_tail = ERROR;
	b_max_diskp = 0;

	/* Point outbuf to start of the output data area. */
	outbuf = b_buff + HEADER_SIZE;

	/* Force an initial read in read1(). */
	br_count = DATA_SIZE;
	br_bufc  = DATA_RES;

	/* Zero the pointers into the output buffer. */
	br_out = out_save = 0;

	/* Allocate space for the first table entry. */
	br_avail = BUFF_SIZE - INT_SIZE;
	
	/* Set the current line counts. */
	b_line = b_start = 1;

	for(;;) {

		if (br_avail <= 0 && out_save == 0) {
			/* The line is too long. */
			error ("Line split.");

			/* End the line. */
			b_settab( b_buff,
				  b_line - b_start,
				  br_out
				);
			b_line++;

			/* Clear the output buffer. */
			put_buf(br_avail);
			br_out = out_save = 0;
			br_avail = BUFF_SIZE - INT_SIZE;
		}

		else if (br_avail <= 0) {

			/*
				Deallocate last table entry and
				reallocate space used by the
				partial line.
			*/
			br_avail += (INT_SIZE + br_out - out_save);

			/* Write out the buffer. */
			put_buf(br_avail);

			/* Move the remainder to the front. */
			sysmove(outbuf + out_save,
				outbuf,
				br_out - out_save);

			/* Reset restart point. */
			br_out   = br_out - out_save;
			out_save = 0;
			br_avail = BUFF_SIZE - INT_SIZE - br_out;
		}

		c = read1();

		if (c == CPMEOF) {

			if (br_out != out_save) {

				/* Finish the last line. */
				b_settab( b_buff,
					  b_line-b_start,
					  br_out	/* 3/8/85 */
					);
				b_line++;
				out_save = br_out;
			}
			else {
				/* No last line after all. */
				br_avail += INT_SIZE;
			}

			/* bug fix:  2/20/84, 4/2/84 */
			if (br_avail !=  BUFF_SIZE) {
				put_buf(br_avail);
			}
			break;
		}

		else if (c == NEWLINE) {

			/* Finish the line. */
			b_settab( b_buff,
				  b_line - b_start,
				  br_out
				);
			br_avail -= INT_SIZE;

			/* Set restart point. */
			b_line++;
			out_save = br_out;
		}

		else if (c == CR) {
			/* Ignore CP/M's pseudo-newline. */
			continue;
		}

		else {

			/* Copy normal character. */
			outbuf [br_out++] = c;
			br_avail--;
		}
	}

	/* Close the user' file. */
	sysclose(b_user_fd);

	/* Special case:  null file. */
	if (b_max_diskp == 0) {
		bufnew();
		return;
	}

	/* Rewrite the last block with correct next field. */

#ifdef CAST
	bp = (struct BLOCK *) b_buff;
#else
	bp = b_buff;
#endif

	bp -> d_next = ERROR;
	put_block(b_buff, b_max_diskp - 1);

	/* Set the pointers to the first and last blocks. */
	b_max_diskp--;
	b_head = 0;
	b_tail = b_max_diskp;

	/*
		Clear all slots.  This is REQUIRED since
		read_file has just overwritten all slots.
	*/
	buf_clr();

	/* Move to the start of the file. */
	b_max_line = b_line - 1;
	b_line = 1;
	b_start = 1;
	b_bp = swap_in(b_head);

	b_fatal = NO;
}


/*
	## read1()
	Get one character from the input file.

	This version of read1 uses all slots as an input buffer.
	The slots to not need to be contiguous in memory
	(which they are generally are NOT because of hidden
	header information used only by sysalloc()).

	This version uses the globals br_count, br_bufc and
	br_bufp to speed up the code.
*/

read1()
{
	if (br_count == DATA_SIZE) {

		if (br_bufc >= DATA_RES - 1) {

			/* Read into buffers. */
			read2();
			br_count = br_bufc = 0;
		}
		else {

			/* Switch to next buffer. */
			br_bufc++;
			br_count = 0;
		}

#ifdef CAST
		br_bufp = (char *) b_bpp [br_bufc];
#else
		br_bufp = b_bpp [br_bufc];
#endif

	}

	/* Get the character and mask off parity bit. */
	return br_bufp [br_count++] & 0x7f;
}


/*
	## Read2()
	Read user file into all slots.
*/

read2()
{
	int i, s;

	for (i = 0; i < DATA_RES; i++) {

		/* Point at the next slot. */

#ifdef CAST
		br_bufp = (char *) b_bpp [i];
#else
		br_bufp = b_bpp [i];
#endif

		/* Read the next sector. */
		s = sysread(b_user_fd, br_bufp);

		if (s == ERROR) {
			disk_rdy();
		}

		/* Force a CPM end of file mark. */
		if (s < READ_SIZE) {
			br_bufp [s * CPM_SIZE] = CPMEOF;
			break;
		}
	}
}


/*
	## swap_all()
	Swap out all dirty blocks.
*/

swap_all()
{
	struct BLOCK *bp;
	int i;

	for (i = 0; i < DATA_RES; i++) {
		bp = b_bpp [i];
		if (bp -> d_status == DIRTY) {
			put_slot (bp);
			bp -> d_status = FULL;
		}
	}
}


/*
	Swap out the first dirty block.   This routine does
	not swap the dirty block since that would waste time.
	This routines is called when nothing else is happening.
*/

#ifdef SWAP
swap_one()
{
	struct BLOCK *bp;
	int i;

	for (i = 0; i < DATA_RES; i++) {
		bp = b_bpp [i];
		if (bp != b_bp && bp -> d_status == DIRTY) {
			put_slot (bp);
			bp -> d_status = FULL;
			break;
		}
	}
}
#endif
			

/*
	## swap_in(diskp)
	Get the block from the disk into a slot in memory.
	Return a pointer to the block.
*/

struct BLOCK *
swap_in(diskp)
int diskp;
{
	struct BLOCK *bp, *swap_new();
	int i, status;

	if (diskp < 0 || diskp > b_max_diskp) {
		cant_happen("swap_in 1");
	}

	/* See whether the block is already in a slot. */
	for (i = 0; i < DATA_RES; i++) {
		bp = b_bpp [i];
		if (bp -> d_status != FREE &&
		    bp -> d_diskp  == diskp) {

			/* Reference the block. */
			do_lru(bp);

			/* Point to the slot. */
			return bp;
		}
	}

	/* Clear a slot for the block. */
	bp = swap_new(diskp);

	/* Read from temp file to block. */
	status = sysseek(b_data_fd, diskp);
	if (status == -1) {
		disk_rdy();
	}

	/* Read the block into the slot. */

#ifdef CAST
	status = sysread(b_data_fd, (char *) bp);
#else
	status = sysread(b_data_fd, bp);
#endif

	if (status == ERROR) {
		disk_rdy();
	}

	/* Swap_new() has already called do_lru(). */

	/* Return a pointer to the block. */
	return bp;
}


/*
	## swap_new(diskp)
	Free a slot for a block located at diskp.
	Swap out the least recently used block if required.
	Return a pointer to the block.
*/

struct BLOCK *
swap_new(diskp)
int diskp;
{
	struct BLOCK *bp, *swap_out();
	int i;

	/* Search for an available slot. */
	for (i = 0; i < DATA_RES; i++) {
		bp = b_bpp [i];
		if (bp -> d_status == FREE) {
			break;
		}
	}

	/* Swap out a block if all blocks are full. */
	if (i == DATA_RES) {
		bp = swap_out();
	}

	/* Make sure the block will be written. */
	bp -> d_status = FULL;
	bp -> d_diskp  = diskp;

	/* Reference the slot. */
	do_lru(bp);

	/* Return a pointer to the slot. */
	return bp;
}


/*
	## swap_out()
	Swap out the least recently used (LRU) slot.
	Return a pointer to the block.
*/

struct BLOCK *
swap_out()
{
	struct BLOCK *bp;
	int i;

	/* Open the temp file if it has not been opened. */
	if (b_data_fd == ERROR) {
		b_data_fd = data_open();
	}

	/* Find the least recently used slot. */
	for (i = 0; ;i++) {
		bp = b_bpp [i];
		if (bp -> d_lru == DATA_RES - 1) {
			break;
		}
	}

	/* Do the actual swapping out if memory is dirty. */
	if (bp -> d_status == DIRTY) {
		put_slot(bp);
		return bp;
	}

	/* d_diskp is not ERROR if status is not DIRTY. */
	if (bp -> d_diskp == ERROR) {
		cant_happen("swap_out");
	}

	/* Indicate that the slot is available. */
	bp -> d_status = FREE;
	bp -> d_diskp  = ERROR;

	/* Return a pointer to the block. */
	return bp;
}


/*
	## swap_sync()
	Swap out blocks found between low and high on the disk.
*/

swap_sync(low, high)
int low, high;
{
	struct BLOCK *bp;
	int disk, i;

	/* Search the slot table for each disk. */
	for (disk = low; disk <= high; disk++) {
		for (i = 0; i < DATA_RES; i++) {
			bp = b_bpp [i];
			if (bp -> d_diskp == disk) {

				/* Write the slot. */
				put_slot(bp);
				bp -> d_status = FULL;
				break;
			}
		}
		if (i == DATA_RES) {
			cant_happen("swap_sync");
		}
	}
}


/*
	## write_file(file_name)
	Write the entire buffer to file.
*/

write_file(file_name)
char *file_name;
{
	/* global:  bw_count */

	struct BLOCK *bp;
	char *data;
	int slot, line, nlines, length, next, count;
	int c;

	/* Open the user file.  Erase it if it exists. */
	b_user_fd = syscreat(file_name);
	if (b_user_fd == ERROR) {
		disk_full();
	}

	/* Copy each block of the file. */
	bw_count = 0;
	for (next = b_head; next != ERROR; ) {

		/* Swap in the next block. */
		bp = swap_in(next);

		/* Get data from the header of the block. */
		next   = bp -> d_next;
		nlines = bp -> d_lines;
		data   = bp -> d_data;
		
		/* Copy each line of the block. */
		count = 0;
		for (line = 0; line < nlines; line++) {

			/* Get length of the line. */
			if (line == 0) {
				length = b_tab(bp, line);
			}
			else {
				length = b_tab(bp,line) -
					 b_tab(bp,line - 1);
			}
			
			/* Copy each char of the line. */
			for (; length; length--) {
				c = data [count++];
				write1(c);
			}

			/* Add CR and LF at end. */
			write1(CR);
			write1(LF);
		}
	}

	/* Force an end of file mark. */
	write1(CPMEOF);

	/* Flush the buffer and close the file. */
	wr_flush();
	sysclose(b_user_fd);

	/* Kludge:  go to line 1 for a reference point. */
	b_bp   = swap_in(b_head);
	b_line = b_start = 1;
}


/*
	## write1(c)
	Write one character to the user's file.
	bw_count is the current position in the file buffer.
*/

write1(c)
char c;
{
	b_buff [bw_count++] = c;
	if (bw_count == CPM_SIZE) {
		if (syswrite(b_user_fd, b_buff, 1) != 1) {
			disk_full();
		}
		bw_count = 0;
	}
}


/*
	## wr_flush()
	Flush b_buff to the user's file.
*/

wr_flush()
{
	if (bw_count == 0) {
		return;
	}
	if (syswrite(b_user_fd, b_buff, 1) != 1) {
		disk_full();
	}
}
*/

wr_flush()
{
	if (bw_count == 0) {
		return;
	}
	if (syswrite(b_user_fd, b_buf