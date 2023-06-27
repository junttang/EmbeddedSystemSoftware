#include "headers.h"

/* Device file name and the major number */
#define DEV_NAME "dev_driver"
#define DEV_MAJOR_NUM 242

/* Case definitions of user-defined ioctl() handler */
#define IOCTL_SET_OPTION _IOW(DEV_MAJOR_NUM, 0, char*)
#define IOCTL_COMMAND _IO(DEV_MAJOR_NUM, 0)

/* Current timer instance */
static Timer cur_timer;

/* Core routines of this module */
static int __init dev_driver_init(void);
static void __exit dev_driver_exit(void);
static int dev_driver_open(struct inode *, struct file *);
static int dev_driver_close(struct inode *, struct file *);
static long dev_driver_ioctl(struct file *, unsigned int, unsigned long);

/* Redirect file I/O functions to user-defined handlers */
static struct file_operations dev_driver_fops = {
    .owner = THIS_MODULE,
    .open = dev_driver_open,
    .release = dev_driver_close,
    .unlocked_ioctl = dev_driver_ioctl
};

/* Internal functions to manage metadata attached to timer and perform operations */
static void _timer_init_data(const int, const int, const char *, const char *);
static void _timer_register(Timer *tm);
static void _timer_print(const Timer*);
static void _timer_start(void);
static void _timer_handler(unsigned long);

/* Data structures for providing blocking */
static atomic_t already_open = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/* Register the device driver provided by this module file, triggered by 'insmod' cmd */
static int __init dev_driver_init(void) {
    int ret_val;
    if ((ret_val = register_chrdev(DEV_MAJOR_NUM, DEV_NAME, &dev_driver_fops)) < 0) {
        printk("[DEV_DRIVER] Module register failed\n");
        return ret_val;
    }
    printk("[DEV_DRIVER] Device created on file(/dev/%s, major: %d)\n", 
        DEV_NAME, DEV_MAJOR_NUM);

    /* Map all the I/O devices in our FPGA board */
    if (fpga_map_devices() == FPGA_ERROR)
        return FPGA_ERROR;
    /* Be prepared to use timer */
    init_timer(&(cur_timer.timer)); 

    return ret_val;
}

/* Remove the device driver provided by this module file, by triggered 'rmmod' cmd */
static void __exit dev_driver_exit(void) {
    printk("[DEV_DRIVER] Driver removed\n");

    /* Remove the module */
    unregister_chrdev(DEV_MAJOR_NUM, DEV_NAME);

    /* Delete timer */
    del_timer_sync(&(cur_timer.timer));

    /* Unmap all the FPGA I/O devices */ 
    fpga_unmap_devices();
}

/* open() Handler for this driver */
static int dev_driver_open(struct inode *inode, struct file *file) {
    /* If file's flag include O_NONBLOCK and is already open, then decline it */
    if ((file->f_flags & O_NONBLOCK) && atomic_read(&already_open))
        return -EAGAIN;

    /* Notify that this module is used now */
    try_module_get(THIS_MODULE);

    while (atomic_cmpxchg(&already_open, 0, 1)) {
        int i, is_sig = 0;

        /* If the module is used (when BLOCK mode), then put cur process to sleep */
        wait_event_interruptible(waitq, !atomic_read(&already_open));

        /* If the slept process is woke up by getting unpredictable signals */
        for (i = 0; i < _NSIG_WORDS && !is_sig; i++)
            is_sig = current->pending.signal.sig[i] &
                ~current->blocked.sig[i];

        if (is_sig) {
            module_put(THIS_MODULE);
            return -EINTR;
        }
    }

    /* Allow the access */
    printk("[DEV_DRIVER] %s called\n", __func__);
    return 0;
}

/* close() Handler for this driver */
static int dev_driver_close(struct inode *inode, struct file *file) {
    int leds;
    /* Set already_open to zero, so one of the processes in the waitq will be
       be able to set already_open back to one and to open the file. All the 
       other processes will be called when already_open is back to one, so they
       will go back to sleep */
    atomic_set(&already_open, 0);

    /* Wake up all the processes in waitq, so if anybody is waiting for the file
       they can have it */
    wake_up(&waitq);

    module_put(THIS_MODULE);

    del_timer_sync(&(cur_timer.timer));
    cur_timer.text[0][0] = '\0'; cur_timer.text[1][0] = '\0';
    cur_timer.date = 0; cur_timer.humid = 0; leds = 0;
    fpga_led_write(leds);                            // Visual effect

    /* Print the weather data */
    _timer_print(&cur_timer);

    printk("[DEV_DRIVER] %s called\n", __func__);
    return 0;
}

/* ioctl() Handler for this driver */
static long dev_driver_ioctl(
    struct file *file, unsigned int ioctl_num, unsigned long ioctl_param
) {
    int ret_val = 0;
    char *user_ptr;
    char read_buf[READ_BUF_SIZE + 1];
    char temp_buf[TEMP_BUF_SIZE] = {'\0'};
    char date_str[TEMP_BUF_SIZE + 1] = {'\0'};
    char weather[WEATHER_BUF_SIZE];
    long temperature, humid;

    /* Select the operation */
    switch (ioctl_num) {
    /* Case1: read all the parameters when the program starts */
    case IOCTL_SET_OPTION:
        printk("[DEV_DRIVER] %s called (SET_OPTION)\n", __func__);

        /* Get string data from the user application */
        user_ptr = (char*)ioctl_param;
        if (strncpy_from_user(read_buf, user_ptr, strlen_user(user_ptr)) >= 0) {
            printk("[DEV_DRIVER] DATA: %s\n", read_buf);
            /* Retrieve each variable from the read string */
            // First three characters indicate temperature
            strncpy(temp_buf, read_buf, 3);
            kstrtol(temp_buf, 10, &temperature);  // make int
            // Following three characters indicate humidity(% unit)
            strncpy(temp_buf, read_buf + 3, 3); 
            kstrtol(temp_buf, 10, &humid);
            // Four characters indicate the temperature
            strncpy(date_str, read_buf + 6, TEMP_BUF_SIZE); 
            // Last multi characters indicate the weather description
            strncpy(weather, read_buf + 10, WEATHER_BUF_SIZE); 

            /* Set metadata of timer as input */
            _timer_init_data(temperature, humid, date_str, weather);
        }
        else ret_val = -EFAULT;
        break;
    /* Case2: Start the timer */
    case IOCTL_COMMAND:
        printk("[DEV_DRIVER] %s called (COMMAND)\n", __func__);
        _timer_start();
        break;
    /* Other cases */
    default: 
        printk("[DEV_DRIVER] Undefined IOCTL command (%u)\n", ioctl_num);
        ret_val = -ENOTTY; 
        break;
    }

    return ret_val;
}

/* Internal Handlers Definitions */
/* Initialize metadata of our timer as given parameters */
static void _timer_init_data(const int temperature, const int humid, const char *date, const char *weather) {
    char temp_str[WEATHER_BUF_SIZE];
    char temp_weather_fore[WEATHER_BUF_SIZE];
    char temp_weather_info[WEATHER_BUF_SIZE];
    printk("[DEV_DRIVER} Inputs: (%d, %d, %s, %s)\n", temperature, humid, date, weather);

    /* Initialize data */
    kstrtol(date, 10, &(cur_timer.date));
    strncpy(temp_weather_fore, weather, WEATHER_BUF_SIZE);
    sprintf(temp_str, "%d", temperature + 1);
    strncpy(temp_weather_info, "temperature ", 12);
    strncpy(temp_weather_info + 12, temp_str, 2);
    strncpy(temp_weather_info + 14, "C", 1);
    printk("[DEV_DRIVER] Weather Fore: %s\n", temp_weather_fore);
    printk("[DEV_DRIVER] Weather Info: %s\n", temp_weather_info);

    /* Set printing data (TEXT_LCD) */
    strncpy(cur_timer.text[0], temp_weather_fore, WEATHER_BUF_SIZE);
    strncpy(cur_timer.text[1], temp_weather_info, WEATHER_BUF_SIZE);

    /* Set metadata */
    cur_timer.interval = 10; // 1second
    cur_timer.humid = humid;
    cur_timer.cnt = 0;
}

/* Register the timer structure to the timer list */
static void _timer_register(Timer *tm) {
    printk("[DEV_DRIVER] Timer is registered\n");
    
    /* Set the timer */
    tm->timer.expires = get_jiffies_64() + tm->interval * (HZ / 10);
    tm->timer.data = (unsigned long)&cur_timer;
    tm->timer.function = _timer_handler;   // This will be called for every expiration

    /* Register to the list */
    add_timer(&(tm->timer));
}

/* Print metadata to the FPGA I/O devices */
static void _timer_print(const Timer *tm) {
    printk("[DEV_DRIVER] Timer performs WRITEs\n");
    
    fpga_fnd_write(tm->date);                        // Date Info ex)0619
    fpga_text_lcd_write(tm->text[0], tm->text[1]);   // Weather description
    fpga_dot_write(tm->humid);                       // Humidity
}

/* Start the timer, with resetting the device */
static void _timer_start(void) {
    printk("[DEV_DRIVER] Timer starts now\n");
    
    /* Reset */
    del_timer_sync(&(cur_timer.timer));
    _timer_print(&cur_timer);     // Print the reset output
    _timer_register(&cur_timer);  // Register the timer
}

/* Callback handler function that is expected to be called whenever the timer expires */
static void _timer_handler(unsigned long timeout) {
    int leds;	
    Timer *tm = (Timer*)timeout;
    printk("[DEV_DRIVER] Timer Handler is called (cnt=%d)\n", ++(tm->cnt));

    /* If the counter reaches the display time threshold, then clear all */
    if (tm->cnt == DISPLAY_TIME) {
        tm->text[0][0] = '\0'; tm->text[1][0] = '\0';
        tm->date = 0; tm->humid = 0; leds = 0;
        fpga_led_write(leds);                            // Visual effect

        /* Print the weather data */
        _timer_print(tm);
    }
    /* Otherwise change the outputs of I/O devices in our FPGA board */
    else {
        /* Change the visual effect for every second */
        if ((tm->cnt % 2) == 0) 
            leds = LED_1 | LED_3 | LED_6 | LED_8;
        else leds = LED_2 | LED_4 | LED_5 | LED_7;
        fpga_led_write(leds);                            // Visual effect
        
        /* Turn off the all the devices */
        if (tm->cnt == 1)
            _timer_print(tm);
        
        /* Register the next timer */
        _timer_register(tm);
    }
}

/* Module installation */
module_init(dev_driver_init);
module_exit(dev_driver_exit);

/* License and author info */
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("20171643, Junhyeok Park");
