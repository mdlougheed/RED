
/*
RED terminal output module
Source:  red6.c

Customizations for VT-100 terminal by Mark D. Lougheed NOV-2020
*/

#include "red.h"


/*
Define the current coordinates of the cursor.
*/

#ifdef HAS_STATIC
static
int outx;
static
int outy;
#endif


/*
Return the current coordinates of the cursor.
*/

outgetx()
{
	return outx;
}

outgety()
{
	return outy;
}


/*
Initialize the globals which describe the terminal.
*/

outinit()
{
	hasdn   = YES;
	hasup   = YES;
	hasins  = YES;
	hasdel  = YES;
	hasint  = YES;
	hascol  = YES;
	haswrap = NO;
}


/*
Output one printable character to the screen.
*/

outchar(c)
char c;
{
	syscout(c);
	outx++;
	return c;
}


/*
Position cursor to position x,y on screen.
0,0 is the top left corner.
*/

outxy(x,y)
int x, y;
{
	outx = x;
	outy = y;
	syscout(27);
	syscout('[');

	syscout('0'+((y+1)/10));  // ASCII most significant digit of row
	syscout('0'+((y+1)%10));  // ASCII least significant digit of row

	syscout(';');

	syscout('0'+((x+1)/10));  // ASCII most significant digit of column
	syscout('0'+((x+1)%10));  // ASCII least significant digit of column

	syscout('H');
}


/*
Erase the entire screen.
Make sure the rightmost column is erased.
*/

outclr()
{
	int i;

    for (i = 0; i < SCRNL; i++) {
		outxy(0, i);
		outdelln();
	}
	outxy(0,0);
}


/*
Delete the line on which the cursor rests.
Leave the cursor at the left margin.
*/

outdelln()
{
	outxy(0,outy);
	outdeol();
}


/*
Delete to end of line.
Assume the last column is blank.
*/

outdeol()
{
	syscout(27);
	syscout('[');
	syscout('K');
}


/*
Hardware insert line.
*/

outins()
{
	syscout(27);
	syscout('[');
	syscout('L');
}


/*
Hardware delete line.
*/

outdel()
{
	syscout(27);
	syscout('[');
	syscout('M');
}


/*
Scroll the screen up.
Assume the cursor is on the bottom line.
*/

outsup()
{
	/* auto scroll */
	outxy(0,SCRNL1);
	syscout(27);
	syscout('[');
	syscout('L');
}


/*
Scroll screen down.
Assume the cursor is on the top line.
*/

outsdn()
{
	/* auto scroll */
	outxy(0,0);
	syscout(27);
	syscout('[');
	syscout('M');
}

