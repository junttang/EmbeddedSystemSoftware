#include <linux/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <jni.h>
#include "android/log.h"

/* Device file address and the major number (PDF conditions) */
const char *dev_file_addr = "/dev/dev_driver";
#define DEV_MAJOR_NUM 242

/* Case definitions of user-defined ioctl() handler */
#define IOCTL_SET_OPTION _IOW(DEV_MAJOR_NUM, 0, char*)
#define IOCTL_COMMAND _IO(DEV_MAJOR_NUM, 0)

/* Length of date string */
#define MAX_LEN 5

/* openDevice */
JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity_openDevice(JNIEnv *env, jclass this, jobject args) {
    int fd;
    struct _Args {
        char date[MAX_LEN];
        int humid;
        double tempCelsius; // Updated data type to double
        char weatherDescription[50]; // Added weather description
    } arg;
    const char *date;
    char ioctl_param[MAX_LEN+8+50]; // Increased ioctl_param size to accommodate weather description

    /* Open the device driver */
    if ((fd = open(dev_file_addr, O_WRONLY)) == -1) {
        __android_log_print(ANDROID_LOG_VERBOSE, "DEV_DRIVER", "Failed to open device driver");
        return -1;
    }

    /* Data assignments */
    jclass argsClass = (*env)->GetObjectClass(env, args);
    jfieldID jdateID = (*env)->GetFieldID(env, argsClass, "date", "Ljava/lang/String;");
    jfieldID jhumidID = (*env)->GetFieldID(env, argsClass, "humid", "I");
    jfieldID jtempID = (*env)->GetFieldID(env, argsClass, "tempCelsius", "D"); // Updated data type to D (double)
    jfieldID jweatherID = (*env)->GetFieldID(env, argsClass, "weatherDescription", "Ljava/lang/String;"); // Added weather description field

    // Date ex) 06/19
    date = (*env)->GetStringUTFChars(env, (*env)->GetObjectField(env, args, jdateID), 0);
    strncpy(arg.date, date, MAX_LEN);
    (*env)->ReleaseStringUTFChars(env, (*env)->GetObjectField(env, args, jdateID), date);

    // Humid ex) 64%
    arg.humid = (*env)->GetIntField(env, args, jhumidID);

    // Temperature
    arg.tempCelsius = (*env)->GetDoubleField(env, args, jtempID); // Updated data type to GetDoubleField

    // Weather description
    const char *weatherDesc = (*env)->GetStringUTFChars(env, (*env)->GetObjectField(env, args, jweatherID), 0);
    strncpy(arg.weatherDescription, weatherDesc, 50);
    (*env)->ReleaseStringUTFChars(env, (*env)->GetObjectField(env, args, jweatherID), weatherDesc);

    /* Set parameters and then pass them */
    sprintf(ioctl_param, "%03d%03d%s%s", (int)arg.tempCelsius, arg.humid, arg.date, arg.weatherDescription); // Updated ioctl_param format
    ioctl(fd, IOCTL_SET_OPTION, ioctl_param);  // Pass option parameters to device
    ioctl(fd, IOCTL_COMMAND);                  // Start the timer device!

    return fd;
}

/* closeDevice */
JNIEXPORT void JNICALL Java_com_example_androidex_MainActivity_closeDevice(JNIEnv *env, jclass this, jint fd) {
    close(fd);
}
