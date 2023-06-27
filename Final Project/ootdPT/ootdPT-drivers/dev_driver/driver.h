#ifndef _DEV_DRIVER_H_
#define _DEV_DRIVER_H_
#include <linux/ioctl.h>

/* Display time (sec unit) */
#define DISPLAY_TIME 1000   // set to 20 when testing

/* Size of buffer for reading inputs */
#define READ_BUF_SIZE 30
#define TEMP_BUF_SIZE 4
#define WEATHER_BUF_SIZE 16
#define LINE_NUM 2

/* Timer to register, and some metadata of it */
typedef struct _timer {
    /* Timer to register */
    struct timer_list timer;
    
    /* Metadata */
    int interval;        // Time interval
    int cnt;             // Total # of counts
    short date;          // Today's date
    int humid;           // Humidity
    char text[LINE_NUM][WEATHER_BUF_SIZE + 1];// Two texts (for each line) -> Weather description
} Timer;

#endif /* _DEV_DRIVER_H_ */
