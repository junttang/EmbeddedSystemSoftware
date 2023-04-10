#ifndef _IPC_H_
#define _IPC_H_

/* Types of processes */
typedef enum _processType { 
    _MAIN,  // Main Process
    _INPUT, // Input Process
    _OUTPUT,// Output Process
    _MERGE  // Merge Process
} PROCESS_TYPE;

/* Constants for semaphore interface */
#define SEM_CNT 4
#define SEM_KEY (key_t)0x20
#define SEM_INPUT_TO_MAIN 0
#define SEM_MAIN_TO_INPUT 1
#define SEM_MAIN_TO_OUTPUT 2
#define SEM_OUTPUT_TO_MAIN 3
#define SEM_MAIN_TO_MERGE 4 // ?
#define SEM_MERGE_TO_MAIN 5 // ?

/* Data structures for semaphore interface */
typedef union _semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
} semun;

/* Semaphores */
struct sembuf p[SEM_CNT];   // For P operation
struct sembuf v[SEM_CNT];   // For V operation

/* Constans for shared memory */
#define SHM_KEY_1 (key_t)0x10   // For Input IPC
#define SHM_KEY_2 (key_t)0x20   // For Output IPC
#define SHM_KEY_3 (key_t)0x30   // For memtable IPC
#define MAX_PAIR 3              // Maximum # of pairs in memtable

/* Shared memory between Main & Input processes */
typedef struct _shmMainIn {
    bool switches[SWITCH_CNT + 1];  // Switch 1 to 9
    int key;                        // VOL+, VOL-, BACK
} SHM_MAIN_IN;

/* Shared memory between Main & Output processes */
typedef struct _shmMainOut {
    int fndBuf;                              // Buffer for FND device
    int ledBuf;                              // Buffer for LED device
    char textlcdBuf[MAX_TEXT_LEN];           // Buffer for TEXT_LCD device
    bool motorOn;                            // Whether the motor is on/off
    bool used[O_DEVICE_CNT];                 // Which device is in use
} SHM_MAIN_OUT;

/* Shared memory between Main & Merge processes */ // ????
typedef struct _shmMemtable {
    int keys[MAX_PAIR];  
    char values[MAX_PAIR][MAX_TEXT_LEN];  
    int pairCnt;
    int totalPutCnt;
    int sstableCnt;
    int sstMergedCnt;
} SHM_MEMTABLE;

/* Share Memories */
SHM_MAIN_IN *shmIn;         // For IPC of Input-Main
SHM_MAIN_OUT *shmOut;       // For IPC of Output-Main
SHM_MEMTABLE *shmMemtable;  // Globally shared memtable

/* Interface functions */
PROCESS_TYPE forks(void);
void killChildren(void);
int allocateSem(void);
void freeSem(const int);
int allocateShm(const key_t, void **, const size_t);
void freeShm(const int);

#endif /* _IPC_H_ */
