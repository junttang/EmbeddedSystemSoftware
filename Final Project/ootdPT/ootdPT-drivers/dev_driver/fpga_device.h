#ifndef _FPGA_DEVICE_H_
#define _FPGA_DEVICE_H_

/* # of FPGA I/O devices */
#define FPGA_DEV_CNT 4

/* Enum type for FPGA I/O devices */
typedef enum _FPGA_DEV { 
    FND, LED, TEXT_LCD, DOT 
} FPGA;

/* LED device output conventions */
#define LED_1               0x00000080
#define LED_2               0x00000040
#define LED_3               0x00000020
#define LED_4               0x00000010
#define LED_5               0x00000008
#define LED_6               0x00000004
#define LED_7               0x00000002
#define LED_8               0x00000001

/* Output buffer size of TEXT_LCD */
#define TEXT_LCD_BUF_SIZE 32

/* # of dots in DOT device */
#define DOT_NUM 9

/* # of digits */
#define DIGIT_NUM 8

/* Error identifier */
#define FPGA_ERROR -1

/* Interfaces to use FPGA I/O devices */
int fpga_map_devices(void);
void fpga_unmap_devices(void);
void fpga_fnd_write(const int);
void fpga_led_write(const int);
void fpga_text_lcd_write(const char *, const char *);
void fpga_dot_write(const int);

#endif /* _FPGA_DEVICE_H_ */
