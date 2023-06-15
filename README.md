# Assignments and Projects of 'Embedded System Software(CSE4116)' 
- This repository contains source codes for assignments and projects of CSE4116 lecture 'Embedded System Software' which I took in spring semester, 2023, in Sogang University.

## 1st Assignment of Embedded System Software 'KV Store on FPGA board'
- The Key-Value Store here is pretty simple and naive because the ultimate goal of this assignment is not that implementing real-world-level Key-Value Store which is normally based on LSM-tree or hash, but that experiencing controlling I/O devices of FPGA board and cross-compilation environment. So do not take seriously about the KV store itself.
- This program satisfies all the requirements described in the guidance PDF. 

## 2nd Assignment of Embedded System Software 'Timer Driver for counting'
- What we have to do here is creating a simple time-counter based on module programming and timer interface provided by linux kernel.
- This program satisfies all the requirements described in the guidance PDF. 

## 3rd Assignment of Embedded System Software 'Stopwatch Driver'
- Similar to the 2nd assignment, but here we need to implement a stopwatch program to measure the time.
- The main difference between 2nd and 3rd is that here we need to split interrupt handlers into top and bottom halves.
- In the Linux Kernel, it is considered good practice to avoid placing lengthy tasks directly within the interrupt handler. Instead, it is recommended to split such tasks into two separate parts known as the top half and the bottom half. The top half is responsible for handling timing-critical actions, while the bottom half takes care of other tasks. This division ensures efficient and responsive interrupt handling while allowing for the execution of non-time-critical operations in a separate context. By separating these tasks, the system can maintain its performance and promptly respond to critical events without being delayed by lengthy operations.

* ALERT
-> **Readers, if you are a student who takes the same lecture in the future, DO NOT JUST COPY these codes. You can refer to the code to help yourself do the homework efficiently, but do not just copying at all. It must be detected via copy detector.**
