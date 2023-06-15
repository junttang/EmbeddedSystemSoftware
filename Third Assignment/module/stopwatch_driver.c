#include "headers.h"

/* Device file name and the major number (PDF conditions) */
#define DEV_NAME "stopwatch"
#define DEV_MAJOR_NUM 242

/* Main interfaces of this module */
static int __init stopwatch_driver_init(void);
static void __exit stopwatch_driver_exit(void);
static int stopwatch_driver_open(struct inode *, struct file *);
static int stopwatch_driver_close(struct inode *, struct file *);
static int stopwatch_driver_write(struct file *, const char __user *, size_t, loff_t *);

/* Redirect file I/O functions to user-defined handlers */
static struct file_operations stopwatch_driver_fops = {
    .owner = THIS_MODULE,
    .open = stopwatch_driver_open,
    .release = stopwatch_driver_close,
    .write = stopwatch_driver_write
};

/* Data structures for providing blocking */
static atomic_t already_open = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/* Internal main functionalities of stopwatch driver */
void _stopwatch_init(void);
void _stopwatch_start(void);
void _stopwatch_pause(void);
void _stopwatch_exit(void);

/* Maximum seconds (3600sec == 1hour) */
#define TIME_LIMIT (60 * 60)   // "_ _:_ _" (min:sec)

/* Global timers we use here */
static Timer tm;
static struct timer_list exit_tm;

/* Internal functions to manage metadata attached to timer and perform operations */
static void _timer_handler(unsigned long);
static void _timer_register(void);
static void _timer_exit_handler(unsigned long);

/* Wait queue for interrupt handling (button press) */
static DECLARE_WAIT_QUEUE_HEAD(intrq);

/* Internal functions to manage interrupts */
void _intrs_register(void);
void _intrs_free(void);

/* # of buttons we use here */
#define BUTTON_CNT 4

/* Enum type for button selection (interrupt handling) */
typedef enum _button_type {
    INIT, HOME, BACK, VOL_UP, VOL_DOWN
} Button;

/* Internal handlers for each interrupt (button pressing) */
static irqreturn_t _home_button_handler(int, void*);
static irqreturn_t _back_button_handler(int, void*);
static irqreturn_t _volup_button_handler(int, void*);
static irqreturn_t _voldown_button_handler(int, void*);

/* Workqueue infrastructure for top and bottom half handling */
static struct workqueue_struct *workq;     // work queue
typedef struct {
    struct work_struct _work;              // work to be registered
    Button button;                         // which button is pressed
} Work;
Work *work;
static void work_function(struct work_struct *work);  // handler

/* Record the lastly pressed button for proper interrupt handling */
static Button prev_pressed = INIT;

/* Stopwatch paused flag */
static int stopwatch_paused = 0;

/* Volume down pressed flag */
static int vol_down_pressed = 0;

/* GPIO Number creator for each button */
static unsigned buttons_gpio[BUTTON_CNT] = {
    IMX_GPIO_NR(1, 11),  // HOME
    IMX_GPIO_NR(1, 12),  // BACK
    IMX_GPIO_NR(2, 15),  // VOLUME UP
    IMX_GPIO_NR(5, 14)   // VOLUME DOWN
};

/* Name strings for each button */
static char *buttons_name[BUTTON_CNT] = {
    "home", "back", "vol+", "vol-"
};

/* Flags used for requesting IRQ number (for each button) */
static int button_flags[BUTTON_CNT] = {
    IRQF_TRIGGER_RISING,
    IRQF_TRIGGER_RISING,
    IRQF_TRIGGER_RISING,
    IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING
};

/* Pointers for each button interrupt handler */
void (*button_handlers[BUTTON_CNT])(int, void*) = {
    (void*)_home_button_handler, 
    (void*)_back_button_handler, 
    (void*)_volup_button_handler, 
    (void*)_voldown_button_handler
};

/* Register the device driver provided by this module file, triggered by 'insmod' cmd */
static int __init stopwatch_driver_init(void) {
    int ret_val;
    if ((ret_val = register_chrdev(DEV_MAJOR_NUM, DEV_NAME, &stopwatch_driver_fops)) < 0 ) {
        printk("[STOPWATCH DRIVER] Module register failed\n");
        return ret_val;
    }
    printk("[STOPWATCH DRIVER] Device created on file(/dev/%s, major: %d)\n", 
        DEV_NAME, DEV_MAJOR_NUM);

    /* Map the FND device in our FPGA board */
    if (fpga_fnd_map() == FND_ERROR)
        return FND_ERROR;
    /* Be prepared to use timer */
    init_timer(&(tm.timer));
    init_timer(&exit_tm);

    /* Create the work queue for bottom half handling */
    workq = create_workqueue("my_queue");
    printk("[INTERRUPT] Workqueue is created\n");

    return ret_val;
}

/* Remove the device driver provided by this module file, by triggered 'rmmod' cmd */
static void __exit stopwatch_driver_exit(void) {
    /* Remove the module */
    unregister_chrdev(DEV_MAJOR_NUM, DEV_NAME);

    /* Delete both timers */
    del_timer_sync(&(tm.timer));
    del_timer_sync(&exit_tm);

    /* Unmap all the FPGA I/O devices */
    fpga_fnd_unmap();

    /* Destroy the work queue */
    flush_workqueue(workq);
    destroy_workqueue(workq);

    printk("[STOPWATCH DRIVER] Driver is now removed\n");
}

/* open() Handler for this driver */
static int stopwatch_driver_open(struct inode *inode, struct file *file) {
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
    printk("[STOPWATCH DRIVER] %s called\n", __func__);
    /* Register all the interrupts that this driver uses */
    _intrs_register();

    return 0;
}

/* close() Handler for this driver */
static int stopwatch_driver_close(struct inode *inode, struct file *file) {
    /* Set already_open to zero, so one of the processes in the waitq will be
       be able to set already_open back to one and to open the file. All the 
       other processes will be called when already_open is back to one, so they
       will go back to sleep */
    atomic_set(&already_open, 0);

    /* Wake up all the processes in waitq, so if anybody is waiting for the file
       they can have it */
    wake_up(&waitq);

    module_put(THIS_MODULE);

    printk("[STOPWATCH DRIVER] %s called\n", __func__);

    /* Release all the interrupts used */
    _intrs_free();

    return 0;
}

/* write() Handler for this driver */
static int stopwatch_driver_write(
    struct file *file, const char __user *buf, size_t count, loff_t *f_pos
) {
    printk("[STOPWATCH DRIVER] %s called\n", __func__);

    /* Initialize the stopwatch */
    _stopwatch_init();
    /* Put the user application on a sleep */
    interruptible_sleep_on(&intrq);
    printk("[STOPWATCH DRIVER] Make user application sleep\n");

    return 0;
}
/* Definitions for interfaces are over here */

/* Internal functionalities of stopwatch */
/* Initialize all the metadata of stopwatch */
void _stopwatch_init(void) {
    /* Reset the FND device */
    fpga_fnd_write(0, 0);
    /* Initialize the metadata */
    tm.elapsed_sec = tm.paused_sec = 0;
}

/* Start the stopwatch device */
void _stopwatch_start(void) {
    /* Delete possibly existing timer */
    del_timer(&(tm.timer));

    /* Register the next timer */
    _timer_register();
}

/* Pause the stopwatch device */
void _stopwatch_pause(void) {
    int paused_time = get_jiffies_64() - tm.prev_expires;

    /* Delete the current timer */
    del_timer(&(tm.timer));

    /* Record the paused time, the function below 
       will register the timer after the re-start */
    tm.paused_sec = paused_time;
}

/* Exit the stopwatch, that is, start the exit timer */
void _stopwatch_exit(void) {
    /* Remove the previous one */
    del_timer(&exit_tm);

    /* Set the exit timer (3 seconds of pressed state) */
    exit_tm.expires = get_jiffies_64() + 3 * HZ;  // 3 seconds
    exit_tm.function = _timer_exit_handler;

    /* Register to the list */
    add_timer(&exit_tm);
}

/* Internal Handlers Definitions */
/* Register the next timer for stopwatch functionality */
static void _timer_register(void) {
    int re_start_time = tm.paused_sec;

    /* Set the timer */
    tm.timer.expires = get_jiffies_64() + HZ - re_start_time;
    tm.timer.data = (unsigned long)&tm;
    tm.timer.function = _timer_handler;

    /* Register to the list */
    add_timer(&(tm.timer));

    tm.paused_sec = 0;
    printk("[TIMER] Register the next timer (restart time: 0.%dsec)\n", re_start_time);
}

/* Callback handler function that is expected to be called whenever the timer expires */
static void _timer_handler(unsigned long timeout) {
    Timer *tm = (Timer*)timeout;
    printk("[TIMER] Timer Handler is called (elapsed_sec=%d)\n", ++(tm->elapsed_sec));
    /* Increment the seconds and record the callback function call time */
    tm->elapsed_sec = tm->elapsed_sec % TIME_LIMIT;
    tm->prev_expires = get_jiffies_64();

    /* Print the time to the FND device */
    fpga_fnd_write((tm->elapsed_sec / 60), (tm->elapsed_sec % 60));

    /* Register the next timer */
    _timer_register();
}

/* Callback handler function for the exit timer */
static void _timer_exit_handler(unsigned long timeout) {
    /* Ends the stopwatch. */
    del_timer(&(tm.timer));
    printk("[TIMER] Stopwatch ends here (exit timer expires now)\n");

    /* Reset the I/O of FPGA */
    fpga_fnd_write(0, 0);

    /* Wakeup the application */
    printk("[INTERRUPT] Wake up user application (retrieve from WaitQ)\n");
    __wake_up(&intrq, 1, 1, NULL);
}

/* Register all the interrupts that this driver uses */
void _intrs_register(void) {
    int i, irq, ret;
    /* Register */
    for (i = 0; i < BUTTON_CNT; i++) {
        gpio_direction_input(buttons_gpio[i]);
        irq = gpio_to_irq(buttons_gpio[i]);
        ret = request_irq(irq, (irq_handler_t)(*button_handlers[i]), 
            button_flags[i], buttons_name[i], 0);
        printk("[INTERRUPT] IRQ number %d for '%s' button\n", irq, buttons_name[i]);
    }
    
    /* Intialize metadata */
    prev_pressed = INIT;
    vol_down_pressed = 0;
}

/* Free all the interrupts */
void _intrs_free(void) {
    int i;

    for (i = 0; i < BUTTON_CNT; i++) {
        free_irq(gpio_to_irq(buttons_gpio[i]), NULL);
        printk("[INTERRUPT] Button '%s' is freed now\n", buttons_name[i]);
    }
}

/* Interrupt handler for 'home' button, it pauses the stopwatch */
static irqreturn_t _home_button_handler(int irq, void* dev_id) {
    int ret;

    /* Continuous pressing handling */
    if (prev_pressed == HOME) {
        printk("[INTERRUPT] Continuous 'home' button pressing ignored\n");
        return IRQ_HANDLED;
    }
    prev_pressed = HOME;
    _stopwatch_start();
    
    /* Make and schedule the bottom half (printks) */
    work = (Work *)kmalloc(sizeof(Work), GFP_KERNEL);
    if (work) {
        INIT_WORK((struct work_struct *)work, work_function);
        work->button = HOME;
        ret = queue_work(workq, (struct work_struct *)work);
    }
    stopwatch_paused = 0;

    return IRQ_HANDLED;
}

/* Interrupt handler for 'back' button, it pauses the stopwatch */
static irqreturn_t _back_button_handler(int irq, void* dev_id) {
    int ret;

    /* Continuous pressing handling */
    if (prev_pressed == BACK || stopwatch_paused) {
        printk("[INTERRUPT] Continuous 'back' button pressing ignored\n");
        return IRQ_HANDLED;
    }
    /* When the stopwatch is running, only 'back' button pressing is allowed */
    if (prev_pressed != HOME && prev_pressed != VOL_DOWN) {
        printk("[INTERRUPT] 'back' button pressing ignored (stopwatch isn't running)\n");
        return IRQ_HANDLED;
    }
    prev_pressed = BACK;
    _stopwatch_pause();
    
    /* Make and schedule the bottom half (printks) */
    work = (Work *)kmalloc(sizeof(Work), GFP_KERNEL);
    if (work) {
        INIT_WORK((struct work_struct *)work, work_function);
        work->button = BACK;
        ret = queue_work(workq, (struct work_struct *)work);
    }
    stopwatch_paused = 1;  // For vol-down & back pressing situation

    return IRQ_HANDLED;
}

/* Interrupt handler for 'vol+' button, it resets the stopwatch */
static irqreturn_t _volup_button_handler(int irq, void* dev_id) {
    int ret;

    /* Continuous pressing handling */
    if (prev_pressed == VOL_UP) {
        printk("[INTERRUPT] Continuous 'vol+' button pressing ignored\n");
        return IRQ_HANDLED;
    }
    prev_pressed = VOL_UP;

    /* Resets the stopwatch. */
    del_timer(&(tm.timer));
    _stopwatch_init();
    
    /* Make and schedule the bottom half (printks) */
    work = (Work *)kmalloc(sizeof(Work), GFP_KERNEL);
    if (work) {
        INIT_WORK((struct work_struct *)work, work_function);
        work->button = VOL_UP;
        ret = queue_work(workq, (struct work_struct *)work);
    }

    return IRQ_HANDLED;
}

/* Interrupt handler for 'vol-' button, it make the stopwatch 
   terminate if 'vol-' button is pressed more than 3 seconds */
static irqreturn_t _voldown_button_handler(int irq, void* dev_id) {
    int ret;

    /* If pressed while already pressed, then stop counting 3 secs */
    if (vol_down_pressed) {
        printk("[INTERRUPT] 'vol-' button pressing handled (counting ends)\n");
        
        del_timer(&exit_tm);
        printk("[TIMER] Existing exit timer is now deleted\n");
        prev_pressed = VOL_DOWN; 
    }
    /* 'vol-' button is pressed, it should be pressed until 3 seconds are elapsed */
    else {
        _stopwatch_exit();
        
        /* Make and schedule the bottom half (printks) */
        work = (Work *)kmalloc(sizeof(Work), GFP_KERNEL);
        if (work) {
            INIT_WORK((struct work_struct *)work, work_function);
            work->button = VOL_DOWN;
            ret = queue_work(workq, (struct work_struct *)work);
        }
    }

    vol_down_pressed = 1 - vol_down_pressed;
    return IRQ_HANDLED;
}

/* Workqueue function (bottom half function) 
   It only prints logging strings since these are not important for stopwatch functionalities */
static void work_function(struct work_struct *work) {
    Work *_work = (Work *)work;
    printk("[WORKQUEUE] Bottom half routine is now handled\n");
    switch(_work->button) {
    case HOME:
        printk("[TIMER] Stopwatch starts now\n");
        printk("[INTERRUPT] 'home' button pressing handled\n"); 
        break; 
    case BACK:
        printk("[TIMER] Stopwatch is paused now (paused time: 0.%dsec)\n", tm.paused_sec);
        printk("[INTERRUPT] 'back' button pressing handled\n");
        break; 
    case VOL_UP:
        printk("[STOPWATCH DRIVER] Reset the stopwatch now\n");
        printk("[STOPWATCH] Stopwatch is now initialized\n");
        printk("[INTERRUPT] 'vol+' button pressing handled\n");
        break; 
    case VOL_DOWN:
        printk("[TIMER] Register the new exit timer (VOL_DOWN pressed)\n");
        printk("[INTERRUPT] 'vol-' button pressing handled (counting starts)\n");
        break;
    default: break; 
    }
    printk("[WORKQUEUE] Bottom half routine is now finished\n");

    kfree((void *)work);
    return;
}
    

/* Module installation */
module_init(stopwatch_driver_init);
module_exit(stopwatch_driver_exit);

/* License and author info */
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("20171643, Junhyeok Park");
