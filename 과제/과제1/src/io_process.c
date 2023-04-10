#include "core.h"

/* Shared memory areas */
extern struct sembuf p[SEM_CNT], v[SEM_CNT];
extern SHM_MAIN_IN *shmIn;
extern SHM_MAIN_OUT *shmOut;

/* Input process handler */
int inputProcess(const int semID) {
    int key, swchs, swch;

    while (1) {
        // Load data from devices onto the shared memory
        if ((key = keyRead()) != 0)
            shmIn->key = key; 
        else if ((swchs = switchRead()) != 0) {
            for (swch = SWITCH_CNT; swch >= 1; swch--) {
                shmIn->switches[swch] = swchs % 10;
                swchs = swchs / 10;
            }
        }
        // Sleep little bit for LCD input
        usleep(130000);

        semop(semID, &v[SEM_INPUT_TO_MAIN], 1);     // Allow main to proceed
        semop(semID, &p[SEM_MAIN_TO_INPUT], 1);     // wait for main to be stopped
    }

    return 0;  // main function control
}

/* Output process handler */
int outputProcess(const int semID) {
    DEVICES dv;
    bool currentInUse[4] = {0, 0, 0, 0};
    fndReset(); ledReset();
    textLcdReset(); motorReset();

    while (1) {
        semop(semID, &p[SEM_MAIN_TO_OUTPUT], 1);    // wait for main to be stopped

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
            // if the device is used, then reset it
            else if (currentInUse[dv]) {
                switch (dv) {
                case FND: fndReset(); break;
                case LED: ledReset(); break;
                case TEXT_LCD: textLcdReset(); break;
                case MOTOR: motorReset(); break;
                default: break;                    
                }
            }
        }
        // Record which device is used now
        for (dv = FND; dv <= MOTOR; dv++) 
            currentInUse[dv] = shmOut->used[dv];

        semop(semID, &v[SEM_OUTPUT_TO_MAIN], 1);    // allow main to proceed
    }

    return 0;  // main function control
}
