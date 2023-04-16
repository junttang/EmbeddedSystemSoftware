default: main.o ipc.o io_process.o kv_api.o device_control.o
	arm-none-linux-gnueabi-gcc -static -Wall -lm -o hw1_20171643 main.o ipc.o io_process.o kv_api.o device_control.o

push: default
	adb push ./hw1_20171643 /data/local/tmp

main.o: headers.h main.h main.c
	arm-none-linux-gnueabi-gcc -static -Wall -lm -c -o main.o main.c

ipc.o: headers.h ipc.h ipc.c
	arm-none-linux-gnueabi-gcc -static -Wall -lm -c -o ipc.o ipc.c

io_process.o: headers.h io_process.h io_process.c
	arm-none-linux-gnueabi-gcc -static -Wall -lm -c -o io_process.o io_process.c

kv_api.o: headers.h kv_api.h kv_api.c
	arm-none-linux-gnueabi-gcc -static -Wall -lm -c -o kv_api.o kv_api.c

device_control.o: headers.h device_control.h device_control.c
	arm-none-linux-gnueabi-gcc -static -Wall -lm -c -o device_control.o device_control.c

clean:
	-rm *.o
	-rm *.out
