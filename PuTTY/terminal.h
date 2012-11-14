/*
 * Internals of the Terminal structure, for those other modules
 * which need to look inside it. It would be nice if this could be
 * folded back into terminal.c in future, with an abstraction layer
 * to handle everything that other modules need to know about it;
 * but for the moment, this will do.
 */

#ifndef PUTTY_TERMINAL_H
#define PUTTY_TERMINAL_H

#include "tree234.h"

typedef struct {
    int y, x;
} pos;

typedef struct termchar termchar;
typedef struct termline termline;

struct termchar {
    /*
     * Any code in terminal.c which definitely needs to be changed
     * when extra fields are added here is labelled with a comment
     * saying FULL-TERMCHAR.
     */
    unsigned long chr;
    unsigned long attr;

    /*
     * The cc_next field is used to link multiple termchars
     * together into a list, so as to fit more than one character
     * into a character cell (Unicode combining characters).
     * 
     * cc_next is a relative offset into the current array of
     * termchars. I.e. to advance to the next character in a list,
     * one does `tc += tc->next'.
     * 
     * Zero means end of list.
     */
    int cc_next;
};

struct termline {
    unsigned short lattr;
    int cols;			       /* number of real columns on the line */
    int size;			       /* number of allocated termchars  */
    int temporary;		       /* TRUE if decompressed from scrollback */
    int cc_free;		       /* offset to first cc in free list */
    struct termchar *chars;
};

struct bidi_cache_entry {
    int width;
    struct termchar *chars;
    int *forward, *backward;	       /* the permutations of line positions */
};


#define in_utf(term) ((term)->utf || (term)->ucsdata->line_codepage==CP_UTF8)

#endif

