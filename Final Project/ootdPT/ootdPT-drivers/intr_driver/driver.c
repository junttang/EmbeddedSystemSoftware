#include "headers.h"

/* Device file name and the major number */
#define DEV_NAME "intr_driver"
#define DEV_MAJOR_NUM 243

/* Main interfaces of this module */
static int __init intr_driver_init(void);
static void __exit intr_driver_exit(void);
static int intr_driver_open(struct inode *, struct file *);
static int intr_driver_close(struct inode *, struct file *);
static int intr_driver_write(struct file *, const char __user *, size_t, loff_t *);

/* Redirect file I/O functions to user-defined handlers */
static struct file_operations intr_driver_fops = {
    .owner = THIS_MODULE,
    .open = intr_driver_open,
    .release = intr_driver_close,
    .write = intr_driver_write
};

/* Data structures for providing blocking */
static atomic_t already_open = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/* Wait queue for interrupt handling (button press) */
static DECLARE_WAIT_QUEUE_HEAD(intrq);

/* Internal functions to manage interrupts */
void _intrs_register(void);
void _intrs_free(void);

/* # of buttons we use here */
#define BUTTON_CNT 1

/* Enum type for button selection (interrupt handling) */
typedef enum _button_type {
    BACK
} Button;

/* Internal handler for a specific interrupt (button pressing) */
static irqreturn_t _back_button_handler(int, void*);

/* Workqueue infrastructure for top and bottom half handling */
static struct workqueue_struct *workq;     // work queue
typedef struct {
    struct work_struct _work;              // work to be registered
    Button button;                         // which button is pressed
} Work;
Work *work;
static void work_function(struct work_struct *work);  // handler

/* GPIO Number creator for each button */
static unsigned buttons_gpio[BUTTON_CNT] = {
    IMX_GPIO_NR(1, 12)     // BACK
};

/* Name strings for each button */
static char *buttons_name[BUTTON_CNT] = {
    "back"
};

/* Flags used for requesting IRQ number (for each button) */
static int button_flags[BUTTON_CNT] = {
    IRQF_TRIGGER_RISING
};

/* Pointers for each button interrupt handler */
void (*button_handlers[BUTTON_CNT])(int, void*) = {
    (void*)_back_button_handler
};

/* Register the device driver provided by this module file, triggered by 'insmod' cmd */
static int __init intr_driver_init(void) {
    int ret_val;
    if ((ret_val = register_chrdev(DEV_MAJOR_NUM, DEV_NAME, &intr_driver_fops)) < 0 ) {
        printk("[INTR_DRIVER] Module register failed\n");
        return ret_val;
    }
    printk("[INTR_DRIVER] Device created on file(/dev/%s, major: %d)\n", 
        DEV_NAME, DEV_MAJOR_NUM);

    /* Create the work queue for bottom half handling */
    workq = create_workqueue("my_queue");
    printk("[INTERRUPT] Workqueue is created\n");

    return ret_val;
}

/* Remove the device driver provided by this module file, by triggered 'rmmod' cmd */
static void __exit intr_driver_exit(void) {
    /* Remove the module */
    unregister_chrdev(DEV_MAJOR_NUM, DEV_NAME);

    /* Destroy the work queue */
    flush_workqueue(workq);
    destroy_workqueue(workq);

    printk("[INTR_DRIVER] Driver is now removed\n");
}

/* open() Handler for this driver */
static int intr_driver_open(struct inode *inode, struct file *file) {
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
    printk("[INTR_DRIVER] %s called\n", __func__);
    /* Register all the interrupts that this driver uses */
    _intrs_register();

    return 0;
}

/* close() Handler for this driver */
static int intr_driver_close(struct inode *inode, struct file *file) {
    /* Set already_open to zero, so one of the processes in the waitq will be
       be able to set already_open back to one and to open the file. All the 
       other processes will be called when already_open is back to one, so they
       will go back to sleep */
    atomic_set(&already_open, 0);

    /* Wake up all the processes in waitq, so if anybody is waiting for the file
       they can have it */
    wake_up(&waitq);

    module_put(THIS_MODULE);

    printk("[INTR_DRIVER] %s called\n", __func__);

    /* Release all the interrupts used */
    _intrs_free();

    return 0;
}

/* write() Handler for this driver */
static int intr_driver_write(
    struct file *file, const char __user *buf, size_t count, loff_t *f_pos
) {
    printk("[INTR_DRIVER] %s called\n", __func__);

    /* Put the user thread on a sleep */
    printk("[INTR_DRIVER] Make user thread(java thread) sleep\n");
    interruptible_sleep_on(&intrq);

    return 0;
}
/* Definitions for interfaces are over here */

/* Internal Handlers Definitions */
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
}

/* Free all the interrupts */
void _intrs_free(void) {
    int i;

    for (i = 0; i < BUTTON_CNT; i++) {
        free_irq(gpio_to_irq(buttons_gpio[i]), NULL);
        printk("[INTERRUPT] Button '%s' is freed now\n", buttons_name[i]);
    }
}

/* Interrupt handler for 'back' button, it will trigger the sleeping java thread */
static irqreturn_t _back_button_handler(int irq, void* dev_id) {
    int ret;

    /* Make and schedule the bottom half (printks) */
    work = (Work *)kmalloc(sizeof(Work), GFP_KERNEL);
    if (work) {
        INIT_WORK((struct work_struct *)work, work_function);
        work->button = BACK;
        ret = queue_work(workq, (struct work_struct *)work);
    }
    
    /* Wake up the sleeping thread (which is a java thread in here) */
    __wake_up(&intrq, 1, 1, NULL);

    return IRQ_HANDLED;
}

/* Workqueue function (bottom half function) 
   It only prints logging strings since these are not important while interrupt handling */
static void work_function(struct work_struct *work) {
    Work *_work = (Work *)work;
    printk("[WORKQUEUE] Bottom half routine is now handled\n");
    switch(_work->button) {
    case BACK:
        printk("[INTERRUPT] 'back' button pressing handled\n");
        break; 
    default: break; 
    }
    printk("[WORKQUEUE] Bottom half routine is now finished\n");

    kfree((void *)work);
    return;
}
    

/* Module installation */
module_init(intr_driver_init);
module_exit(intr_driver_exit);

/* License and author info */
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("20171643, Junhyeok Park");
