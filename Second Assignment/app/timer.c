#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/ioctl.h>

/* Device file address and the major number (PDF conditions) */
const char *dev_file_addr = "/dev/dev_driver";
#define DEV_MAJOR_NUM 242

/* Case definitions of user-defined ioctl() handler */
#define IOCTL_SET_OPTION _IOW(DEV_MAJOR_NUM, 0, char*)
#define IOCTL_COMMAND _IO(DEV_MAJOR_NUM, 0)

/* Main program */
int main(int argc, char *argv[]) {
    int i, one_non_zero, fd;
    int interval, count, init;
    char ioctl_param[11] = {'\0'};

    /* Arguments verification */
    if (argc != 4) {
        printf("Error: Usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
        return -1;
    }
    interval = atoi(argv[1]);
    count = atoi(argv[2]);
    init = atoi(argv[3]);

    /* TIMER_INTERVAL & TIMER_CNT verification */
    if (interval == 0 || count == 0) {
        perror("Error: Interval or count must not be zero.\n");
        return -1;
    }
    if ((interval < 1 || interval > 100) || (count < 1 || count > 100)) {
        perror("Error: Interval or count must be in a range of 1~100.\n");
        return -1;
    }

    /* TIMER_INIT input with verfication */
    one_non_zero = 0;
    if (strlen(argv[3]) != 4) {
        perror("Error: Init string must be in a length of 4 characters.\n");
        return -1;
    }
    for (i = 0; i < 4; i++) {
        if (!isdigit(argv[3][i]) || argv[3][i] == '9') {
            perror("Error: Init string must have digits of 1~8.\n");
            return -1;
        }
        if (argv[3][i] - '0' > 0) {
            if (one_non_zero != 0) {
                printf("Error: Init string must have only one non-zero digit.\n");
                return -1;
            }
            one_non_zero = 1;
        }
    }
    if (one_non_zero == 0) {
        perror("Error: Init string must have one non-zero digit.\n");
        return -1;
    }

    /* Open timer device */
    if ((fd = open(dev_file_addr, O_WRONLY)) == -1) {
        perror("Error opening device file\n");
        return -1;
    }

    /* Set parameters and then pass them */
    sprintf(ioctl_param, "%03d%03d%04d", interval, count, init);
    ioctl(fd, IOCTL_SET_OPTION, ioctl_param);  // Pass option parameters to device
    ioctl(fd, IOCTL_COMMAND);                  // Start the timer device!

    /* Close the device */
    close(fd);
    return 0;
}
