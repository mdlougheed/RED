/*
	RED main program -- Full C version (Includes PROGERRS.$$$ support)

	Source:  red2.c
	Version: November 19, 1985; May 16, 1986

	Copyright (C) 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions and updates to mode/key operational consistency & convienience
	Mark D. Lougheed - APR, 2020
	
	Added ANSI terminal ESCape sequence traps
	MDL - NOV-2020
*/

#include "red.h"


/*
	Define the possible values of mode and lastmode.
*/

#define CMNDMODE 1	/* command mode		*/
#define INSMODE  2	/* insert mode		*/
#define EDITMODE 3	/* edit mode		*/
#define OVERMODE 4	/* overtype mode 	*/
#define ESCAPEMODE 5	/* escape mode		*/
#define EXITMODE 6	/* exit the editor	*/
#define CCMODE 7	/* auto invoke cc	*/

/*
	Define internal function numbers.
*/

#define BOL	129
#define EOL	130
#define HOME	131
#define END_PG	132
#define MIDDLE	133
#define KILL	134
#define SEARCH	135
#define GOTO	136
#define XCHNG	137
#define UP_HALF	138
#define DN_HALF	139
#define PREV_ERR 140
#define NEXT_ERR 141


int def0mode;	/* initial mode and mode after commands. */
int def1mode;	/* mode after UP_INS, DOWN_INS.	*/
int def2mode;	/* mode after UP, DOWN		*/

main(argc, argv)
int  argc;
char **argv;
{
	char * err_msg();

	int mode;	/* current mode.			*/
	int lastmode;	/* last mode (used by ESCAPEMODE).	*/
	int lastc;	/* last command	(used by AGAIN).	*/
	int holdc;	/* queued input character.		*/
	int c;		/* current input character.		*/
	char args [MAXLEN];
	char buffer [SCRNW1];
	int v;
	int x,y,topline;
	char *p;

	/* Ready the system module. */
	sysinit();

	/* Output from fmt module goes to screen. */
	fmtassn(NO);

	/* Clear filename [] for save(), resave(). */
	g_file [0] = EOS;

	/* Ready the work file if it exists. */
	if (setjmp(DISK_ERR) == ERROR) {
		/* Error in bufinit(). */
		return;
	}

	/* Set tabs, clear the screen and sign on. */
	fmtset(8);
	outinit();
	outclr();
	pmtclr();
	edclr();

	/* Initialize the buffer routines and handle error file, if present. */
	bufinit();

	/*
		11/16/85
		Set the auto load file from file named in error file.
	*/
	if (g_file [0] != EOS && argc == 1) {
		argv[1] = &g_file;
		argc    = 2;
	}

	/* Start off in the default mode. */
	lastmode = mode = DEFIMODE;
	def0mode = mode;
	def1mode = DEF1MODE;
	def2mode = DEF2MODE;

	edgetln();		/* Initialize window module. */
	outxy(0,SCRNL1);

	pmthold(YES);
	message(SIGNON);
	message(VERSION);
	message(COPYRIGHT);
	message("Additions by Mark D. Lougheed, NOV 2020");
	message("");
	message(XSIGN);
	message(XSIGN1);
	message("");
	pmthold(NO);

	/* Set error recovery point for auto-load code. */
	if (setjmp(DISK_ERR) == ERROR) {
		/* Error in load() in argc >= 2 case below. */
		argc = 0;
	}

	/* Never auto-load a file if a previous session was resumed. */
	if (bufchng() == YES) {
		/* Draw the first part of the resumed file. */
		bufgo(1);
		edclr();
		edgo(1,0);
	}
	else if (argc >= 2) {
		/* Auto-load a file from the command line. */
		pmtmode("-- loading --");
		strcpy(args, "load ");
		strcat(args, argv [1]);
		if (load(args) == YES) {
			syswait();
		}
		else {
			/* Preserve the signon message. */
			bufout(1,1,2);
			outxy(0,1);
		}
	}
	else {
		/* No file was loaded. Preserve the signon message. */
		bufout(1,1,2);
		outxy(0,1);
	}

	/* Set the disk error recovery point. */
	setjmp(DISK_ERR);

	lastc = NULL;
	holdc = NULL;
	for (;;) {

		/* Update the mode on the prompt line. */
		switch(mode) {
		case EDITMODE:		pmtmode("edit:");	break;
		case INSMODE:		pmtmode("insert:");	break;
		case OVERMODE:		pmtmode("overtype:");	break;
		case ESCAPEMODE:	pmtmode("escape:");	break;
		}

		/* Get the next character. */
		if (holdc != NULL) {
			c = holdc;
			holdc = NULL;
		}
		else {
			c = syscin();
		}

		/* Substitute the last command for the AGAIN key. */
		if (c == AGAIN) {
			c = lastc;
		}

		/* Double ESC sets command mode - MDL 2020 */
		if(c==ESCAPE && mode==ESCAPEMODE){
			lastmode = mode = EDITMODE;
			c = CMND;
		}

		/* Translate escapes and edit mode commands. */
		if (mode == EDITMODE || mode == ESCAPEMODE) {
			c = tolower(c);
			switch(c) {
			case UP:	c = UP;		break; /* MDL 2020 */
			case ' ':	c = RIGHT;	break;
			case 'b':	c = BOL;	break;
			case 'd':	c = PAGE_DN;	break; /* MDL 2020 */
			case 'e':	c = EOL;	break;
			case 'h':	c = HOME;	break;
			case 'i':	c = INS;	break; /* MDL 2020 */
			case 'g':	c = GOTO;	break;
			case 'k':	c = KILL;	break;
			case 'm':	c = MIDDLE;	break;

/* Deleted for consistency with ED2
 * MDL 2020
			case 'p':	c = PAGE_DN;	break;
			case 'q':	c = PAGE_UP;	break;
*/

			case 's':	c = SEARCH;	break;
			case 'u':	c = PAGE_UP;	break; /* MDL 2020 */
			case 'x':	c = XCHNG;	break;
			case 'z':	c = END_PG;	break;
			case '-':	c = UP_HALF;	break;
			case '+':	c = DN_HALF;	break;
			case '>':	c = NEXT_ERR;	break;
			case '<':	c = PREV_ERR;	break;
			default:
				break;
			}
		}

		/* Remember the what the last function was. */
		lastc = c;

		/* Restore previous mode in escape mode. */
		if (mode == ESCAPEMODE) {
			mode = lastmode;
		}

		/* Do the requested function. */

		if(EDIT==UP && c==UP)  /* MDL 2020 */
			{
			edup();
			mode=def2mode;
			}

		switch(c) {

		case NULL:	break;

		case CMND:	mode = command();
				if (mode == EXITMODE) {
					outxy(0, SCRNL1);
					fmtcrlf();
					return;
				}
				else if (mode == CCMODE) {
					outxy(0, SCRNL1);
					fmtcrlf();
					execl("CC", g_file, 0);
				}
				break;

		case EDIT:	lastmode = mode = EDITMODE;
				break;

		case INS:	lastmode = mode = INSMODE;
				break;

		case OVER:	lastmode = mode = OVERMODE;
				break;


		case ESCAPE:	lastmode = mode;
				mode = ESCAPEMODE;
				break;

		case UP_INS:	ednewup();
				lastmode = mode = def1mode;
				break;

		case DOWN_INS:	ednewdn();
				lastmode = mode = def1mode;
				break;

		case UP:	edup();
				mode = def2mode;
				break;

		case DOWN:	eddn();
				mode = def2mode;
				break;

		case LEFT:	edleft();	break;
		case RIGHT:	edright();	break;
		case WORD_F:	edfword();	break;
		case WORD_B:	edbword();	break;

		case JOIN:	edjoin();	break;
		case SPLIT:	edsplit();	break;
		case UNDO:	edabt();	break;
		case DEL1:	eddel();	break;
		case DEL2:	ed2del();	break;
		case ZAP:	edzap();	break;

		case VERBATIM:
			pmtmode("verbatim:");
			edins(syscin());
			break;

		case SCRN_DN:
			syswait();
			while (chkkey() == NO) {
				eddn();
				if (bufnrbot()) {
					eddn();
					break;
				}
			}
			break;

		case PAGE_DN:
			edgo(min(
				bufmax(),
				bufln()+(SCRNL1-outgety())+SCRNL/2
				),
			     0);
			break;

		case PAGE_UP:
			edgo(max(0,bufln()-outgety()-SCRNL/2+3),
			     0);
			break;

		case UP_HALF:
			/* Move up a half page. */
			edgo(max(0,bufln()-(SCRNL2/2)),0);
			break;

		case DN_HALF:
			/* Move down a half page. */
			edgo(min(bufmax(),bufln()+(SCRNL2/2)),0);
			break;

		case SCRN_UP:
			syswait();
			while (bufattop() == NO && chkkey() == NO) {
				edup();
			}
			break;

		case HOME:
			edgo(max(0,bufln()-outgety()+1),0);
			lastc = PAGE_UP;
			break;

		case END_PG:
			edgo(min(bufmax(),bufln()+SCRNL1-outgety()),0);
			lastc = PAGE_DN;
			break;

		case MIDDLE:
			edgo(min(bufmax(),bufln()+SCRNL1/2-outgety()+1),0);
			break;

		case BOL:
			edbegin();
			lastc = HOME;
			break;

		case EOL:
			edend();
			lastc = END_PG;
			break;

		case GOTO:
			pmtmode("goto: ");
			getcmnd(buffer);
			if(number(buffer,&v)) {
				edgo(v, 0);
			}
			break;

		case KILL:
			pmtmode("kill:");
			c = syscin();
			if (control(c)) {
				holdc = c;
			}
			else {
				edkill(c);
			}
			break;

		case SEARCH:
			pmtmode("search:");
			c = syscin();
			if (control(c)) {
				holdc = c;
			}
			else {
				edsrch(c);
			}
			break;

		case XCHNG:
			pmtmode("eXchange:");
			edchng(syscin());
			break;

		case NEXT_ERR:
			if (err_this() == -2) {
				break;
			}
			err_next();
			p = err_msg();
			if (p) {
				edgo(err_this(),0);
				cmndmess(p);
			}
			break;

		case PREV_ERR:
			if (err_this() == -1) {
				break;
			}
			err_prev();
			p = err_msg();
			if (p) {
				edgo(err_this(),0);
				cmndmess(p);
			}
			break;
			
		default:

			if (control(c)) {
				break;
			}

			if (mode == INSMODE) {
				edins(c);
			}
			else if (mode == OVERMODE) {
				edchng(c);
			}
		}
	}
}


/*
	Return TRUE if c is a control char.
*/

control(c)
char c;
{
	return c != TAB && (c >= 127 || c < 32);
}


/*
	Handle command mode.
*/

command()
{
	int  k, v;
	char c;
	char args [SCRNW1];
	char *argp, *skipbl();

	/* Make sure the current line is saved. */
	edrepl();

	pmtmode("command: ");
	getcmnd(args);
	c = args [0];

	switch(c) {
	case EDIT:	return EDITMODE;
	case INS:	return INSMODE;
	case OVER:	return OVERMODE;
	}

	/* Only one command may start with the letter 'g' */
	if (tolower(args [0]) == 'g'){
		argp = skipbl(args+1);
		if (argp [0] == EOS) {
			/* Do nothing. */
			;
		}
		else if (number(argp, &v) == YES) {
			edgo(v, 0);
		}
		else {
			cmndmess("Bad line number.");
		}
	}
	else if (lookup(args, "cc")) {
		if (chkbuf() == YES) {
			bufend();
			return CCMODE;
		}
	}
	else if (lookup(args,"change")) {
		change(args);
	}
	else if (lookup(args,"clear")) {
		/* clear() conflicts with AZTEC func. */
		clear1();
		pmtfn();
	}
	else if (lookup(args,"copy")) {
		copy(args);
	}
	else if (lookup(args,"def0ins")) {
		def0mode = INSMODE;
	}
	else if (lookup(args,"def0over")) {
		def0mode = OVERMODE;
	}
	else if (lookup(args,"def0edit")) {
		def0mode = EDITMODE;
	}
	else if (lookup(args,"def1ins")) {
		def1mode = INSMODE;
	}
	else if (lookup(args,"def1over")) {
		def1mode = OVERMODE;
	}
	else if (lookup(args,"def1edit")) {
		def1mode = EDITMODE;
	}
	else if (lookup(args,"def2ins")) {
		def2mode = INSMODE;
	}
	else if (lookup(args,"def2over")) {
		def2mode = OVERMODE;
	}
	else if (lookup(args,"def2edit")) {
		def2mode = EDITMODE;
	}
	else if (lookup(args,"delete")) {
		delete(args);
	}
	else if (lookup(args,"dos") || lookup(args, "exit")) {
		if (chkbuf() == YES) {
			bufend();
			return EXITMODE;
		}
	}

	/* comment out -----
	else if (lookup(args,"dump")) {
		bufdump();
	}
	----- end comment out */

#ifdef ERR_CMND
	else if (lookup(args,"errors")) {
		errors();
	}
#endif
	else if (lookup(args,"extract")) {
		extract(args);
	}
	else if (lookup(args, "findr")) {
		findr(args);
	}
	else if (lookup(args,"find")) {
		find(args);
	}

#ifdef HELP_CMND
	else if (lookup(args, "help")) {
		help();
	}
#endif

	else if (lookup(args,"inject")) {
		inject(args);
	}
	else if (lookup(args,"list")) {
		list(args);
	}
	else if (lookup(args,"load") || lookup(args, "red")) {
		load(args);
		pmtfn();
	}
	else if (lookup(args,"move")) {
		move(args);
	}
	else if (lookup(args,"name")) {
		name(args);
		pmtfn();
	}
	else if (lookup(args, "nowrap")) {
		haswrap = NO;
	}
	else if (lookup(args,"resave")) {
		resave();
	}
	else if (lookup(args,"save")) {
		save();
	}
	else if (lookup(args,"search")) {
		search(args);
	}

#ifdef SUSPEND
	else if (lookup(args,"quit")) {
		/* Make sure the file is named. */
		if (g_file [0] == EOS) {
			cmndmess("File not named.");
		}
		else {
			bufsusp();
			return EXITMODE;
		}
	}
#endif

	else if (lookup(args,"tabs")) {
		tabs(args);
	}
	else if (lookup(args, "wrap")) {
		haswrap = YES;
	}
	else if (lookup(args,"")) {
		;
	}
	else {
		cmndmess("Command not found.");
	}

	/* Do not exit. */
	return def0mode;
}


/*
	Return TRUE if line starts with command.
*/

lookup(line,cmnd)
char *line, *cmnd;
{
	while(*cmnd) {
		/* Watch out:  tolower may be a macro. */
		if (tolower(*line) != *cmnd) {
			return NO;
		}
		else {
			line++;
			cmnd++;
		}
	}
	return *line == EOS || *line == ' ' || *line == TAB;
}


/*
	Get next command into argument buffer.
*/

getcmnd(args)
char *args;
{
	int i, xpos, x, y;
	int length;
	char c;

	/* Remember the cursor position. */
	x      = outgetx();
	y      = outgety();
	xpos   = pmtlast();
	outxy(xpos, 0);

	length = 0;
	while ((c = syscin()) != CR) {
		pmtupd();

		if (c == EDIT || c == INS || c == OVER) {
			args [0] = c;
			length = 1;
			break;
		}

		if ( (c == DEL1 || c == LEFT) && length > 0) {
			outxy(xpos, 0);
			outdeol();
			length--;
			/* Redraw the field. */
			for (i = 0; i < length; i++) {
				if (args [i] == TAB) {
					outchar(' ');
				}
				else {
					outchar(args [i]);
				}
			}
		}
		else if (c == UNDO) {
			outxy(xpos, 0);
			outdeol();
			length = 0;
		}
		else if (c == TAB && length + xpos < SCRNW1) {
			args [length++] = TAB;
			outchar(' ');
		}
		else if (c < 32 || c == 127) {
			/* Ignore control characters. */
			continue;
		}
		else {
			if (length + xpos < SCRNW1) {
				args [length++] = c;
				outchar(c);
			}
		}
	}
	args [length] = EOS;

	/* Restore the cursor. */
	outxy(x, y);
}
		args [length++] = c;
				outchar(c);
			}
		}
	}
	arg