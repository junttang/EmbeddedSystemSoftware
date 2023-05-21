#include "headers.h"

/* For logging */
#define IO_DEBUG

/* Semaphores */
extern struct sembuf p[SEM_CNT], v[SEM_CNT];

/* Shared memories */
extern SHM_MAIN_IN *shmIn;
extern SHM_MAIN_OUT *shmOut;

/* Input & Output process handler */
int ioProcess(const int semID) {
    bool usedBefore[4] = {0, 0, 0, 0};
    int key, swchs, swch; DEVICES dv;
    fndPrint(0); ledPrint(0);
    textlcdPrint(""); motorPrint(0);

    while (1) {
        /* 1. Input Routine */
        // Load switches input data from devices onto the shared memory
        shmIn->key = -1;
        for (swch = SWITCH_CNT; swch >= 1; swch--)
            shmIn->switches[swch] = 0;

        // Read key input or switches input
        if ((key = keyRead()) != 0)
            shmIn->key = key; 
        else {
            if ((swchs = switchRead()) != 0) {
                // RESET button pressed
                if (swchs == SWITCH_RESET) {
                    for (swch = SWITCH_CNT; swch >= 1; swch--) 
                        shmIn->switches[swch] = 1;
                }
                // Otherwise like SW_1, SW_2, etc
                else {	
                    for (swch = SWITCH_CNT; swch >= 1; swch--) {
                        shmIn->switches[swch] = swchs % 10;
                        swchs = swchs / 10;
                    }
                }
            }
        }
        // Sleep little bit for LCD input
        usleep(130000);

        // Synchronization
        semop(semID, &v[SEM_MAIN_LOCK], 1);     // Allow main to proceed
        semop(semID, &p[SEM_IO_LOCK_1], 1);     // Wait for main to be stopped
        semop(semID, &v[SEM_MERGE_LOCK], 1);    // Allow merge to proceed
        semop(semID, &p[SEM_IO_LOCK_2], 1);     // Wait for merge to be stopped

        /* 2. Output Routine */        
        for (dv = FND; dv <= MOTOR; dv++) {
            // print only if the device is in use
            if (shmOut->used[dv]) {
                switch (dv) {
                case FND: fndPrint(shmOut->fndBuf); break;
                case LED: ledPrint(shmOut->ledBuf); break;
                case TEXT_LCD: textlcdPrint(shmOut->textlcdBuf); break;
                case MOTOR: motorPrint(shmOut->motorOn); break;
                default: break;
                }
            }
            // if the device is used in the right before turn, then reset it
            else if (usedBefore[dv]) {
                switch (dv) {
                case FND: fndPrint(0); break;
                case LED: ledPrint(0); break;
                case TEXT_LCD: textlcdPrint(""); break;
                case MOTOR: motorPrint(0); break;
                default: break;                    
                }
            }
        }
        // Record which device is used in current turn
        for (dv = FND; dv <= MOTOR; dv++) 
            usedBefore[dv] = shmOut->used[dv];

        // Run MOTOR for a second when there's a background MERGE occured
        if (shmOut->waitFlag || shmMerge->bgMergeFlag) {
            usleep(400000);
            shmOut->waitFlag = 0;
            shmMerge->bgMergeFlag = 0;
        }
    }

    return 0;  // main function control
}

