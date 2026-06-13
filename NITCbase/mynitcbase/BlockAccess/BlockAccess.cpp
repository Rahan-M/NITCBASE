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