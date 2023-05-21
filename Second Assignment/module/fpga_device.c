#include "headers.h"

/* Metadata of FPGA I/O devices */
char *fpga_addr[FPGA_DEV_CNT];
static const int FPGA_DEV_ADDR[] = { 0x08000004, 0x08000016, 0x08000090, 0x08000210 };
static const int FPGA_DEV_PROT[] = { 0x2, 0x1, 0x32, 0xA };
static const char *FPGA_DEV_NAME[] = { "FND", "LED", "TEXT_LCD", "DOT" };

/* Fixed dot configurations for printing 1 ~ 8 and blank */
static const unsigned char dot_digit[DIGIT_NUM + 1][DOT_NUM + 1] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// ' '
    {0x0c, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e},// '1'
    {0x7e, 0x7f, 0x03, 0x03, 0x3f, 0x7e, 0x60, 0x60, 0x7f, 0x7f},// '2'
    {0xfe, 0x7f, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7e},// '3'
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7f, 0x7f, 0x06, 0x06},// '4'
    {0x7f, 0x7f, 0x60, 0x60, 0x7e, 0x7f, 0x03, 0x03, 0x7f, 0x7e},// '5'
    {0x60, 0x60, 0x60, 0x60, 0x7e, 0x7f, 0x63, 0x63, 0x7f, 0x3e},// '6'
    {0x7f, 0x7f, 0x63, 0x63, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03},// '7'
    {0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x3e},// '8'
};

/* Map all the I/O devices provided by our FPGA board */
int fpga_map_devices(void) {
    FPGA dev;
    printk("[FPGA DEVICE] Open all the necessary I/O devices\n");

    for (dev = FND; dev <= DOT; dev++) {
        fpga_addr[dev] = ioremap(FPGA_DEV_ADDR[dev], FPGA_DEV_PROT[dev]);
        if (fpga_addr[dev] == NULL) {
            printk("[FPGA DEVICE] Failed to map device '%s'\n", FPGA_DEV_NAME[dev]);
            return FPGA_ERROR;
        }
    }

    return 0;
}

/* Unmap all the I/O devices provided by our FPGA board */
void fpga_unmap_devices(void) {
    FPGA dev;
    printk("[FPGA DEVICE] Close all the open I/O devices\n");

    for (dev = FND; dev <= DOT; dev++) 
        iounmap(fpga_addr[dev]);
}

/* Print the digit to FND device */
void fpga_fnd_write(const int idx, const int digit) {
    unsigned short value = digit << (12 - 4*idx);
    outw(value, (unsigned int)fpga_addr[FND]);
    printk("[FPGA DEVICE] FND WRITE: %d (idx: %d)\n", digit, idx);
}

/* Print the digit to LED device */
void fpga_led_write(const int digit) {
    unsigned short value = ((digit > 0) ? (1 << (8 - digit)) : 0);
    outw(value, (unsigned int)fpga_addr[LED]);
    printk("[FPGA DEVICE] LED WRITE: %d (LED%d)\n", digit, digit);
}

/* Print two text lines to TEXT_LCD device */
void fpga_text_lcd_write(const char *text1, const int text1_prev_idx, const char *text2, const int text2_prev_idx) {
    unsigned short value;
    unsigned char text_buf[TEXT_LCD_BUF_SIZE + 1] = {0};
    int i, text1_len, text1_idx, text2_len, text2_idx;
    text1_len = strlen(text1); text2_len = strlen(text2);
    text1_idx = text1_prev_idx; text2_idx = TEXT_LCD_BUF_SIZE / 2 + text2_prev_idx;

    /* Text at the first line */
    // Fill the front remainders of first line with spaces
    memset(text_buf, ' ', text1_idx);
    strncat(text_buf, text1, text1_len);
    printk("[FPGA DEVICE] TEXT_LCD WRITE: \"%-16s\" (line1)\n", text_buf);

    /* Text at the second line */
    // Fill the remainders of first line with spaces
    memset(text_buf + text1_len + text1_idx, ' ', text2_idx - (text1_len + text1_idx));
    strncat(text_buf, text2, text2_len);
    // Fill the remainders of second line with spaces
    memset(text_buf + text2_len + text2_idx, ' ', TEXT_LCD_BUF_SIZE - (text2_len + text2_idx));
    text_buf[TEXT_LCD_BUF_SIZE] = '\0';
    printk("[FPGA DEVICE] TEXT_LCD WRITE: \"%-16s\" (line2)\n", text_buf + (TEXT_LCD_BUF_SIZE / 2));

    /* Write to buffer */
    for (i = 0; i < TEXT_LCD_BUF_SIZE; i += 2) {
        value = (text_buf[i] & 0xFF) << DIGIT_NUM | (text_buf[i + 1] & 0xFF);
        outw(value, (unsigned int)fpga_addr[TEXT_LCD] + i);
    }
}

/* Print the digit to the DOT device. */
void fpga_dot_write(const int digit) {
    int i;
    printk("[FPGA DEVICE] DOT WRITE: %d\n", digit);

    /* Write to buffer */
    for (i = 0; i <= DOT_NUM; i++) 
        outw(dot_digit[digit][i] & 0x7F, (unsigned int)fpga_addr[DOT] + i * 2);
}
