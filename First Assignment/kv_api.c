#include "headers.h"

/* For logging */
#define DEBUG_PROJ

/* Shared memories */
extern SHM_MAIN_IN *shmIn;
extern SHM_MAIN_OUT *shmOut;
extern SHM_MEMTABLE *shmMemtable;

/* Keypad info */
const char KEYPAD[][3] = {
    "",
    ".QZ", "ABC", "DEF",  
    "GHI", "JKL", "MNO",  
    "PRS", "TUV", "WXY"   
};

/* Internal handlers for key-value interface */
const void _Put(putArgs *);
const void _Get(getArgs *);
const void _Merge(mergeArgs*);

/* Key-Value functionalities */
const bool insertMemtable(const char *, const char *, int *);
const int searchTables(const char *, char *, int *);
const int compactTables(char *);

/* String for value buffer initialization of PUT operation */
char *initValueBuf = "                               ";    // 31 spaces, 1 null

/* API wrapper function for PUT operation */
void Put(putArgs *args, const bool modeChanged) {
    bool noResetFlag = 0;
    int i;
    
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = 0;

    // First time for current mode
    if (modeChanged) 
        initPutArgs(args);

    // Check if RESET key is pressed
    for (i = 1; i <= SWITCH_CNT; i++) {
        if (!shmIn->switches[i]) {
            noResetFlag = 1;
            break;
        }
    }
    if (!noResetFlag)
        args->resetFlag = 1;
    else {
        // Verify switches input
        if (shmIn->switches[2] && shmIn->switches[3])
            args->clearFlag = 1;
        else if (shmIn->switches[5] && shmIn->switches[6]) 
            args->numAlphaMode = 1;    // Number input or alphabet input
        else if (shmIn->switches[8] && shmIn->switches[9]) 
            args->memtableInsert = 1;  // Insert to the memtable
    }

    // Set keypad inputs
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = shmIn->switches[i];

    _Put(args);
}

/* PUT operation handler */  
const void _Put(putArgs *args) {
    static char key[MAX_KEY_LEN + 1];
    static char value[MAX_TEXT_LEN + 1];
    static char inserted[MAX_TEXT_LEN + 1];
    static int index = 0, prev = -1, option = 0, leds = LED_1;
    static bool numMode = 1, inputStart = 0;
    char temp[MAX_KEY_LEN + 1], c = -1; 
    bool changed = 0, insertSuccess = 0;
    int order, fnd, i;

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
    shmOut->used[MOTOR] = 1; shmOut->motorOn = 0;

    // Get the device time
    time_t devTime; time(&devTime);
    const struct tm *timeInfo = localtime(&devTime);
    const int devSecond = timeInfo->tm_sec;

    // Initialize if it's first time
    if (args->firstFlag) {
        strncpy(key, "0000", MAX_KEY_LEN); shmOut->fndBuf = 0;
        strncpy(value, initValueBuf, MAX_TEXT_LEN);
        memcpy(shmOut->textlcdBuf, value, MAX_TEXT_LEN);
        numMode = 1;            // Default is number input mode
        index = 0;              // Start filling the text string from idx 0 
        prev = -1;              // Previous keypad info (for alphabet mode)
        option = 0;             // Sub-option of specific key (for alphabet mode)
        changed = 0;            // Is it changed?
        inputStart = 0;         // Initial LED is LED_1
        leds = LED_1;           // Turn on the LED_1
        args->keyInputMode = 1; // Default is key input mode
        args->firstFlag = 0;    // Reset the flag
    }

    // Case1: Change the mode (key to value)
    else if (args->resetFlag) {
        args->keyInputMode = 0;
        index = 0; prev = -1; option = 0;
        args->resetFlag = 0;
    } 
    // Case2: Clear the string
    else if (args->clearFlag) {
        if (args->keyInputMode) 
            strncpy(key, "0000", MAX_KEY_LEN);
        else strncpy(value, initValueBuf, MAX_TEXT_LEN);
        index = 0; prev = -1; option = 0; changed = 1;
        args->clearFlag = 0;
    } 
    // Case3: Change the alphabet <-> number (only in value input mode)
    else if (args->numAlphaMode && !args->keyInputMode) {
        numMode = !numMode;     // Reverse num/alpha mode
        prev = -1; option = 0;
        args->numAlphaMode = 0;
    }
    // Case4: Memtable insertion
    else if (args->memtableInsert) {
#ifdef DEBUG_PROJ
        printf("Memtable before insert\n");
        for (i = 0; i < shmMemtable->pairCnt; i++)
            printf("%d %d %s\n", shmMemtable->totalPutCnt + i + 1, 
                shmMemtable->keys[i], shmMemtable->values[i]);
#endif
        key[4] = '\0';    // Set null character (for preventing any error)
        insertSuccess = insertMemtable(key, value, &order);   // Insert
#ifdef DEBUG_PROJ
        printf("Memtable after insert\n");
        for (i = 0; i < shmMemtable->pairCnt; i++)
            printf("%d %d %s\n", shmMemtable->totalPutCnt + i + 1, 
                shmMemtable->keys[i], shmMemtable->values[i]);
#endif
        // Turn on all the LEDs for a second
        leds = LED_1 | LED_2 | LED_3 | LED_4 |
               LED_5 | LED_6 | LED_7 | LED_8;
        prev = -1; option = 0; changed = 1;
        inputStart = 0;     // To turn on only LED_1
        args->memtableInsert = 0;
        args->firstFlag = 1;

        // Print inserted information for a second
        if (insertSuccess) {
            for (i = 0; ; i++) {
                if (i == MAX_TEXT_LEN - 1 || value[i] == ' ') {
                    value[i] = '\0';
                    break;
                }
            }
            sprintf(inserted, "(%d, %s, %s)", shmMemtable->totalPutCnt + (order + 1), key, value);
            memcpy(shmOut->textlcdBuf, inserted, MAX_TEXT_LEN);
        }

        // Initialize for the next input
        strncpy(key, "0000", MAX_KEY_LEN);
        strncpy(value, initValueBuf, MAX_TEXT_LEN);
        shmOut->fndBuf = 0;
    } 
    // Case4: Just insert character
    else {
        for (i = 1; i <= SWITCH_CNT; i++) {
            if (args->keypad[i]) {
                // Number mode
                if (numMode) {
                    c = '0' + i;
                    prev = -1; option = 0;
                }
                // Alphabet mode (only in value input mode)
                else if (!args->keyInputMode) {
                    if (i == prev) 
                        option = (option + 1) % 3;
                    else {
                        if (prev != -1)
                            index++;
                        prev = i; option = 0;
                    }
                    c = KEYPAD[i][option];
                }
                inputStart = changed = 1;
                break;
            }
        }
    }
    // If input started, then keep turning on corresponding LEDs 
    if (inputStart) {
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
    shmOut->ledBuf = leds;
    if (!changed)
        return;

    // Append char to the string
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
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
                if (numMode)
                    index++;
            }
        }
    }
#ifdef DEBUG_PROJ
    printf("PUT: key=%s, value=%s\n", key, value);
#endif
    // Print to FND/LCD/LED via shared memory (while input)
    if (!insertSuccess) {
        if (args->keyInputMode) {
            strncpy(temp, key, MAX_KEY_LEN);
            fnd = atoi(temp);
            shmOut->fndBuf = fnd;
        }
        else memcpy(shmOut->textlcdBuf, value, MAX_TEXT_LEN);
    }
    else shmOut->waitFlag = 1;
}

/* API wrapper function for GET operation */
void Get(getArgs *args, const bool modeChanged) {
    bool noResetFlag = 0;
    int i;

    for (i = 0; i <= SWITCH_CNT; i++)
        args->keypad[i] = 0;

    // First time for current mode
    if (modeChanged) 
        initGetArgs(args);

    // Check if RESET key is pressed
    for (i = 1; i <= SWITCH_CNT; i++) {
        if (!shmIn->switches[i]) {
            noResetFlag = 1;
            break;
        }
    }
    if (!noResetFlag)
        args->resetFlag = 1;
    else {
        // Verify switches input
        if (shmIn->switches[2] && shmIn->switches[3])
            args->clearFlag = 1;
    }

    // Set keypad inputs
    for (i = 0; i <= SWITCH_CNT; i++) 
        args->keypad[i] = shmIn->switches[i];

    _Get(args);
}

/* GET operation handler */  
const void _Get(getArgs *args) {
    static char key[MAX_TEXT_LEN];
    static char value[MAX_TEXT_LEN];
    static char inserted[MAX_TEXT_LEN + 1];
    static int index = 0, leds = LED_1;
    static bool inputStart = 0;
    int valueFound = 0; bool changed = 0;
    char temp[MAX_KEY_LEN + 1], c = -1; 
    int order, fnd, i;

    // Turn on devices we need here
    shmOut->used[FND] = 1;
    shmOut->used[LED] = 1;
    shmOut->used[TEXT_LCD] = 1;
    shmOut->used[MOTOR] = 1; shmOut->motorOn = 0;

    // Get the device time
    time_t devTime; time(&devTime);
    const struct tm *timeInfo = localtime(&devTime);
    const int devSecond = timeInfo->tm_sec;

    // Initialize if it's first time
    if (args->firstFlag) {
        strncpy(key, "0000", MAX_KEY_LEN); shmOut->fndBuf = 0;
        memcpy(shmOut->textlcdBuf, initValueBuf, MAX_TEXT_LEN);
        index = 0;              // Start filling the key string from idx 0 
        changed = 1;            // Is it changed?
        inputStart = 0;         // Initial LED is LED_1
        leds = LED_1;           // Turn on the LED_1
        args->firstFlag = 0;
    }

    // Case1: Retrieve one pair from KV store
    else if (args->resetFlag) {
        valueFound = searchTables(key, value, &order);   // value will be updated!
        if (!valueFound)  // If search failed
            strncpy(inserted, "Error", 6);
#ifdef DEBUG_PROJ
        // printf("GET result: value=%s\n", value);
	if (valueFound == 1)
            printf("%d %s %s\n", shmMemtable->totalPutCnt + order + 1, key, value);
        else if (valueFound == 2)
            printf("%d %s %s\n", order, key, value);
#endif
        // Turn on all the LEDs for a second
        leds = LED_1 | LED_2 | LED_3 | LED_4 |
               LED_5 | LED_6 | LED_7 | LED_8;
        index = 0; changed = 1;
        inputStart = 0;
        args->firstFlag = 1;
        args->resetFlag = 0;

        // Print inserted information for a second
        if (valueFound) {
            for (i = 0; ; i++) {
                if (i == MAX_TEXT_LEN - 1 || value[i] == ' ') {
                    value[i] = '\0';
                    break;
                }
            }
            if (valueFound == 1)
                sprintf(inserted, "(%d, %s, %s)", shmMemtable->totalPutCnt + order + 1, key, value);
            else sprintf(inserted, "(%d, %s, %s)", order, key, value);
        }

        memcpy(shmOut->textlcdBuf, inserted, MAX_TEXT_LEN);
        strncpy(key, "0000", MAX_KEY_LEN);
        strncpy(value, initValueBuf, MAX_TEXT_LEN);
        shmOut->fndBuf = 0;
        shmOut->waitFlag = 1;
    } 
    // Case2: Clear the string
    else if (args->clearFlag) {
        strncpy(key, "0000", MAX_KEY_LEN);
        index = 0;
        changed = 1;
        args->clearFlag = 0;
    }    
    // Case3: Just insert character case
    else {
        for (i = 1; i <= SWITCH_CNT; i++) {
            if (args->keypad[i]) {
                c = '0' + i;
                if (!inputStart)
                    strncpy(value, initValueBuf, MAX_TEXT_LEN);
                inputStart = changed = 1;
                break;
            }
        }
    }
    if (inputStart) { 
        if ((devSecond % 2) == 0) 
            leds = LED_3;
        else leds = LED_4;
    }
    shmOut->ledBuf = leds;
    if (!changed) 
        return;

    // Append char to the string
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
        // Insert in a circular manner
        if (index >= MAX_KEY_LEN)     // FND (key input)
            index = 0;
        key[index++] = c;
    }
#ifdef DEBUG_PROJ
    printf("GET: key=%s\n", key);
#endif
    // Print to FND/LCD/LED via shared memory (while input)
    if (!shmOut->waitFlag) {
        strncpy(temp, key, MAX_KEY_LEN);
        fnd = atoi(temp);
        shmOut->fndBuf = fnd;
        shmOut->ledBuf = leds;
    }
}

/* API wrapper function for MERGE operation */
void Merge(mergeArgs *args, const bool modeChanged) {
    bool noResetFlag = 0;
    int i;

    // First time for current mode
    if (modeChanged)
        initMergeArgs(args);

    // Check if RESET key is pressed
    for (i = 1; i <= SWITCH_CNT; i++) {
        if (!shmIn->switches[i]) {
            noResetFlag = 1;
            break;
        }
    }
    if (!noResetFlag)
        args->resetFlag = 1;

    _Merge(args);
}

/* MERGE operation handler */  
const void _Merge(mergeArgs* args) {
    char text[MAX_TEXT_LEN + 10];
    char temp[MAX_TEXT_LEN];
    int mergeFlag = 0;

    // Turn on devices we need here
    shmOut->used[FND] = 0;
    shmOut->used[LED] = 0;
    shmOut->used[TEXT_LCD] = 1;
    shmOut->used[MOTOR] = 1; shmOut->motorOn = 0;

    // Perform MERGE when reset pressed!
    if (args->resetFlag) {
        strncpy(text, initValueBuf, MAX_TEXT_LEN);
        strncpy(temp, initValueBuf, MAX_TEXT_LEN);
#ifdef DEBUG_PROJ
        int i, tempNo, tempKey; FILE *sst;
        char fileName[MAX_TEXT_LEN], tempValue[MAX_TEXT_LEN];
        for (i = 1; ; i++) {        // Print all the data of sstables (before compaction)
            sprintf(fileName, "%d.sst", i);
            if ((sst = fopen(fileName, "r")) == NULL)
                break;
            if (i <= shmMemtable->sstMergedCnt)
                printf("- (x) File %s: ", fileName);
            else printf(" - File %s: ", fileName);
            while (fscanf(sst, "%d %d %s", &tempNo, &tempKey, tempValue) != EOF) 
                printf("(%d,%d,%s) ", tempNo, tempKey, tempValue);
            printf("\n");
            fclose(sst);
        }
#endif
        mergeFlag = compactTables(text);        // Trigger compaction
#ifdef DEBUG_PROJ
        for (i = 1; ; i++) {        // Print all the data of sstables (after compaction)
            sprintf(fileName, "%d.sst", i);
            if ((sst = fopen(fileName, "r")) == NULL)
                break;
            if (i <= shmMemtable->sstMergedCnt)
                printf("- (x) File %s: ", fileName);
            else printf(" - File %s: ", fileName);
            while (fscanf(sst, "%d %d %s", &tempNo, &tempKey, tempValue) != EOF) 
                printf("(%d,%d,%s) ", tempNo, tempKey, tempValue);
            printf("\n");
            fclose(sst);
        }
#endif
        if (mergeFlag) {   // Print to LCD/MOTOR via shared memory
            sprintf(temp, " %d", mergeFlag);   // mergeFlag: # of pairs
            for (i = 0; ; i++) {
                if (i == MAX_TEXT_LEN - 1 || text[i] == ' ') {
                    text[i] = '\0';
                    break;
                }
            }
            strncat(text, temp, strlen(temp));      // ex) "3.sst 6"
            memcpy(shmOut->textlcdBuf, text, MAX_TEXT_LEN);
            shmOut->motorOn = 1;
#ifdef DEBUG_PROJ
            printf("---> %d pairs are inserted to the new file %s\n", mergeFlag, text);
#endif      
            shmOut->waitFlag = 1;   // Wait for a second (MOTOR)
        }
        args->resetFlag = 0;
    }
}

/* Initialize fields of putArgs */
void initPutArgs(putArgs *pargs) {
    pargs->firstFlag = 1;
    pargs->clearFlag = 0;
    pargs->resetFlag = 0;
    pargs->keyInputMode = 1;
    pargs->numAlphaMode = 0;
    pargs->memtableInsert = 0;    
}

/* Initialize fields of getArgs */
void initGetArgs(getArgs *gargs) {
    gargs->firstFlag = 1;
    gargs->clearFlag = 0;
    gargs->resetFlag = 0;
}

/* Initialize fields of mergeArgs */
void initMergeArgs(mergeArgs *margs) {
    margs->resetFlag = 0;
}

/* Insert key-value pair into the memtable */
const bool insertMemtable(const char *keyStr, const char *valueStr, int *order) {
    char fileName[MAX_SSTABLE + 5];     // ex) 1234.sst
    char key[MAX_KEY_LEN + 1]; 
    strncpy(key, keyStr, MAX_KEY_LEN);
    FILE *sst; int i, keyNum;

    // If key or value is as same as default, then decline the request
    if (!strncmp(keyStr, "0000", MAX_KEY_LEN + 1) ||
        !strncmp(valueStr, initValueBuf, MAX_TEXT_LEN))
        return 0;
    
    // Flush if memtable is full
    if (shmMemtable->pairCnt == 3) {
        shmMemtable->sstableCnt++;
        sprintf(fileName, "%d.sst", shmMemtable->sstableCnt);
        if ((sst = fopen(fileName, "w")) == NULL)
            perror("Error occurs while openning SSTable (PUT)\n");
        for (i = 0; i < MAX_PAIR; i++)
            fprintf(sst, "%d %d %s\n", (++shmMemtable->totalPutCnt), 
                shmMemtable->keys[i], shmMemtable->values[i]);
        
        shmMemtable->pairCnt = 0;
        fclose(sst);
#ifdef DEBUG_PROJ
        printf("Flush occurs!\n");
        int tempNo, tempKey; char tempVal[MAX_TEXT_LEN];
        if ((sst = fopen(fileName, "r")) == NULL)
            perror("Error occurs while openning SSTable (PUT_DEBUG)\n");
        printf("FileName: %s\n", fileName);
        for (i = 0; i < MAX_PAIR; i++) {
            fscanf(sst, "%d %d %s", &tempNo, &tempKey, tempVal); 
            printf("%d %d %s\n", tempNo, tempKey, tempVal);
        }
#endif
    }

    // Memtable insertion
    keyNum = atoi(key);
    if (keyNum > 9999) keyNum /= 10;     // Just for avoiding any error
    shmMemtable->keys[shmMemtable->pairCnt] = keyNum;    // Inserted as integer (for comparison when MERGE)
    strncpy(shmMemtable->values[shmMemtable->pairCnt], valueStr, strlen(valueStr));
    *order = shmMemtable->pairCnt;       // For printing
    shmMemtable->pairCnt++;

    return 1;
}

/* Retrieve the latest value of passed key */
const int searchTables(const char *keyStr, char *valueStr, int *order) {
    char fileName[MAX_SSTABLE + 5];     // ex) 1234.sst
    char temp[MAX_KEY_LEN + 1]; 
    strncpy(temp, keyStr, MAX_KEY_LEN);
    int key = atoi(temp), i, tempNo, tempKey, valueFound = 0;
    char tempValue[MAX_TEXT_LEN];
    FILE *sst; 

    // First search key in the memtable
    if (shmMemtable->pairCnt > 0) {
        for (i = (shmMemtable->pairCnt - 1); i >= 0; i--) {
            if (key == shmMemtable->keys[i]) {
                strncpy(valueStr, shmMemtable->values[i], strlen(shmMemtable->values[i]));
                *order = i;
                valueFound = 1;
                break;  // The first found value is the latest (cuz it searches from end)
            }
        }
    }
#ifdef DEBUG_PROJ
    if (valueFound)
        printf("Target value is retrieved from the memtable!\n");
    else {
        printf("Memtable doesn't have the target value, so let's search sstables\n");
        printf("* Current # of sstables: %d   * Current # of merged ssts: %d\n", 
            shmMemtable->sstableCnt, shmMemtable->sstMergedCnt);
    }
#endif
    // If search failed in the memtable, then let's try same thing in sstables
    if (!valueFound && shmMemtable->sstableCnt > 0) {
        // Start searching from the latest sstable
        for (i = shmMemtable->sstableCnt; i >= (shmMemtable->sstMergedCnt + 1); i--) {
            sprintf(fileName, "%d.sst", i);
            if ((sst = fopen(fileName, "r")) == NULL)
                perror("Error occurs while openning SSTable (GET)\n");
            while (fscanf(sst, "%d %d %s", &tempNo, &tempKey, tempValue) != EOF) {
                if (tempKey == key) {
                    *order = tempNo;
                    strncpy(valueStr, tempValue, strlen(tempValue));
                    valueFound = 2;     // The lastly found value is the latest
                }
            }
            fclose(sst);
            if (valueFound) {   // If found, then stop
#ifdef DEBUG_PROJ
                printf("Target value is retrieved from sstable %s!\n", fileName);
#endif
                break;
            }
        }
    }
#ifdef DEBUG_PROJ
    if (!valueFound)
        printf("Target value cannot be found, so, it's an error!\n");
#endif
    // If program reach here without strncpy, then GET finally fails
    return valueFound;
}

/* Trigger compaction: merge two sstables with sorting pairs and create new table */
const int compactTables(char *sstableName) {
    char fileName[MAX_SSTABLE + 5];     // ex) 1234.sst
    char tempName[MAX_SSTABLE + 5];
    char tempValue[MAX_PAIR_NUM][MAX_TEXT_LEN], c;
    int valuePointer[MAX_PAIR_NUM], tempKey[MAX_PAIR_NUM]; 
    int mergeStartFileNo = shmMemtable->sstMergedCnt + 1;
    int valueSstables = shmMemtable->sstableCnt - shmMemtable->sstMergedCnt;
    int i, j, totalPairCnt = 0, pairCnt = 0, temp;
    bool passFlag, putNoReadFlag = 0;
    FILE *sst, *tempSst; 

    // If there's more than 2 sstables in the disk, then perform merge!
    if (shmMemtable->sstableCnt && valueSstables >= 2) {
        // Get all the pairs in two victim sstables
        for (i = mergeStartFileNo; i <= mergeStartFileNo + 1; i++) {
            sprintf(fileName, "%d.sst", i);
#ifdef DEBUG_PROJ
            printf("Selected victim of compaction: %s\n", fileName);
#endif
            if ((sst = fopen(fileName, "r")) == NULL)
                perror("Error occurs while openning SSTable (MERGE)\n");
            while (fscanf(sst, "%d %d %s", &temp, &tempKey[pairCnt], tempValue[pairCnt]) != EOF) {
                // Record the PUTed number of first found entry
                if (!putNoReadFlag) {     // for proper counting of new sstable
                    putNoReadFlag = 1;
                    totalPairCnt = temp - 1;
                }
                valuePointer[pairCnt] = pairCnt;    // Value pointer for avoiding costly value copying
                passFlag = 0;

                // If current key is already found before, then only get value and copy to prev entry
                for (j = 0; j < pairCnt; j++) {
                    if (tempKey[pairCnt] == tempKey[j]) {
                        strncpy(tempValue[j], tempValue[pairCnt], MAX_TEXT_LEN);
                        passFlag = 1;       // And don't increment the index (for omitting this key)
                        break;
                    }
                }
                if (!passFlag)  // Increment the index only if the key is found for the first time
                    pairCnt++;
                totalPairCnt++;  // Total pair count for counting
            }
            fclose(sst);
        }
#ifdef DEBUG_PROJ
        printf("Temporary Storage for pairs\n");
        for (i = 0; i < pairCnt; i++)
            printf("%d %s\n", tempKey[i], tempValue[valuePointer[i]]);
#endif
        // Bubble sort for key sorting (ascending order)
        for (i = 0; i < pairCnt - 1; i++) {
            for (j = 0; j < pairCnt - i - 1; j++) {
                if (tempKey[j] > tempKey[j + 1]) {
                    temp = tempKey[j];
                    tempKey[j] = tempKey[j + 1];
                    tempKey[j + 1] = temp;

                    // Value pointer literally points where real value is
                    temp = valuePointer[j];   // for speedy merge operation
                    valuePointer[j] = valuePointer[j + 1];
                    valuePointer[j + 1] = temp;
                }
            }       // Actual values are never moved, only pointers are moved 
        }
#ifdef DEBUG_PROJ
        printf("Temporary Storage for pairs (after sorting)\n");
        for (i = 0; i < pairCnt; i++)
            printf("%d %s\n", tempKey[i], tempValue[valuePointer[i]]);
#endif
        // Make a room for new sstable, by pushing all the left sstables
        for (i = shmMemtable->sstableCnt; i >= mergeStartFileNo + 2; i--) {
            sprintf(fileName, "%d.sst", i);
            sprintf(tempName, "%d.sst", i + 1);     // Change the name of file
            if ((sst = fopen(fileName, "rb")) == NULL)
                perror("Error occurs while openning SSTable (MERGE)\n");
            if ((tempSst = fopen(tempName, "wb")) == NULL)
                perror("Error occurs while openning SSTable (MERGE)\n");

            // Just copy the contents of file
            while (!feof(sst)) {
                fread(&c, sizeof(char), 1, sst);
                fwrite(&c, sizeof(char), 1, tempSst);
            }
            fclose(sst);
            fclose(tempSst);
        } 
        // Create a new sstable!
        sprintf(fileName, "%d.sst", mergeStartFileNo + 2);
        if ((sst = fopen(fileName, "w")) == NULL)
            perror("Error occurs while openning SSTable (MERGE)\n");
        for (i = 0; i < pairCnt; i++) 
            fprintf(sst, "%d %d %s\n", (totalPairCnt - pairCnt + i + 1), 
                tempKey[i], tempValue[valuePointer[i]]);
        fclose(sst);
#ifdef DEBUG_PROJ
        printf("Created new sstable %s\n", fileName);
#endif
        shmMemtable->sstMergedCnt += 2;
        shmMemtable->sstableCnt += 1;
        strncpy(sstableName, fileName, strlen(fileName));
    }
#ifdef DEBUG_PROJ
    else printf("There's no MERGE needed here!\n");
#endif
    // If there's no merge, then pairCnt would be zero!
    return pairCnt;
}
