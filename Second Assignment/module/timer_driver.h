#ifndef _TIMER_DRIVER_H_
#define _TIMER_DRIVER_H_
#include <linux/ioctl.h>

/* Size of buffer for reading inputs */
#define READ_BUF_SIZE 10
#define TEMP_BUF_SIZE 4

/* Number of lines */
#define LINE_NUM 2
/* Maximum length of text */
#define TEXT_LEN 16

/* Rotate when state changed 8 times since reset. */
#define ROTATE_CNT 8

/* Timer to register, and some metadata of it */
typedef struct _timer {
    /* Timer to register */
    struct timer_list timer;
    
    /* Metadata (related to timer) */
    int interval;        // Interval for countering
    int max_cnt;         // Max limit of counts
    int cnt;             // Total # of counts

    /* Metadata (related to text_lcd) */
    char text[LINE_NUM][TEXT_LEN + 1];// Two texts (for each line)
    short text_idx[LINE_NUM];     // Indexes to start printing each line
    int text_dir[LINE_NUM];      // Direction of forwarding of each text
    // 0 means right, and 1 means left
    
    /* Metadata (related to fnd & dot) */
    short digit;         // Current digit
    short digit_idx;     // Current digit's index (for fnd)
} Timer;

#endif /* _TIMER_DRIVER_H_ */
