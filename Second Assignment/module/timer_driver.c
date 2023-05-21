#include "headers.h"

/* Device file name and the major number (PDF conditions) */
#define DEV_NAME "dev_driver"
#define DEV_MAJOR_NUM 242

/* Case definitions of user-defined ioctl() handler */
#define IOCTL_SET_OPTION _IOW(DEV_MAJOR_NUM, 0, char*)
#define IOCTL_COMMAND _IO(DEV_MAJOR_NUM, 0)

/* Current timer instance */
static Timer cur_timer;

/* Core routines of this module */
static int __init timer_driver_init(void);
static void __exit timer_driver_exit(void);
static int timer_driver_open(struct inode *, struct file *);
static int timer_driver_close(struct inode *, struct file *);
static long timer_driver_ioctl(struct file *, unsigned int, unsigned long);

/* Redirect file I/O functions to user-defined handlers */
static struct file_operations timer_driver_fops = {
    .owner = THIS_MODULE,
    .open = timer_driver_open,
    .release = timer_driver_close,
    .unlocked_ioctl = timer_driver_ioctl
};

/* Internal functions to manage metadata attached to timer and perform operations */
static void _timer_init_data(const int, const int, const char *);
static void _timer_register(Timer *tm);
static void _timer_print(const Timer*);
static void _timer_start(void);
static void _timer_handler(unsigned long);

/* Data structures for providing blocking */
static atomic_t already_open = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/* Two text lines for TEXT_LCD */
static char *student_id = "2017643";
static char *student_name = "Junhyeok Park";

/* Register the device driver provided by this module file, triggered by 'insmod' cmd */
static int __init timer_driver_init(void) {
    int ret_val;
    if ((ret_val = register_chrdev(DEV_MAJOR_NUM, DEV_NAME, &timer_driver_fops)) < 0) {
        printk("[TIMER DRIVER] Module register failed\n");
        return ret_val;
    }
    printk("[TIMER DRIVER] Device created on file(/dev/%s, major: %d)\n", 
        DEV_NAME, DEV_MAJOR_NUM);

    /* Map all the I/O devices in our FPGA board */
    if (fpga_map_devices() == FPGA_ERROR)
        return FPGA_ERROR;
    /* Be prepared to use timer */
    init_timer(&(cur_timer.timer)); 

    return ret_val;
}

/* Remove the device driver provided by this module file, by triggered 'rmmod' cmd */
static void __exit timer_driver_exit(void) {
    printk("[TIMER DRIVER] Driver removed\n");

    /* Remove the module */
    unregister_chrdev(DEV_MAJOR_NUM, DEV_NAME);

    /* Delete timer */
    del_timer_sync(&(cur_timer.timer));

    /* Unmap all the FPGA I/O devices */ 
    fpga_unmap_devices();
}

/* open() Handler for this driver */
static int timer_driver_open(struct inode *inode, struct file *file) {
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
    printk("[TIMER DRIVER] %s called\n", __func__);
    return 0;
}

/* close() Handler for this driver */
static int timer_driver_close(struct inode *inode, struct file *file) {
    /* Set already_open to zero, so one of the processes in the waitq will be
       be able to set already_open back to one and to open the file. All the 
       other processes will be called when already_open is back to one, so they
       will go back to sleep */
    atomic_set(&already_open, 0);

    /* Wake up all the processes in waitq, so if anybody is waiting for the file
       they can have it */
    wake_up(&waitq);

    module_put(THIS_MODULE);

    printk("[TIMER DRIVER] %s called\n", __func__);
    return 0;
}

/* ioctl() Handler for this driver */
static long timer_driver_ioctl(
    struct file *file, unsigned int ioctl_num, unsigned long ioctl_param
) {
    int ret_val = 0;
    char *user_ptr;
    char read_buf[READ_BUF_SIZE + 1];
    char temp_buf[TEMP_BUF_SIZE] = {'\0'};
    char fnd_init[TEMP_BUF_SIZE + 1] = {'\0'}; long interval, cnt;

    /* Select the operation */
    switch (ioctl_num) {
    /* Case1: read all the parameters when the program starts */
    case IOCTL_SET_OPTION:
        printk("[TIMER DRIVER] %s called (SET_OPTION)\n", __func__);

        /* Get string data from the user application */
        user_ptr = (char*)ioctl_param;
        if (strncpy_from_user(read_buf, user_ptr, strlen_user(user_ptr)) >= 0) {
            /* Retrieve each variable from the read string */
            // First three characters indicate interval value
            strncpy(temp_buf, read_buf, 3);
            kstrtol(temp_buf, READ_BUF_SIZE, &interval);  // make int
            // Following three characters indicate count value
            strncpy(temp_buf, read_buf + 3, 3); 
            kstrtol(temp_buf, READ_BUF_SIZE, &cnt);
            // Final four characters indicate the initial FND
            strncpy(fnd_init, read_buf + 6, TEMP_BUF_SIZE); 

            /* Set metadata of timer as input */
            _timer_init_data(interval, cnt, fnd_init);
        }
        else ret_val = -EFAULT;
        break;
    /* Case2: Start the timer */
    case IOCTL_COMMAND:
        printk("[TIMER DRIVER] %s called (COMMAND)\n", __func__);
        _timer_start();
        break;
    /* Other cases */
    default: 
        printk("[TIMER DRIVER] Undefined IOCTL command (%u)\n", ioctl_num);
        ret_val = -ENOTTY; 
        break;
    }

    return ret_val;
}

/* Internal Handlers Definitions */
/* Initialize metadata of our timer as given parameters */
static void _timer_init_data(const int interval, const int cnt, const char *init) {
    int i;
    printk("[TIMER_DRIVER} Inputs: (%d, %d, %s)\n", interval, cnt, init);

    /* Initialize data */
    for (i = 0; i < TEMP_BUF_SIZE; i++) {
        if (init[i] != '0') {
            cur_timer.digit = init[i] - '0';
            cur_timer.digit_idx = i;
            break;
        }
    }
    strncpy(cur_timer.text[0], student_id, strlen(student_id));
    strncpy(cur_timer.text[1], student_name, strlen(student_name));
    cur_timer.text_idx[0] = cur_timer.text_idx[1] = 0;
    cur_timer.text_dir[0] = cur_timer.text_dir[1] = 0;

    cur_timer.interval = interval;
    cur_timer.max_cnt = cnt;
    cur_timer.cnt = 0;
}

/* Register the timer structure to the timer list */
static void _timer_register(Timer *tm) {
    printk("[TIMER_DRIVER] Timer is registered\n");
    
    /* Set the timer */
    tm->timer.expires = get_jiffies_64() + tm->interval * (HZ / 10);
    tm->timer.data = (unsigned long)&cur_timer;
    tm->timer.function = _timer_handler;   // This will be called for every expiration

    /* Register to the list */
    add_timer(&(tm->timer));
}

/* Print metadata to the FPGA I/O devices */
static void _timer_print(const Timer *tm) {
    printk("[TIMER_DRIVER] Timer performs WRITEs\n");
    
    fpga_fnd_write(tm->digit_idx, tm->digit);
    fpga_led_write(tm->digit);
    fpga_text_lcd_write(tm->text[0], tm->text_idx[0], tm->text[1], tm->text_idx[1]);
    fpga_dot_write(tm->digit);
}

/* Start the timer, with resetting the device */
static void _timer_start(void) {
    printk("[TIMER_DRIVER] Timer starts now\n");
    
    /* Reset */
    del_timer_sync(&(cur_timer.timer));
    _timer_print(&cur_timer);     // Print the reset output
    _timer_register(&cur_timer);  // Register the timer
}

/* Callback handler function that is expected to be called whenever the timer expires */
static void _timer_handler(unsigned long timeout) {
    int i;
    Timer *tm = (Timer*)timeout;
    printk("[TIMER_DRIVER] Timer Handler is called (cnt=%d)\n", ++(tm->cnt));

    /* If the counter reaches the maximum count limit, then clear all */
    if (tm->cnt == tm->max_cnt) {
        tm->text[0][0] = '\0'; 
        tm->text[1][0] = '\0';
        tm->digit = 0;
    }
    /* Otherwise change the outputs of I/O devices in our FPGA board */
    else {
        // Advance in a circular manner
        if (tm->digit == ROTATE_CNT)
            tm->digit = 1;
        else tm->digit++;
        if ((tm->cnt % ROTATE_CNT) == 0) {
            tm->digit_idx++;
            tm->digit_idx %= 4;
        }

        // Update texts of TEXT_LCD, following conditions mentioned in PDF
        for (i = 0; i <= 1; i++) {      // 'i' means which line is considered
            if (tm->text_dir[i] == 0) {
                tm->text_idx[i]++;
                if (tm->text_idx[i] + strlen(tm->text[i]) > TEXT_LCD_BUF_SIZE / 2) {
                    tm->text_idx[i] -= 2;
                    tm->text_dir[i] = 1;
                }
            } 
            else {
                tm->text_idx[i]--;
                if (tm->text_idx[i] < 0) {
                    tm->text_idx[i] += 2;
                    tm->text_dir[i] = 0;
                }
            }
        }

        // Register the next timer
        _timer_register(tm);
    }

    /* Print the updates */
    _timer_print(tm);
}

/* Module installation */
module_init(timer_driver_init);
module_exit(timer_driver_exit);

/* License and author info */
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("20171643, Junhyeok Park");
