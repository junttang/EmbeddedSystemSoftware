/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_example_androidex_TurnOnOffThread */

#ifndef _Included_com_example_androidex_TurnOnOffThread
#define _Included_com_example_androidex_TurnOnOffThread
#ifdef __cplusplus
extern "C" {
#endif
#undef com_example_androidex_TurnOnOffThread_MIN_PRIORITY
#define com_example_androidex_TurnOnOffThread_MIN_PRIORITY 1L
#undef com_example_androidex_TurnOnOffThread_NORM_PRIORITY
#define com_example_androidex_TurnOnOffThread_NORM_PRIORITY 5L
#undef com_example_androidex_TurnOnOffThread_MAX_PRIORITY
#define com_example_androidex_TurnOnOffThread_MAX_PRIORITY 10L
/*
 * Class:     com_example_androidex_TurnOnOffThread
 * Method:    openDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_example_androidex_TurnOnOffThread_openDevice
  (JNIEnv *, jclass);

/*
 * Class:     com_example_androidex_TurnOnOffThread
 * Method:    writeDevice
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_example_androidex_TurnOnOffThread_writeDevice
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_example_androidex_TurnOnOffThread
 * Method:    closeDevice
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_example_androidex_TurnOnOffThread_closeDevice
  (JNIEnv *, jclass, jint);

#ifdef __cplusplus
}
#endif
#endif
