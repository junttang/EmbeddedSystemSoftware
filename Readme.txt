* Title "Key-Value Store"
- by Junhyeok Park, 20171643
- last revision: 16/04/23

* Description
- This repository contains source codes of my development for the first assignment of CSE4116 'Embedded System Software' lecture taken in spring semester, Sogang University.

* Environment
- You need to install 'arm-none-linux-gnueabi-gcc' cross compiler provided by ARM to compile this program.
- And also you need a FPGA board to run this program, especially the one given by our lecture.
- Using minicom is preferred for proper monitoring of this program running on the board.

* Configurations
- headers.h: it includes all the header files necessary for compiling this program.
- main.h: all the fundamental routines of this program and the handlers for Main Process and Merge Process are described in here.
- device_control.h: it constructs the controlling system of I/O devices of FPGA board, by using device drivers that are provided in the 2nd exerice.
- ipc.h: it provides synchronization methods and shared memories, which are crucial for communication of multiple processes forked during the control flow of this program.
- io_process.h: it defines I/O process of this program.
- kv_api.h: Key-Value interfaces of this program, including PUT, GET, MERGE operations based on the simple array based Key-Value Store concept described in the guidance PDF.
- Makefile: Makefile for this program.
~> "make": compilation
~> "make push": adb push action (to /data/local/tmp of board)
- Readme.txt: this file
- hw1_20171643.pdf: report file that describes my development in very detail.

* Notes
- You can find out detailed explanation of this program in the attached PDF report.
- If you have any question for this program, then please contact me via e-mail or cyber campus. (e-mail: junttang123@naver.com)
