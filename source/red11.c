/*
	RED buffer routines -- Full C version
	Part 2 -- line routines.

	Source:  red11.c
	Version: September 18, 1983;  February 20, 1984

	Copyright (C) 1983, 1984 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#include "red.h"

                  
/*
	## b_getnum(char_ptr)
	Get an integer given a pointer to char.

	## b_putnum(char_ptr)
	Put an integer given a pointer to char.

	W A R N I N G:  These routines are used to get and
	put entries in the index table.  Make SURE that
	the INT_SIZE constant does, in fact,  correspond
	to the size on an int.   If it does not,  the buffer
	routines will die a horrible death.
*/

int
b_getnum(p)
char *p;
{

#ifdef CAST
	return * (int *) p;
#else
	int *ip;

	ip = p;
	return *ip;
#endif

}

b_putnum(p, num)
char *p;
int num;
{

#ifdef CAST
 	* (int *) p = num;
#else
	int *ip;

	ip = p;
	*ip = num;
#endif

}


/*
	The following routines manage the index table,
	which grows down from the end of each block.

	## b_tab(bp, index)
	## b1_tab(index)
	Get an entry of the index table.

	## b_settab(bp, index, value)
	## b1_settab(index, value)
	Set an entry of the index table.
*/

int
b1_tab(index)
int index;
{
	return b_tab(b_bp, index);
}

int
b_tab(bp, index)
struct BLOCK *bp;
int index;
{
	return b_getnum(
		bp->d_data + BUFF_SIZE - (INT_SIZE*(index+1)));
}

b1_settab(index, value)
int index, value;
{
	b_settab(b_bp, index, value);
}

b_settab(bp, index, value)
struct BLOCK  *bp;
int index, value;
{
	b_putnum(bp->d_data + BUFF_SIZE - (INT_SIZE*(index+1)),
		 value);
}


/*
	## b_len(bp, line)
	## b1_len(line)
	Return the length of a line.

	## b_ptr(bp, line)
	## b1_ptr(line)
	Return a pointer to a line.

	## b_prefix(bp, line)
	## b1_prefix(line)
	Return the number of chars befor the start of a line.
*/

int
b1_length(line)
int line;
{
	return b_length(b_bp, line);
}

int
b_length(bp, line)
struct BLOCK *bp;
int line;
{
	if (line < 0 || line >= bp -> d_lines) {
		cant_happen("b_len");
	}

	if (bufatbot()) {
		/* The last line is always null. */
		return 0;
	}
	else if (line == 0) {
		return b_tab(bp, line);
	}
	else {
		return b_tab(bp, line) - b_tab(bp, line - 1);
	}
}
		
char *
b1_ptr(line)
{
	char *b_ptr();

	return b_ptr(b_bp, line);
}

char *
b_ptr(bp, line)
struct BLOCK  *bp;
int line;
{
	if (line < 0 || line >= bp -> d_lines) {
		cant_happen("b_ptr");
	}
	if (line == 0) {
		return bp -> d_data;
	}
	else {
		return bp -> d_data + b_tab(bp, line - 1);
	}
}

int
b1_prefix(line)
int line;
{
	return b_prefix(b_bp, line);
}

int
b_prefix(bp, line)
struct BLOCK *bp;
int line;
{
	if (line < 0 || line >= bp -> d_lines) {
		cant_happen("b_prefix");
	}
	if (line == 0) {
		return 0;
	}
	else {
		return b_tab (bp, line - 1);
	}
}


/*
	The following routines make accessing fields of blocks
	a bit easier.  They also save a few bytes of memory.

	## b1_nlines()
	## b_nlines(bp)
	Return the number of lines in a block.

	## b1_avail()
	## b1_avail(bp)
	Return the number of free characters in a block.
*/

int
b1_nlines()
{
	return b_bp -> d_lines;
}

int
b_nlines(bp)
struct BLOCK *bp;
{
	return bp -> d_lines;
}

int
b1_avail()
{
	return b_avail(b_bp);
}

int
b_avail(bp)
struct BLOCK *bp;
{
	if (bp -> d_lines == 0) {
		return BUFF_SIZE;
	}
	else {
		return BUFF_SIZE - b_tab(bp, bp->d_lines-1) -
		       (INT_SIZE * bp->d_lines);
	}
}


/*
	## bufdel()
	Delete the current line.
*/

bufdel()
{
	buf_d(1);
	combine();
}


/*
	## bufdeln(n)
	Delete multiple lines starting with the current line.
	nlines is the number of lines to delete.
*/

bufdeln(nlines)
int nlines;
{
	buf_d(nlines);
	combine();
}
	

/*
	## buf_d(n)
	Internal delete routine.  

	This routine does not combine blocks so that bufrepl()
	can wait to the last possible moment to call combine().
*/

buf_d(nlines)
int nlines;
{
	int lines, slines;

	while (nlines > 0 && !bufatbot()) {

		/* The current block will become dirty. */
		is_dirty(b_bp);
		b_cflag = YES;

		/*
			Precompute the number of lines in the
			block and the number of lines befor
			the current line.
		*/

		lines  = b_bp -> d_lines;
		slines = b_line - b_start;

		/* Delete lines block by block. */
		if (slines == 0 && nlines >= lines) {

			/* Delete the whole block. */
			del_bl(lines);
			nlines -= lines;
			check_block("delete 1");
		}
		else if (nlines >= lines - slines) {

			/* Delete tail of the block. */
			del_tail(slines, lines - slines);
			nlines -= (lines - slines);
			check_block("delete 2");
		}
		else {
			/* Delete from middle of the block. */
			del_mid(nlines);
			check_block("delete 3");
			break;
		}
	}
}


/*
	## del_bl(nlines)
	Delete the current block containing 'nlines' lines.

	There is much more to this code than meets the eye.
	First,  notice that using combine() here would slow
	down the a  multiple-line  delete  much  too  much.
	Second,  we never want to call free_block() for the
	first block since we want any recovery  program  to
	find  the  first block of the work file at block 0.
	Third,  the  call  to  combine()  at the end of the
	bufdeln() routine will eventually fill up  block  0
	unless the entire file is deleted.
*/

del_bl(nlines)
int nlines;
{
	struct BLOCK *bp1, *swap_in(), *oldblock;
	int back, next, current;

	/* Point to the previous and next blocks. */
	back    = b_bp -> d_back;
	next    = b_bp -> d_next;
	current = b_bp -> d_diskp;

	/* Remember the current slot. */
	oldblock = b_bp;

	/* Special case block 0. */
	if (current == 0) {

		/* Zero the block but do not actually delete. */
		b_bp -> d_lines = 0;
		is_dirty(oldblock);

		/* Move to next block. */
		if (next != ERROR) {
			b_bp = swap_in(next);
		}
		b_start = b_line = 1;
	}
	else if (next != ERROR) {

		/* Move to the START of the next block. */
		b_bp    = swap_in(next);
		b_start = b_line;

		/* Adjust back pointer of new block. */
		b_bp -> d_back = back;
		is_dirty(b_bp);

		/* Adjust next pointer. */
		bp1 = swap_in(back);
		bp1 -> d_next = next;
		is_dirty(bp1);

		/* Actually delete the old block. */
		free_block(oldblock);
	}
	else if (back != ERROR) {

		/*
			Move the the END of the previous block.
			Set bufatbot() true.
		*/
		b_bp     = swap_in(back);
		b_start -= b_bp -> d_lines;
		b_line   = b_start + b_bp -> d_lines;

		/* Adjust forward pointer of new block. */
		b_bp -> d_next = next;
		is_dirty(b_bp);

		/* Adjust pointer to the last block. */
		b_tail = back;

		/* Actually delete the old slot. */
		free_block(oldblock);
	}
	else {
		/* Only block 0 has both links == ERROR. */
		cant_happen("del_bl");
	}

	/* Adjust total number of lines. */
	b_max_line -= nlines;
}


/*
	## del_mid(dlines)
	Delete dlines from the current block.
	There is at least one line after the deleted lines.
*/

del_mid(dlines)
int dlines;
{
	char * source, * dest;
	int  i, length, limit, line, nlines, offset;

	/* Compute some constants. */
	line   = b_line - b_start;
	nlines = b_bp -> d_lines;

	/* Compress the block. */
	source = b1_ptr(line + dlines);
	dest   = b1_ptr(line);
	length = b1_tab(nlines - 1) - b1_tab(line + dlines - 1);
	sysmove(source, dest, length);

	/* Compress the index table. */
	offset = b1_prefix(line + dlines) - b1_prefix(line);
	limit  = nlines - dlines;
	for (i = line; i < limit; i++) {
		b1_settab(i, b1_tab(i + dlines) - offset);
	}

	/* Adjust the counts. */
	b_bp -> d_lines -= dlines;
	b_max_line      -= dlines;
}


/*
	## del_tail(slines, nlines)
	Delete the current line and all followings lines in
	the current block.
	slines is the number of preceeding lines in the block.
*/

del_tail(slines, nlines)
int slines, nlines;
{
	int next;
	struct BLOCK *swap_in();

	/* Adjust the line count. */
	b_bp -> d_lines = slines;
	is_dirty(b_bp);

	next = b_bp -> d_next;
	if (next != ERROR) {
		/* Move the current block forward. */
		b_bp    = swap_in(next);
		b_start = b_line;
	}

	/* Adjust the total number of lines. */
	b_max_line -= nlines;
}


/*
	## bufgetln(line, linelen)
	Copy the current line from the buffer to line [].
	The size of line [] is linelen.
	Return k = the length of the line.
	If k > linelen then truncate k - linelen characters.
*/

bufgetln(line, linelen)
char	*line;
int	linelen;
{
	int	count, limit;
	char	*src;

	/* Return null line at the bottom of the buffer. */
	if (bufatbot()) {
		line [0] = NEWLINE;
		return 0;
	}

	/* Copy line to buffer. */
	src   = b1_ptr    (b_line - b_start);
	count = b1_length (b_line - b_start);
	limit = min(count, linelen - 1);
	sysmove(src, line, limit);
	
	/* End with NEWLINE. */
	line [limit] = NEWLINE;

	/* Return the number of characters in the line. */
	return count;
}


/*
	## bufins(insline, inslen)
	Insert line before the current line.  Thus, the line
	number of the current line does not change.  The line
	ends with NEWLINE.

	This is fairly crude code, as it can end up splitting
	the current block into up to three blocks.  However,
	the combine() routine does an effective job of keeping
	the size of the average block big enough.
*/

bufins(insline, inslen)
char insline [];
int  inslen;
{
	struct BLOCK *bp2, *split_block();
	char *dest, *source;
	int  i, length, line, nlines, prefix;

	if (inslen > BUFF_SIZE) {
		cant_happen("bufins 1");
	}

	/* See if the new line will fit. */
	if (inslen + INT_SIZE > b1_avail()) {

		/* Split off the trailing lines. */
		bp2 = split_block();

		/* See if there is enough room in the old block. */
		if (inslen + INT_SIZE > b1_avail()) {

			/* Move on to the next block. */
			b_start += b_bp -> d_lines;
			b_bp     = bp2;

			/*
			At this point,  the new line is the
			the first line of the block.  Alas,
			we may still have to split off the
			trailing lines so the new line will fit.
			*/

			if (inslen + INT_SIZE > b1_avail()) {

				/* Split the block a second time. */
				split_block();
			}
		}
	}

	if (inslen + INT_SIZE > b1_avail()) {
		cant_happen("bufins 2");
	}


	/*
		At this point,  we know that the new line can
		be inserted before the current line with room
		for any following lines.
	*/

	is_dirty(b_bp);
	line   = b_line - b_start;
	nlines = b_bp -> d_lines;

	if (nlines == 0) {

		/* Copy line to empty block. */
		sysmove(insline, b_bp -> d_data, inslen);

		/* Create an index table. */
		b1_settab(0, inslen);
	}
	else if (line >= nlines) {

		/* Copy line to end of block. */
		sysmove(insline,
			b_bp -> d_data + b1_tab(nlines - 1),
			inslen);

		/* Append index to index table. */
		b1_settab(nlines, b1_tab(nlines - 1) + inslen);
	}
	else {

		/* Make a hole at the current line. */
		source = b1_ptr(line);
		dest   = b1_ptr(line) + inslen;
		prefix = b1_prefix(line);
		length = b1_tab(nlines - 1) - prefix;
		sysmove(source, dest, length);

		/* Copy the new line into the hole. */
		sysmove(insline, source, inslen);

		/* Make a hole in the index table. */
		for (i = nlines; i > line; i--) {
			b1_settab(i, b1_tab(i - 1) + inslen);
		}

		/* Copy the new index into the hole. */
		b1_settab(line, prefix + inslen);
	}
		
	/*
		Special case: inserting a null line at the
	 	end of the file is not a significant change.
	*/
	if ( ! (inslen == 0 && bufnrbot()) ) {
		b_cflag = YES;
	}

	/* Bump the counts. */
	b_bp -> d_lines++;
	b_max_line++;

	/*
		It DOES make sense to call combine here.
		In the most common case,  after a split the
		current block might be small enough to be
		combined with the preceding block.
	*/
	combine();

	/* Check the format of the block. */
	check_block("bufins");
}


/*
	## combine()
	Combine the current block with the preceeding and
	following blocks if possible.
	Make the new block the current bloc.
*/

combine()
{
	_combine(b_bp -> d_back,  b_bp -> d_diskp);
	_combine(b_bp -> d_diskp, b_bp -> d_next);
}


/*
	## _combine(diskp1, diskp2)
	Combine two blocks into one if possible.
	Make the new block the current block.

	Note that nline1 can be 0 because of the special case
	code in the del_bl() routine.  nline2 is never 0.
*/

_combine(diskp1, diskp2)
int diskp1, diskp2;
{
	struct BLOCK *bp1, *bp2, *bp3, *swap_in();
	char	*source, *dest;
	int	i, length, limit, nlines1, nlines2, offset;

	/* Make sure the call makes sense. */
	if (diskp1 == ERROR || diskp2 == ERROR) {
		return;
	}

	/* Get the two blocks. */
	bp1 = swap_in(diskp1);
	bp2 = swap_in(diskp2);

	if ( bp1 -> d_next != diskp2 ||
	     bp2 -> d_back != diskp1
	   ) {
		cant_happen("combine 1");
	}
	
	/* Do nothing if the blocks are too large. */
	if (b_avail(bp1) + b_avail(bp2) < BUFF_SIZE) {
		return;
	}

	/* Compute the number of lines in each block. */
	nlines1 = bp1 -> d_lines;
	nlines2 = bp2 -> d_lines;
	if (nlines2 <= 0) {
		cant_happen("combine");
	}

	/* Copy buffer 2 to end of buffer 1. */

	source = bp2 -> d_data;
	if (nlines1 == 0) {
		dest   = bp1 -> d_data;
		offset = 0;
	}
	else {
		dest   = bp1 -> d_data + b_tab(bp1, nlines1-1);
		offset = b_tab(bp1, nlines1 - 1);
	}
	length = b_tab(bp2, nlines2 - 1);
	sysmove(source, dest, length);

	/* Copy table 2 to table 1. */
	for (i = 0; i < nlines2; i++) {
		b_settab(bp1, i + nlines1,
			 b_tab(bp2, i) + offset);
	}

	/* Both blocks are now dirty. */
	is_dirty(bp1);
	is_dirty(bp2);

	/* Adjust the back pointer of the next block. */
	if (bp2 -> d_next != ERROR) {
		bp3 = swap_in(bp2 -> d_next);
		bp3 -> d_back = bp1 -> d_diskp;
		is_dirty(bp3);
	}

	/*
		Adjust the current block if needed.
	 	The value of b_start must be decremented
	 	by the OLD value of bp1 -> d_lines.
	*/

	if (b_bp == bp2) {
		b_bp     = bp1;
		b_start -= bp1 -> d_lines;
	}

	/* Adjust the header for block 1. */
	bp1 -> d_lines += bp2 -> d_lines;
	bp1 -> d_next   = bp2 -> d_next;

	/* Adjust the pointers to the last block. */
	if (diskp2 == b_tail) {
		b_tail = diskp1;
	}

	/* Slot 2 must remain in core until this point. */
	free_block(bp2);

	/* Check the format of the block. */
	check_block("combine");
}


/*
	## free_block(block)
	Put the block in the slot on the free list.
*/

free_block(bp)
struct BLOCK * bp;
{
	/*
		Each block in the free list contains the
		pointer to the next block in the d_next field.
	*/
	bp -> d_next = b_free;
	b_free = bp -> d_diskp;

	/* Erase the block. */
	bp -> d_lines = 0;

	/* Make sure the block is rewritten. */
	is_dirty(bp);
}


/*
	## new_block(blockp)
	Create a new block linked after the current block.
	Return a pointer to the new block.
*/

struct BLOCK *
new_block (blockp)
{
	struct BLOCK *bp1, *bp2, *swap_in(), *swap_new();
	int diskp;

	/* Get a free disk sector. */
	if (b_free != ERROR) {

		/* Take the first block on the free list. */
		diskp = b_free;

		/* Put the block in a free slot. */
		bp1 = swap_in(diskp);

		/* Adjust the head of the free list. */
		b_free = bp1 -> d_next;
	}	
	else {
		/* Get a free slot. */
		diskp = ++b_max_diskp;
		bp1   = swap_new(diskp);
	}

	/* Link the new block after the current block. */
	bp1  -> d_next = b_bp -> d_next;
	bp1  -> d_back = b_bp -> d_diskp;
	b_bp -> d_next = diskp;
	if (bp1 -> d_next != ERROR) {
		bp2 = swap_in(bp1 -> d_next);
		bp2 -> d_back = diskp;
		is_dirty(bp2);
	}
	
	/* The block is empty. */
	bp1 -> d_lines = 0;
	is_dirty(bp1);

	/* Return a pointer to the new block. */
	return bp1;
}


/*
	## split_block()
	Split the current block before the current line.
	The current line, and following lines are put in
	a new block.
*/

struct BLOCK *
split_block()
{
	struct	BLOCK *bp2;
	char	*dest, *source;
	int	nlines, nlines1, nlines2;
	int	i, length, line, offset;

	/* Create a new block. */
	bp2 = new_block();

	/* Mark both blocks as dirty. */
	is_dirty(b_bp);
	is_dirty(bp2);

	/*
		Count the lines in each block.
		Adjust nline1 and nlines1 if bufatbot().	
	*/
	line    = b_line - b_start;
	nlines  = b_bp -> d_lines;
	nlines1 = min(nlines, b_line - b_start);
	nlines2 = max(0, nlines - nlines1);

	/* Copy data area to new block. */
	if (!bufatbot()) {
		offset = b1_prefix(line);
		source = b_bp -> d_data + offset;
		dest   = bp2  -> d_data;
		length = b1_tab(nlines - 1) - offset;
		sysmove(source, dest, length);
	}

	/* Copy index to new block. */
	for(i = 0; i < nlines2; i++) {
		b_settab(bp2, i, b1_tab(i + nlines1) - offset);
	}

	/* Adjust the headers. */
	bp2  -> d_lines = nlines2;
	b_bp -> d_lines = nlines1;

	/* Adjust the pointer to the last block. */
	if (b_bp -> d_diskp == b_tail) {
		b_tail = bp2 -> d_diskp;
	}

	/* Return a pointer to the new block. */
	return bp2;
}
f (b_bp -> d_diskp == b_tail) {
		b_tail = bp2 -> d_diskp;
	}

	/* Return a pointe