* Title "Stopwatch Driver Module"
- by Junhyeok Park, 20171643
- last revision: 28/05/23

* Description
- This repository contains source codes of my development for the third assignment of CSE4116 'Embedded System Software' lecture taken in spring semester, Sogang University.

* Environment
- You need to install 'arm-none-linux-gnueabi-gcc' cross compiler provided by ARM to compile this program.
- And also you need a FPGA board to run this program, especially the one given by our lecture.
- Using minicom is preferred for proper monitoring of this program running on the board.

* Configurations
- module: repository containing all the source codes for constructing the Stopwatch Driver
   - headers.h: it includes all the header files necessary for compiling the Stopwatch Driver Module.
   - fpga_fnd.h: it constructs the controlling system of FND device of FPGA board, by using a device driver package provided previously.
   - stopwatch_driver.h: it defines the main module file of Stopwatch Driver, while including all the necessary routine to satisfy all conditions described in the guidance PDF, like timer installation, interrupts handling, and etc. 
   - Makefile: Makefile for this module.
     ~> "make": compilation 
     ~> "make push": adb push action (to /data/local/tmp of board)
- app: repository containing the user application 'app' which is created for a verifcation of the module.
   - app.c: it performs open(...), write(...), and close(...) respectively.
   - Makefile: Makefile for this program.
     ~> "make": compilation 
     ~> "make push": adb push action (to /data/local/tmp of board)
- Readme.txt: this file
- Document.pdf: report file that describes my development in very detail.

* Notification
- Driver name: "stopwatch" (/dev/stopwatch)
- Major number: 242 (minor: 0)
(After you finished compilations of both module and app, then push both created module and 'app' program to /data/local/tmp of FPGA board. If push succeeds, then type below commands sequentially. 1) "insmod stopwatch.ko"
2) "mknod /dev/stopwatch c 242 0"
3) "./app"
4) "rmmod stopwatch" (if you want to remove the driver))

* Notes
- You can find out detailed explanation of this program in the attached PDF report.
- If you have any question for this program, then please contact me via e-mail or cyber campus. (e-mail: junttang123@naver.com)
