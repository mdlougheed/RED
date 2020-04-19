/*
	RED buffer routines -- Full C version (with error file support)
	Part 1 -- goto, output and status routines.

	Error support:  bufinit() calls err_init();

 	Source:  red10.c
	Version: February 4, 1985, November 19, 1985; May 9, 1986

	Copyright (C) 1983, 1984, 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#include "red.h"


/*
	Boundary conditions:

	1.  Only bufins() can extend the buffer, NOT
	    bufgo() and bufdn().

	2.  bufatbot() is true when the current line is
	    PASSED the last line of the buffer.  Both
	    bufgo() and bufdn() can cause bufatbot() to
	    become true.  bufgetln() returns a zero length
	    line if bufatbot() is true.

	3.  b_max_line is the number of lines in the buffer.
	    However, b_line == b_max_line + 1 is valid and
	    it means that b_line points at a null line.

	4.  All buffer routines assume that the variables
            b_bp, b_line and b_start describe the current
	    line when the routine is called.  Thus, any
	    routine which changes the current line must
	    update these variables.
*/


/*
	Define the format of the status line used only by
	the bufsusp() and bufinit() routines.
*/

#define MAGIC 1234
#define STATUS_LENGTH (25+SYSFNMAX)

struct STATUS {
	char stype      [5];	/* Magic byte		*/
	char stail      [5];	/* saved b_tail		*/
	char smax_diskp [5];	/* saved b_max_diskp	*/
	char smax_line  [5];	/* saved b_max_line	*/
	char sfree      [5];	/* saved b_free		*/
	char sfile [SYSFNMAX];	/* saved file name	*/
};


/*
	## bufatbot()
	Return YES if at bottom of buffer (past the last line).
*/

bufatbot()
{
	return (b_line > b_max_line);
}


/*
	## bufattop()
	Return YES if at top of buffer.
*/

bufattop()
{
	return (b_line == 1);
}


/*
	## bufchng()
	Return YES if the buffer has been changed.
*/

bufchng()
{
	return b_cflag;
}


/*
	## buf_clr()
	Clear the slot table.  This should be done after
	read_file() overwrites  the slot table.
*/

buf_clr()
{
	struct BLOCK *bp;
	int i;

	for (i = 0; i < DATA_RES; i++) {
		bp = b_bpp [i];
		bp -> d_back   = ERROR;
		bp -> d_next   = ERROR;
		bp -> d_lines  = 0;
		bp -> d_status = FREE;
		bp -> d_lru    = i;
		bp -> d_diskp  = ERROR;
	}
}


/*
	## bufdn()
	Move towards end of buffer.
*/

bufdn()
{
	if (bufatbot()) {
		return;
	}
	else {
		b_line++;
		buf_gofw();
	}
}


/*
	## bufend()
	Clean up any temporary files.
*/

bufend()
{
	if (b_data_fd != ERROR) {
		sysclose(b_data_fd);	/* Bug fix: 6/17/84 */
		b_data_fd = ERROR;	/* Bug fix: 6/17/84 */
		sysunlink(DATA_FILE);
	}
}


/*
	## bufgo(n)
	Go to line n.
	Set b_bp, b_line, b_start.
*/

bufgo(n)
int n;
{
	struct BLOCK *swap_in();
	int distance, oldline;

	/* Put the request in range. */
	oldline  = b_line;
	b_line   = min(n, b_max_line + 1);
	b_line   = max(1, b_line);
	distance = b_line - oldline;

	if (distance == 0) {

		/* We are already at the requested line. */
		return;
	}
	else if (distance == 1) {

		/* Go forward from here. */
		buf_gofw();
		return;
	}
	else if (distance == -1) {

		/* Go back from here. */
		buf_gobk();
		return;
	}
	else if (distance > 0) {
		if ( b_line >
		     oldline + ((b_max_line - oldline) / 2)
		   ) {

			/* Search back from end of file. */
			b_bp  = swap_in(b_tail);
			b_start =
			    1 + b_max_line - b_bp -> d_lines;
			buf_gobk();
			return;
		}
		else {

			/* Search forward from here. */
			buf_gofw();
			return;
		}
	}
	else {
		if (b_line < oldline / 2) {

			/* Search from start of file. */
			b_bp    = swap_in(b_head);
			b_start = 1;
			buf_gofw();
			return;
		}
		else {

			/* Search back from here. */
			buf_gobk();
			return;
		}
	}
}


/*
	## buf_gobk()
	Search backwards from block for b_line.
	The starting line number of the block is b_start.
	Set b_bp and b_start.
*/

buf_gobk ()
{
	struct BLOCK *swap_in();
	int diskp;

	if (b_bp == ERROR ||
	    b_start < 1 || b_start > b_max_line ||
	    b_line  < 1 || b_line  > b_max_line + 1) {

		cant_happen("buf_gobk 1");
	}

	/* Scan backward for the proper block. */
	while (b_start > b_line) {

		/* Get the previous block in memory. */
		diskp = b_bp -> d_back;
		if (diskp == ERROR) {
			cant_happen("buf_gobk 2");
		}
		b_bp = swap_in(diskp);

		/* Calculate the start of the next block. */
		b_start -= b_bp -> d_lines;
		if (b_start <= 0) {
			cant_happen("buf_gobk 3");
		}
	}
}


/*
	## buf_gofw()
	Search forward from parcel par for line n.
	Set b_bp and b_start.
*/

buf_gofw ()
{
	struct BLOCK *swap_in();
	int diskp;

	/* The last line is always null. */
	if (bufatbot()) {
		return;
	}

	if (b_bp == ERROR ||
	    b_start < b_start ||
	    b_start < 1 || b_start > b_max_line ||
	    b_line  < 1 || b_line  > b_max_line + 1) {

		cant_happen("buf_gofw 1");
	}

	/* Scan forward to the proper block. */
	while (b_start + b_bp -> d_lines <= b_line) {

		/* Get the start of the next block. */
		b_start += b_bp -> d_lines;

		/* Swap in the next block. */
		diskp = b_bp -> d_next;
		if (diskp == ERROR || b_start > b_max_line) {
			cant_happen("buf_gofw 2");
		}
		b_bp = swap_in(diskp);
	}
}


/*
	## bufinit()
	Initialize the buffer module.
	If the work file exists,  read block 0 into slot 0.
	Otherwise,  call bufnew to clear everything.
*/

bufinit()
{
	char *sysalloc();
	struct BLOCK *bp;
	struct STATUS *sp;
	int i, type;
	char *p;

	/*
		11/16/85
		Read error file if it exists.
		Do it now, before buffers are allocated.
	*/
	b_line = 1;
	err_init();

	/* The data file has not been opened yet. */
	b_data_fd = ERROR;

	/* Dynamically allocate all slots. */
	for (i = 0; i < MAX_RES; i++) {

		p = sysalloc (BLOCK_SIZE);
		if (p == 0) {
			break;
		}

#ifdef CAST
		b_bpp [i] = (struct BLOCK *) p;
#else
		b_bpp [i] = p;
#endif

	}

	/* Set pseudo constant. */
	DATA_RES = i;

	/* The code requires at least three buffers. */
	if (DATA_RES < 3) {
		error("Not enough room for buffers.");
		exit();
	}


#ifdef SUSPEND
	/* Do nothing if no work file. */
	if (sysexists(DATA_FILE) == NO) {
		bufnew();
		return;
	}
	else {
		b_data_fd = sysopen(DATA_FILE, 2);
		if (b_data_fd == ERROR) {
			error("Can not re-open work file.");
			exit();
		}
	}

	/* Read the first block of the work file. */
	b_bp = b_bpp [0];
	sysread(b_data_fd, b_bp);

	/*
		Restore RED's status which was written by
		the bufsusp() routine.
	*/
	b_head = 0;

	/* comment out -----
	sscanf(b_bp -> d_data, "%x %x %x %x %x %s\0",
		&type,
		&b_tail, &b_max_diskp, &b_max_line, &b_free,
		g_file);
	b_max_put = b_max_diskp;
	----- end comment out */

#ifdef CAST
	sp = (struct STATUS *) b_bp -> d_data;
#else
	sp = b_bp -> d_data;
#endif

	type        = atoi(sp -> stype);
	b_tail      = atoi(sp -> stail);
	b_max_diskp = atoi(sp -> smax_diskp);
	b_max_line  = atoi(sp -> smax_line);
	b_free      = atoi(sp -> sfree);
	strcpy(g_file, sp -> sfile);

	b_max_put   = b_max_diskp;	/* Bug fix: 5/24/84 */

	if (type != MAGIC) {
		/* Do NOT erase the work file!! */
		error("Previous work file garbled.");
		exit();
	}

	/* Free all slots. */
	buf_clr();

	/* Delete the status line. */
	b_line = b_start = 1;
	swap_in(0);
	bufdel();

	/* Make sure that the buffer will be saved. */
	b_cflag = YES;

	/* Do not erase work file on disk error. */
	b_fatal = NO;
#else
	bufnew();
#endif

#ifdef BDS
#ifndef OLDBDS
	/* KLUDGE:	release the preallocated file buffers so that
			sysfopen() and sysfcreat() can work.
	*/
	free(sysbuf);
#endif
#endif

}


/*
	## bufln()
	Return the current line number.
*/

bufln()
{
	return b_line;
}


/*
	Return the maximum line number.
*/

bufmax()
{
	return b_max_line;
}


/*
	## bufnew()
	Clear the buffer module.
*/

bufnew()
{
	struct BLOCK *bp;

	/* Free all slots. */
	buf_clr();

	/* Allocate the first slot. */
	b_bp        = b_bpp [0];
	b_head      = 0;
	b_tail      = 0;
	b_max_diskp = 0;
	b_max_put   = 0;
	b_bp -> d_diskp  = 0;
	b_bp -> d_status = DIRTY;

	/* Make sure temp file is erased. */
	if (b_data_fd != ERROR) {
		sysclose(b_data_fd);
		b_data_fd = ERROR;
		sysunlink(DATA_FILE);
	}
	b_free = ERROR;

	/* Set the current and last line counts. */
	b_line     = 1;
	b_max_line = 0;
	b_start    = 1;

	/* Indicate that the buffer has not been changed */
	b_cflag = NO;

	/* Do not erase work file on disk error. */
	b_fatal = NO;
}


/*
	## bufnrbot()
	Return YES if buffer is near the bottom line.
*/

bufnrbot()
{
	return (b_line >= b_max_line);
}


/*
	## bufout(topline, topy, nlines)
	Put nlines lines from buffer starting with line topline at
	position topy of the screen.
*/

bufout(topline, topy, nlines)
int topline, topy, nlines;
{
	int l, x, y;

	x = outgetx();
	y = outgety();
	l = b_line;

	while (nlines > 0) {
		outxy(0, topy++);
		bufoutln(topline++);
		nlines--;
		if (hasint == YES) {
			sysintr(topline,topy,nlines);
			break;
		}
	}
	outxy(x,y);
	bufgo(l);
}
	

/*
	## bufoutln(line)
	Print one line on screen.
*/

bufoutln(line)
int line;
{
	char buffer [MAXLEN1];
	int n;

	bufgo(line);

	if ( (b_max_line == 0 && line == 2) ||
	     (b_max_line >  0 && line == b_max_line + 1)) {
		fmtsout("---------------- End of file. ----------------",0);
		outdeol();
	}
	else if (line > b_max_line) {
		outdeol();
	}
	else {
		n = bufgetln(buffer, MAXLEN);
		n = min(n, MAXLEN);
		buffer [n] = NEWLINE;
		fmtsout(buffer, 0);
		outdeol();
	}
}



/*
	#bufrepl(line, n)
	Replace current line with the line that p points to.
	The new line is of length n.
*/

bufrepl(line, n)
char line [];
int n;
{
	/* Do not replace null line.  Just insert. */
	if (bufatbot()) {
		bufins(line, n);
		return;
	}

	/* Do not combine blocks until after insertion. */
	buf_d(1);
	bufins(line, n);
	combine();
}


/*
	## bufreset(window_file)
	Save the work file in a temporary file in preparation
	for changing windows on the screen.

	NOTE:  This routine is not used at present.
*/

bufreset(window_file)
char window_file;
{
	/* Make sure the work file is written. */
	swap_all();

	/* Close the work file. */
	sysclose(b_data_fd);
	b_data_fd = ERROR;
	
	/* Rename the work file to be the window file. */
	sysrename(DATA_FILE, window_file);
}


/*
	## bufsaved()
	Indicate that the file has been saved.
*/

bufsaved()
{
	b_cflag = NO;
}


/*
	## bufsusp()
	Suspend RED's execution for restart later.
*/

#ifdef SUSPEND
bufsusp()
{
	struct STATUS *sp;
	int length, i;
	char line [MAXLEN];

	/* Bug fix 2/4/85: make sure the data file is open. */
	if (b_data_fd == ERROR) {
		b_data_fd = data_open();
	}

	/*
		Allocate space for the line.
		(This will ALWAYS be in block 0.)
	*/
	bufgo(1);
	for (i = 0; i < STATUS_LENGTH; i++) {
		line [i] = ' ';
	}
	bufins(line, STATUS_LENGTH);

	/*
		Set up the file status line.
		The bufinit() routines reads this line.

		The call to sprintf will also work but takes
		up about 400 hex bytes more space.
	*/

	/* comment out ----- (sprintf is a BIG function.)
	sprintf(line, "%4x %4x %4x %4x %4x %s\0",
		MAGIC, b_tail, b_max_diskp, b_max_line,
		b_free, g_file);
	----- end comment out */

#ifdef CAST
	sp = (struct STATUS *) line;
#else
	sp = line;
#endif

	length = itoc(MAGIC, sp -> stype, 5);
	sp -> stype [length] = EOS;

	length = itoc(b_tail, sp -> stail, 5);
	sp -> stail [length] = EOS;

	length = itoc(b_max_diskp, sp -> smax_diskp, 5);
	sp -> smax_diskp [length] = EOS;

	length = itoc(b_max_line, sp -> smax_line, 5);
	sp -> smax_line [length] = EOS;

	length = itoc(b_free, sp -> sfree, 5);
	sp -> sfree [length] = EOS;

	strcpy(sp -> sfile, g_file);

	/*
		Rewrite the status line WITHOUT changing
		the disk status.  This is fairly tricky;
		a call to bufrepl() here would not work.
	*/
	sysmove(line, b_bp -> d_data, STATUS_LENGTH);
	
	/* Make sure that work file is completely written. */
	swap_all();
	sysclose(b_data_fd);
}
#endif


/*
	## bufup()
	Move towards the head of the file.
*/

bufup()
{
	if (bufattop()) {
		return;
	}
	else {
		b_line--;
		buf_gobk();
	}
}
owards the he