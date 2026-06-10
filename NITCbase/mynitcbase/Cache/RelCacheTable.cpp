#include "RelCacheTable.h"
#include <cstring>

RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

/*
Get the relation catalog entry for the relation with rel-id `relId` from the cache
NOTE: this function expects the caller to allocate memory for `*relCatBuf`
Final State Achieved in Stage 3
*/

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuf){
    if(relId<0 || relId>MAX_OPEN)
        return E_OUTOFBOUND;
        
        
    // if there's no entry at the rel-id
    if(relCache[relId]==nullptr)
        return E_RELNOTOPEN;
        
    *relCatBuf=relCache[relId]->relCatEntry;
    return SUCCESS;
}

/* Converts a relation catalog record to RelCatEntry struct
We get the record as Attribute[] from the BlockBuffer.getRecord() function.
This function will convert that to a struct RelCatEntry type.
NOTE: this function expects the caller to allocate memory for `*relCatEntry`
Final State Achieved in Stage 3
*/

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry){
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs=(int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs=(int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk=(int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk=(int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk=(int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}



/* will return the searchIndex for the relation corresponding to `relId
NOTE: this function expects the caller to allocate memory for `*searchIndex`
Final State Achieved in Stage 4
*/
int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex){
    if(relId<0 || relId>MAX_OPEN)
        return E_OUTOFBOUND;

    // if there's no entry at the rel-id
    if(relCache[relId]==nullptr)
        return E_RELNOTOPEN;

    *searchIndex=relCache[relId]->searchIndex;
    return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex){
    if(relId<0 || relId>MAX_OPEN)
        return E_OUTOFBOUND;

    // if there's no entry at the rel-id
    if(relCache[relId]==nullptr)
        return E_RELNOTOPEN;

    relCache[relId]->searchIndex=*searchIndex;
    return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId){
    RecId initVal={-1, -1};
    return setSearchIndex(relId, &initVal);
}