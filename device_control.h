#ifndef _DEVICE_CONTROL_H_
#define _DEVICE_CONTROL_H_

/* Device names */
#define DEVICE_CNT 6                // FND, LED, TEXT_LCD, MOT, KEY, SWITCH
#define O_DEVICE_CNT 4              // FND, LED, TEXT_LCD, MOT
typedef enum _devices { 
    FND, LED, TEXT_LCD, MOTOR,      // Output devices
    KEY, SWITCH                     // Input devices
} DEVICES;

/* Constants for FND device */
#define MAX_DIGIT 4

/* Constants for LED device */
#define FPGA_BASE_ADDR      0x08000000
#define LED_ADDR                  0x16
#define LED_1               0x00000080
#define LED_2               0x00000040
#define LED_3               0x00000020
#define LED_4               0x00000010
#define LED_5               0x00000008
#define LED_6               0x00000004
#define LED_7               0x00000002
#define LED_8               0x00000001

/* Constants for TEXT_LCD device */
#define MAX_TEXT_LEN 32

/* Constants for KEY device */
#define MAX_KEY_CNT 64

/* Constants for SWITCH device */
#define SWITCH_CNT 9
#define SWITCH_RESET 24680246
#define PRESSED 1
#define NOT_PRESSED 0

/* Options for error reporting */
#define ERROR_MSG 0
#define LOG_MSG   1

/* Device control nvironments routines */
void devOpen(void);
void devClose(void);

/* OUTPUTs */
/* FND-related routine */
void fndPrint(const int);
/* LED-related routine */
void ledPrint(const int);
/* TEXT_LCD-related routine */
void textlcdPrint(const char *);
/* MOTOR-related routine */
void motorPrint(const bool);

/* INPUTs */
/* KEY-related routine */
int keyRead(void);
/* SWITCH-related routine */
int switchRead(void);

#endif /* _DEVICE_CONTROL_H_ */
