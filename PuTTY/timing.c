/*
 * timing.c
 * 
 * This module tracks any timers set up by schedule_timer(). It
 * keeps all the currently active timers in a list; it informs the
 * front end of when the next timer is due to go off if that
 * changes; and, very importantly, it tracks the context pointers
 * passed to schedule_timer(), so that if a context is freed all
 * the timers associated with it can be immediately annulled.
 */

#include <assert.h>
#include <stdio.h>

#include "putty.h"
#include "tree234.h"

struct timer {
    timer_fn_t fn;
    void *ctx;
    long now;
};

static tree234 *timers = NULL;
static tree234 *timer_contexts = NULL;
static long now = 0L;

static int compare_timer_contexts(void *av, void *bv)
{
    char *a = (char *)av;
    char *b = (char *)bv;
    if (a < b)
	return -1;
    else if (a > b)
	return +1;
    return 0;
}

static void init_timers(void)
{
    if (!timers) {
	timers = newtree234(compare_timers);
	timer_contexts = newtree234(compare_timer_contexts);
	now = GETTICKCOUNT();
    }
}

