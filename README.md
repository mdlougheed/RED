#THIS IS A WORK IN PROGRESS
# RED Screen Editor: v7.1
April 19, 2020


RED is a lightweight, screen-oriented, plain-text file editor.  RED is an improved editor over ED2 (EDSCREEN) by including more features such as text block move,copy,insertion and extraction - and is capable of editing larger files than can fit into CP/M available memory, owing to virtual (disk) memory buffer page swapping.

RED for CP/M is updated from the original Edward K. Ream program first published in Dr. Dobbs Journal #81 & #82, JUL/AUG 1983.  The original source code for this version of RED is part of the BDS-C v1.6, released into the public domain, September 20, 2002.  The editor maintains its BDS-C compiler compatibility, debug and error correction feature set.
[https://www.bdsoft.com/resources/bdsc.html](https://www.bdsoft.com/resources/bdsc.html)

Please refer to the BDS-C user guide for information on how to use the debugging and error correction features of RED.

Further information on all RED editor commands can be found in the BDS-C user guide.  The block move and copy commands are excerpted here. 


Excerpts from the original Dr. Dobbs Journal Volume 8 articles are in this repository.  The main compilation can be found here:
[https://archive.org/details/dr_dobbs_journal_vol_08/mode/2up](https://archive.org/details/dr_dobbs_journal_vol_08/mode/2up)

For retro-computing enthusiasts, updates in this also version include workflow similar to ED2 (EDSCREEN), WordStar style cursor navigation, "double ESC" to enter command mode.  The default terminal is the (Kaypro) ADM-3A.

The CP/M COM file in this repository is set up for Kaypro.  It works great with the Virtual Kaypro Emulator!
http://sims.durgadas.com/kaypro/kaypro.html


## Mode Switching ##

There are four modes to the editor: 

- Command - Press ESC twice from any mode to enter command mode.
- Edit
- Insert
- Escape

Initially the editor is in Edit mode.  The current mode is shown on the top line of the editor along with the cursor's current line and column, and filename.

----------

**Command mode key:** (ESC ESC) or CTRL-C

- Enters command mode. [See Command Mode commands.](#command-mode)  
- Command mode prompt is shown as a colon **':'** on the top line of the screen 

**Edit mode key:** (CTRL-E or UP key)

- Enter edit mode.  [See Edit Mode commands.](#edit-mode)


**Insert Mode key:** (CTRL-N or "i" from Edit Mode) 

- Enter insert mode. [See Insert Mode commands.](#insert-mode)

**Escape Mode key:** (ESC from any mode) 

- Enter escape mode. [See Escape Mode.](#escape-mode)



## Special Characters ##

   All special characters may be used in edit and insert
modes, but only the command, edit, insert, undo and delete character keys may be used in command mode.

The function of each special character is given below:

## Cursor Movement: WordStar Style ##

**UP key:** (CTRL-E, or UP arrow key)

Move the cursor up one line unless the
cursor is already at the top line of the file.
Enter edit mode.

**DOWN key:** (CTRL-X, or DOWN arrow key)

Move the cursor down one line unless the
cursor is already at the bottom line of the file.
Enter edit mode.

**LEFT key:** (CTRL-S or LEFT arrow key)

Move the cursor left one character.

**RIGHT key:** (CTRL-D or RIGHT arrow key)

Move the cursor right one character.

## Line Insertion & Deletion, Splitting & Joining ##
**Insert up key:** (CTRL-U)

Insert a new line above the current line and enter
insert mode.

**Insert down key:** (Enter)

Insert a new line below the current line and enter
insert mode.

**Delete line key:** (CTRL-Y)

Delete the line on which the cursor rests.

**Split line key:**  (CTRL-L)

Split the current line into two lines.

**Join lines key:**  (CTRL-O)

Append the current line to the line above it.
Then delete the lower line.



## Character insertion & deletion and undo ##

While in insert mode, type character keys to insert text at the cursor
 
**Delete character key:** (Backspace)

Delete the character to the left of the cursor.


**Undo (abandon edits) key:** (CTRL-A)

Undo any editing done since the cursor last came to 
the current line.

Although less convenient, repeating previous functions is still available using the NULL key, CTRL-@.


## Edit Mode ##

   The following are the edit mode commands:

**'c' or ESC  (command):** Enter command mode.

**'i'  (insert):** Enter insert mode.

**'space':** Move the cursor right one character.

**'u'  (scroll page up):** Scroll the cursor up rapidly.  Hit any key to stop.

**'d'  (scroll page down):** Scroll the cursor down one page at a time.

**'b' (beginning):** Move the cursor to the beginning of the current line.

**'e'  (end):** Move the cursor to the end of the current line.

**'g' [line #]  (go to line):** Move the cursor to the start of the indicated line.

**'k' [char]  (kill up to [char]):** Delete from the cursor up to, but not including <char>. Delete to the end of the line if <char> does not appear to the right of the cursor. Do not delete anything if <char> is a special character.

**s [char]  (search for [char]):** Move cursor to the next occurrence of <char> to the right of the cursor. Move cursor to the end of the current line if <char> does not appear to the right of the cursor.

**x [char]  (eXchange one character):** If <char> is not a special character then <char> replaces the character under the cursor.

**any other characters are (ignored):** If <char> is neither a special character nor an edit mode mode command it is completely ignored.



## Insert Mode ##

Use insert mode to enter multiple lines of text into the buffer. All characters which are not special characters are simply inserted into the buffer.

## Command Mode ##

Use command mode to load or save files and do operations
that effect the edit buffer.


**g [n] (go):** Enter edit mode and set the cursor on line **[n]**.  If **[n]** is not entered, the current line is used for **[n]**.


**search [line range]:** Print all lines on the display which contain an instance of **[search mask]**.

- Question marks match any character in **[search mask]**.
- A leading up arrow (^) anchors the search mask to the start of the line.
- The '?' and '^' characters are treated just as in the change command.
- [line range] ** is a formatted as....**

**find:** Search for the next occurrence of **[search mask]**.

- Enter edit mode if **[search mask]** is found. Otherwise, stay in command mode.
- The '?' and '^' characters are treated just as in the change command.
 
**change [line range]:**
Change the first instance of **[search mask]** on each line in the line range to **[change mask]**.

- Question marks in **[change mask]** match the character that the corresponding question mark matched in **[search mask]**.

**delete [line range]:** Delete all lines with numbers in [line range].


**move :**  Moves a block of lines from one place in the buffer to another. 

The move command takes three arguments, the first line to move, the last line to move, and the line after which the lines are to be moved. Only one line is moved if only two line numbers are given.

- Examples:
- move 1 2 3; Moves lines 1--2 after line 3
- move 1 2 0; Moves lines 1--2 before line 1
- move 2 10 2; Moves line 2 after line 10
 


**copy:**  Works just like the move command except that a copy of the lines is moved so that the original lines stay where they were.


- Examples:
- copy 1 2 3; Copies lines 1--3 after line 3
- copy 1 2 0; Copies lines 1--2 before line 1
- copy 1 8 1; Copies line 1 after line 8


**extract:** Copies a block of lines to a file without erasing the block from the buffer.

**Take care with this command: the file is erased if it already exists.**

Another caution: **integers are legal file names**, so make sure you include the file name.

- Examples:
- extract abc; extracts the whole current file to "abc"
- extract 1 2; extracts line 2 to file "1"
- extract abc 1 2; extracts lines 1--2 to file "abc"
- extract f 1 9999; extracts the whole current file to file "f".

**inject:** Is the companion to the extract command.  The inject command adds a file to the buffer. It does not replace the buffer as does the load command.

- Examples:
- inject abc; injects file "abc" after current line
- inject abc 0; injects file "abc" before line 1
- inject abc 9999; injects file "abc" at end of file
- inject abc 50; injects file "abc" after line 50



----------

**clear:** Erase the entire buffer.

**load [filename]:** Erase the buffer, then load it with the file named by [filename].

- [filename] becomes the current file name which is used by the save and resave commands.

**name [filename]:** Make [filename] the current file name for use by the save and resave commands.

**append [filename]:** Append the file <filename> to  the main buffer at the current cursor position.

**save:** Save the buffer in the file named in the load or name commands. **The file must not
already exist.**

**resave:** Save the buffer in the file named in the load or name commands. **The file must already exist.**

**dos:** Return to the operating system (exit from the editor).

- User is prompted whether to discard edits. 

----------

**tabs [number]:** Cause tabs to be printed as [number] blanks on the screen and on the list device.

**list [line range]:** List all lines with numbers in <line range> on the list device (printer).

----------

## Escape Mode ##
From any mode (command, edit, insert) - allows the use of edit mode commands, then returns to the current mode.
