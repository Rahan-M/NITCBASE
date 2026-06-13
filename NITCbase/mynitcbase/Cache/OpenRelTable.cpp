#include "OpenRelTable.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>
using namespace std;

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free=true;
  }

  /************ 1. Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** 1.a setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK); // for operations on relational catalog block
  Attribute relCatRecord[RELCAT_NO_ATTRS]; // for storing records of relational catalog
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT); // fetch relational catalog entry for relational catalog :)

  RelCacheEntry relCacheEntry; // what we will store in relational cache soon
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry); // transfer what's present as record to the relatonal cache entry
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** 1.b setting up Attribute Catalog relation in the Relation Cache Table ****/
  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block=RELCAT_BLOCK;
  relCacheEntry.recId.slot=RELCAT_SLOTNUM_FOR_ATTRCAT;

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  RelCacheTable::relCache[ATTRCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


  // // 1.c Setting up student
  // HeadInfo relCatHeader;
  // relCatBlock.getHeader(&relCatHeader);
  // int slot=-1;
  // int numRels=relCatHeader.numEntries;
  // for(int i=0;i<numRels;i++){
  //   // Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog
  //   relCatBlock.getRecord(relCatRecord, i);
  //   if(strcmp("Students", relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0){
  //     slot=i;
  //     break;
  //   }
  // }

  // if(slot!=-1){
  //   RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  //   relCacheEntry.recId.block=RELCAT_BLOCK;
  //   relCacheEntry.recId.slot=slot;
  //   RelCacheTable::relCache[ATTRCAT_RELID+1] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  //   *(RelCacheTable::relCache[ATTRCAT_RELID+1]) = relCacheEntry;
  // }else cout<<"\n\n\nRelation NOT FOUND\n\n\n\n";


  /************2. Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** 2.a setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);

  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  AttrCacheEntry* head=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
  AttrCacheEntry* temp=head;

  int m=RelCacheTable::relCache[RELCAT_RELID]->relCatEntry.numAttrs; 
  // n stores number of attributes in relational catalog, Which is 6 ofcourse
  for(int i=0;i<m;i++){
      // iterate through all the attributes of the relation catalog and create a linked
      attrCatBlock.getRecord(attrCatRecord, i); // first 6 slots are for relational catalog
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &temp->attrCatEntry);  
      temp->recId.block=ATTRCAT_BLOCK;
      temp->recId.slot=i;

      if(i<m-1)
        temp->next=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
      else
        temp->next=nullptr;
      temp=temp->next;
  } 

  AttrCacheTable::attrCache[RELCAT_RELID] = head /* head of the linked list */;
  
  /**** 2.b setting up Attribute Catalog relation in the Attribute Cache Table ****/
  head=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
  temp=head;

  int n=RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry.numAttrs; 
  // n stores number of attributes in attr catalog, Which is 6 ofcourse
  for(int i=m;i<m+n;i++){
      // iterate through all the attributes of the relation catalog and create a linked
      attrCatBlock.getRecord(attrCatRecord, i); // slots 6-11 are for attribute catalog attributes
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &temp->attrCatEntry);  
      temp->recId.block=ATTRCAT_BLOCK;
      temp->recId.slot=i;

      if(i<m+n-1)
      temp->next=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
      else
        temp->next=nullptr;
      temp=temp->next;
  } 
  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  AttrCacheTable::attrCache[ATTRCAT_RELID] = head /* head of the linked list */;

  // // 2.c Student STAGE 3 EXERCICE
  // if(slot!=-1){
  //   head=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
  //   temp=head; 
  //   int numAttrs=RelCacheTable::relCache[ATTRCAT_RELID+1]->relCatEntry.numAttrs; 
  //   for(int i=12;i<12+numAttrs;i++){
  //     // iterate through all the attributes of the relation catalog and create a linked
  //     attrCatBlock.getRecord(attrCatRecord, i); // slots 6-11 are for attribute catalog attributes
  //     AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &temp->attrCatEntry);  
  //     temp->recId.block=ATTRCAT_BLOCK;
  //     temp->recId.slot=i;

  //     if(i<12+numAttrs-1)
  //       temp->next=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
  //     else
  //       temp->next=nullptr;
  //     temp=temp->next;
  //   } 
  //   AttrCacheTable::attrCache[ATTRCAT_RELID+1] = head /* head of the linked list */;
  // }

  tableMetaInfo[RELCAT_RELID].free=false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
  tableMetaInfo[ATTRCAT_RELID].free=false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable() {
    // close all open relations (from rel-id = 2 onwards. Why?)
    for (int i = 2; i < MAX_OPEN; ++i) {
      if (!tableMetaInfo[i].free) {
        OpenRelTable::closeRel(i); // we will implement this function later
      }
    }

    // free all the memory that you allocated in the constructor
    for(int i=0;i<=1;i++){
        free(RelCacheTable::relCache[i]);
        RelCacheTable::relCache[i] = nullptr;
        
        AttrCacheEntry* head=AttrCacheTable::attrCache[i];
        AttrCacheEntry* temp=nullptr;
        // n stores number of attributes in relational catalog, Which is 6 ofcourse
        while(head!=nullptr){
          temp=head;
          head=head->next;
          free(temp);
        } 
        AttrCacheTable::attrCache[i] = nullptr;
    }
}

int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
  for(int i=0;i<MAX_OPEN;i++)
    if(tableMetaInfo[i].free)
      return i;

      // if found return the relation id, else return E_CACHEFULL.
  return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
  for(int i=0;i<MAX_OPEN;i++)
    if(!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName)==0)
      return i;

  // if found return the relation id, else indicate that the relation do not
  // have an entry in the Open Relation Table.
  return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  int relId=OpenRelTable::getRelId(relName);
  if(relId>=0){
    // (checked using OpenRelTable::getRelId())
    // return that relation id;
    return relId;
  }

  /* find a free slot in the Open Relation Table
  using OpenRelTable::getFreeOpenRelTableEntry(). */
  // let relId be used to store the free slot.
  
  relId=OpenRelTable::getFreeOpenRelTableEntry();
  if (relId==E_CACHEFULL){
    return E_CACHEFULL;
  }


  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId;
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute target;
  strcpy(target.sVal, relName);

  relcatRecId=BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, target, EQ);
  if (relcatRecId.block==-1 || relcatRecId.slot==-1 ) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */

  RecBuffer recBuffer(relcatRecId.block);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  recBuffer.getRecord(relCatRecord, relcatRecId.slot);
  RelCacheEntry *relCacheEntry=(RelCacheEntry*) malloc(sizeof(RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
     
  //  update the recId field of this Relation Cache entry to relcatRecId.
  relCacheEntry->recId=relcatRecId;
    
  //  use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
  RelCacheTable::relCache[relId]=relCacheEntry;
  
  
  
  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry *attrCacheEntry=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
  AttrCacheEntry *listHead=attrCacheEntry;
  int numAttrs=relCacheEntry->relCatEntry.numAttrs;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  for(int i=0;i<numAttrs;i++){
    /* let attrcatRecId store a valid record id an entry of the relation, relName,
    in the Attribute Catalog.*/
    RecId attrCatRecId=BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, target, EQ);
    RecBuffer recBuffer(attrCatRecId.block);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    // read the record entry corresponding to attrcatRecId and create an
    // Attribute Cache entry on it using RecBuffer::getRecord() and
    recBuffer.getRecord(attrCatRecord, attrCatRecId.slot);
    // AttrCacheTable::recordToAttrCatEntry().
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    // update the recId field of this Attribute Cache entry to attrcatRecId.
    attrCacheEntry->recId=attrCatRecId;
    if(i==numAttrs-1)
      attrCacheEntry->next=nullptr;
    else 
      attrCacheEntry->next=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
    
    attrCacheEntry=attrCacheEntry->next;
    // add the Attribute Cache entry to the linked list of listHead .*/
    // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
  }

  // set the relIdth entry of the AttrCacheTable to listHead.
  AttrCacheTable::attrCache[relId]=listHead;

  /****** Setting up metadata in the Open Relation Table for the relation******/
  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free=false;
  strcpy(tableMetaInfo[relId].relName, relName);

  return relId;
}


int OpenRelTable::closeRel(int relId) {
  if (relId==0 || relId==1) {
    return E_NOTPERMITTED;
  }

  if (relId<0 || relId>=MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free) {
    return E_RELNOTOPEN;
  }

  if (RelCacheTable::relCache[relId]->dirty){

    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), record);

    // recid field of relCache stores recordId of where the relation catalog entry of this relation
    // i.e this is where the relation catalog entry of relId relation is stored
    RecId recId=RelCacheTable::relCache[relId]->recId;
  
    // declaring an object of RecBuffer class to write back to the buffer
    RecBuffer relCatBlock(recId.block);

    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    relCatBlock.setRecord(record, recId.slot);
  }
  
  free(RelCacheTable::relCache[relId]);
  RelCacheTable::relCache[relId]=nullptr;

  /****** Releasing the Attribute Cache entry of the relation ******/
  // free the memory allocated in the attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  
  // (because we are not modifying the attribute cache at this stage,
  // write-back is not required. We will do it in subsequent
  // stages when it becomes needed)
  
  AttrCacheEntry* head=AttrCacheTable::attrCache[relId];
  AttrCacheEntry* temp=nullptr;
  // n stores number of attributes in relational catalog, Which is 6 ofcourse
  while(head!=nullptr){
    temp=head;
    head=head->next;
    free(temp);
  } 
  AttrCacheTable::attrCache[relId] = nullptr;

  /****** Set the Open Relation Table entry of the relation as free ******/
  // update `tableMetaInfo` to set `relId` as a free slot
  tableMetaInfo[relId].free=true;
  return SUCCESS;
}
