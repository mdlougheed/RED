
/*
RED special key definitions and compiler options.
Source:  red1.h
February 6, 1985

"Wordstar" inspired control key setup and consistency with ED2 
by Mark D. Lougheed APR-2020
*/


/*
Define how RED will switch modes by default.
*/

#define DEFIMODE EDITMODE
#define DEF1MODE INSMODE
#define DEF2MODE EDITMODE


/*
Define which keys are used for special edit functions.
*/

#define AGAIN 0		/* NULL   --> ^@ */
#define UP_INS 10	/* CTRL-J --> LF */
#define DOWN_INS 13	/* CTRL-M --> CR */

#define UP 5		/* CTRL-E */
#define DOWN 24		/* CTRL-X */
#define LEFT 19		/* CTRL-S */
#define RIGHT 4		/* CTRL-D */

#define INS 14		/* CTRL-N */
#define OVER 20		/* CTRL-T */
#define EDIT 5		/* CTRL-E --> SAME AS UP */
#define CMND 3		/* CTRL-C */
#define ESCAPE 27	/* ESC */

#define DEL1 8		/* CTRL-H --> BS */
#define DEL2 127	/* DEL */
#define ZAP 25		/* CTRL-Y */
#define UNDO 21		/* CTRL-U */
#define SPLIT 12	/* CTRL-L */
#define JOIN 15		/* CTRL-O */
#define VERBATIM 22	/* CTRL-V */

#define WORD_B 1	/* CTRL-A */
#define WORD_F 6	/* CTRL-F */

#define PAGE_DN 16	/* CTRL-P */
#define PAGE_UP 17	/* CTRL-Q */

#define SCRN_DN 26	/* CTRL-Z */
#define SCRN_UP 23	/* CTRL-W */


/*
Define length and width of screen and printer.
*/

#define SCRNW 80
#define SCRNW1 79

#define SCRNL 24
#define SCRNL1 23
#define SCRNL2 22

#define LISTW 132



*/

#define SCRNW 80
#define SCRNW1 79

#define SCRNL 24
#define SCRNL1 23
#define SCRNL2 22

#define LISTW 