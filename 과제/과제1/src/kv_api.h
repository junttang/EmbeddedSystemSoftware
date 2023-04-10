#ifndef _KV_API_H_
#define _KV_API_H_

/* Maximum # of sstables in this KV store */
#define MAX_SSTABLE 10   // Cover up to 9,999,999,999 tables

/* Arguments for PUT operation */
typedef struct _putArgs {
    bool keypad[SWITCH_CNT + 1];
    bool firstFlag;
    bool resetFlag;
    bool keyInputMode;
    bool numAlphaMode;
    bool MemtableInsert;
} putArgs;

/* Arguments for GET operation */
typedef struct _getArgs {
    bool keypad[SWITCH_CNT + 1];
    bool firstFlag;
    bool resetFlag;
} getArgs;

/* Arguments for MERGE operation */
typedef struct _mergeArgs {
    bool keypad[SWITCH_CNT + 1];
    bool firstFlag;
    bool resetFlag;
} mergeArgs;

/* API functions for Key-Value Interface */
bool Put(putArgs *, const bool);
bool Get(getArgs *, const bool);
bool Merge(mergeArgs *, const bool);

#endif /* _KV_API_H_ */
