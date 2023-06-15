#ifndef _FPGA_FND_H_
#define _FPGA_FND_H_

/* Error identifier */
#define FND_ERROR -1

/* Interfaces to use FPGA I/O FND device */
int fpga_fnd_map(void);
void fpga_fnd_unmap(void);
void fpga_fnd_write(const int, const int);

#endif /* _FPGA_FND_H_ */
