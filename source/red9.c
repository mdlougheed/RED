/*
	RED general utilities -- Full C version

	Source:  red9.c
	Version: September 21, 1983

	Copyright (C) 1983 by Enteleki, Inc.
	All Rights Reserved

	Additions by Mark D. Lougheed APR-2020
*/

#include "red.h"


/*
	Return YES if the first token in args is a number.
	Return the value of the number in  *val.
*/

number(args,val)
char *args;
int  *val;
{
	if (!isdigit(args [0])) {
		return NO;
	}
	else {
		*val = atoi(args);
		return YES;
	}
}


/*
	Output adecimal integer n in field width >= w.
	Left justify the number in the field.
*/

putdec(n,w)
int n,w;
{
	char chars[10];
	int i,nd;

	nd = itoc(n,chars,10);
	i  = 0;
	while (i < nd) {
		syscout(chars[i++]);
	}
	i = nd;
	while (i++ < w) {
		syscout(' ');
	}
}


/*
	Convert integer n to character string in str.
*/

itoc(n,str,size)
int n;
char *str;
int size;
{
	int absval;
	int len;
	int i,j,k;

	absval = abs(n);

	/* Generate digits. */
	str[0] = 0;
	i = 1;
	while (i < size) {
		str[i++] = (absval % 10)+'0';
		absval   = absval / 10;
		if (absval == 0) {
			break;
		}
	}

	/* Generate sign. */
	if (i < size && n < 0) {
		str[i++] = '-';
	}
	len = i-1;

	/* Reverse sign, digits. */
	i--;
	j = 0;
	while (j < i) {
		k      = str[i];
		str[i] = str[j];
		str[j] = k;
		i--;
		j++;
	}
	return len;
}


/*
	User error routine.
*/

error(message)
char *message;
{
	int x, y;

	/* Save cursor. */
	x = outgetx();
	y = outgety();

	pmtmess("Error: ",message);

	/* Wait for any key. */
	syscin();

	/* Restore cursor. */
	outxy(x, y);
}


/*
	User warning routine.
*/

warning(message)
char *message;
{
	int x, y;

	/* Save cursor. */
	x = outgetx();
	y = outgety();

	pmtmess("Warning: ",message);

	/* Wait for any key. */
	syscin();

	/* Restore cursor. */
	outxy(x, y);
}
gety();

	pmtmess("Warning: "