#include "core.h"

/* PIDs for forked processes */
static pid_t inputPID;
static pid_t outputPID;
static pid_t mergePID;

/* Fork four processes needed for our project */
PROCESS_TYPE forks(void) {
    pid_t pid;
    // First child -> Input Process
    if ((pid = fork()) == 0) 
        return _INPUT;
    else {
        inputPID = pid;
        // Second child -> Output process
        if ((pid = fork()) == 0) 
            return _OUTPUT;
        else {
            outputPID = pid;
            // Third child -> Merge process
            if ((pid = fork()) == 0) 
                return _MERGE;
            else mergePID = pid;
        }
    }
    return _MAIN;
}

/* Kill all the child processes */
void killChildren(void) {
    if (kill(inputPID, SIGKILL) == -1) 
        perror("Error while killing input process!");
    else waitpid(inputPID, NULL, 0);

    if (kill(outputPID, SIGKILL) == -1) 
        perror("Error while killing output process!");
    else waitpid(outputPID, NULL, 0);

    if (kill(mergePID, SIGKILL) == -1) 
        perror("Error while killing output process!");
    else waitpid(mergePID, NULL, 0);
}

/* Allocate the semaphore for each process */
int allocateSem(void) {
    int semID, i;
    semun op; op.val = 0;

    // Get semaphores
    if ((semID = semget(SEM_KEY, SEM_CNT, IPC_CREAT)) == -1) 
        perror("Error while getting semaphore!");
    
    // Initialize corresponding data structures
    for (i = 0; i < SEM_CNT; i++) {
        if (semctl(semID, i, SETVAL, op) == -1) 
            perror("Error while setting up semaphore!");
        p[i].sem_num = i; 
        p[i].sem_flg = SEM_UNDO; 
        p[i].sem_op = -1; 
        
        v[i].sem_num = i;
        v[i].sem_flg = SEM_UNDO;
        v[i].sem_op = 1;
    }

    return semID;
}

/* Free the semaphores */
void freeSem(const int semID) {
    if (semctl(semID, 0, IPC_RMID, 0) == -1) 
        perror("Error while removing semaphore!");
}

/* Allocate the shared memory area */
int allocateShm(const key_t key, void **buf, const size_t bufferSize) {
    int shmID;

    // Allocate the shared memory
    if ((shmID = shmget(key, bufferSize, IPC_CREAT)) == -1) 
        perror("Error while getting shared memory!");

    // Attach buffers to the shared memory
    if ((*buf = shmat(shmID, 0, 0)) == (void *) -1) 
        perror("Error while attaching to shared memory!");

    return shmID;
}

/* Free the shared memories */
void freeShm(const int shmID) {
    if (shmctl(shmID, IPC_RMID, 0) == -1) 
        perror("Error while removing shared memory!");
}
