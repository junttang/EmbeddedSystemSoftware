default:
	arm-none-linux-gnueabi-gcc -Wall -static -o app app.c

push: default
	adb push -p app /data/local/tmp

clean:
	rm -rf app
