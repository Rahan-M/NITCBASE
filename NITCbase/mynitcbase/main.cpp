#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>
using namespace std;

void displayRelations(){
    // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);

  // create header for relation catalog
  HeadInfo relCatHeader;
  
  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);

  // Disk functions expects 2 arguments, 1 buffer of size 2048 bytes and block number

  for(int i=0;i<relCatHeader.numEntries;i++){
    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int currAttrCatBlock=ATTRCAT_BLOCK;
    while(currAttrCatBlock!=-1){
      RecBuffer attrCatBuffer(currAttrCatBlock);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);
  
      // now we retrieve all atributes of all relations and print only if relation is ours
      for (int j=0;j<attrCatHeader.numEntries;j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);
        // declare attrCatRecord and load the attribute catalog entry into it
  
        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0) {
          const char *attrName = attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal;
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrName, attrType);
        }
      }
      currAttrCatBlock=attrCatHeader.rblock;
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
  // displayRelations();
  return FrontendInterface::handleFrontend(argc, argv);
}