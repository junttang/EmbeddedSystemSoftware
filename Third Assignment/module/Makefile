obj-m	:= stopwatch.o
stopwatch-objs := stopwatch_driver.o fpga_fnd.o

default:
	$(MAKE) -C /work/achroimx_kernel SUBDIRS=$(shell pwd) modules ARCH=arm

push: default
	adb push -p $(subst .o,.ko,$(obj-m)) /data/local/tmp

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .*.cmd
	rm -rf .tmp_versions
