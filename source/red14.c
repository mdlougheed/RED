/*
	RED -- Error Handling Module

	Source: red14.c
	Started: November 15, 1985
	Version: May 9, 1986

	Copyright (C) 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/
#include "red.h"

/*
	Initialize the err_num[] and err_ptr[] tables from the error file.
*/
err_init()
{
	FILE *fd, *sysfopen();
	char *sysalloc();

#ifdef OLDBDS
	char filebuf[BUFSIZ];
#else
	char filebuf[1];	/* dummy */
#endif

	char buffer[MAXLEN];
	char err_count[10];
	int n;
	char c, *p, *p2, *ptr;

	/* No entries yet in the error tables. */
	err_line = -1;
	err_max  = 0;

	/* Do nothing if no error file. */
	fd = sysfopen(ERR_FILE, filebuf);
	if (fd == NULL) {
		return;
	}

	/* Read each error record in turn. */
	while ((n = sysfgets(fd, buffer, MAXLEN)) >= 0) {
		if (err_max >= MAX_ERR) {
			pmtmess("additional errors not shown", "");
			syscin();
			goto close;
		}
		if (n > MAXLEN) {
			pmtmess("line truncated.", "");
			syscin();
			n = MAXLEN - 1;
		}
		buffer [n] = '\0';

		/* Scan for the end of the file name. */
		p = &buffer;
		for (p2 = p; ;p2++) {
			if (!*p2) {
				pmtmess("bad error file", "");
				syscin();
				goto close;
			}
			if (*p2 == ':' && *(p2+1) == ' ') {
				*p2++ = '\0';
				break;
			}
		}

		/* Point err_ptr[] at a copy of the file name. */
		ptr = sysalloc(strlen(p) + 1);
		strcpy(ptr, p);
		err_fptr[err_max] = ptr;
	
		/* The first file name becomes the global file. */
		if (err_max == 0) {
			strcpy(g_file, p);
		}

		for (p2++,p = p2; ;p2++) {
			c = *p2;
			if (*p2 == ':' && *(p2+1) == ' ') {
				*p2++ = '\0';
				break;
			}
			else if (c < '0' || c > '9') {
				pmtmess("bad error file", "");
				syscin();
				goto close;
			}
		}

		/* Put the line number in err_num[]. */
		err_num [err_max] = atoi(p);

		/* Point err_ptr[] at a copy of the error message. */
		ptr = sysalloc(strlen(p2) + 1);
		strcpy(ptr, p2);
		err_ptr[err_max] = ptr;
		err_max++;
	}

 	itoc(err_max, err_count, 10);	
	if (err_max == 1) {
		pmtmess("1 error during compilation","");
	}
	else {
		pmtmess(err_count, "errors during compilation");
	}
	syscin();

close:
	sysfclose(fd);
}

/*
	Return the pointer to the message for the current error.
*/
char *
err_msg()
{
	if (err_line >= err_max || err_line < 0) {
		return 0;
	}
	else {
		return err_ptr[err_line];
	}
}

/*
	Update line numbers as the result of a block move.

	The append() routine has already copied the block and
	adjusted numbers accordingly.
*/
err_move(fstart, fend, tstart)
int fstart, fend, tstart;
{
	int length;
	int i, entry;

	length = fend - fstart + 1;
	
	/* Adjust the lines in the moved block. */
	if (tstart > fstart) {
		for (i = 0; i < err_max; i++) {
			entry = err_num[i];
			if (entry >= fstart && entry < fstart + length) {
				/* In moved block. */
				err_num[i] = entry + (tstart-fend) ;
			}
			else if (entry >= fstart + length && entry <= tstart) {
				err_num[i] = entry - length;
			}
			else if (entry > tstart + length) {
				err_num[i] = entry - length;
			}
		}
	}
	else {
		for (i = 0; i < err_max; i++) {
			entry = err_num[i];
			if (entry >= fstart + length &&
			    entry < fstart + length + length) {
				/* In moved block. */
				err_num[i] = entry+tstart-fstart-length+1;
			}
			else if (entry >= fstart + length + length) {
				err_num[i] = entry - length;
			}
		}
	}
}

/*
	Adjust err_num[] to reflect n lines inserted after the indictated line.
*/
err_ins(line, count)
{
	int i;

	for (i = 0; i < err_max; i++) {
		if (err_num[i] > line) {
			err_num[i] += count;
		}
	}
}

/*
	Adjust err_num[] to reflect n deleted lines.
	Do NOT adjust line numbers for supposedly deleted lines.
*/
err_del(line, n)
{
	int i;

	for (i = 0; i < err_max; i++) {
		if (err_num[i] >= line + n) {
			err_num[i] -= n;
		}
	}
}

/*
	Advance to the next line in error in the current file.
*/
err_next()
{
	for (;;) {
		if (err_line >= err_max) {
			return;
		}
		if (++err_line >= err_max) {
			return;
		}
		if (streq_(g_file, err_fptr [err_line])) {
			return;
		}
	}
}

/*
	Go the the next line in error in the current file.
*/
err_prev()
{
	for(;;) {
		if (err_line < 0) {
			return;
		}
		if (--err_line < 0) {
			return;
		}
		if (streq_(g_file, err_fptr [err_line])) {
			return;
		}
	}
}

/*
	Compare two strings but ignoring case.
*/
streq_(s1, s2)
char *s1, *s2;
{
	for (;;) {
		if (!*s1) {
			return !*s2;
		}
		else if (tolower(*s1) != tolower(*s2)) {
			return 0;
		}
		else {
			s1++;
			s2++;
		}
	}
}

/*
	Return the line number of the current error.
*/
int
err_this()
{
	if (err_line >= err_max) {
		return -2;
	}
	else if (err_line < 0) {
		return -1;
	}
	else {
		return err_num[err_line];
	}
}
ine >= err_max) {
		return -2;
	}
	else if (err_line < 0)