#include "core.h"

/* Descriptors of devices */
int devFD[DEVICE_CNT];

/* Addresses of devices that we use here */
const char DEVICE_ADDR[DEVICE_CNT][25] = {
    "/dev/fpga_fnd", 
    "/dev/mem", 
    "/dev/fpga_text_lcd", 
    "/dev/fpga_step_motor",  // Output devices
    "/dev/input/event0", 
    "/dev/fpga_push_switch"  // Input devices
};

/* Flags for each open function call of devices */
const int OPEN_FLAGS[DEVICE_CNT] = {
    O_RDWR,                 // FND
    O_RDWR | O_SYNC,        // LED
    O_WRONLY,               // TEXT_LCD
    O_WRONLY,               // MOTOR
    O_RDONLY | O_NONBLOCK,  // KEY
    O_RDWR                  // SWITCH
};

/* Assigned addresses */
unsigned long* mappedAddr;
unsigned char* ledAddr;

/* Strings for error reporting */
const char DEVICE_NAME[DEVICE_CNT][10] = { 
    "FND", "LED", "TEXT_LCD", "MOT", 
    "KEY", "SWITCH" 
};
const char MSG_TYPE[2][10] = { 
    "ERROR", "LOG"
};

/* Error reporting function */
static void devMsg(const DEVICES, const bool msgOption, const char *, ...);

/* Open all the devices we need here */
void devOpen(void) {
    DEVICES dv;
    for (dv = FND; dv <= SWITCH; dv++) {
        if ((devFD[dv] = open(DEVICE_ADDR[dv], OPEN_FLAGS[dv])) < 0)
            devMsg(dv, ERROR_MSG, "Error occurs in device opens\n");
    }

    // Memory mapping for LED device
    mappedAddr = (unsigned long*)mmap(NULL, FPGA_MAP_LEN, 
        PROT_READ | PROT_WRITE, MAP_SHARED, devFD[LED], FPGA_BASE_ADDR);
    if (mappedAddr == MAP_FAILED) 
        devMsg(LED, ERROR_MSG, "Error occurs in memory mapping FPGA\n");
    ledAddr = (unsigned char*)((void*)mappedAddr + LED_ADDR_OFFSET);
}

/* Close all the devices we need here */
void devClose(void) {
    DEVICES dv;
    for (dv = FND; dv <= SWITCH; dv++) {
        if (close(devFD[dv]) < 0) 
            devMsg(dv, ERROR_MSG, "Error occurs in device closes\n");
    }

    // Unmap the memory mapped area
    if (munmap((void*) mappedAddr, FPGA_MAP_LEN) < 0) 
        devMsg(LED, ERROR_MSG, "Error occurs in memory unmapping FPGA\n");
}

/* Print data to the device (FND) */
void fndPrint(const int data) {
    unsigned char digits[MAX_DIGIT + 1] = { '\0' };
    int i, digit = 1;

    if (data < 0 || data > 9999) 
        devMsg(FND, ERROR_MSG, "Error occurs in FND printing (wrong data)\n");

    // Construct the array
    for (i = MAX_DIGIT - 1; i >= 0; i--) {
        digits[i] = (data / digit) % 10;
        digit = digit * 10;
    }

    // Write to device
    if (write(devFD[FND], digits, MAX_DIGIT) < 0) 
        devMsg(FND, ERROR_MSG, "Error occurs in device write function (FND)\n");

    devMsg(FND, LOG_MSG, "FND Write: %d\n", data);
}

/* Reset the device (FND) */
void fndReset(void) {
    devMsg(FND, LOG_MSG, "Reset Action - ");
    fndPrint(0);
}

/* Print data to the device (LED) */
void ledPrint(const int data) {
    if (data < 0 || data > 255) 
        devMsg(LED, ERROR_MSG, "Error occurs in LED printing (wrong data)");

    // Write to device
    *ledAddr = *((int*) data);

    devMsg(LED, LOG_MSG, "LED Write: %d\n", data);
}

/* Reset the device (LED) */
void ledReset(void) {
    devMsg(FND, LOG_MSG, "Reset Action - ");
    ledPrint(0);
}

/* Print data to the device (LCD) */
void textlcdPrint(const char* data) {
    char tempBuf[MAX_TEXT_LEN + 1] = { '\0' };
    int len = strlen(data);

    if (len > MAX_TEXT_LEN) 
        devMsg(LED, ERROR_MSG, "Error occurs in LED printing (wrong data)");
    strncpy(tempBuf, data, MAX_TEXT_LEN);
    memset((tempBuf + len), ' ', (MAX_TEXT_LEN - len));

    // Write to device
    if (write(devFD[TEXT_LCD], tempBuf, MAX_TEXT_LEN) < 0) 
        devMsg(TEXT_LCD, ERROR_MSG, "Error occurs in device write function (TEXT_LCD)\n");
    
    devMsg(LED, LOG_MSG, "TEXT_LCD Write: %s\n", tempBuf);
}

/* Reset the device (LCD) */
void textLcdReset(void) {
    devMsg(TEXT_LCD, LOG_MSG, "Reset Action - ");
    textlcdPrint("");
}

/* Print data to the device (MOTOR) */
void motorPrint(const bool motorOn) {
    unsigned char motorState[3];

    if (motorOn) {
        memset(motorState, 0, sizeof(motorState));
        motorState[0]=(unsigned char)1; 
	    motorState[1]=(unsigned char)1;     
	    motorState[2]=(unsigned char)64;   
        if (write(devFD[MOTOR], motorState, 3) < 0) 
            devMsg(MOTOR, ERROR_MSG, "Error occurs in device write function (MOTOR)\n");

        devMsg(MOTOR, LOG_MSG, "MOTOR Write: Run State\n");
    }
    else devMsg(MOTOR, LOG_MSG, "MOTOR Write: Stopped State\n");
}

void motorReset(void) {
    devMsg(MOTOR, LOG_MSG, "Reset Action - ");
    motorPrint(0);
}

/* Read data from the device (KEY) */
int keyRead(void) {
    struct input_event keyBuf[KEY_CNT];

    if (read(devFD[KEY], keyBuf, sizeof(keyBuf)) >= sizeof(struct input_event)) {
        if (keyBuf[0].value == PRESSED) {
            if (keyBuf[0].code == KEY_VOLUMEDOWN ||
                keyBuf[0].code == KEY_VOLUMEUP ||
                keyBuf[0].code == KEY_BACK) {
                devMsg(MOTOR, LOG_MSG, "KEY Read: %d\n", keyBuf[0].code);
                return keyBuf[0].code;
            }
        } 
    }
    devMsg(MOTOR, ERROR_MSG, "Error occurs in device read function (KEY)\n");
}

/* Read data from the device (SWITCH) */
int switchRead(void) {
    unsigned char switchBuf[SWITCH_CNT];
    int i, swch = 0;

    read(devFD[SWITCH], &switchBuf, sizeof(switchBuf));
    for (i = 0; i < SWITCH_CNT; i++) {
        swch = swch * 10;
        if (switchBuf[i] == PRESSED) 
            swch += 1;
    }

    devMsg(SWITCH, LOG_MSG, "KEY Read: %09d\n", swch);
    return swch;
}

/* Error reporting function */
static void devMsg(const DEVICES device, const bool msgOption, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[%s] %s \t - ", MSG_TYPE[msgOption], DEVICE_NAME[device]);
    vfprintf(format, args);
    va_end(args);
    if (msgOption == ERROR_MSG)
        exit(-1);
}
