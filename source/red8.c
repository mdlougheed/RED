/*
	RED operating system module -- Full C version

	Source:  red8.c
	Version: February 7, 1985; May 16, 1986
		 Kaypro support added: October 12, 1984

	Copyright (C) 1983, 1984, 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed NOV-2020
*/

#include "red.h"

/* comment out -----
#define KAYPRO 1
----- end comment out */

/*
	NOTE:  This module should contain ALL routines that
	might have to be changed for different compilers or
	different operating systems.  Please let me know if
	you have to make a change to a routine that is  NOT
	in this module  -- that's a portability bug.

	Some routines in this module probably will probably
	work regardless of operating system.  These I have
	called PORTABLE routines.
*/

/*
	Initialize the system module.

	The storage allocation routines are a real problem.
	The AZTEC and DRI compilers completely botch it.
*/
sysinit()
{
	/*
		System dependent part.
		This code sets the "moat" (in Unix terminology),
		i.e., the amount of space that is guarenteed for
		use by the stack by the memory allocation routines.
		If you don't reserve enough space for the stack
		the inject, extract, move and copy commands will crash.
	*/

#ifdef BDS
	FILE *kludge;
	char * alloc();

	_allocp = NULL;		/* for alloc() */
	rsvstk(2000);		/* 10/4/83 */

#ifndef OLDBDS
	/*
		KLUDGE!!  Allocate space for two file buffers so we can give
		them back to the system after allocating file buffers.
	*/
	sysbuf = alloc(sizeof (*kludge) + sizeof (*kludge));
#endif
#endif

	/*
		PORTABLE part.
	*/
	sysnl    = 0;
	sysccnt  = 0;
	sysrcnt  = 0;
	syslastc = 0;
}

/*
	Return a pointer to a block of n contiguous bytes.
	Return NULL (i.e.,  0) if fewer than n bytes remain.
*/
char *
sysalloc(n)
int n;
{

#ifdef BDS
	char * alloc();
	return alloc(n);
#else
	return 0;
#endif

}

/*
	Move a block of memory.

	This code MUST work regardless of whether the
	source pointer is above or below the dest pointer.

	The for loop below is PORTABLE,  but this is time-
	critical code;  it should work as fast as possible.
	It may be worth your while to rewrite it in assembly
	language.
*/

sysmove(source, dest, count)
char source [], dest [];
int count;
{

#ifdef BDS
	movmem(source, dest, count);
#else
	int i;

	if (count <= 0) {
		return;
	}
	else if (source > dest) {
		/* Copy head first. */
		for (i = 0; i < count; i++) {
			dest [i] = source [i];
		}
	}
	else {
		/* Copy tail first. */
		for (i = count - 1; i >= 0; i--) {
			dest [i] = source [i];
		}
	}
#endif

}

/*
	Save info for interrupted screen update.
	This routine is PORTABLE.
*/
sysintr(systl, systy, sysn)
int systl, systy, sysn;
{
	systopl = systl;
	systopy = systy;
	sysnl   = max(0,sysn);
}


/*
	Return -1 if no character is ready from the keyboard.
	Otherwise, return the character itself.

	This routine handles typeahead buffering.  It would
	also handle the repeat key,  if that feature is used,
	which it is NOT at present.  The code enclosed in
	comments handles the repeat key.

	NOTE:  The bdos(6, -1) function is a call to CP/M.
	It returns ZERO (not -1) if no character is ready from
	the console.  Otherwise,  it returns the character.

	NOTE:  This routine WILL need to be rewritten for
	operating sytems other than CP/M 80 and CP/M 68K.
*/
int
syscstat()
{
	int c, i;
	int c1;

	/* Always look for another character. */
	/* Trap ANSI terminal ESCapes - MDL NOV-2020 */
	do{
#ifdef KAYPRO
		if (!(inp(7) & 1)) {
			c = 0;
		}
		else {
			c = inp(5);
		}
#else
		c = bdos(6,-1);
#endif

		if (c != 0) {
			syslastc = c;
			sysrcnt  = 0;
			syscbuf [sysccnt++] = c;
		}

	} while(c != 0);	/* Collect "rapid" entry characters
				 * which may indicate a ESCape sequence
				 */

	if (sysccnt > 0) {
		/* If we have more than 3 characters in the buffer
		 * we may have an ESCape sequence
		 */
		if(sysccnt >= 3){
 			c1=syscbuf[sysccnt-1];
			if(syscbuf[sysccnt-2]=='[' && syscbuf[sysccnt-3]==0x1b)
				{
				/* We've trapped an escape sequence!
				 * So parse it out.
				 */
				sysccnt-=3;
				switch(c1){
					case 'A': c1=UP; break;
					case 'B': c1=DOWN; break;
					case 'C': c1=RIGHT; break;
					case 'D': c1=LEFT; break;
				}

			syscbuf[sysccnt++]=c1;
			}
		}

		return syscbuf [--sysccnt];
	}
	else {
		return -1;
	}
}

/*
	o  Wait for next character from the console.
	o  Do NOT echo the character.
	o  Print any waiting lines if there is no input ready.
        o  Swap out any dirty blocks if nothing else is happening.
	   (The SWAP constant enables this feature)

	This routine is PORTABLE, since it uses syscstat().
*/
int
syscin()
{
	int c;

	while ((c=syscstat()) == -1) {

		/* Output queued ? */
		if (hasint == YES && sysnl > 0) {
			/* Print one line. */
			bufout(systopl, systopy, sysnl);
		}

#ifdef SWAP
		/* Do not interrupt screen activity. */
		if (sysnl == 0) {
			/* Swap out one dirty buffer. */
			swap_one();
		}
#endif

	}

#ifdef KAYPRO
	/* Handle cursor keys and the numeric keypad. */
	if (c > 127) {
		switch (c) {
			case 177:	return '0';
			case 192:	return '1';
			case 193:	return '2';
			case 194:	return '3';
			case 208:	return '4';
			case 209:	return '5';
			case 210:	return '6';
			case 225:	return '7';
			case 226:	return '8';
			case 227:	return '9';

			case 228:	return '-';
			case 211:	return ',';
			case 178:	return '.';
			case 195:	return DOWN_INS;

			case 241:	return UP;
			case 242:	return DOWN;
			case 243:	return LEFT;
			case 244:	return RIGHT;

			default:	return c & 0x7f;
		}
	}
#endif

	return c & 0x7f;
}

/*
	Wait for all console output to be finished.
	This routine is PORTABLE.
*/
syswait()
{
	while (hasint == YES && sysnl > 0) {
		bufout(systopl, systopy, sysnl);
	}
}

/*
	Print one character on the console.

	This routine WILL have to be rewritten for operating
	system other than CP/M 80 or CP/M 68K.
*/
syscout(c)
char c;
{
	bdos(6,c);
	return(c);
}

/*
	Print character on the printer.

	This routine WILL have to be rewritten for operating
	systems other than CP/M 80 or CP/M 68K.
*/
syslout(c)
char c;
{
	bdos(5,c);
	return c;
}

/* ----- BUFFERED file I/O routines -----

	RED uses two types of file routines:

	BUFFERED  file  routines  allow one character at a time
	to be read from or written to a file.    Buffered files
	are   created  and  opened   with  the  sysfopen()  and
	sysfcreat() routines,   which return a pointer to FILE.
	FILE  is usually defined to be a struct,   but FILE may
	be defined to be anything at all so  long  as  all  the
	buffered file routines agree on what it is.

	UNBUFFERED  routines  allow  blocks  of  characters  to
	be read from a file.   Unbuffered files are created and
	opened  with  the  sysopen()  and  syscreat() routines.
 	(Note  the  slightly  different names from the buffered
	counterparts).   Sysopen() and sycreat() must return an
	integer.

	RED  never  mixes  buffered  and  unbuffered  routines.
	That is,  no BUFFERED routine is ever called to operate
	on a file which  has  been  opened  with  sysopen()  or
	syscreat()  and  no   UNBUFFERED   file routine is ever
	called to operate on a file which has been opened  with
	sysfopen() or sysfcreat().

	Buffered files are open either for reading or writing --
	never  for  both.  RED follows this restriction in order
	to make writing this  module  easier.    Buffered  INPUT
	files are opened with sysfopen().  Buffered OUTPUT files
	are created with sysfcreat().

	All file routines in this module are SEMI-PORTABLE, i.e.,
	they will work on any system which supports the Unix
	system calls.  If your operating system does not support
	Unix-style calls,  then you may have to do a LOT of work
	to get these calls up and running.
*/

/*
	Create a buffered output file.
	The file is cleared (erased) if it already exists.
	A pointer to FILE is returned if all goes well.
	NULL (0), is returned if the file can not be created.
*/

FILE *
sysfcreat(filename, buffer)
char *filename;
FILE * buffer;
{

#ifdef OLDBDS
	int fcreat();
	return (fcreat(filename, buffer) == -1) ? NULL : buffer;
#else
	FILE *fopen();
	FILE *value;

	value = fopen(filename, "w");
	return ((value == -1) ? NULL : value);

	/* comment out -----
	return (fopen(filename, "w")
	----- end comment out */
#endif

}

/*
	Open a buffered input file.
	Position the file at its start.
	A pointer to FILE is returned if all goes well.  
	NULL (0) is returned if the file does not exist.
*/
FILE *
sysfopen(filename, buffer)
char *filename;
FILE *buffer;
{

#ifdef OLDBDS
	int fopen();
	return (fopen(filename, buffer) == -1) ? NULL : buffer;
#else
	FILE *fopen();
	FILE *value;

	value = fopen(filename, "r");
	return ((value == -1) ? NULL : value);

	/* comment out -----
	return fopen(filename, "r");
	----- end comment out */
#endif

}

/*
	Close a buffered file.
	Return OK (0) or ERROR (-1).
*/
int
sysfclose(fd)
FILE *fd;
{

#ifdef BDS
	return fclose(fd);
#endif

}

/*
	Prepare a buffered output file to be closed.
	Return OK (0) or ERROR (-1).
	
*/

int
sysfflush(fd)
FILE *fd;
{
	putc(CPMEOF, fd);
	return fflush(fd);
}

/*
	Get the next charcharacer from a buffered input file.
	The high bit of the character MUST be set to zero.

	ERROR or CPMEOF may be returned on end-of-file.
	RED checks for either,  so choose whichever (or both)
	is easiest for your system.
*/

int
sysgetc(fd)
FILE *fd;
{
	int c;

	c = getc(fd);

	/* Be careful not to mask off bit on EOF. */
	return (c == ERROR) ? ERROR : c & 0x7f;

}

/*
	Read the next line from buffered input file.
	End the line with an NEWLINE.
	Return the count of the characters read.
	Return ERROR (-1) on end-of-file.
*/

int
sysfgets(fd, buffer, maxlen)
FILE *fd;
char *buffer;
int maxlen;
{
	int c, count;

	count = 0;
	while(1) {
		c = sysgetc(fd);
		if (c == CR) {
			/* Ignore pseudo newline. */
			continue;
		}
		else if (c == CPMEOF || c == ERROR) {
			buffer [min(count, maxlen-2)] = NEWLINE;
			buffer [min(count, maxlen-1)] = EOS;
			return ERROR;
		}
		else if (c == NEWLINE) {
			buffer [min(count, maxlen-2)] = NEWLINE;
			buffer [min(count, maxlen-1)] = EOS;
			return count;
		}
		else if (count < maxlen - 2) {
			buffer [count++] = c;
		}
		else {
			count++;
		}
	}
}

/*
	Put one character to a buffered output file.
	Returns the character itself,  or ERROR if the
	disk becomes full.
*/
int
sysputc(c, fd)
char c;
FILE *fd;
{
	return putc(c, fd);
}

/* ----- UNBUFFERED file I/O routines -----

	See comments  at the start of the section
	concerning BUFFERED I/O routines.
*/

/*
	Create an unbuffered file.  Erase it if it exists.
	Leave it open for read/write access.
	Return a small positive int or ERROR  (-1).
*/
int
syscreat(filename)
char * filename;
{

#ifdef BDS
	int fd;
	char * fcb;

	/* Check for an existing r/o file */
	fd = open(filename, 0);
	if (fd != -1) {
		fcb = fcbaddr(fd);
		if (fcb [9] & 0x80) {
			warning("r/o file");
			close(fd);
			return -1;
		}
		else {
			close(fd);
		}
	}

	/* Not read only.  Create it. */
	return creat(filename);
#endif

}
	
/*
	Open an existing file for unbuffered i/o.
		Mode 0 -- read only access
		Mode 1 -- write only access
		Mode 2 -- read/write access
	Return OK (0) or ERROR (-1).
*/
int
sysopen(name, mode)
char *name;
int mode;
{

#ifdef BDS
	int fd;
	char * fcb;

	fd = open(name, mode);
	if (fd == -1) {
		return -1;
	}
	fcb = fcbaddr(fd);
	if (fcb [9] & 0x80) {
		warning("r/o file");
	}
	return fd;
#endif

}

/*
	Close an unbuffered file.
	Return OK (0) or ERROR (-1).
*/
int
sysclose(fd)
int fd;
{
	return close(fd);
}

/*
	Erase the file from the file system.
	RED makes sure that the file is NOT open when
	sysunlink() is called.
	Return ERROR (-1) if the file does not exist or
	can not be erased.
*/
int
sysunlink(filename)
char * filename;
{
	return unlink(filename);
}

/*
	Rename the old file to be the new file.
	RED makes sure that the file is NOT open when
	sysrename() is called.
*/
sysrename(oldname, newname)
char *oldname, *newname;
{

#ifdef BDS
	rename(oldname, newname);
#endif

}

/*
	Seek to the specified block of an unbuffered file.
	Return OK (0) or ERROR (-1).
*/
int
sysseek(fd, block)
int fd, block;
{

#ifdef BDS
	return seek(fd, READ_SIZE * block, 0);
#endif

}

/*
	Read one block (READ_SIZE sectors) from an unbuffered
	file into the buffer of size DATA_SIZE.
	Return the number of sectors read or -1 if an
	end-of-file occurs immediately.
	If less than a full block is read,  put a CPMEOF mark
	after the last byte.
*/
int
sysread(fd, buffer)
int fd;
char * buffer;
{

#ifdef BDS
	return read(fd, buffer, READ_SIZE);
#else
	int n;

	n = read(fd, buffer, DATA_SIZE);
	if (n == ERROR) {
		return 0;
	}
	else if (n == DATA_SIZE) {
		return READ_SIZE;
	}
	else {
		/* Force an end-of-file mark */
		buffer [n] = CPMEOF;
		return READ_SIZE;
	}
#endif

}

/*
	Write n sectors from the buffer to an unbuffered file.
	Return n if all goes well.
	Return ERROR (-1) if there is some problem.
*/
int
syswrite(fd, buffer, n)
int fd;
char * buffer;
int n;
{

#ifdef BDS
	return write(fd, buffer, n);
#else
	return (write(fd, buffer, n*CPM_SIZE) != n*CPM_SIZE) ? ERROR : n;
#endif

}

/*
	Return YES if the file exists and NO otherwise.
	This routine is PORTABLE.
*/
int
sysexists(filename)
char * filename;
{
	int fd;

	if ((fd = sysopen(filename, 0)) != ERROR) {
		sysclose(fd);
		return YES;
	}
	else {
		return NO;
	}
}

/*
	Copy a file name from args to buffer.
	This routine is SEMI-PORTABLE,  since it depends,
	to a certain extent,  on what constitutes a file name.
*/
syscopfn(args,buffer)
char *args, *buffer;
{
	int n;

	for (n = 0;
	     n < SYSFNMAX -1 && args [n] != EOS && args [n] != ' ';
	     n++) {

		buffer [n] = args [n];
	}
	buffer[n] = EOS;
}
   n < SYSFNMAX -1 && args [n] != EOS && args [n] != ' ';
	     n++) {

		buffer [n] 