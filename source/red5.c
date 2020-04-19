/*
	RED output format module -- Full C version

	Source:  red5.c
	Version: September 16, 1983;  October 16, 1983

	Copyright (C) 1983 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#include "red.h"

#ifdef HAS_STATIC

static
int fmttab;	/* maximal tab length			*/
static
int fmtdev;	/* device flag -- YES/NO = LIST/CONSOLE	*/
static
int fmtwidth;	/* devide width.  LISTW/SCRNW1		*/

/*
	fmtcol[i] is the first column at which buf[i] is printed.
	fmtsub() and fmtlen() assume fmtcol[] is valid on entry.
*/
static
int fmtcol[MAXLEN1];

#endif


/*
	Direct output from this module to either the console or
	the list device.
*/

fmtassn(listflag)
int listflag;
{
	if (listflag == YES) {
		fmtdev = YES;
		fmtwidth = LISTW;
	}
	else {
		fmtdev = NO;
		fmtwidth = SCRNW1;
	}
}


/*
	Adjust fmtcol[] to prepare for calls on fmtout() and
	fmtlen().

	NOTE:  this routine is needed as an efficiency measure.
	Without fmtadj(), calls on fmtlen() become too slow.
*/

fmtadj(buf,minind,maxind)
char *buf;
int minind, maxind;
{
int k;
	/* Line always starts at left margin. */
	fmtcol[0] = 0;

	/* Start scanning at minind. */
	k = minind;
	while (k<maxind) {
		fmtcol[k+1] = fmtcol[k]+fmtlench(buf[k],fmtcol[k]);
		k++;
	}
}


/*
	Return column at which at which buf[i] will be printed.
*/

fmtlen(buf,i)
char *buf;
int i;
{
	return(fmtcol[i]);
}

/*
	Print buf[i] ... buf[j-1] on current device so long as
	characters will not be printed in last column.
*/

fmtsubs(buf,i,j)
char *buf;
int i, j;
{
	int k;

	if (fmtcol[i] >= fmtwidth) {
		return;
	}

	/* Position the cursor. */
	outxy(fmtcol[i],outgety());
	while (i < j) {
		if (fmtcol[i+1] > fmtwidth) {
			break;
		}
		fmtoutch(buf[i],fmtcol[i]);
		i++;
	}

	/* Clear rest of the line. */
	outdeol();
}


/*
	Print string which ends with NEWLINE or EOS
	to current device.
	Truncate the string if it is too long.
*/

fmtsout(buf,offset)
char *buf; int offset;
{
	char c;
	int col,k;

	col = 0;
	while (c = *buf++) {
		if (c == NEWLINE) {
			break;
		}
		k = fmtlench(c,col);
		if (col + k + offset > fmtwidth) {
			break;
		}
		fmtoutch(c,col);
		col += k;
	}
}


/*
	Return length of char c at column col.
*/

fmtlench(c,col)
char c;
int col;
{
	if (c == TAB) {
		/* tab every fmttab columns */
		return(fmttab-(col%fmttab));
	}
	else if (c<32) {
		/* control char */
		return(2);
	}
	else {
		return(1);
	}
}


/*
	Output one character to current device.
	Convert tabs to blanks.
*/

fmtoutch(c,col)
char c;
int col;
{
	int k;

	if (c == TAB) {
		k = fmtlench(TAB,col);
		while ((k--)>0) {
			fmtdevch(' ');
		}
	}
	else if (c<32) {
		fmtdevch('^');
		fmtdevch(c+64);
	}
	else {
		fmtdevch(c);
	}
}


/*
	Output character to current device.
*/

fmtdevch(c)
char c;
{
	if (fmtdev == YES) {
		syslout(c);
	}
	else {
		outchar(c);
	}
}

/*
	Output a CR and LF to the current device.
*/

fmtcrlf()
{
	if (fmtdev == YES) {
		syslout(CR);
		syslout(LF);
	}
	else if (hasdel == YES) {
		outxy(0, 1);
		outdel();
		outxy(0, SCRNL1);
		outdelln();
	}
	else {
		outxy(0,SCRNL1);
		syscout(CR);
		syscout(LF);
		pmtzap();
		pmtupd();
	}
}


/*
	Set tabs at every n columns.
*/

fmtset(n)
int n;
{
	fmttab = max(1,n);
}
;
		pmtzap();
		p