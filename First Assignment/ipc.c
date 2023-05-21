#include "headers.h"

/* PIDs for forked processes */
static pid_t ioPID;
static pid_t mergePID;

/* Fork two processes needed for our project */
PROCESS_TYPE forks(void) {
    pid_t pid;
    // First child -> I/O Process
    if ((pid = fork()) == 0) 
        return _IO;
    else {
        ioPID = pid;
        // Second child -> Merge process
        if ((pid = fork()) == 0) 
            return _MERGE;
        else mergePID = pid;
    }
    return _MAIN;
}

/* Kill all the child processes */
void killChildren(void) {
    if (kill(ioPID, SIGKILL) == -1) 
        perror("Error occurs during killing child processes (IO)");
    else waitpid(ioPID, NULL, 0);

    if (kill(mergePID, SIGKILL) == -1) 
        perror("Error occurs during killing child processes (MERGE)");
    else waitpid(mergePID, NULL, 0);
}

/* Allocate the semaphore for each process */
int allocateSem(void) {
    int semID, i;
    semun op; op.val = 0;

    // Get semaphores
    if ((semID = semget(SEM_KEY, SEM_CNT, IPC_CREAT)) == -1) 
        perror("Error occurs while allocating semaphore");
    
    // Initialize corresponding data structures
    for (i = 0; i < SEM_CNT; i++) {
        if (semctl(semID, i, SETVAL, op) == -1) 
            perror("Error occurs while calling semctl system call");
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
        perror("Error occurs while freeing semaphore");
}

/* Allocate the shared memory area */
int allocateShm(const key_t key, void **buf, const size_t bufferSize) {
    int shmID;

    // Allocate the shared memory
    if ((shmID = shmget(key, bufferSize, IPC_CREAT)) == -1) 
        perror("Error occurs while allocating shared memory");

    // Attach buffers to the shared memory
    if ((*buf = shmat(shmID, 0, 0)) == (void *) -1) 
        perror("Error occurs while calling shmat system call");

    return shmID;
}

/* Free the shared memories */
void freeShm(const int shmID) {
    if (shmctl(shmID, IPC_RMID, 0) == -1) 
        perror("Error occurs while freeing shared memory");
}
