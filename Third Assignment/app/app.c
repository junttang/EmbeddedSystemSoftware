#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/ioctl.h>

/* Device file for our device driver */
#define DEV_NAME "/dev/stopwatch"

/* Simple application to run /dev/stopwatch */
int main(void) {
    int fd;
    if ((fd = open(DEV_NAME, O_WRONLY)) == -1) {
        perror("[APP] Error occurs in device file open\n");
        return -1;
    }
    /* Trigger the driver */
    write(fd, NULL, 0);

    close(fd);
    return 0;
}
