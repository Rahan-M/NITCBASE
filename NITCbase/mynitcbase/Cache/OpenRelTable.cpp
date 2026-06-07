#include "OpenRelTable.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>
using namespace std;

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  /************ 1. Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** 1.a setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK); // for operations on relational catalog block
  Attribute relCatRecord[RELCAT_NO_ATTRS]; // for storing records of relational catalog
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT); // fetch relational catalog entry for relational catalog :)

  struct RelCacheEntry relCacheEntry; // what we will store in relational cache soon
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry); // transfer what's present as record to the relatonal cache entry
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** 1.b setting up Attribute Catalog relation in the Relation Cache Table ****/
  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block=RELCAT_BLOCK;
  relCacheEntry.recId.slot=RELCAT_SLOTNUM_FOR_ATTRCAT;

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


  // 1.c Setting up student
  HeadInfo relCatHeader;
  relCatBlock.getHeader(&relCatHeader);
  int slot=-1;
  int numRels=relCatHeader.numEntries;
  for(int i=0;i<numRels;i++){
    // Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog
    relCatBlock.getRecord(relCatRecord, i);
    if(strcmp("Students", relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0){
      slot=i;
      break;
    }
  }

  if(slot!=-1){
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block=RELCAT_BLOCK;
    relCacheEntry.recId.slot=slot;
    RelCacheTable::relCache[ATTRCAT_RELID+1] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID+1]) = relCacheEntry;
  }else cout<<"\n\n\nRelation NOT FOUND\n\n\n\n";


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

  // 2.c Student STAGE 3 EXERCICE
  if(slot!=-1){
    head=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
    temp=head; 
    int numAttrs=RelCacheTable::relCache[ATTRCAT_RELID+1]->relCatEntry.numAttrs; 
    for(int i=12;i<12+numAttrs;i++){
      // iterate through all the attributes of the relation catalog and create a linked
      attrCatBlock.getRecord(attrCatRecord, i); // slots 6-11 are for attribute catalog attributes
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &temp->attrCatEntry);  
      temp->recId.block=ATTRCAT_BLOCK;
      temp->recId.slot=i;

      if(i<12+numAttrs-1)
        temp->next=(AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
      else
        temp->next=nullptr;
      temp=temp->next;
    } 
    AttrCacheTable::attrCache[ATTRCAT_RELID+1] = head /* head of the linked list */;
  }
}

OpenRelTable::~OpenRelTable() {
    // free all the memory that you allocated in the constructor
    free(RelCacheTable::relCache[RELCAT_RELID]);
    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    int m=RelCacheTable::relCache[RELCAT_RELID]->relCatEntry.numAttrs; 
    for(int i=0;i<=1;i++){
        AttrCacheEntry* head=AttrCacheTable::attrCache[i];
        AttrCacheEntry* temp=nullptr;
        // n stores number of attributes in relational catalog, Which is 6 ofcourse
        while(head!=nullptr){
            temp=head;
            head=head->next;
            free(temp);
        } 
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  if(strcmp(relName ,RELCAT_RELNAME)==0) return RELCAT_RELID;
  if(strcmp(relName ,ATTRCAT_RELNAME)==0) return ATTRCAT_RELID;
  if(strcmp(relName ,"Students")==0) return ATTRCAT_RELID+1;

  return E_RELNOTOPEN;
}