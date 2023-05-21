default:
	arm-none-linux-gnueabi-gcc -Wall -static -o app timer.c

push: default
	adb push -p app /data/local/tmp

clean:
	rm -rf app
