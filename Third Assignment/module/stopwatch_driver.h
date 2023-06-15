#ifndef _STOPWATCH_H_
#define _STOPWATCH_H_

/* Timer to register, and some metadata of it */
typedef struct _timer {
    /* Timer to register */
    struct timer_list timer;

    /* Metadata of stopwatch */
    unsigned int elapsed_sec;    /* Elapsed seconds for the stopwatch */
    unsigned long prev_expires;  /* Time that the timer expired lastly (right before) */
    unsigned long paused_sec;    /* Time that the stopwatch paused lastly */
} Timer;

#endif /* _STOPWATCH_H_ */

