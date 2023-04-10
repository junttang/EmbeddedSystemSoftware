#include "core.h"

/* Shared memory areas */
extern SHM_MAIN_IN *shmIn;
extern SHM_MAIN_OUT *shmOut;
extern SHM_MEMTABLE *shmMemtable;

/* Keypad arrays */
const char KEYPAD[][3] = {
    "",
    ".QZ", "ABC", "DEF",  
    "GHI", "JKL", "MNO",  
    "PRS", "TUV", "WXY"   
};

/* Internal function for key-value interface */
const void insertMemtable(const char *, const char *);
const int searchTables(const char *, char *);

/* API wrapper function for PUT operation */
bool Put(putArgs *args, const bool modeChanged) {
    args->firstFlag = 0; args->keyInputMode = 1;
    args->resetFlag = 0; args->numAlphaMode = 0;
    args->MemtableInsert = 0;
    
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = 0;

    // First time for current mode
    if (modeChanged) 
        args->firstFlag = 1;

    // Verify switches input
    if (shmIn->switches[2] && shmIn->switches[3]) {
        args->resetFlag = 1;
        args->keyInputMode = 0;    // Value Input Mode starts!
    }
    else if (shmIn->switches[5] && shmIn->switches[6]) 
        args->numAlphaMode = 1;    // Number input or alphabet input
    else if (shmIn->switches[8] && shmIn->switches[9]) 
        args->MemtableInsert = 1;  // Insert to the memtable

    // Set keypad inputs
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = shmIn->switches[i];

    return _Put(args);
}

/* PUT operation handler */  
bool _Put(const putArgs *args) {
    static char key[MAX_TEXT_LEN] = {'\0'};
    static char value[MAX_TEXT_LEN] = {'\0'};
    static int index = 0, prev = -1, option = 0;
    static bool numMode = 1;
    static int leds = LED_1;
    bool moveIndex = 1, changed = 0;
    char temp[5], c = -1; 
    int fnd, i;

    // Turn on devices we need here
    shmOut->used[LED] = 1;
    if (args->keyInputMode) {
        shmOut->used[FND] = 1;
        shmOut->used[TEXT_LCD] = 0;
    }
    else {
        shmOut->used[FND] = 0;
        shmOut->used[TEXT_LCD] = 1;
    }

    // Get the device time
    time_t devTime; time(&devTime);
    const struct tm *timeInfo = localtime(&devTime);
    const int devSecond = timeInfo->tm_sec;

    // Initialize if it's first time
    if (args->firstFlag) {
        memset(key, ' ', MAX_TEXT_LEN);
        memset(value, ' ', MAX_TEXT_LEN);
        numMode = 1;    // Default is number input mode
        index = 0;      // Start filling the text string from idx 0 
        prev = -1;      // Previous key input
        option = 0;     // Sub-option of specific key
        changed = 1;    // Is it changed?
        leds = LED_1;   // Turn on the LED_1
    }

    // Case1: Clear the string and change the input mode (key to value)
    if (args->resetFlag) {
        if (args->keyInputMode)
            memset(key, ' ', MAX_TEXT_LEN);
        else memset(value, ' ', MAX_TEXT_LEN);
        index = 0;
        prev = -1;
        option = 0;
        changed = 1;
    } 
    // Case2: Change the alphabet <-> number (only in value input mode)
    else if (args->numAlphaMode && !args->keyInputMode) {
        numMode = !numMode;     // Reverse num/alpha mode
        prev = -1;
        option = 0;
        changed = 1;
    }
    // Case3: Memtable insertion
    else if (args->MemtableInsert) {
        insertMemtable(key, value);

        // Turn on all the LEDs for a second
        leds = LED_1 | LED_2 | LED_3 | LED_4 |
               LED_5 | LED_6 | LED_7 | LED_8;
        prev = -1;
        option = 0;
        changed = 1;
    } 
    // Case4: Just insert character
    else {
        for (i = 1; i <= SWITCH_CNT; i++) {
            if (args->keypad[i]) {
                // Number mode
                if (numMode) {
                    c = '0' + i;
                    prev = -1;
                    option = 0;
                }
                // Alphabet mode (only in value input mode)
                else if (!args->keyInputMode) {
                    if (i == prev) {
                        option = (option + 1) % 3;
                        moveIndex = 0;
                    } 
                    else {
                        prev = i;
                        option = 0;
                    }
                    c = KEYPAD[i][option];
                }
                changed = 1;
                break;
            }
        }
        if (args->keyInputMode) {
            if ((devSecond % 2) == 0) 
                leds = LED_3;
            else leds = LED_4;
        }
        else {
            if ((devSecond % 2) == 0) 
                leds = LED_7;
            else leds = LED_8;
        }
    }
    if (!changed) return 0;

    // Append char to the string
    if (c != -1) {
        // Insert in a circular manner
        if (args->keyInputMode) {
            if (index >= 4)     // FND (key input)
                index = 0;
        }
        else {
            if (index >= MAX_TEXT_LEN)
                index = 0;      // LCD (value input)
        }
        // Alphabet is banned to be inserted when key input mode
        if (!(args->keyInputMode && !numMode)) {
            if (args->keyInputMode)
                key[index++] = c;
            else {
                value[index] = c;
                if (moveIndex)
                    index++;
            }
        }
    }

    // Print to FND/LCD/LED via shared memory
    if (args->keyInputMode) {
        strncpy(temp, key, 4);
        fnd = atoi(temp);
        shmOut->fndBuf = fnd;
    }
    else memcpy(shmOut->textlcdBuf, value, MAX_TEXT_LEN);
    shmOut->ledBuf = leds;

    return 1;
}

/* API wrapper function for GET operation */
bool Get(getArgs *args, const bool modeChanged) {
    args->firstFlag = 0;
    args->resetFlag = 0;

    for (i = 0; i <= SWITCH_CNT; i++)
        args->keypad[i] = 0;

    // First time for current mode
    if (modeChanged) 
        args->firstFlag = 1;

    // Verify switches input
    if (shmIn->switches[2] && shmIn->switches[3]) 
        args->resetFlag = 1;
 
    // Set keypad inputs
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = shmIn->switches[i];

    return Get(args);
}

/* GET operation handler */  
bool _Get(const getArgs *args) {
    static char key[MAX_TEXT_LEN] = {'\0'};
    static char value[MAX_TEXT_LEN] = {'\0'};
    static int index = 0, leds = LED_1;
    bool valueFound = 0, changed = 0;
    char temp[5], c = -1; 
    int fnd, i;

    // Turn on devices we need here
    shmOut->used[FND] = 1;
    shmOut->used[LED] = 1;
    shmOut->used[TEXT_LCD] = 1;

    // Get the device time
    time_t devTime; time(&devTime);
    const struct tm *timeInfo = localtime(&devTime);
    const int devSecond = timeInfo->tm_sec;

    // Initialize if it's first time
    if (args->firstFlag) {
        memset(key, ' ', MAX_TEXT_LEN);
        index = 0;      // Start filling the key string from idx 0 
        changed = 1;    // Is it changed?
        leds = LED_1;   // Turn on the LED_1
    }

    // Case1: Clear the string
    if (args->resetFlag) {
        valueFound = searchTables(key, value);   // value will be updated!
        memset(key, ' ', MAX_TEXT_LEN);
        if (!valueFound)  // If search failed
            strncpy(value, "Error", 6);

        // Turn on all the LEDs for a second
        leds = LED_1 | LED_2 | LED_3 | LED_4 |
               LED_5 | LED_6 | LED_7 | LED_8;

        index = 0;
        changed = 1;
    } 
    // Just insert character case
    else {
        for (i = 1; i <= SWITCH_CNT; i++) {
            if (args->keypad[i]) {
                c = '0' + i;
                changed = 1;
                break;
            }
        }
        if ((devSecond % 2) == 0) 
            leds = LED_3;
        else leds = LED_4;
    }
    if (!changed) return 0;

    // Append char to the string
    if (c != -1) {
        // Insert in a circular manner
        if (index >= 4)     // FND (key input)
            index = 0;
        key[index++] = c;
    }

    // Print to FND/LCD/LED via shared memory
    strncpy(temp, key, 4);
    fnd = atoi(temp);
    shmOut->fndBuf = fnd;
    memcpy(shmOut->textlcdBuf, value, MAX_TEXT_LEN); // Error or Value
    shmOut->ledBuf = leds;

    return 1;
}

/* API wrapper function for MERGE operation */
bool Merge(const mergeArgs *args, const bool modeChanged) {
    args->firstFlag = 0;
    args->resetFlag = 0;

    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = 0;

    // First time for current mode
    if (modeChanged) 
        args->firstFlag = 1;

    // Check for associated actions
    if (shmIn->switches[2] && shmIn->switches[3]) {
        /* MERGE */
        args->resetFlag = 1;
    } 
    
    // Set keypad inputs
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = shmIn->switches[i];

    return Merge(args);
}

/* MERGE operation handler */  
bool _Merge(const mergeArgs* args) {
    shmOut->used[MOTOR] = 1;
    shmOut->used[TEXT_LCD] = 1;

    char text[MAX_TEXT_LEN] = {'\0'};
    bool mergeFlag = 0;
    shmOut->motorOn = 0;

    // Initialization
    if (args->firstFlag) {
        memset(text, ' ', MAX_TEXT_LEN);
    }

    // Clear the string and change the input mode (key to value)
    if (args->resetFlag) {
        shmOut->motorOn = 1;
        /* GET */
        memcpy(text, /*...*/, MAX_TEXT_LEN);
        mergeFlag = 1;
    } 

    // Print to FND/LCD via shared memory
    if (mergeFlag)
        memcpy(shmOut->textlcdBuf, value, MAX_TEXT_LEN);

    return 1;
}

const void insertMemtable(const char *keyStr, const char *valueStr) {
    char fileName[MAX_SSTABLE + 5];     // ex) 1234.sst
    char key[5]; strncpy(key, keyStr, 4);
    FILE *sst; 
    int i;

    // Flush if memtable is full
    if (shmMemtable->pairCnt == 3) {
        // // Case1: Flush triggers MERGE -> then flush memtable to latest sstable first, then MERGE
        // if (((shmMemtable->sstableCnt + 1) % 3) == 0) {
        //     sprintf(fileName, "%d.sst", (shmMemtable->sstableCnt - 1));
        //     if ((sst = fopen(fileName, "r+")) == NULL)
        //         perror("Error occurs while openning SSTable (append)\n");
        //     fseek(sst, 0, SEEK_END);
        //     for (i = 0; i < MAX_PAIR; i++)
        //         fprintf(sst, "%d %d %s\n", (++shmMemtable->totalPutCnt), 
        //             shmMemtable->keys[i], shmMemtable->values[i]);
        //     /* 그 다음 MERGE */
        //     /* ... */
        // }
        // // Case2: Otherwise, just flush to newly created sstable
        shmMemtable->sstableCnt++;
        sprintf(fileName, "%d.sst", shmMemtable->sstableCnt);
        if ((sst = fopen(fileName, "w")) == NULL)
            perror("Error occurs while openning SSTable (write)\n");
        for (i = 0; i < MAX_PAIR; i++)
            fprintf(sst, "%d %d %s\n", (++shmMemtable->totalPutCnt), 
                shmMemtable->keys[i], shmMemtable->values[i]);
        
        shmMemtable->pairCnt = 0;
    }

    // Memtable insertion
    shmMemtable->keys[shmMemtable->pairCnt] = atoi(key);
    strncpy(shmMemtable->values[shmMemtable->pairCnt], valueStr, strlen(valueStr));
    shmMemtable->pairCnt++;
}

/* Retrieve the latest value of passed key */
const int searchTables(const char *keyStr, char *valueStr) {
    char fileName[MAX_SSTABLE + 5];     // ex) 1234.sst
    char temp[5]; strncpy(temp, keyStr, 4);
    int key = atoi(temp), i;
    int tempNo, tempKey;
    char tempValue[MAX_TEXT_LEN];
    bool valueFound = 0;
    FILE *sst; 

    // First search key in the memtable
    if (shmMemtable->pairCnt > 0) {
        for (i = (shmMemtable->pairCnt - 1); i >= 0; i--) {
            if (key == shmMemtable->keys[i]) {
                strncpy(valueStr, shmMemtable->values[i], strlen(shmMemtable->values[i]));
                valueFound = 1;   // The lastly found value is the latest
            }
        }
    }

    // If search failed in the memtable, then let's try same thing in sstables
    if (!valueFound && shmMemtable->sstableCnt > 0) {
        // Start searching from the latest sstable
        for (i = shmMemtable->sstableCnt; i >= (shmMemtable->sstMergedCnt + 1); i--) {
            sprintf(fileName, "%d.sst", i);
            if ((sst = fopen(fileName, "r")) == NULL)
                perror("Error occurs while openning SSTable (GET)\n");
            while (fscanf(sst, "%d %d %s", &tempNo, &tempKey, tempValue) != EOF) {
                if (tempKey == key) {
                    strncpy(valueStr, tempValue, strlen(tempValue));
                    valueFound = 1;     // The lastly found value is the latest
                }
            }
            if (valueFound)   // If found, then stop
                break;
        }
    }
    // If program reach here without strncpy, then GET finally fails
    return valueFound;
}

// 이제 Merge 구현해야함. Merge 구현하고 나서 테스트 및 코드 폴리싱 및 보고서 작성 (04/10)
