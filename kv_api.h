#ifndef _KV_API_H_
#define _KV_API_H_

/* Maximum # of sstables in this KV store */
#define MAX_SSTABLE 10     // It can cover upto 9,999,999,999 tables

/* Maximum key length */
#define MAX_KEY_LEN 4

/* Maximum # of pairs in one sstable (i think it's enough for evaluation) */
#define MAX_PAIR_NUM 100

/* Arguments for PUT operation */
typedef struct _putArgs {
    bool keypad[SWITCH_CNT + 1];    // Keypad info
    bool firstFlag;                 // First attempt since the mode changed
    bool clearFlag;                 // Clear the input status
    bool resetFlag;                 // Change the key input mode to value input mode
    bool keyInputMode;              // Literally key input mode
    bool numAlphaMode;              // It means input character type has been changed
    bool memtableInsert;            // Memtable insertion flag
} putArgs;

/* Arguments for GET operation */
typedef struct _getArgs {
    bool keypad[SWITCH_CNT + 1];    // Keypad info
    bool firstFlag;                 // First attempt since the mode changed
    bool clearFlag;                 // Clear the input status
    bool resetFlag;                 // Retrieve one pair from the KV store
} getArgs;  

/* Arguments for MERGE operation */
typedef struct _mergeArgs {
    bool resetFlag;                 // Trigger the compaction
} mergeArgs;

/* API functions for Key-Value Interface */
void Put(putArgs *, const bool);
void Get(getArgs *, const bool);
void Merge(mergeArgs *, const bool);

/* Procedures for argument initialization */
void initPutArgs(putArgs *);
void initGetArgs(getArgs *);
void initMergeArgs(mergeArgs *);

#endif /* _KV_API_H_ */
