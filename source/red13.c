/*
	RED buffer routines -- Full C version
	Part 4 -- debugging routines.

	Source:  red13.c
	Version: December 3, 1984; May 14, 1985

	Copyright (C) 1983, 1984 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#include "red.h"
#define PRINTER stdout
#undef DEBUG


/*
	The observer effect:  None of the routines of this file
	should ever call swap_in() because swap_in() causes
	all kinds changes to the data structures which these
	routines are trying to print out.
*/


/*
	Dump global variables, all resident slots and
	the current block.
*/

#ifdef DEBUG
bufdump()
{
	outxy(0,SCRNL1);
	dump_vars();
	dump_memory();
	dump_block(b_bp);

	pmtzap();
	pmtupd();
	syscin();
	pmtzap();
	edclr();
	edgo(bufln(), 0);
}
#endif

/*
	System error routine.
*/

cant_happen(message)
char *message;
{
	pmtmess(message, ":can't happen");
	/* comment out -----
	bufdump();
	----- end comment out */
	exit();
}


/*
	Check the current block for consistency.
*/

check_block(message)
char *message;
{
#ifdef DEBUG
	int avail, i, nlines, total;

	nlines = b_bp -> d_lines;
	avail  = b1_avail();

	if (b_bp == ERROR ||
	    b_line < 0 || b_line > b_max_line + 1) {

		error("In check block 1.");
		cant_happen(message);
	}

	if ( nlines < 0 ||
	     nlines >= BUFF_SIZE ||
	     avail  < 0
	   ) {

		error("In Check block 2.");
		cant_happen(message);
	}

	/* Make sure there are at least enough lines. */
	for (i = 0; i < nlines; i++) {
		total = b1_tab(i);
		if (total < 0 || total > BUFF_SIZE - avail) {

			error("In check block 3.");
			cant_happen(message);
		}
	}
#endif
}


/*
	Dump the current block.
*/

#ifdef DEBUG
dump_block(bp)
struct BLOCK *bp;
{
	char *buffer;
	int  c, count, i, j, limit, nlines, offset;

#ifdef AZTEC
	FILE *PRINTER;

	PRINTER = fopen("con:", "w");
#endif

	nlines = bp -> d_lines;
	buffer = bp -> d_data;
        offset = 0;
	
	for (i = 0; i < nlines; i++) {

		limit = b_tab(bp, i);
		count = limit - offset;

		fprintf(PRINTER,
		"line %3d, offset %3d, length %3d, total %3d:  ",
		i + 1, offset, count, limit);

		if (count < 0) {
			return;
		}

		if (count >= 50) {
			fprintf(PRINTER, "\n");
		}
		for (j = 0; j < count && j < 80; j++) {
			c = buffer [offset + j] & 127;
			if (c == '\t') {
				fprintf(PRINTER, " ");
			}
			else if (c < 32) {
				fprintf(PRINTER,"^%c", c + 64);
			}
			else {
				fprintf(PRINTER, "%c", c);
			}
		}
		fprintf(PRINTER, "\n");
		offset = limit;
	}

#ifdef AZTEC
	fclose(PRINTER);
#endif

}
#endif DEBUG

/*
	Dump all the resident slots.
*/

#ifdef DEBUG
dump_memory()
{
	struct BLOCK *bp;
	int i;

#ifdef AZTEC
	FILE *PRINTER;

	PRINTER = fopen("con:", "w");
#endif

	for (i = 0; i < DATA_RES; i++) {
		bp = b_bpp [i];

		fprintf(PRINTER, "slot %2d, ", i);

#ifdef HAS_LONG
		fprintf(PRINTER,
		"address %4lx, back %3d, diskp %3d, next %3d, ",
		bp, bp -> d_back, bp -> d_diskp, bp -> d_next);
#else
		fprintf(PRINTER,
		"address %4x, back %3d, diskp %3d, next %3d, ",
		bp, bp -> d_back, bp -> d_diskp, bp -> d_next);
#endif

		fprintf(PRINTER,
		"lines %3d, lru %3d, status %3d, avail %3d\n",
		bp -> d_lines, bp -> d_lru,  bp -> d_status,
		b_avail(bp));
	}

#ifdef AZTEC
	fclose(PRINTER);
#endif

}
#endif DEBUG

/*
	Dump all global variables.
*/

#ifdef DEBUG
dump_vars()
{

#ifdef AZTEC
	FILE *PRINTER;

	PRINTER = fopen("con:", "w");
#endif

	fprintf(PRINTER,
	"start %d line %d, maxline %d\n",
	b_start, b_line, b_max_line);

	fprintf(PRINTER,
	"head %d, tail %d, free %d, max_diskp %d\n",
	b_head, b_tail, b_free, b_max_diskp);

#ifdef HAS_LONG
	fprintf(PRINTER,
	"address %lx, back %d, diskp %d, next %d, avail %d, ",
	b_bp, b_bp -> d_back,  b_bp -> d_diskp,
	b_bp -> d_next, b1_avail());
#else
	fprintf(PRINTER,
	"address %x, back %d, diskp %d, next %d, avail %d, ",
	b_bp, b_bp -> d_back,  b_bp -> d_diskp,
	b_bp -> d_next, b1_avail());
#endif

	fprintf(PRINTER,
	"lines %d, lru %d, status %d\n",
	b_bp -> d_lines, b_bp -> d_lru,  b_bp -> d_status);

#ifdef AZTEC
	fclose(PRINTER);
#endif

}
#endif DEBUG
us %d\n",
	b_bp -> d_lines, b_bp -> d_lru,  b_bp -> d_status);

#ifdef AZTEC
	fcl