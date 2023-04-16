#include "headers.h"

/* For logging */
#define DEBUG_FLAG

/* Semaphores */
extern struct sembuf p[SEM_CNT], v[SEM_CNT];

/* Shared memory areas */
extern SHM_MAIN_IN *shmIn;
extern SHM_MAIN_OUT *shmOut;
extern SHM_MERGE_IO *shmMerge;
extern SHM_MEMTABLE *shmMemtable;

/* Internal functions */
const void initProgram(int *, int *, int *, int *, int *);
const void exitProgram(const int, const int, const int, const int, const int);
const int mainProcess(const int);
const int mergeProcess(const int);

/* For persistency of Key-Value Store */
const void loadMetadata(void);
const void persistMetadata(void);

/* Ctrl+C termination handler */
static void sig_handler(int signo);

/* For clearing TEXT_LCD when the mode changed */
extern char *initValueBuf;

/* main Process */
int main(void) {
    int semID, shmInID, shmOutID, shmMergeID, shmMemID, ret; 
    initProgram(&semID, &shmInID, &shmOutID, &shmMergeID, &shmMemID);

    // Forks four processes
    switch (forks()) {  
    case _MAIN:         // Main process       
        // Signal Handling of CTRL+C pressed situation
        signal(SIGINT, (void*)sig_handler);
        ret = mainProcess(semID); break;
    case _IO:           // I/O process
        ret = ioProcess(semID); break;
    case _MERGE:        // Merge process
        ret = mergeProcess(semID); break;
    default: perror("Error occurs during the process creation\n");
    } 
    if (ret == 1) 
        exitProgram(semID, shmInID, shmOutID, shmMergeID, shmMemID);

    return 0;
}

/* Main Process handler */
const int mainProcess(const int semID) {
    static MODE_TYPE mode = PUT, prevMode = PUT;
    bool exitFlag = 0;
    DEVICES dv;

    // Arguments for Key-Value interface
    putArgs pargs; initPutArgs(&pargs); 
    getArgs gargs; initGetArgs(&gargs);
    mergeArgs margs; initMergeArgs(&margs);
#ifdef DEBUG_FLAG
    printf("Mode -> 0 (PUT)\n");
#endif

    while (1) {
        // Wait for input from I/O process
        semop(semID, &p[SEM_MAIN_LOCK], 1);
        bool modeChanged = 0;
        
        // Verify key input
        switch (shmIn->key) {
        case KEY_VOLUMEDOWN:    // Prev mode
            if (mode == PUT)
                mode = MERGE;
            else mode--;
            modeChanged = 1;
            break;
        case KEY_VOLUMEUP:      // Next mode
            mode++;
            mode %= MODE_CNT;
            modeChanged = 1;
            break;
        case KEY_BACK:          // Exit
            exitFlag = 1;
            break;
        default: break;
        }
        // Clear the output when the mode changed 
        if (modeChanged) {    
            shmOut->fndBuf = 0;
            memcpy(shmOut->textlcdBuf, initValueBuf, MAX_TEXT_LEN);
        }
        shmIn->key = -1;
#ifdef DEBUG_FLAG
        if (modeChanged)
            printf("\nMode Changed -> %d\n(PUT:0, GET:1, MERGE:2)\n", mode);
#endif
        if (exitFlag)
            break;

        switch (mode) {
        case PUT: Put(&pargs, modeChanged); break;        // PUT
        case GET: Get(&gargs, modeChanged); break;        // GET
        case MERGE: Merge(&margs, modeChanged); break;    // MERGE
        default: break;
        }
        
        // Allow input process to work
        semop(semID, &v[SEM_IO_LOCK_1], 1);
    }

    // Shutdown all the devices for termination
    for (dv = FND; dv <= MOTOR; dv++) 
        shmOut->used[dv] = 0; 

    return 1;    // main function control
}

/* Merge Process handler */
const int mergeProcess(const int semID) {
    mergeArgs margs; initMergeArgs(&margs);
    int swch;

    while (1) {
        semop(semID, &p[SEM_MERGE_LOCK], 1);    // wait for I/O

        // Merge process triggers compaction only if there's more than 3 sstables
        if ((shmMemtable->sstableCnt - shmMemtable->sstMergedCnt) >= 3) {
            // Automatically set RESET condition (for triggering)
            for (swch = SWITCH_CNT; swch >= 1; swch--) 
                shmIn->switches[swch] = 1;
            
            // Merge (it must happen)
            Merge(&margs, 0);

            // Run MOTOR for a second 
            shmOut->used[MOTOR] = 1;
            shmOut->motorOn = 1;
            shmMerge->bgMergeFlag = 1;          // Let IO process know merge happen (for MOTOR)
        }
        semop(semID, &v[SEM_IO_LOCK_2], 1);     // allow I/O to proceed
    }

    return 0;    // main function control
}

/* Initializing routines of this application */
const void initProgram(int *semID, int *shmInID, int *shmOutID, int *shmMergeID, int *shmMemID) {
    // Allocate a semaphore
    *semID = allocateSem();

    // Allocate shared memories
    *shmInID = allocateShm(SHM_KEY_1, (void**) &shmIn, sizeof(SHM_MAIN_IN));
    *shmOutID = allocateShm(SHM_KEY_2, (void**) &shmOut, sizeof(SHM_MAIN_OUT));
    *shmMergeID = allocateShm(SHM_KEY_3, (void**) &shmMerge, sizeof(SHM_MERGE_IO));
    *shmMemID = allocateShm(SHM_KEY_4, (void**) &shmMemtable, sizeof(SHM_MEMTABLE));

    // Initialize shared memories
    memset(shmIn->switches, 0, sizeof(shmIn->switches));
    shmIn->key = -1;
    shmOut->fndBuf = shmOut->ledBuf = 0; shmOut->motorOn = shmOut->waitFlag = 0;
    memset(shmOut->textlcdBuf, '\0', sizeof(shmOut->textlcdBuf));
    memset(shmOut->used, 0, sizeof(shmOut->used));
    
    shmMerge->bgMergeFlag = 0;

    memset(shmMemtable->keys, 0, sizeof(shmMemtable->keys));
    memset(shmMemtable->values, '\0', sizeof(shmMemtable->values));
    shmMemtable->pairCnt = shmMemtable->totalPutCnt 
        = shmMemtable->sstMergedCnt = shmMemtable->sstableCnt = 0;
    
    // Open all the devices
    devOpen();
    
    // Load existing metadata of key-value store
    loadMetadata();
}

/* Clearing routines when termination */
const void exitProgram(const int semID, const int shmInID, const int shmOutID, const int shmMergeID, const int shmMemID) {
    // Kill all the child processes
    killChildren();

    // Close all the devices
    devClose();

    // Free all the IPCs
    freeSem(semID);
    freeShm(shmInID);
    freeShm(shmOutID);
    freeShm(shmMergeID);
    freeShm(shmMemID);

    // Persist metadata of key-value store
    persistMetadata();
}

/* Load metadata of Key-Value Store */
const void loadMetadata(void) {
    char fileName[MAX_TEXT_LEN];
    bool loadedFlag = 1;
    FILE *sst;
     
    sprintf(fileName, "kvstore_meta.txt"); 
    if ((sst = fopen(fileName, "r")) == NULL)
        loadedFlag = 0;
    if (loadedFlag) {
        fscanf(sst, "%d %d %d", &(shmMemtable->sstableCnt), &(shmMemtable->sstMergedCnt), 
            &(shmMemtable->totalPutCnt));
        fclose(sst);
    } 
#ifdef DEBUG_FLAG
    if (loadedFlag)
        printf("[PERSIST] Metadata %d, %d, %d is loaded!\n", shmMemtable->sstableCnt, 
            shmMemtable->sstMergedCnt, shmMemtable->totalPutCnt);
    else printf("[PERSIST] There's no previous metadata in our system.\n");
#endif
}

/* Persist metadata of Key-Value Store */
const void persistMetadata(void) {
    char fileName[MAX_TEXT_LEN];
    FILE *sst;
    int i;

    // If there's more than one entry in the memtable, then persist it!
    if (shmMemtable->pairCnt) {
        sprintf(fileName, "%d.sst", ++(shmMemtable->sstableCnt));
        if ((sst = fopen(fileName, "w")) == NULL)
            perror("Error occurs while persisting metadata (memtable)\n");
        for (i = 0; i < shmMemtable->pairCnt; i++)
            fprintf(sst, "%d %d %s\n", (++shmMemtable->totalPutCnt), 
                shmMemtable->keys[i], shmMemtable->values[i]);
        fclose(sst); 
#ifdef DEBUG_FLAG
        printf("[PERSIST] Pairs in memtable ");
        for (i = 0; i < shmMemtable->pairCnt; i++) 
            printf("- %d, %s\n", shmMemtable->keys[i], shmMemtable->values[i]); 
        printf("are(is) persisted to File %s\n", fileName); 
#endif
    }
#ifdef DEBUG_FLAG
    else printf("[PERSIST] There's no entries to persist in Memtable\n");
#endif

    sprintf(fileName, "kvstore_meta.txt");
    if ((sst = fopen(fileName, "w")) == NULL)
        perror("Error occurs while persisting metadata\n");
    fprintf(sst, "%d %d %d\n", shmMemtable->sstableCnt, 
        shmMemtable->sstMergedCnt, shmMemtable->totalPutCnt);
    fclose(sst);

#ifdef DEBUG_FLAG
    printf("[PERSIST] Metadata %d, %d, %d is persisted!\n", shmMemtable->sstableCnt, 
        shmMemtable->sstMergedCnt, shmMemtable->totalPutCnt);
#endif
}

/* SIGINT handler for persistency in case of crash */
static void sig_handler(int signo) {
    persistMetadata();
    exit(0);
}

