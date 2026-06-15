#include "BlockAccess.h"
#include <stdlib.h>
#include <cstring>
#include <iostream>
using namespace std;

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // relId is index of this relation in the relation cache
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // let block and slot denote the record id of the record being currently checked
    int block, slot;
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)
        RelCatEntry relCatBuf;
        RelCacheTable::getRelCatEntry(relId, &relCatBuf);

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        block=relCatBuf.firstBlk;
        slot=0;
        // block = first record block of the relation
        // slot = 0
    }
    else
    {
       // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer currBlock(block);
        HeadInfo headInfo;
        // get header of the block using RecBuffer::getHeader() function
        currBlock.getHeader(&headInfo);
        int n=headInfo.numAttrs;

        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char  *slotMap = new unsigned char [headInfo.numSlots];
        currBlock.getSlotMap(slotMap);

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot>=headInfo.numSlots)
        {
            // update block = right block of block
            block=headInfo.rblock;
            // update slot = 0
            slot=0;
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotMap[slot]==SLOT_UNOCCUPIED){
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        // we only have attribute name, we don't where in the record it is present
        // so we use the function we defined in AttrCacheTable
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
        /* use the attribute offset to get the value of the attribute from
           current record */

        int offset=attrCatEntry.offset;
        // get the record with id (block, slot) using RecBuffer::getRecord()
        // Attribute *record=(Attribute*) malloc(n*sizeof(Attribute));
        Attribute *record= new Attribute[n];
        currBlock.getRecord(record, slot);


        // will store the difference between the attributes
        // we have to see if record compared to attrVal so record goes first
        int cmpVal=compareAttrs(record[offset], attrVal, attrCatEntry.attrType);  
        // set cmpVal using compareAttrs()

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId newIndex={block, slot};
            RelCacheTable::setSearchIndex(relId, &newIndex);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    // cout<<"Block access layer invoked last "<<oldName<<' '<<newName<<endl;
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);
    
    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId newRel=linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelationName, EQ);
    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    
    if(newRel.block!=-1 || newRel.slot!=-1)
        return E_RELEXIST;
    
    /* reset the searchIndex of the relation catalog using
    RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    
    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    RecId oldRel=linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    
    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    
    if(oldRel.block==-1 || oldRel.slot==-1)
        return E_RELNOTEXIST;

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer relCatBuffer(oldRel.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, oldRel.slot);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, newName);
    printf("%s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
    // printf("%s\n", attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal);
    relCatBuffer.setRecord(relCatRecord, oldRel.slot);
    // relCatBuffer.getRecord(relCatRecord, oldRel.slot);
    printf("%s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
    // cout<<"Block access layer invoked last"<<endl;
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    
    //for i = 0 to numberOfAttributes :
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID); 
    int numAttrs=relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    for(int i=0;i<numAttrs;i++){
        //    linearSearch on the attribute catalog for relName = oldRelationName
        RecId attrId=linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
        //    get the record using RecBuffer.getRecord
        RecBuffer attrCatBuffer(attrId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, attrId.slot);
        //    update the relName field in the record to newName
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        //    set back the record using RecBuffer.setRecord
        attrCatBuffer.setRecord(attrCatRecord, attrId.slot);
    }

    // cout<<"Block access layer invoked last"<<endl;
    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    // Search for the relation with name relName in relation catalog using linearSearch()
    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal, relName);
    RecId relId=linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    if(relId.block==-1 || relId.slot==-1)
        return E_RELNOTEXIST;

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        RecId attrId=linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if(attrId.block==-1 || attrId.slot==-1)
            break;

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer attrCatBuffer(attrId.block);
        attrCatBuffer.getRecord(attrCatEntryRecord, attrId.slot);

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName)==0){
            attrToRenameRecId=attrId;
            break;
        }

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName)==0)
            return E_ATTREXIST;
    }

    if (attrToRenameRecId.block ==-1 && attrToRenameRecId.slot ==-1)
        return E_ATTRNOTEXIST;


    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at attrToRenameRecId.slot */
    RecBuffer attrCatBuffer(attrToRenameRecId.block);
    attrCatBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    //   update the AttrName of the record with newName
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    //   set back the record with RecBuffer.setRecord
    attrCatBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // cout<<record[1].nVal<<endl;
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    /* first record block of the relation (from the rel-cat entry)*/;
    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    /* number of slots per record block */
    int numOfAttributes = relCatEntry.numAttrs;
    /* number of attributes of the relation */

    int prevBlockNum = -1;
    // keeps track of prev element in linked list
    /* block number of the last element in the linked list = -1 */

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer recBuffer(blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
        HeadInfo headInfo;
        recBuffer.getHeader(&headInfo);
        
        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotMap[numOfSlots];
        recBuffer.getSlotMap(slotMap);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        for(int i=0;i<numOfSlots;i++)
            if(slotMap[i]==SLOT_UNOCCUPIED){
                rec_id.block=blockNum;
                rec_id.slot=i;
                break;
            }

        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */
        if(rec_id.slot!=-1)
            break;
        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
        prevBlockNum=blockNum;
        blockNum=headInfo.rblock;
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if(rec_id.slot==-1){
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        if(relId==RELCAT_RELID) 
            return E_MAXRELATIONS;
        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        RecBuffer newRecBuffer;
        // get the block number of the newly allocated block
        int newBlock=newRecBuffer.getBlockNum();
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        if (newBlock == E_DISKFULL) {
            return E_DISKFULL;
        }
        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block=newBlock;
        rec_id.slot=0;

        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
            set the block's header as follows:
            blockType: REC, pblock: -1
            lblock
                  = -1 (if linked list of existing record blocks was empty
                         i.e this is the first insertion into the relation)
                  = prevBlockNum (otherwise),
            rblock: -1, numEntries: 0,
            numSlots: numOfSlots, numAttrs: numOfAttributes
            (use BlockBuffer::setHeader() function)
        */
        HeadInfo newHeader;
        newHeader.blockType=REC;
        newHeader.pblock=-1;
        newHeader.lblock=prevBlockNum; 
        // -1 if no blocks for this rel till now
        newHeader.rblock=-1;
        newHeader.numEntries=0;
        newHeader.numSlots=numOfSlots;
        newHeader.numAttrs=numOfAttributes;
        
        newRecBuffer.setHeader(&newHeader);
        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
        unsigned char newSlotMap[numOfSlots];
        for(int i=0;i<numOfSlots;i++)
            newSlotMap[i]=SLOT_UNOCCUPIED;
        newRecBuffer.setSlotMap(newSlotMap);

        // if prevBlockNum != -1
        if(prevBlockNum!=-1){
            // create a RecBuffer object for prevBlockNum
            RecBuffer prevRecBuffer(prevBlockNum);

            // get the header of the block prevBlockNum and
            HeadInfo prevHeader;
            prevRecBuffer.getHeader(&prevHeader);
            
            // update the rblock field of the header to the new block
            // number i.e. rec_id.block
            prevHeader.rblock=rec_id.block;
            // (use BlockBuffer::setHeader() function)
            prevRecBuffer.setHeader(&prevHeader);
        }
        else{
            // update first block field in the relation catalog entry to the
            relCatEntry.firstBlk=newBlock;
            // new block (using RelCacheTable::setRelCatEntry() function)
        }
        // update last block field in the relation catalog entry to the
        relCatEntry.lastBlk=newBlock;

        // new block (using RelCacheTable::setRelCatEntry() function)
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    RecBuffer recBuffer(rec_id.block);

    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    recBuffer.setRecord(record, rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)

    unsigned char slotMap[numOfSlots];
    recBuffer.getSlotMap(slotMap);
    slotMap[rec_id.slot]=SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    HeadInfo headInfo;
    recBuffer.getHeader(&headInfo);
    headInfo.numEntries++;
    recBuffer.setHeader(&headInfo);
    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry); 
    return SUCCESS;
}