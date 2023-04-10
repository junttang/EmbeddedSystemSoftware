#ifndef _DEVICE_CONTROL_H_
#define _DEVICE_CONTROL_H_

/* Device names */
#define DEVICE_CNT 6        // FND, LED, TEXT_LCD, MOT, KEY, SWITCH
#define O_DEVICE_CNT 4      // FND, LED, TEXT_LCD, MOT
typedef enum _devices { 
    FND, LED, TEXT_LCD, MOTOR,      // Output devices
    KEY, SWITCH                     // Input devices
} DEVICES;

/* Constants for FND device */
#define MAX_DIGIT 4

/* Constants for LED device */
#define FPGA_BASE_ADDR      0x08000000
#define FPGA_MAP_LEN        0x00001000
#define LED_ADDR_OFFSET     0x00000016
#define LED_1               0x10000000
#define LED_2               0x01000000
#define LED_3               0x00100000
#define LED_4               0x00010000
#define LED_5               0x00001000
#define LED_6               0x00000100
#define LED_7               0x00000010
#define LED_8               0x00000001

/* Constants for TEXT_LCD device */
#define MAX_TEXT_LEN 32

/* Constants for KEY device */
#define KEY_CNT 64

/* Constants for SWITCH device */
#define SWITCH_CNT 9
#define PRESSED 1

/* Options for error reporting */
#define ERROR_MSG 0
#define LOG_MSG   1

/* Creating environments routines */
void devOpen(void);
void devClose(void);

/* FND-related routines */
void fndPrint(const int);
void fndReset(void);

/* LED-related routines */
void ledPrint(const int);
void ledReset(void);

/* TEXT_LCD-related routines */
void textlcdPrint(const char *);
void textLcdReset(void);

/* MOTOR-related routines */
void motorPrint(const bool);
void motorReset(void);

/* KEY-related routines */
int keyRead(void);

/* SWITCH-related routines */
int switchRead(void);

#endif /* _DEVICE_CONTROL_H_ */
