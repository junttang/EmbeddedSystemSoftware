#include "headers.h"

/* Mapped address of FND device */
static char *fnd_addr;

/* Bus address of FND device */
#define FND_ADDR 0x08000004
#define FND_MAP_SIZE 0x2

/* Map the NFD device provided by our FPGA board */
int fpga_fnd_map(void) {
    printk("[FND DEVICE] Open the FND device\n");

    fnd_addr = ioremap(FND_ADDR, FND_MAP_SIZE);
    if (fnd_addr == NULL) {
        printk("[FND DEVICE] Failed to map the FND device\n");
        return FND_ERROR;
    }

    return 0;
}

/* Unmap the FND device provided by our FPGA board */
void fpga_fnd_unmap(void) {
    printk("[FND DEVICE] Close all the open I/O devices\n");
    iounmap(fnd_addr);
}

/* Print the time(min:sec) to FND device */
void fpga_fnd_write(const int min, const int sec) {
    int i, time; unsigned short value = 0;
    time = 100 * min + sec;
    for (i = 0; i < 4; i++, time = time / 10) 
        value += (time%10) << (4*i);

    outw(value, (unsigned int)fnd_addr);
    printk("[FND DEVICE] FND WRITE: (%02d:%02d)\n", min, sec);
}
