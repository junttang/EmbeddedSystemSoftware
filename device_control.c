#include "headers.h"

/* For logging */
#define MSG_DISABLE

/* Descriptors of device drivers */
int devFD[DEVICE_CNT];

/* Addresses of device drivers that we use here */
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

/* Assigned addresses for LED control */
unsigned long *mappedAddr;
unsigned char *ledAddr;

/* Open all the devices we need here */
void devOpen(void) {
    DEVICES dv;
    for (dv = FND; dv <= SWITCH; dv++) {
        if ((devFD[dv] = open(DEVICE_ADDR[dv], OPEN_FLAGS[dv])) < 0)
            perror("Error occurs in device opens");
    }

    // Memory mapping for LED device
    mappedAddr = (unsigned long*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, 
        MAP_SHARED, devFD[LED], FPGA_BASE_ADDR);
    if (mappedAddr == MAP_FAILED) 
        perror("Error occurs in memory mapping FPGA");
    ledAddr = (unsigned char*)((void*)mappedAddr + LED_ADDR);
}

/* Close all the devices we need here */
void devClose(void) {
    DEVICES dv;
    // Turn off all the output devices
    fndPrint(0); ledPrint(0);
    textlcdPrint(""); motorPrint(0);

    for (dv = FND; dv <= SWITCH; dv++) {
        if (close(devFD[dv]) < 0) 
            perror("Error occurs in device closes");
    }

    // Unmap the memory mapped area (of LED)
    if (munmap((void*) mappedAddr, 4096) < 0) 
        perror("Error occurs in memory unmapping FPGA");
}

/* Print data to the device (FND) */
void fndPrint(const int data) {
    unsigned char digits[MAX_DIGIT + 1] = { '\0' };
    int i, digit = 1;
    if (data < 0 || data > 9999) 
        perror("Error occurs in FND printing (wrong data)");

    // Construct the string
    for (i = MAX_DIGIT - 1; i >= 0; i--) {
        digits[i] = (data / digit) % 10;
        digit = digit * 10;
    }

    // Write to device
    write(devFD[FND], digits, MAX_DIGIT);
}

/* Print data to the device (LED) */
void ledPrint(const int data) {
    int leds = data;
    if (leds < 0 || leds > 255) 
        perror("Error occurs in LED printing (wrong data)");

    // Write to device
    *ledAddr = leds;
}

/* Print data to the device (LCD) */
void textlcdPrint(const char* data) {
    char tempBuf[MAX_TEXT_LEN + 1] = { '\0' };
    int len = strlen(data);
    if (len > MAX_TEXT_LEN) 
        perror("Error occurs in TEXT_LCD printing (wrong data)");
    strncpy(tempBuf, data, MAX_TEXT_LEN);
    memset((tempBuf + len), ' ', (MAX_TEXT_LEN - len));

    // Write to device
    write(devFD[TEXT_LCD], tempBuf, MAX_TEXT_LEN);
}

/* Print data to the device (MOTOR) */
void motorPrint(const bool motorOn) {
    unsigned char motorState[3];

    memset(motorState, 0, sizeof(motorState));
    if (motorOn) {
        motorState[0]=(unsigned char)1; 
        motorState[1]=(unsigned char)1;     
        motorState[2]=(unsigned char)64;   
    }
    else {
        motorState[0]=(unsigned char)0; 
        motorState[1]=(unsigned char)0;     
        motorState[2]=(unsigned char)0;   
    }

    // Write to device
    write(devFD[MOTOR], motorState, 3);
}

/* Read data from the device (KEY) */
int keyRead(void) {
    struct input_event keyBuf[MAX_KEY_CNT];

    // Read from the device
    if (read(devFD[KEY], keyBuf, sizeof(keyBuf)) >= sizeof(struct input_event)) {
        if (keyBuf[0].value == PRESSED) {
            if (keyBuf[0].code == KEY_VOLUMEDOWN || 
                keyBuf[0].code == KEY_VOLUMEUP || 
                keyBuf[0].code == KEY_BACK) {
                keyBuf[0].value = NOT_PRESSED;
                return keyBuf[0].code;
            }
        } 
    }

    // No Key read
    return 0;
}

/* Read data from the device (SWITCH) */
int switchRead(void) {
    unsigned char switchBuf[SWITCH_CNT];
    int i, swch = 0;

    // Read from the device
    read(devFD[SWITCH], &switchBuf, sizeof(switchBuf));
    
    // RESET Pressed
    if (switchBuf[0] == 80 && switchBuf[SWITCH_CNT - 1] == 96)
        swch = SWITCH_RESET;
    // Otherwise (normal switch)
    else {
        for (i = 0; i < SWITCH_CNT; i++) {
            swch = swch * 10;
            if (switchBuf[i] == PRESSED) 
                swch += 1;
        }
    }

    return swch;
}
