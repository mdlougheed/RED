/*
	RED command mode commands -- Full C version (with error file support)

	Source:  red3.c
	Version: November 18, 1985; May 9, 1986

	Copyright (C) 1983, 1984, 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/
#include "red.h"

/*
	Changes lines.
*/

change(args)
char *args;
{
	int  from, to, junk;

	/* Check the arguments. */
	if (get3args(args,&from,1,&to,HUGE,&junk,-1)==ERROR) {
		return;
	}

	/* rflag = YES;  wflag = NO;  cflag = YES */
	srch1(from, to, YES, NO, YES);
}


/*
	Clear main buffer and file name.
	WARNING:  clear() is an AZTEC library function.
*/

clear1()
{
	/* Make sure it is ok to clear buffer. */
	if (chkbuf() == YES) {
		g_file [0] = 0;
		outclr();
		bufnew();
		edgetln();
		outxy(0, 1);
		bufout(1, 1, 2);
		pmtzap();		/* 1/7/84 */
	}
}


/*
	Copy a block of memory without erasing it.
*/

copy(args)
char *args;
{
	movecopy(args, "usage: copy <block> <n>", "-- copying --", NO);
}


/*
	Delete multiple lines.
*/

delete(args)
char *args;
{
	int from, to, dummy;

	/* Check the request. */
	if(get3args(args,&from,bufln(),&to,-1,&dummy,-1)==ERROR){
		return;
	}
	if (to == -1) {
		to = from;
	}

	/* Say we've started. */
	pmtmode("-- deleting --");

	/* Go to first line to be deleted. */
	bufgo(from);

	/* Delete all line between from and to. */
	bufdeln(to-from+1);

	/* 11/16/85 Update error pointers. */
	err_del(from, to-from+1);

	/* Redraw the screen. */
	bufgo(from);
	edclr();
	edgo(from, 0);
}


/*
	Extract command.
	Copy lines to a file.
*/

extract(args)
char *args;
{
	char locfn [SYSFNMAX];
	int  from, to, junk;
	char *skiparg(), *skipbl();


	/* Get file name which follows command. */
	if (name1(args, locfn) == ERROR) {
		return;
	}
	if (locfn [0] == EOS) {
		cmndmess("usage: extract <filename> first last");
		return;
	}

	/* Skip over command,  so get3args() will skip filename. */
	args = skiparg(args);
	args = skipbl (args);

	/* Get optional line numbers. */
	get3args(args, &from, bufln(), &to, -1, &junk, -1);
	if (to == -1) {
		to = from;
	}

	/* Extract the file. */
	xtrct(locfn, from, to, "-- extracting --");
}


/*
	Search for a pattern.
	find -- search forward.  findr -- search backwards.
*/

find(args)
char *args;
{
	int  start, last, junk;

	/* Get starting place for search. */
	if(get3args(args,&start,bufln()+1,&last,HUGE,&junk,-1)==ERROR){
		return -1;
	}

	/* rflag = NO;  wflag = YES;  cflag = NO */
	srch1 (start, last, NO, YES, NO);
}


/*
	Same as find(),  but in reverse.
*/

findr(args)
char *args;
{
	int  start, junk;

	/* Get starting place for search. */
	if (get3args(args,&start,bufln()-1,&junk,-1,&junk,-1)==ERROR){
		return -1;
	}

	/* rflag = NO;  wflag = YES;  cflag = NO */
	return srch1 (max(start,1), 1, NO, YES, NO);
}


#ifdef HELP_CMND
#define MESS message
help()
{

MESS  ("Command Default args   Description");
MESS  ("");
MESS  ("change 1 9999		Change all lines in <line range>");
MESS  ("clear			Reset the editor");
MESS  ("copy   <block> <n>	Copy lines in <block> after line <n>");
MESS  ("delete <current line>	Delete one or more lines");
MESS  ("exit			Exit from the editor");
MESS  ("extract <file> <block>	Create a file from a block");
MESS  ("find	<current line>	Search for a pattern");
MESS  ("findr	<current line>	Backward find");
MESS  ("g <n>			Enter edit mode at line <n>");
MESS  ("help			Print this message");
MESS  ("inject <file>           Add a line to the buffer");
MESS  ("list	1 9999		List lines to the printer");
MESS  ("load   <file>		Replace the buffer with <filename>");
MESS  ("move   <block> <n>	Move lines of <block> after line <n>");
MESS  ("name   <file>		Set filename");
MESS  ("nowrap			Disable line wrapping");
#ifdef SUSPEND
MESS  ("quit			Exit and save work file");
#endif
MESS  ("resave			Save the buffer to an existing file");
MESS  ("save			Save the buffer to a new file");
MESS  ("search	1 9999		Search for pattern");
MESS  ("tabs    8		Set tabs to every <n> columns");
MESS  ("wrap			Enable line wrapping");

	/* Wait for any character and redraw the screen. */
	pmtupd();
	syscin();
	pmtzap();
	edclr();
	edgo(bufln(), 0);

}
#endif


/*
	Errors command.
	List the lines of progerrs.$$$
*/

#ifdef ERR_CMND
errors()
{
	char buf [1000];
	char err_count[10];
	int count;

	/* Clear the screen. */
	outclr();
	edclr();
	pmtzap();
	pmtupd();

	/* Print the messages. */
	message("");
	for(count = 0; count < err_max && count < SCRNL2-1; count++) {
	 	itoc(err_num[count], err_count, 10);	
		strcpy(buf, err_fptr [count]);
		strcat(buf, ": ");
		strcat(buf, err_count);
		strcat(buf, ": ");
		strcat(buf, err_ptr[count]);
		message(buf);
	}
	if (count < err_max) {
		message("----- remaining entries not listed -----");
	}
	else {
		message("");
	}
	
	/* Wait for any character and redraw the screen. */
	pmtupd();
	syscin();
	pmtzap();
	edclr();
	edgo(bufln(), 0);
}
#endif

/*
	Inject command.
	Load a file into main buffer at current location.
	This command does NOT change the current file name.
*/

inject(args)
char *args;
{
	char locfn [SYSFNMAX];
	int  oldline, junk;
	char *skiparg(), *skipbl();


	/* Get file name which follows command. */
	if (name1(args, locfn) == ERROR) {
		return;
	}
	if (locfn [0] == EOS) {
		cmndmess("usage: inject <filename> line");
		return;
	}

	/* Skip over command,  so get3args() will skip filename. */
	args = skiparg(args);
	args = skipbl (args);

	/* Get optional line number. */
	get3args(args, &oldline, bufln(), &junk, -1, &junk, -1);

	/* Load the file at oldline. */
	append(locfn, oldline, "-- injecting --");

	/* Redraw the screen. */
	bufgo(oldline);
	edclr();
	edgo(oldline, 0);
}


/*
	Print lines to list device.
*/

list(args)
char *args;
{
	char linebuf [MAXLEN1];
	int n;
	int from, to, dummy, line, oldline;

	/* Save the buffer's current line. */
	oldline = bufln();

	/* Get starting, ending lines to print. */
	if (get3args(args,&from,1,&to,HUGE,&dummy,-1)==ERROR) {
		return;
	}

	/* Say we've started. */
	pmtmode("-- listing --");

	/* Print lines one at a time to list device. */
	line = from;
	while (line <= to) {

		/* Make sure prompt goes to console. */
		fmtassn(NO);

		/* Check for interrupt. */
		if (chkckey() == YES) {
			break;
		}

		/* Print line to list device. */
		fmtassn(YES);

		bufgo(line++);
		if (bufatbot()) {
			break;
		}
		n = bufgetln(linebuf,MAXLEN1);
		n = min(n,MAXLEN);
		linebuf [n] = NEWLINE;
		fmtsout(linebuf,0);
		fmtcrlf();
	}

	/* Redirect output to console. */
	fmtassn(NO);

	/* Restore cursor. */
	bufgo(oldline);
}


/*
	Load file into buffer.
	Return YES if the file was loaded correctly.
*/

load (args)
char *args;
{
	char buffer [MAXLEN];	/* disk line buffer */
	char locfn  [SYSFNMAX];  /* file name */
	int n;
	int topline;

	/* Get filename following command. */
	if (name1(args,locfn) == ERROR) {
		return NO;
	}

	if (locfn [0] == EOS) {
		cmndmess("No file argument.");
		return NO;
	}

	/* Give user a chance to save the buffer. */
	if (chkbuf() == NO) {
		return NO;
	}

	/* Open the new file. */
	if (sysexists(locfn) == NO) {
		cmndmess("File not found.");
		return NO;
	}

	/* Say we've started. */
	pmtmode("-- loading --");

	/* Update file name. */
	syscopfn(locfn, g_file);
	pmtfn();
	pmtupd();

	/* Clear the buffer. */
	bufnew();

	/* Read the whole file into the buffer. */
	read_file(g_file);

	/* Indicate that the buffer is fresh. */
	bufsaved();

	/*	Set current line to line 1.
		Redraw the screen.
	*/
	bufgo(1);
	edclr();
	edgo (1, 0);
	return YES;
}


/*
	Move a block of lines.
*/

move(args)
char *args;
{
	/* Copy a block,  then delete it. */
	movecopy(args, "usage: move <block> <n>", "-- moving --", YES);
}


/*
	Change current file name.
*/

name(args)
char *args;
{
	name1(args, g_file);
}

name1(args, filename)
char *args, *filename;
{
	char *skiparg(), *skipbl();

	/* Skip command. */
	args = skiparg(args);
	args = skipbl(args);

	/* Copy filename. */
	syscopfn(args, filename);
	return OK;
}


/*
	Save the buffer in an already existing file.
*/

resave()
{
	int n, oldline;

	/* Save line number. */
	oldline = bufln();

	/* Make sure file has a name. */
	if (g_file [0] == EOS) {
		cmndmess("File not named.");
		return;
	}

	/* The file must exist for resave. */
	if (sysexists(g_file) == NO) {
		cmndmess("File not found.");
		return;
	}

	/* Say we've started. */
	pmtmode("-- resaving --");

	/* Write out the whole buffer. */
	write_file(g_file);

	/* Indicate that the buffer has been saved. */
	bufsaved();

	/* Restore line number. */
	bufgo(oldline);
}


/*
	Save the buffer in a new file.
*/

save()
{
	int file, n, oldline;

	/* Save current line number. */
	oldline = bufln();

	/* Make sure the file is named. */
	if (g_file [0] == EOS) {
		cmndmess("File not named.");
		return;
	}

	/* File must NOT exist for save. */
	if (sysexists(g_file) == YES) {
		cmndmess("File exists.");
		return;
	}

	/* Say we've started. */
	pmtmode("-- saving --");

	/* Write out the whole buffer. */
	write_file(g_file);

	/* Indicate buffer saved. */
	bufsaved();

	/* Restore line number. */
	bufgo(oldline);
}


/*
	Search for a pattern.
*/

search(args)
char *args;
{
	int from, to, junk;

	/* Check the request. */
	if (get3args(args,&from,1,&to,HUGE,&junk,-1)==ERROR) {
		return;
	}

	/* r_flag = NO,  w_flag = NO, c_flag = YES. */
	srch1(from, to, NO, NO, YES);
}


/*
	Search/change utility routine.
	Redraw the screen if the pattern is found.

	s_pat[]   contains the search pattern.
	r_pat[]   contains the change pattern.
	s_start   is first line to search.
	s_end     is last  line to search.
	r_flag    is YES if change pattern is used.
	w_flag    is YES if search wraps around.
	c_flag    is YES if search continues after a match.
*/

int
srch1(s_start, s_end, r_flag, w_flag, c_flag)
int   s_start, s_end, r_flag, w_flag, c_flag;
{
	char s_pat [MAXLEN1];
	char r_pat [MAXLEN1];

	int oldbuf;
	int oldstart, oldend;
	int a_flag, old_plen;
	int start_anchor,  end_anchor;
	int code;

	/* Get search mask. */
	pmtmode("Search mask? ");
	getcmnd(s_pat);
	if (strlen(s_pat) == 0) {
		return;
	}
	
	if (r_flag) {
		pmtmode("Change mask? ");
		getcmnd(r_pat);
	}

	/* Remember the current line. */
	oldbuf = bufln();

	/* Go to first line. */
	bufgo(s_start);
	if (bufatbot()) {
		bufup();
	}

	/* Remember the initial params. */
	oldstart = bufln();
	s_start  = bufln();
	oldend   = s_end;
	old_plen = strlen(s_pat);

	/* Set start-of-line anchor. */
	start_anchor = (s_pat [0] == '^') ? 1 : 0;

	/* Set end-of-line anchor. */
	end_anchor = (s_pat [old_plen - 1] == '$') ? 1 : 0;

	/* Delete trailing anchor from search pattern. */
	s_pat [old_plen - end_anchor] = EOS;

	/* Adjust plen to reflect only non-anchor characters. */
	old_plen -= (start_anchor + end_anchor);

	/* Enable prompts in srch2(). */
	a_flag = 0;

	pmtmode("-- searching --");

	/* Search all lines in between s_start and s_end. */
	if (s_start <= s_end) {
		while (s_start <= s_end) {

			/* Check for user abort. */
			if (chkckey() == YES) {
				break;
			}

			code = srch2( s_pat, r_pat, r_flag,
			              c_flag, &a_flag, 
				      start_anchor,  end_anchor,
				      old_plen);

			if (code == YES) {
				return;
			}
			else if (code == NO) {
				/* Remember the last match point. */
				oldbuf = bufln();
				pmtmode("-- searching --");
			}

			if (bufnrbot() && w_flag == NO) {
				break;
			}
			else if (bufnrbot() && w_flag == YES) {
				/* Wrap around search. */
				w_flag = NO;
				bufgo(1);
				s_start = 1;
				s_end   = oldstart;
			}
			else {
				bufgo(++s_start);
			}

		}

		/*  Return to the last line with a match. */
		if (a_flag == 'a') {	/* 4/24/84 */
			/* Force redraw. */
			edclr();
		}
		edgo(oldbuf, 0);
		return;
	}
	else {
		s_end = max(1, s_end);
		while(s_start >= s_end) {

			/* Check for user abort. */
			if (chkckey() == YES) {
				break;
			}

			code = srch2( s_pat, r_pat, r_flag,
			              c_flag, &a_flag,
				      start_anchor,  end_anchor,
				      old_plen);

			if (code == YES) {
				return;
			}
			else if (code == NO) {
				oldbuf = bufln();
			}

			if (bufln() == 1 && w_flag == NO) {
				break;
			}
			else if (bufln() == 1 && w_flag == YES) {
				w_flag = NO;
				bufgo(HUGE);
				bufup();
				s_start = bufln();
				s_end   = oldstart;
			}
			else {
				bufgo(--s_start);
			}
		}

		/* Return to the last line that matched. */
		if (a_flag == 'a') {	/* 4/25/84 */
			/* Force redraw of screen. */
			edclr();
		}
		edgo(oldbuf, 0);
		return;
	}
}



/*
	Search one line for all instances of old_pat.
	If r_flag is YES, replace them by new_pat.
	If c_flag is NO,  exit after finding the first instance.
	If a_flag is 'a', do not prompt the user.
	plen == strlen(old_pat).

	Return YES   if search should stop.
	Return No    if search should continue.
	Return ERROR if no match found.
*/

srch2(old_pat, new_pat, r_flag, c_flag, a_flag, s_anchor,  e_anchor, plen)
char * old_pat, * new_pat;
int  r_flag, c_flag;
int  * a_flag;
int  s_anchor,  e_anchor, plen;
{
	char old_line [MAXLEN1];
	char new_line [MAXLEN1];
	int  old_length, col, mode, match, xpos;

	/* Get the current line into oldline[] */
	old_length = bufgetln(old_line, MAXLEN1);
	old_length = min(old_length, MAXLEN);
	old_line [old_length] = EOS;

	/* No match is possible if old_pat[] is too long. */
	if (s_anchor && e_anchor && plen != old_length) {
		return ERROR;
	}
	else if (plen > old_length) {
		return ERROR;
	}

	/* Set starting column. */
	col = (e_anchor) ? old_length - plen : 0;

	/* Set prompting mode. */
	mode = * a_flag;

	/* Remember whether any match was seen on this line. */
	match = NO;

	/* Search column by column. */
	while (col < old_length) {
		if (amatch(old_line + col, old_pat + s_anchor) == NO) {
			if (s_anchor || e_anchor) {
				return ERROR;
			}
			else {
				col++;
				continue;
			}
		}
		else {
			/* Remember that a match was seen. */
			match = YES;
		}

		/* Show the screen before any replacement. */
		if (mode != 'a') { 	/* 3/1/84 */
			edgo(bufln(), col);
			syswait();
			xpos = outgetx();
		}

		/* Draw the proposed change on the screen. */
		if (r_flag == YES) {
			replace(old_line, new_line,
				old_pat + s_anchor, new_pat, col);
			bufrepl(new_line, strlen(new_line));
			if (mode != 'a') {	/* 3/1/84 */
				outxy(0, outgety());
				bufoutln(bufln());
				outxy(xpos, outgety());
			}
		}

		/* Stop the search if continue flag is NO. */
		if (c_flag == NO) {
			return YES;
		}

		/* Prompt the user unless in 'all' mode. */
		if (mode == 'a') {
			/* Update the search line. */
			old_length = bufgetln(old_line, MAXLEN1);
			old_length = min(old_length, MAXLEN);
			old_line [old_length] = EOS;
			col     += strlen(new_pat);
		}
		else if (r_flag == NO) {
			/* Give search-mode prompt. */
			pmtmode("next, exit? ");
			mode = syscin();
			mode = tolower(mode);
			if (mode != 'n') {
				return YES;
			}
			else {
				/* Do not rescan matched pattern. */
				col += plen;
			}
		}
		else {
			/* Give change-mode prompt. */
			pmtmode("yes, no, all, exit? ");
			mode = syscin();
			mode = tolower(mode);
			*a_flag = mode;

			if (mode == 'y' || mode == 'a') {
				/* Update the search pattern. */
				old_length = bufgetln(old_line, MAXLEN1);
				old_length = min(old_length, MAXLEN);
				old_line [old_length] = EOS;
			}
			else {
				/* Undo the change. */
				bufrepl(old_line, strlen(old_line));
				outxy(0, outgety());
				bufoutln(bufln());
				outxy(xpos, outgety());
			}
	
			if (mode == 'y' || mode == 'a') {
				/* Do not rescan replacement text. */
				col += strlen(new_pat);
			}
			else if (mode == 'n') {
				/* Continue the search on this line. */
				col += strlen(old_pat);
			}
			else {
				/* Default is 'e' */
				return YES;
			}
		}

		/* Anchored searches examine line only once. */
		if (s_anchor || e_anchor) {
			return NO;
		}
	}

	/* Indicate whether any match was found on the line. */
	return (match) ? NO : ERROR;
}


/*
	Set tab stops for fmt routines.
*/

tabs(args)
char *args;
{
	int n, junk;

	/* Default is every 8 columns. */
	if (get3args(args,&n,8,&junk,-1,&junk,-1)==ERROR){
		return;
	}
	fmtset(n);

	/* Redraw the screen. */
	edclr();
	edgo(bufln(), 0);
}


/*
	----- Utility routines start here -----
*/


/*
	Insert file whose name is fname after line where.
	(where can be zero,  in which case insert at start of file.)
	Use promt as the promt line mode while doing so.
*/

append(fname, where, prompt)
char *fname;
int where;
char *prompt;
{
	FILE *fd, *sysfopen();
	char buffer [MAXLEN];	/* disk line buffer */
	int  oldline, n;
	int  count;

#ifdef OLDBDS
	char filebuf [BUFSIZ];	/* file buffer */
#else
	char filebuf [1];	/* dummy */
#endif

	/* Open the new file. */
	fd = sysfopen(fname, filebuf);
	if (fd == NULL) {
		cmndmess("Temporary file not found.");
		return;
	}

	/* Say that we've started. */
	pmtmode(prompt);

	/* Go to after proper line,  unless line is zero. */
	bufgo(where);
	if (where) {
		bufdn();
	}

	/* Read the file into the buffer. */
	count = 0;
	while ((n = sysfgets(fd,buffer,MAXLEN)) >= 0) {
		if (n > MAXLEN) {
			cmndmess("line truncated.");
			n = MAXLEN;
		}
		bufins(buffer,n);
		count++;
		bufdn();
	}

	/* Close the file. */
	sysfclose(fd);

	/* 11/16/85 Update the error tables. */
	err_ins(where, count);
}


/*
	Create a file named fname from start to finish.
	Set the prompt mode to prompt while doing so.
*/

xtrct(fname, start, finish, prompt)
char * fname;
int start, finish;
char * prompt;
{
	FILE *fd, *sysfcreat();
	int i;
	int count, length;
	int oldline;
	char buffer [MAXLEN1];

#ifdef OLDBDS
	char filebuf [BUFSIZ];
#else
	char filebuf [1];	/* dummy */
#endif

	/* Say we've started. */
	pmtmode(prompt);

	/* Open a temporary file. */
	fd = sysfcreat(fname, filebuf);
	if (fd == NULL) {
		cmndmess("Can not open temporary file.");
		return;
	}

	/* Save the current line. */
	oldline = bufln();

	/* Copy the block to the temp file. */
	bufgo(start);
	for (count = finish - start + 1; count; count--) {
		length = bufgetln(buffer, MAXLEN);
		if (length < 0) {
			cmndmess("Error reading line.");
		}
		for (i = 0; i < length; i++) {
			sysputc(buffer [i], fd);
		}
		sysputc(CR, fd);
		sysputc(LF, fd);
		bufdn();
		if (bufatbot()) {
			break;
		}
	}

	/* Close the file. */
	sysfflush(fd);
	sysfclose(fd);

	/* Restore the current line. */
	bufgo(oldline);
}


/*
	Move or copy a block of lines.
*/

movecopy(args, usage, prompt, erase)
char *args, *usage, *prompt;
int erase;
{
	int count, i, length;
	int fstart, fend, tstart;
	char buffer [MAXLEN1];

	/* Get two or three args. */
	if (get3args(args,&fstart,-1,&fend,-1,&tstart,-1)==ERROR){
		return;
	}
	if (fstart == -1 || fend == -1) {
		cmndmess(usage);
		return;
	}
	if (tstart == -1) {
		tstart = fend;
		fend   = fstart;
	}

	/* Make sure the last line exists. */
	bufgo(max(fstart, tstart));
	if (bufatbot()) {
		bufup();
		if (fstart >= tstart) {
			fend = bufln();
		}
		else {
			tstart = bufln();
		}
	}		

	/*
	The 'to block' and 'from block' must not overlap.
	fstart must be > 0, tstart must be >=  0.
	*/
	if (fend < fstart ||
	    fstart <= 0 ||
	    tstart < 0 ||
	    (tstart >=  fstart && tstart < fend)
	   ) {
		cmndmess(usage);
		return;
	}

	/* Extract block to TEMP_FILE. */
	xtrct(TEMP_FILE, fstart, fend, prompt);

	/* Inject TEMP_FILE at tstart. */
	append(TEMP_FILE, tstart, prompt);

	if (erase) {
		err_move(fstart, fend, tstart);

		count = fend - fstart + 1;
		/* Erase 'from block'. */
		if (fstart < tstart) {
			bufgo(fstart);
			
		}
		else {
			bufgo(fstart + count);
		}
		bufdeln(count);
	}

	/* Erase the TEMP_FILE. */
	sysunlink(TEMP_FILE);

	/* Redraw the screen. */
	bufgo(tstart);
	edclr();
	edgo(tstart, 0);
}


/*
	Return YES if buffer may be drastically changed.
*/

chkbuf()
{
	int c;
	int x, y;

	/* Save cursor position. */
	x = outgetx();
	y = outgety();

	if (bufchng() == NO) {
		return YES;
	}

	pmtmess("", "Buffer not saved.  Proceed ?  ");
	c = syscout(syscin());

	/* Restore cursor postion. */
	outxy(x, y);

	/* Watch out:  tolower may be a macro. */
	if (tolower(c) == 'y') {
		return YES;
	}
	else {
		return NO;
	}
}


/*
	Print message.
*/

message(s)
char *s;
{
	fmtsout(s,0);
	fmtcrlf();
}


/*
	Get one, two or three arguments.
	Missing arguments are set to default values.
*/

get3args(args, val1, def1, val2, def2, val3, def3)
char *args;
int  *val1, *val2, *val3;
int  def1, def2, def3;
{
	char *skiparg(), *skipbl();

	/* Skip the command. */
	args = skiparg (args);
	args = skipbl (args);

	/* Set defaults. */
	*val1 = def1;
	*val2 = def2;
	*val3 = def3;

	/* Check first arg. */
	if (*args == EOS) {
		return OK;
	}
	if (number (args, val1) == NO) {
		cmndmess("Bad argument.");
		return ERROR;
	}

	/* Skip over first argument. */
	args = skiparg(args);
	args = skipbl(args);

	/* Check second argument. */
	if (*args == EOS) {
		return OK;
	}
	if (number(args, val2) == NO) {
		cmndmess("Bad argument.");
		return ERROR;
	}

	/* Skip over third argument. */
	args = skiparg(args);
	args = skipbl(args);

	/* Check third argument. */
	if (*args == EOS) {
		return OK;
	}

	if (number (args, val3) == NO) {
		cmndmess("Bad argument.");
		return ERROR;
	}
	else {
		return OK;
	}
}


/*
	Skip over all except EOS, and blanks.
*/

char *
skiparg(args)
char *args;
{
	while (*args != EOS && *args != ' ') {
		args++;
	}
	return args;
}


/*
	Skip over all blanks.
*/

char *
skipbl(args)
char *args;
{
	while (*args == ' ') {
		args++;
	}
	return args;
}


/*
	Return YES if the user has pressed any key.
*/

chkkey()
{
	int c;

	c = syscstat();
	return (c == -1) ? NO : YES;
}


/*
	Return YES if the user has pressed any control key.
*/

chkckey()
{
	int c;

	c = syscstat();
	return (c != -1 && ((c & 0x7f) < 32)) ? YES : NO;
}


/*
	Return YES if the pattern in pat[] starts at line [0].
	A question mark in pat[] matches any character.
*/

amatch(line, pat)
char *line, *pat;
{
	while(*pat != EOS) {
		if (*pat == *line) {
			pat++;
			line++;
		}
		else if (*pat == '?' && *line != EOS) {
			pat++;
			line++;
		}
		else {
			return NO;
		}
	}
	return YES;
}


/*
	Replace oldpat in oldline by newpat starting at col.
	Put result in newline.
	Return number of characters in newline.
*/

replace(oldline,newline,oldpat,newpat,col)
char *oldline, *newline, *oldpat, *newpat;
int col;
{
	int k;
	char *tail, *pat;

	/* Copy oldline preceding col to newline. */
	k = 0;
	while (k < col) {
		newline [k++] = *oldline++;
	}

	/* Remember where end of oldpat in oldline is. */
	tail = oldline;
	pat = oldpat;
	while (*pat++ != EOS) {
		tail++;
	}

	/*
		Copy newpat to newline.
		Use oldline and oldpat to resolve question
		marks in newpat.
	*/
	while (*newpat != EOS) {
		if (k > MAXLEN-1) {
			cmndmess("New line too long.");
			return ERROR;
		}
		if (*newpat != '?') {
			/* Copy newpat to newline. */
			newline [k++] = *newpat++;
			continue;
		}

		/* Scan for '?' in oldpat. */
		while (*oldpat != '?') {
			if (*oldpat == EOS) {
				cmndmess(
				"Too many ?'s in change mask."
				);
				return ERROR;
			}
			oldpat++;
			oldline++;
		}

		/* Copy char from oldline to newline. */
		newline [k++] = *oldline++;
		oldpat++;
		newpat++;
	}

	/* Copy oldline after oldpat to newline. */
	while (*tail != EOS) {
		if (k >=  MAXLEN-1) {
			cmndmess("New line too long.");
			return ERROR;
		}
		newline [k++] = *tail++;
	}
	newline [k] = EOS;
	return k;
}


/*
	Print a cmndmess on the command line and wait for a key.
*/

cmndmess(mess)
char * mess;
{
	int x, y;

	/* Save cursor. */
	x = outgetx();
	y = outgety();

	pmtmess("", mess);

	/* Wait for any key. */
	syscin();

	/* Restore cursor. */
	outxy(x, y);
}
;