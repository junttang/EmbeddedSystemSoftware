#include <linux/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <jni.h>
#include "android/log.h"

/* Device file address and the major number */
const char *dev_file_addr = "/dev/intr_driver";
#define DEV_MAJOR_NUM 243

/* Just for the calling itself */
#define TEMP_LEN 10

/* openDevice */
JNIEXPORT jint JNICALL Java_com_example_androidex_TurnOnOffThread_openDevice(JNIEnv *env, jclass this) {
    int fd;

    /* Open the device driver */
    if ((fd = open(dev_file_addr, O_WRONLY)) == -1) {
        __android_log_print(ANDROID_LOG_VERBOSE, "INTR_DRIVER", "Failed to open device driver");
        return -1;
    }

    return fd;
}

/* writeDevice */
JNIEXPORT void JNICALL Java_com_example_androidex_TurnOnOffThread_writeDevice(JNIEnv *env, jclass this, jint fd) {
    char param[TEMP_LEN] = {'\0'}; 
    
    /* Write to the device driver, but it will just make the current thread
       be slept. The current java thread will be woken up by the 'back' button
       interrupt handler which is installed by the 'intr_driver' in the device. */
    write(fd, param, TEMP_LEN);
}

/* closeDevice */
JNIEXPORT void JNICALL Java_com_example_androidex_TurnOnOffThread_closeDevice(JNIEnv *env, jclass this, jint fd) {
    close(fd);
}
