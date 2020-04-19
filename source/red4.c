/*
	RED window module -- Full C version (with error file support)

	Source:  red4.c
	Version: May 9, 1986

	Copyright (C) 1983, 1984, 1985, 1986 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/
#include "red.h"

#ifdef HAS_STATIC

static
char	editbuf[MAXLEN];	/* the edit buffer	*/
static
int	editp;			/* cursor: buffer index	*/
static
int	editpmax;		/* length of buffer	*/
static
int	edcflag;		/* buffer change flag	*/
static
int	edaflag;		/* auto mode flag	*/
static
int	edtop;			/* current top of screen */

#endif


/*
	Abort any changes made to current line.
*/

edabt()
{
	/* Get unchanged line and reset cursor. */
	edgetln();
	edredraw();
	edbegin();
	edcflag = NO;
}


/*
	Put cursor at beginning of current line.
*/

edbegin()
{
        editp = 0;
	outxy(0, outgety());
}


/*
	Move the cursor back one word.
*/

edbword()
{
	int c;

	/* Move up one line if at left margin. */
	if (editp == 0) {
		if (!bufattop()) {
			edup();       
			edend();
		}
		return;
	}

	/* Scan for white space. */
	while (--editp > 0) {
		c = editbuf [editp];
		if (c == ' ' || c == TAB) {
			break;
		}
	}

	/* Scan for non-white space. */
	while (editp > 0) {
		c = editbuf [--editp];
		if (c != ' ' && c != TAB) {
			break;
		}
	}

	/* Reset cursor. */
	outxy(edxpos(),outgety());
}


/*
	Change editbuf[editp] to c.
	Don't make change if line would become to long.
*/

edchng(c)
char c;
{
	char oldc;
	int k;

	/* If at right margin then insert char. */
	if (editp >= editpmax) {
		edins(c);
		return;
	}

	/* Change char and print length of line. */
	oldc = editbuf[editp];
	editbuf[editp] = c;
	fmtadj(editbuf, editp, editpmax);

	k = fmtlen(editbuf,editp+1);
	if (k > SCRNW1) {

		/*
			The line would become too long.
			Undo the change.
		*/
		editbuf[editp] = oldc;
		fmtadj(editbuf, editp, editpmax);
	}

	/* comment out -----
	else if (hascins && hascdel && editp < editpmax) {
		editbuf[editp] = oldc;
		fmtadj(editbuf, editp, editpmax);
		editp++;
		eddel();
		edins(c);
	}
	----- end comment out */

	else {
		/* Set change flag, bump cursor, redraw line. */
		edcflag = YES;
		editp++;
		edadj();
		edredraw();
	}
}


/*
	Indicate that the screen has been changed by
	an agent outside of this module.
*/

edclr()
{
	edtop = -1;
}


/*
	Delete the char to left of cursor if it exists.
*/

eddel()
{
	int i, k;

	/* Just move left one column if past end of line. */
	if (edxpos() < outgetx()) {
		outxy(outgetx()-1, outgety());
		return;
	}

	/* Do nothing if cursor is at left margin. */
	if (editp == 0) {
		return;
	}
	edcflag = YES;

	/* Compress buffer (delete char). */
	k = editp;
	while (k < editpmax) {
		editbuf[k-1] = editbuf[k];
		k++;
	}

	/* Update pointers, update line. */
	editp--;
	editpmax--;
	edredraw();
}


/*
	Delete the character under the cursor.
*/

ed2del()
{
	int i, k;

	/* Just move left one column if past end of line. */
	if (editp > 0 && editp == editpmax && edxpos() <= outgetx()) {
		outxy(outgetx()-1, outgety());
		return;
	}

	/* Do nothing if cursor is at left margin. */
	if (editpmax == 0) {
		return;
	}
	edcflag = YES;

	/* Compress buffer (delete char). */
	k = editp+1;
	while (k < editpmax) {
		editbuf[k-1] = editbuf[k];
		k++;
	}

	/* Adjust the cursor if now at end. */
	if (editp+1 >= editpmax && editp > 0) {
		editp--;
	}
	editpmax--;
	if (editp == editpmax && editp > 0) {
		editp--;
	}
	edredraw();
}


/*
	Edit the next line.  Do not go to end of buffer.
*/

eddn()
{
	int oldx;

	/* Save visual position of cursor. */
	oldx = outgetx();

	/* Replace current edit line. */
	edrepl();

	/* Do not go past last non-null line. */
	if (bufnrbot()) {
		if (edatbot()) {
			ed_sup(bufln()-SCRNL2+1);
			bufout(bufln() + 1, SCRNL1, 1);
			outxy(0, SCRNL2);
		}
		return;
	}

	/* Move down one line in buffer. */
	bufdn();
	edgetln();

	/*
		Put cursor as close as possible on this
		new line to where it was on the old line.
	*/

	editp = edscan(oldx);

	/* Update screen. */
	if (edatbot()) {
		ed_sup(bufln()-SCRNL2);
		outxy(oldx, SCRNL1);
	}
	else {
		outxy(oldx, outgety()+1);
	}
	return;
}


/*
	Put cursor at the end of the current line.
	Make sure it doesn't go off the screen.
*/

edend()
{
	editp = editpmax;
	edadj();
	outxy(edxpos(),outgety());
}


/*
	Move the cursor forward one word.
	Move to the next line if at end of line.
*/

edfword()
{
	int c;

	/* Move down one line if at right margin. */
	if (editp == editpmax) {
		eddn();
		edbegin();
		return;
	}

	/* Scan for white space. */
	while (++editp < editpmax) {
		c = editbuf [editp];
		if (c == ' ' || c == TAB) {
			break;
		}
	}

	/* Scan for non-white space. */
	while (editp < editpmax) {
		c = editbuf [++editp];
		if (c != ' ' && c != TAB) {
			break;
		}
	}

	/* Reset cursor. */
	edadj();
	outxy(edxpos(),outgety());
}


/*
	Start editing line n;  set editp to x.

	At one time I thought that the search and change
	commands needed a separate version of this routine
	that would try to draw the line at the top of the
	screen.  However,  that did not provide enough
	context for the person looking at the patterns.
*/

edgo(newline, x)
int newline, x;
{
	int line;
	int oldlast;

	/* Save the status of the screen. */
	oldlast  = edtop + SCRNL2;

	/* Replace current line. */
	edrepl();

	/* Go to new line. */
	bufgo(newline);

	/* Prevent going past end of buffer. */
	if (bufatbot()) {
		bufup();
	}
	newline = bufln();

	/* Do not redraw the whole screen if not needed. */
	if (edtop > 0 && newline >= edtop && newline <= oldlast) {
		/* Just move to new line. */
		line = 1 + (newline - edtop);
	}
	else if (bufln() + SCRNL/2 > bufmax()) {
		/* The line is near the end of file. */
		edtop = max(1,  bufmax() - (SCRNL-3));
		line = 1 + (bufln() - edtop);
		bufout(edtop, 1, SCRNL1);
	}
	else {
		/* Draw the line in the middle of the screen. */
		edtop = max(1, bufln() - SCRNL/2);
		line  = 1 + (bufln() - edtop);
		bufout(edtop, 1, SCRNL1);
	}

	edgetln();
	editp = x;
	outxy(edxpos(), line);
}


/*
	Insert c into the buffer if possible.
	Return YES if the line was auto-split.
*/

edins(c)
char c;
{
	int i, k;
	int oldcflag;

	/* Do nothing if edit buffer is full. */
	if (editpmax >= MAXLEN) {
		return NO;
	}

	/* Provisionally say we've made a change. */
	oldcflag = edcflag;
	edcflag  = YES;

	/* Fill out line if we are past its end. */
	if (editp == editpmax && edxpos() < outgetx()) {
		k = outgetx() - edxpos();
		editpmax = editpmax + k;
		while (k-- > 0) {
			editbuf [editp++] = ' ';
		}
		editp = editpmax;
	}

	/* Make room for inserted character. */
	k = editpmax;
	while (k > editp) {
		editbuf[k] = editbuf[k-1];
		k--;
	}

	/* Insert character.  Update pointers. */
	editbuf[editp] = c;
	editp++;
	editpmax++;

	/* Find where the cursor will be. */
	fmtadj(editbuf,editp-1,editpmax);
	k = fmtlen(editbuf,editp);

	if (k > SCRNW1 && editp == editpmax && haswrap) {
		/* Auto-split the line (line wrap) */

		/* Scan for start of current word. */
		k = editp - 1;
		while (k >= 0 && editbuf[k] != ' ' && editbuf[k] != TAB) {
			k--;
		}

		/* Never split a word. */
		if (k < 0) {
			eddel();
			edcflag = oldcflag;
			return NO;
		}

		/* Split the line at the current word. */
		editp = k + 1;
		edsplit();
		edend();
		return YES;
	}
	else if (k > SCRNW1) {

		/*
			Cursor would move off the screen.
			Delete what we just inserted.
		*/
		eddel();
		edcflag = oldcflag;
	}
	else {
		/* Set change flag, redraw line. */
		edredraw();
	}
	return NO;
}


/*
	Join (concatenate) the current line with the one above it.
	Put cursor at the join point unless it would be off screen.
*/

edjoin()
{
	int k1, k2;
	int k;

	/* Do nothing if at top of file. */
	if (bufattop()) {
		return;
	}

	/* Replace lower line temporarily. */
	edrepl();

	/* Get upper line into buffer. */
	bufup();
	k1 = bufgetln(editbuf, MAXLEN);

	/* Append lower line to buffer. */
	bufdn();
	k2 = bufgetln(editbuf+k1, MAXLEN-k1);

	/* Abort if the line isn't wide enough. */
	if (k1 + k2 > MAXLEN1) {
		bufgetln(editbuf,MAXLEN);
		return;
	}

	/* Replace upper line, set cursor to middle of line. */
	bufup();
	editpmax = k1 + k2;
	editp = k1;
	edadj();
	edcflag = YES;
	edrepl();

	/* Delete the lower line from the buffer. */
	bufdn();
	bufdel();

	/*
		Delete the lower line on the screen,
		move up and redraw.
	*/
		
	if (!edattop()) {
		ed_del();
		outxy(outgetx(), outgety() - 1);
		bufup();
		edredraw();
	}
	else {
		bufup();
		edtop = bufln();

		/* Redraw the ENTIRE line. */
		fmtadj (editbuf,0,editpmax);
		fmtsubs(editbuf,0,editpmax);
		outxy(edxpos(),outgety());
	}

	/* 11/16/85 Update the error tables. */
	err_del(bufln(), 1);
}


/*
	Delete WORDS until end of line or c found.
*/

edkill(c)
char c;
{
	int k,p;
	int lastc;

	/* Do nothing if at right margin. */
	if (editp == editpmax) {
		return;
	}
	edcflag = YES;

	/*
		Count number of deleted characters.
		The matched char must start a word.
	*/
	k = 1;
	lastc = ' ';
	while ((editp+k) < editpmax) {
		if ( editbuf[editp+k] == c &&
		     (!isalpha(lastc) || !isalpha(c))
		   ) {
			break;
		}
		else {
			lastc = editbuf[editp+k];
			k++;
		}
	}

	/* Compress buffer (delete chars). */
	p = editp+k;
	while (p < editpmax) {
		editbuf[p-k] = editbuf[p];
		p++;
	}

	/* Update buffer size, redraw line. */
	editpmax = editpmax-k;
	edredraw();
}


/*
	Move cursor left one column.
	Never move the cursor off the current line.
*/

edleft()
{
	int k;

	/* If past right margin, move left one column. */
	if (edxpos() < outgetx()) {
		outxy(max(0, outgetx()-1), outgety());
	}

	/* Inside the line.  move left one character. */
	else if (editp != 0) {
		editp--;
		outxy(edxpos(),outgety());
	}
}


/*
	Insert a new blank line below the current line.
*/

ednewdn()
{
	/*
		Make sure there is a current line and 
		Put the current line back into the buffer.
	*/
	if (bufatbot()) {
		bufins(editbuf,editpmax);
	}
	edrepl();

	/* Move past current line. */
	bufdn();

	/* Insert place holder:  zero length line. */
	bufins(editbuf,0);

	/* Start editing the zero length line. */
	edgetln();

	/* Update the screen. */
	ed_ind();

	/* 11/16/85 Update the error tables. */
	err_ins(bufln() - 1, 1);
}


/*
	Insert a new blank line above the current line.
*/

ednewup()
{
	/* Put current line back in buffer. */
	edrepl();

	/* Insert zero length line at current line. */
	bufins(editbuf,0);

	/* Start editing the zero length line. */
	edgetln();

	/* Update the screen. */
	ed_inu();

	/* 11/16/85 Update the error tables. */
	err_ins(bufln() - 1, 1);
}


/*
	Move cursor right one character.
	Never move the cursor off the current line.
*/

edright()
{
	/* If we are outside the line move right one column. */
	if (edxpos() < outgetx()) {
		outxy (min(SCRNW1, outgetx()+1), outgety());
	}

	/* If we are inside a tab move to the end of it. */
	else if (edxpos() > outgetx()) {
		outxy (edxpos(), outgety());
	}

	/* Move right one character if inside line. */
	else if (editp < editpmax) {
		editp++;
		edadj();
		outxy(edxpos(),outgety());
	}

	/* Else move past end of line. */
	else {
		outxy (min(SCRNW1, outgetx()+1), outgety());
	}
}


/*
	Split the current line into two parts.
	Scroll the first half of the old line up.
*/

edsplit()
{
	int p, q;
	int k;

	/* Indicate that edit buffer has been saved. */
	edcflag = NO;

	/* Replace current line by the first half of line. */
	if (bufatbot()) {
		bufins(editbuf, editp);
	}
	else {
		bufrepl(editbuf, editp);
	}

	/* Redraw the first half of the line. */
	p = editpmax;
	q = editp;
	editpmax = editp;
	editp = 0;
	edredraw();

	/* Move the second half of the line down. */
	editp = 0;
	while (q < p) {
		editbuf [editp++] = editbuf [q++];
	}
	editpmax = editp;
	editp = 0;

	/* Insert second half of the line below the first. */
	bufdn();
	bufins(editbuf, editpmax);

	/* Insert a line on the screen and draw it. */
	ed_ind();
	edredraw();

	/* 11/16/85 Update the error tables. */
	err_ins(bufln(), 1);
}


/*
	Move cursor right until end of line or char c found.
	Do not move the cursor off the end of the line.
*/

edsrch(c)
char c;
{
	int lastc;

	/* Do nothing if at right margin. */
	if (editp == editpmax) {
		return;
	}

	/* Scan for search character. */
	editp++;
	if (editpmax == 0) {
		lastc = ' ';
	}
	else {
		lastc = editbuf [editp];
	}
	while (editp < editpmax) {
		if ( editbuf[editp] == c &&
		     (!isalpha(lastc) || !isalpha(c))
		   ) {
			break;
		}
		else {
			lastc = editbuf[editp];
			editp++;
		}
	}

	/* Reset cursor. */
	edadj();
	outxy(edxpos(),outgety());
}


/*
	Move cursor up one line if possible.
*/

edup()
{
	int oldx;

	/* Save visual position of cursor. */
	oldx = outgetx();

	/* Put current line back in buffer. */
	edrepl();

	/* Done if at top of buffer. */
	if (bufattop()) {
		return;
	}

	/* Start editing the previous line. */
	bufup();
	edgetln();

	/*
		Put cursor on this new line as close as
		possible to where it was on the old line.
	*/
	editp = edscan(oldx);

	/* Update screen. */
	if (edattop()) {
		ed_sdn(bufln());
		outxy(oldx, 1);
	}
	else {
		outxy(oldx, outgety()-1);
	}
}


/*
	Delete the current line.
*/

edzap()
{
	int k;

	/* Delete the line in the buffer. */
	bufdel();

	/* Move up one line if now at bottom. */
	if (bufatbot() && edattop()) {
		bufup();
		edtop = bufln();
		edgetln();
		edredraw();
	}
	else if (bufatbot()) {
		ed_del();
		outxy(0, outgety() - 1);
		bufup();
		edgetln();
	}
	else {
		ed_del();
		edgetln();
	}

	/* 11/16/85 Update the error tables. */
	err_del(bufln(), 1);
}


/* ----- utility routines (not used outside this file) ----- */


/*
	Adjust the cursor position so the cursor stays on screen.
	This must be called whenever the cursor could move right,
	i.e., in edchng(), edend(), edjoin(), edright() and edsrch().
*/

edadj()
{
	while (fmtlen(editbuf, editp) > SCRNW1) {
		editp--;
	}
}


/*
	Return true if the current edit line is being
	Displayed on the bottom line of the screen.
*/

edatbot()
{
	return outgety() == SCRNL1;
}


/*
	Return true if the current edit line is being
	displayed on the bottom line of the screen.
*/

edattop()
{
	return outgety() == 1;
}


/*
	Redraw edit line from index to end of line and
	reposition cursor.
*/

edredraw()
{
	fmtadj(editbuf,0,editpmax);
	fmtsubs(editbuf,max(0,editp-1),editpmax);
	outxy(edxpos(),outgety());
}


/*
	Return the x position of the cursor on screen.
*/

edxpos()
{
	return fmtlen(editbuf, editp);
}


/*
	Fill edit buffer from current main buffer line.
	The caller must check to make sure the main
	buffer is available.
*/

edgetln()
{
	int k;

	/* Put cursor on left margin, reset flag. */
	editp = 0;
	edcflag = NO;

	/* Get edit line from main buffer. */
	k = bufgetln(editbuf,MAXLEN);
	if (k > MAXLEN) {
		error("line truncated");
		editpmax = MAXLEN;
	}
	else {
		editpmax = k;
	}
	fmtadj(editbuf,0,editpmax);
}


/*
	Replace current main buffer line by edit buffer.
	The edit buffer is NOT changed or cleared.
*/

edrepl()
{
	/* Do nothing if nothing has changed. */
	if (edcflag == NO) {
		return;
	}

	/* Make sure we don't replace the line twice. */
	edcflag = NO;

	/* Insert instead of replace if at bottom of file. */
	if (bufatbot()) {
		bufins(editbuf,editpmax);
	}
	else {
		bufrepl(editbuf,editpmax);
	}
}


/*
	Set editp to the largest index such that
	buf[editp] will be printed <= xpos
*/

edscan(xpos)
int xpos;
{
	editp = 0;
	while (editp < editpmax) {
		if (fmtlen(editbuf,editp) < xpos) {
			editp++;
		}
		else {
			break;
		}
	}
	return editp;
}


/*
	Scroll the screen up.  Topline will be new top line.
*/

ed_sup(topline)
int topline;
{
	edtop = topline;
	if (hasdel == YES) {

		/* Delete line 1. */
		outxy(0, 1);
		outdel();

		/* Redraw bottom line. */
		bufout(topline+SCRNL2,SCRNL1,1);
	}
	else if (hasup == YES) {

		/* Hardware scroll. */
		outsup();

		/* Redraw bottom line. */
		bufout(topline+SCRNL2,SCRNL1,1);

		/* Restore the prompt line. */
		pmtzap();
	}
	else {

		/* Redraw whole screen. */
		bufout(topline,1,SCRNL1);

		/* Restore the prompt line. */
		pmtzap();
	}
}


/*
	Scroll screen down.  Topline will be new top line.
*/

ed_sdn(topline)
int topline;
{
	edtop = topline;
	if (hasins == YES) {

		/* Insert a line above line 1. */
		outxy(0, 1);
		outins();

		/* Redraw top line. */
		bufout(topline,1,1);
	}
	else if (hasdn == YES) {

		/* Hardware scroll. */
		outsdn();

		/* Redraw top line. */
		bufout(topline,1,1);

		/* Redraw the prompt line. */
		pmtzap();
	}
	else {

		/* Redraw whole screen. */
		bufout(topline,1,SCRNL1);

		/* Redraw the prompt line. */
		pmtzap();
	}
}


/*
	Insert one line below the current line on the screen.
*/

ed_ind()
{
	int y;

	if (edatbot()) {
		ed_sup(bufln()-SCRNL2);
		outxy(edxpos(),SCRNL1);
	}
	else if (hasins == YES) {
		y = outgety();
		outxy(0, y+1);
		outins();
		outxy(edxpos(), y+1);
	}
	else {
		y = outgety();
		bufout(bufln(), y+1, SCRNL1-y);
		outxy(edxpos(), y+1);
	}
}


/*
	Insert a line above the current line on the screen.
*/

ed_inu()
{
	int y;

	if (hasins == YES) {
		y = outgety();
		outins();
		outxy(edxpos(), y);
	}
	else if (edattop()) {
		ed_sdn(bufln());
		outxy(edxpos(), 1);
	}
	else {
		y = outgety();
		bufout(bufln(), y, SCRNL - y);
		outxy(edxpos(), y);
	}
}


/*
	Delete one line from the screen.
*/

ed_del()
{
	int y;

	/* Remember what line we are on. */
	y = outgety();

	if (hasdel == YES) {
		outdel();
		bufout(bufln() + SCRNL1 - y, SCRNL1, 1);
	}
	else if (edattop()) {
		ed_sup(bufln());
		bufout(bufln() + SCRNL1 - y, SCRNL1, 1);
	}
	else {
		bufout(bufln(), y, SCRNL - y);
	}

	/* Set cursor to beginning of line. */
	outxy(0, y);
}
, SCRNL1, 1);
	}
	else {
		bufout(bufln(), y, SCRNL - y);
	}

	/* Set cursor to beginning of line. */
	outxy(0, y);
