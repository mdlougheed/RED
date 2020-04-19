/*
	BDS C Header file for full C version of RED
	Source:  red.h or redh.bds
	Version: see below

	Copyright (C) 1983, 1984, 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#define VERSION	"BDS C version 7.1, APR-2020."


/*
	The following constants select various options.
	If a constant is #define'd below,  the option is enabled.
	If the constant is NOT #define'd,  the option is disabled.

	OLDBDS:		code is for BDS C (Pre V1.6)
	BDS:		code is for BDS C (ANY version).
	AZTEC:		code is for the AZTEC C compiler.
	DRI:		code is for the DRI C compiler.

	HAS_STATIC:	the selected compiler supports statics.
	HAS_LONG:	the selected compiler supports longs.
	CAST:		the selected compiler supports casts.
	INT_SIZE:	must evaluate to sizeof(int)

	HELP_CMND:	enables help command.
	SWAP:		enables auto-swapping of dirty blocks.
	SUSPEND:	enables quit and auto-restart of RED.
	DEBUG:		enables checking of block formats.
	ASM:		enables assembly language code.

*/

#define BDS		1
#define DEBUG		1
#define INT_SIZE	2

#include <stdio.h>
#include "red1.h"
#include "redbuf.h"


/*
	Define the standard signon messages.
*/

#define SIGNON	"Welcome to RED."
#define COPYRIGHT "Copyright (C) 1986 by Enteleki, Inc."

#ifdef HELP_CMND
#define XSIGN  "Type control-c help for help."
#define XSIGN1 ""
#else
#define XSIGN  ""
#define XSIGN1 ""
#endif


/*
	Define constants describing a text line.
	These constants must be less than the maximum block
	size defined in redbuf.h.
*/

#define MAXLEN	200	/* max chars per line */
#define MAXLEN1	201	/* MAXLEN + 1 */


/*
	Define operating system constants.
*/

#define SYSFNMAX 20		/* CP/M file name length + 1 */

#define TEMP_FILE "@@TEMP@@.TMP"	/* name of temp file */
#define DATA_FILE "@@DATA@@.TMP"	/* name of work file */
#define ERR_FILE  "PROGERRS.$$$"


/*
	Define misc. constants.
*/

#define EOS	0	/* code sometimes assumes \0	*/
#define ERROR   -1	/* error code			*/
#define OK	0	/* opposite of ERROR		*/
#define YES	1	/* must be nonzero		*/
#define NO	0	/* must be zero			*/
#define CR	13	/* carriage return		*/
#define LF	10	/* line feed			*/
#define NEWLINE 10	/* newline character		*/
#define TAB	9	/* tab character		*/
#define CPMEOF 0x1a	/* end-of-file mark for CP/M	*/
#define HUGE	32000	/* practical infinity		*/


/*
	Define global file name.
*/

char g_file [SYSFNMAX];		/* file name for (re)save */


/*
	Define the global recovery point for disk errors.
*/

char DISK_ERR [JBUFSIZE];


/*
	Define globals used to describe the terminal.

	At present,  they are set by the outinit routine,
	but the sysinit routine would be a better place if
	you want to support multiple terminals without
	recompiling RED.
*/

int	hasdn;	 /* has scroll down			*/
int	hasup;	 /* has scroll up			*/
int	hasins;	 /* has insert line (ABOVE current line)*/
int	hasdel;	 /* has delete line			*/
int	hasint;	 /* use interrupt driven screen		*/
int	hascol;	 /* put columns on prompt line		*/
int	haswrap; /* use word wrap			*/


/*
	The BDS C compiler does not support true statics.
	All such variables must be defined here.
	See separate source files for further comments.
*/


char	editbuf [MAXLEN];	/* used by red4.c	*/
int	editp;
int	editpmax;
int	edcflag;
int	edtop;

int	fmttab;			/* used by red5.c	*/
int	fmtdev;
int	fmtwidth;
int	fmtcol [MAXLEN1];

int	outx, outy;		/* used by red6.c	*/

				/* used by red7.c	*/
int	pmt_zapped;
int	pmt_hold;
char	pmt_mode [MAXLEN];
int	pmt_newmd;
int	pmt_newfn;
int	pmt_col;
int	pmt_line;

char	sysinbuf [128];		/* used by red8.c	*/
int	sysincnt;
char	syscbuf [MAXLEN];
int	sysccnt;
int	sysrcnt;
int	syslastc;
int	systopl;
int	systopy;
int	sysnl;
char *	sysbuf;			/* SUPER KLUDGE		*/

/*
	Define Globals used by the buffer routines.
*/

int	DATA_RES;	/* pseudo constant		*/
			/* no greater than MAX_RES	*/

int	b_fatal;	/* clear buffer on error	*/
int	b_cflag;	/* buffer changed flag		*/

int	b_line;		/* current line number		*/
int	b_max_line;	/* highest line number		*/
int	b_start;	/* first line of current block	*/

int	b_head;		/* first block's disk pointer	*/
int	b_tail;		/* last block's disk pointer	*/
int	b_max_diskp;	/* last block allocated		*/
int	b_max_put;	/* last block written		*/

int	b_data_fd;	/* file descriptor of data file	*/
int	b_user_fd;	/* file descriptor of user file	*/
int	b_free;		/* head of list of free blocks	*/

char	b_buff [DATA_SIZE];	/* temporary buffer. 	*/

struct BLOCK * b_bp;	/* mem pointer to current block	*/


/*
	Define an array of pointers to each slot.
	The DATA_RES pseudo constant tells how many
	slots have actually been allocated.
*/

struct BLOCK * b_bpp [MAX_RES];


/*
	Define variables used only by read_file() and read1().
	They are used to speed up the code.
*/

char *	br_bufp;	/* pointer to input buffer	*/
int	br_bufc;	/* number of current buffer	*/
int	br_count;	/* index into input buffer	*/

int	br_avail;	/* number of free characters	*/
int	br_out;		/* index into outbuf		*/

/*
	Define variables used only by write_file() and write1().
	They are used to speed up the code.
*/

int	bw_count;	/* index into buffer		*/

#define MAX_ERR 100
int	err_num[MAX_ERR];	/* Number of line in error.	*/
char *	err_ptr[MAX_ERR];	/* Pointer to error message.	*/
char *	err_fptr[MAX_ERR];	/* Pointer to file name.	*/
int	err_line;		/* Current error line.		*/
int	err_max;		/* Number of errors.		*/
fptr[MAX_ERR];	/* Point