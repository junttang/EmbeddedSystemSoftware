#include "core.h"

/* Semaphores */
extern struct sembuf p[SEM_CNT], v[SEM_CNT];

/* Shared memory areas */
extern SHM_MAIN_IN *shmIn;
extern SHM_MAIN_OUT *shmOut;
extern SHM_MEMTABLE *shmMemtable;

/* Internal functions */
const void initProgram(int *, int *, int *, int *);
const void exitProgram(const int, const int, const int, const int);
const void mainProcess(const int);

/* main Process */
int main(void) {
    int semID, shmInID, shmOutID, shmMemID, ret;
    initProgram(&semID, &shmInID, &shmOutID, &shmMemID);
    
    // Forks four processes
    switch (forks()) {  
    case _MAIN:         // Main process       
        ret = mainProcess(semID); break;
    case _INPUT:        // Input process
        ret = inputProcess(semID); break;
    case _OUTPUT:       // Output process
        ret = outputProcess(semID); break;
    case _MERGE:        // Merge process
        /* mergeProcess */ break;
    default: perror("Process creation errors!\n");
    }
    if (ret == 1) 
        exitProgram(semID, shmInID, shmOutID, shmMemID);

    return 0;
}

/* main Process handler */
const int mainProcess(const int semID) {
    static modeType mode = PUT;
    bool exitFlag = 0;
    DEVICES dv;

    // Arguments for Key-Value interface
    putArgs pargs; 
    getArgs gargs; 
    mergeArgs margs; 

    while (1) {
        // Wait for input from I/O process
        semop(semID, &p[SEM_INPUT_TO_MAIN], 1);
        bool modeChanged = 0, outputFlag = 0;

        // Verify key input
        switch (shmIn->key) {
        case KEY_VOLUMEDOWN:    // Prev mode'
            if (mode == PUT)
                mode = MERGE;
            else mode--;
            modeChanged = 1;
            break;
        case KEY_VOLUMEUP:      // Next mode
            mode++;
            mode %= MODES_CNT;
            modeChanged = 1;
            break;
        case KEY_BACK:          // Exit
            exitFlag = 1;
            break;
        default: break;
        }
        if (exitFlag)
            break;

        switch (mode) {
        case PUT: outputFlag = Put(&pargs, modeChanged); break;        // PUT
        case GET: outputFlag = Get(&gargs, modeChanged); break;        // GET
        case MERGE: outputFlag = Merge(&margs, modeChanged); break;    // MERGE
        default: break;
        }
        // Execute I/O process only if there's a change
        if (outputFlag) {
            semop(semID, &v[SEM_MAIN_TO_OUTPUT], 1);  // Allow I/O process to proceed
            semop(semID, &p[SEM_OUTPUT_TO_MAIN], 1);  // Wait for I/O process work
        }
        // Allow input process to work
        semop(semID, &v[SEM_MAIN_TO_INPUT], 1);
    }

    // Shutdown all the devices for termination
    for (dv = FND; dv <= MOTOR; dv++) 
        shmOut->used[i] = 0;
    semop(semID, &v[SEM_MAIN_TO_OUTPUT], 1);
    semop(semID, &p[SEM_OUTPUT_TO_MAIN], 1);

    return 1;
}

/* Init routines of program */
const void initProgram(int *semID, int *shmInID, int *shmOutID, int *shmMemID) {
    // Allocate a semaphore
    *semID = allocateSem();

    // Allocate shared memories
    *shmInID = allocateShm(SHM_KEY_1, (void**) &shmIn, sizeof(SHM_MAIN_IN));
    *shmOutID = allocateShm(SHM_KEY_2, (void**) &shmOut, sizeof(SHM_MAIN_OUT));
    *shmMemID = allocateShm(SHM_KEY_3, (void**) &shmMemtable, sizeof(SHM_MEMTABLE));

    // Initialize shared memories
    memset(shmIn->switches, 0, sizeof(shmIn->switches));
    shmIn->key = -1;

    shmOut->fndBuf = shmOut->ledBuf = 0; shmOut->motorOn = 0;
    memset(shmOut->textlcdBuf, '\0', sizeof(shmOut->textlcdBuf));
    memset(shmOut->used, 0, sizeof(shmOut->used));
    
    memset(shmMemtable->keys, 0, sizeof(shmMemtable->keys));
    memset(shmMemtable->values, '\0', sizeof(shmMemtable->values));
    shmMemtable->pairCnt = shmMemtable->totalPutCnt = 
        shmMemtable->sstMergedCnt = shmMemtable->sstableCnt = 0;
    
    // Open all the devices
    openDevices();
}

/* Clearing routines when termination */
const void exitProgram(const int semID, const int shmInID, const int shmOutID, const int shmMemID) {
    killChildren();
    closeDevices();
    freeSem(semID);
    freeShm(shmInID);
    freeShm(shmOutID);
    freeShm(shmMemID);
}
