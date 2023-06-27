echo "7 6 1 7" > /proc/sys/kernel/printk
rmmod dev_driver.ko
insmod dev_driver.ko
mknod /dev/dev_driver c 242 0
rmmod intr_driver.ko
insmod intr_driver.ko
mknod /dev/intr_driver c 243 0
chmod 777 /dev/dev_driver
chmod 777 /dev/intr_driver
