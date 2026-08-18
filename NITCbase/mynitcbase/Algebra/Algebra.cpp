#include "Algebra.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unordered_set>
using namespace std;

// will return if a string can be parsed as a floating point number
bool isNumber(char *str) {
  int len;
  float ignore;
  /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}

/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
  // get the srcRel's rel-id (let it be srcRelid), using OpenRelTable::getRelId()
  int srcRelId = OpenRelTable::getRelId(srcRel);
  // if srcRel is not open in open relation table, return E_RELNOTOPEN
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
  //    return E_ATTRNOTEXIST if it returns the error
  AttrCatEntry attrCatEntry;
  if(AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry)!=SUCCESS)
    return E_ATTRNOTEXIST;
  
  /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
  int type = attrCatEntry.attrType;
  Attribute attrVal;
  if (type == NUMBER) {
    if (isNumber(strVal)) {       // the isNumber() function is implemented below
      attrVal.nVal = atof(strVal);
    } else {
      return E_ATTRTYPEMISMATCH;
    }
  } else if (type == STRING) {
    strcpy(attrVal.sVal, strVal);
  }

  if(strcmp(targetRel, "null")==0 || strcmp(targetRel, "NULL")==0){
    /*** Selecting records from the source relation ***/
    // Before calling the search function, reset the search to start from the first hit
    RelCacheTable::resetSearchIndex(srcRelId);
    

    // get relCatEntry using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    /************************
    The following code prints the contents of a relation directly to the output
    console. Direct console output is not permitted by the actual the NITCbase
    specification and the output can only be inserted into a new relation. We will
    be modifying it in the later stages to match the specification.
    ************************/

    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; ++i) {
      AttrCatEntry attrCatEntry;
      // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
      printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");

    int cnt=0;
    while (true) {
      RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op, cnt);

      if (searchRes.block != -1 && searchRes.slot != -1) {
          // get the record at searchRes using BlockBuffer.getRecord
          RecBuffer recBuffer(searchRes.block);
          // print the attribute values in the same format as above
          Attribute record[relCatEntry.numAttrs];
          recBuffer.getRecord(record, searchRes.slot);

        // print the attribute values in the same format as above
          printf("|");
          for (int i = 0; i < relCatEntry.numAttrs; ++i) {
              AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
              if(attrCatEntry.attrType==STRING)
                  printf(" %s |", record[i].sVal);
                  else
                  printf(" %d |", (int)record[i].nVal);
          }
          printf("\n");

      } else {

        // (all records over)
        break;
      }
    }

    return SUCCESS;
  }


  /*** Creating and opening the target relation ***/
  // Prepare arguments for createRel() in the following way:
  // get RelcatEntry of srcRel using RelCacheTable::getRelCatEntry()
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
  /* the no. of attributes present in src relation */ 
  int src_nAttrs = relCatEntry.numAttrs;

  /* let attr_names[src_nAttrs][ATTR_SIZE] be a 2D array of type char
  (will store the attribute names of rel). */
  char attr_names[src_nAttrs][ATTR_SIZE];
  // let attr_types[src_nAttrs] be an array of type int
  int attr_types[src_nAttrs];

  // iterate through 0 to src_nAttrs-1 :
  for(int i=0;i<src_nAttrs;i++){
    // get the i'th attribute's AttrCatEntry using AttrCacheTable::getAttrCatEntry()
    AttrCatEntry tempAttrCatEntry;
    AttrCacheTable::getAttrCatEntry(srcRelId, i, &tempAttrCatEntry);
    // fill the attr_names, attr_types arrays that we declared with the entries
    // of corresponding attributes
    strcpy(attr_names[i], tempAttrCatEntry.attrName);
    attr_types[i]=tempAttrCatEntry.attrType;
  }

  // Create the relation for target relation by calling Schema::createRel()
  // by providing appropriate arguments
  int ret=Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
  // if the createRel returns an error code, then return that value.
  if(ret!=SUCCESS)
    return ret;

  /* Open the newly created target relation by calling OpenRelTable::openRel()
      method and store the target relid */
  int targetRelId=OpenRelTable::openRel(targetRel);
  /* If opening fails, delete the target relation by calling Schema::deleteRel()
      and return the error value returned from openRel() */
  if(targetRelId<0 || targetRelId>=MAX_OPEN){
    Schema::deleteRel(targetRel);
    return targetRelId;
  }

  /*** Selecting and inserting records into the target relation ***/
  /* Before calling the search function, reset the search to start from the
      first using RelCacheTable::resetSearchIndex() */
  Attribute record[src_nAttrs];

  /*
      The BlockAccess::search() function can either do a linearSearch or
      a B+ tree search. Hence, reset the search index of the relation in the
      relation cache using RelCacheTable::resetSearchIndex().
      Also, reset the search index in the attribute cache for the select
      condition attribute with name given by the argument `attr`. Use
      AttrCacheTable::resetSearchIndex().
      Both these calls are necessary to ensure that search begins from the
      first record.
  */
  RelCacheTable::resetSearchIndex(srcRelId);
  AttrCacheTable::resetSearchIndex(srcRelId, attr); // for b+ tree search
  int cnt=0;
  while(BlockAccess::search(srcRelId, record, attr, attrVal, op, cnt)==SUCCESS){
    ret = BlockAccess::insert(targetRelId, record);
    // if (insert fails) {
    if(ret!=SUCCESS){
      // close the targetrel(by calling Schema::closeRel(targetrel))
      Schema::closeRel(targetRel);
      // delete targetrel (by calling Schema::deleteRel(targetrel))
      Schema::deleteRel(targetRel);
      return ret;
    }
  }
  cout<<cnt<<endl;
  Schema::closeRel(targetRel);
  return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
  // record is an array of strings with each entry corresponding to noe record
  // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
  // return E_NOTPERMITTED;
  // cout<<atof(record[1])<<endl;
  if(strcmp(relName, RELCAT_RELNAME)==0 || strcmp(relName, RELCAT_RELNAME)==0)
    return E_NOTPERMITTED;

  // get the relation's rel-id using OpenRelTable::getRelId() method
  int relId = OpenRelTable::getRelId(relName);

  // if relation is not open in open relation table, return E_RELNOTOPEN
  // (check if the value returned from getRelId function call = E_RELNOTOPEN)
  if(relId == E_RELNOTOPEN) return E_RELNOTOPEN;
  
  // get the relation catalog entry from relation cache
  // (use RelCacheTable::getRelCatEntry() of Cache Layer)
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(relId, &relCatEntry);

  /* if relCatEntry.numAttrs != numberOfAttributes in relation,
      return E_NATTRMISMATCH */
  if(relCatEntry.numAttrs!=nAttrs)
    return E_NATTRMISMATCH;

  // let recordValues[numberOfAttributes] be an array of type union Attribute
  Attribute recordValues[nAttrs];
  AttrCatEntry attrCatEntry;
  
  /*
      Converting 2D char array of record values to Attribute array recordValues
    */
  // iterate through 0 to nAttrs-1: (let i be the iterator)
  for(int i=0;i<nAttrs;i++){
      // get the attr-cat entry for the i'th attribute from the attr-cache
      // (use AttrCacheTable::getAttrCatEntry())
      AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);
      int type=attrCatEntry.attrType;
      // let type = attrCatEntry.attrType;

      if (type == NUMBER){
          // if the char array record[i] can be converted to a number
          // (check this using isNumber() function)
          if(isNumber(record[i])){
            // cout<<i<<endl;
            recordValues[i].nVal=atof(record[i]);
            /* convert the char array to numeral and store it
                at recordValues[i].nVal using atof() */
          }
          else{
            return E_ATTRTYPEMISMATCH;
          }
      }
      else if (type == STRING)
        strcpy(recordValues[i].sVal,record[i]);    
      // copy record[i] to recordValues[i].sVal
  }
  // insert the record by calling BlockAccess::insert() function
  // let retVal denote the return value of insert call
  // cout<<recordValues[1].nVal<<endl;
  int retVal=BlockAccess::insert(relId, recordValues);

  return retVal;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {

  /*srcRel's rel-id (use OpenRelTable::getRelId() function)*/
  int srcRelId = OpenRelTable::getRelId(srcRel);

  // if srcRel is not open in open relation table, return E_RELNOTOPEN
  if(srcRelId==E_RELNOTOPEN)
    return E_RELNOTOPEN;

  if(strcmp(targetRel, "null")==0 || strcmp(targetRel, "NULL")==0){
    /*** Selecting records from the source relation ***/
    // Before calling the search function, reset the search to start from the first hit
    RelCacheTable::resetSearchIndex(srcRelId);
    

    // get relCatEntry using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    /************************
    The following code prints the contents of a relation directly to the output
    console. Direct console output is not permitted by the actual the NITCbase
    specification and the output can only be inserted into a new relation. We will
    be modifying it in the later stages to match the specification.
    ************************/
    
    int numAttrs=relCatEntry.numAttrs;
    printf("|");
    for (int i = 0; i < numAttrs; ++i) {
      AttrCatEntry attrCatEntry;
      // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
      printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");

    Attribute record[numAttrs];
    while (BlockAccess::project(srcRelId, record)==SUCCESS){
      // print the attribute values in the same format as above
      AttrCatEntry attrCatEntry;
      printf("|");
      for (int i = 0; i < numAttrs; ++i) {
          AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
          if(attrCatEntry.attrType==STRING)
              printf(" %s |", record[i].sVal);
              else
              printf(" %d |", (int)record[i].nVal);
      }
      printf("\n");
    }

    return SUCCESS;
  }

  // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

  // get the no. of attributes present in relation from the fetched RelCatEntry.
  int numAttrs=relCatEntry.numAttrs;

  // attrNames and attrTypes will be used to store the attribute names
  // and types of the source relation respectively
  char attrNames[numAttrs][ATTR_SIZE];
  int attrTypes[numAttrs];

  /*iterate through every attribute of the source relation :
      - get the AttributeCat entry of the attribute with offset.
        (using AttrCacheTable::getAttrCatEntry())
      - fill the arrays `attrNames` and `attrTypes` that we declared earlier
        with the data about each attribute
  */
  for(int i=0;i<numAttrs;i++){
    // get the i'th attribute's AttrCatEntry using AttrCacheTable::getAttrCatEntry()
    AttrCatEntry tempAttrCatEntry;
    AttrCacheTable::getAttrCatEntry(srcRelId, i, &tempAttrCatEntry);
    // fill the attr_names, attr_types arrays that we declared with the entries
    // of corresponding attributes
    strcpy(attrNames[i], tempAttrCatEntry.attrName);
    attrTypes[i]=tempAttrCatEntry.attrType;
  }

  /*** Creating and opening the target relation ***/

  // Create a relation for target relation by calling Schema::createRel()
  int ret=Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);
  
  // if the createRel returns an error code, then return that value.
  if(ret!=SUCCESS)
    return ret;
  cout<<targetRel<<" created succesfully"<<endl;

  // Open the newly created target relation by calling OpenRelTable::openRel()
  // and get the target relid
  int targetRelId=OpenRelTable::openRel(targetRel);
  // If opening fails, delete the target relation by calling Schema::deleteRel() of
  // return the error value returned from openRel().
  if(targetRelId<0 || targetRelId>=MAX_OPEN){
    Schema::deleteRel(targetRel);
    return targetRelId;
  }
  

  /*** Inserting projected records into the target relation ***/

  // Take care to reset the searchIndex before calling the project function
  // using RelCacheTable::resetSearchIndex()
  RelCacheTable::resetSearchIndex(srcRelId);
  Attribute record[numAttrs];

  while (BlockAccess::project(srcRelId, record)==SUCCESS){
    // record will contain the next record
    ret = BlockAccess::insert(targetRelId, record);

    if (ret!=SUCCESS) {
        // close the targetrel by calling Schema::closeRel()
        Schema::closeRel(targetRel);
        // delete targetrel by calling Schema::deleteRel()
        Schema::deleteRel(targetRel);
        return ret;
    }
  }

  // Close the targetRel by calling Schema::closeRel()
  Schema::closeRel(targetRel);
  return SUCCESS;
}

// CREATE TABLE Toys(id NUM, name STR, colour STR, stock NUM);
// INSERT INTO Toys VALUES FROM toys.csv;
// Open Table Toys;
// SELECT name,colour FROM Toys INTO ToyColours;

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
  /*srcRel's rel-id (use OpenRelTable::getRelId() function)*/
  int srcRelId = OpenRelTable::getRelId(srcRel);
  // if srcRel is not open in open relation table, return E_RELNOTOPEN
  if(srcRelId==E_RELNOTOPEN)
  return E_RELNOTOPEN;
  
  // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
  // cout<<"Got here"<<endl;

  // get the no. of attributes present in relation from the fetched RelCatEntry.
  int numAttrs=relCatEntry.numAttrs;

  // declare attr_offset[tar_nAttrs] an array of type int.
  // where i-th entry will store the offset in a record of srcRel for the
  // i-th attribute in the target relation.
  int attr_offset[tar_nAttrs];
  // let attr_types[tar_nAttrs] be an array of type int.
  // where i-th entry will store the type of the i-th attribute in the
  // target relation.
  int attr_types[tar_nAttrs];


  /*** Checking if attributes of target are present in the source relation
       and storing its offsets and types ***/

  // iterate through 0 to tar_nAttrs-1 :
  for(int i=0;i<tar_nAttrs;i++){
    // - get the attribute catalog entry of the attribute with name tar_attrs[i].
    AttrCatEntry tempAttrCatEntry;
    int ret=AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &tempAttrCatEntry);
    // - if the attribute is not found return E_ATTRNOTEXIST
    if(ret!=SUCCESS)
      return E_ATTRNOTEXIST;
    // - fill the attr_offset, attr_types arrays of target relation from the
    // corresponding attribute catalog entries of source relation
    attr_offset[i]=tempAttrCatEntry.offset;
    attr_types[i]=tempAttrCatEntry.attrType;
  }
  
  if(strcmp(targetRel, "null")==0 || strcmp(targetRel, "NULL")==0){
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[numAttrs];

    printf("|");
    for (int i = 0; i < tar_nAttrs; ++i) {
      printf(" %s |", tar_Attrs[i]);
    }
    printf("\n");

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
      // Attribute proj_record[tar_nAttrs];

      //iterate through 0 to tar_attrs-1:
      //    proj_record[attr_iter] = record[attr_offset[attr_iter]]
      // for(int i=0;i<tar_nAttrs;i++)
      //   proj_record[i]=record[attr_offset[i]];
      
      AttrCatEntry attrCatEntry;
      printf("|");
      for (int i = 0; i < tar_nAttrs; ++i) {
          AttrCacheTable::getAttrCatEntry(srcRelId, attr_offset[i], &attrCatEntry);
          if(attrCatEntry.attrType==STRING)
              printf(" %s |", record[attr_offset[i]].sVal);
              else
              printf(" %d |", (int)record[attr_offset[i]].nVal);
      }
      printf("\n");      
    }

    return SUCCESS;
  }

  /*** Creating and opening the target relation ***/
  // Create a relation for target relation by calling Schema::createRel()
  int ret=Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
  // if the createRel returns an error code, then return that value.
  if(ret!=SUCCESS)
    return ret;
    
  // Open the newly created target relation by calling OpenRelTable::openRel()
  // and get the target relid
  int targetRelId=OpenRelTable::openRel(targetRel);
  // If opening fails, delete the target relation by calling Schema::deleteRel()
  if(targetRelId<0 || targetRelId>=MAX_OPEN){
    Schema::deleteRel(targetRel);
    // and return the error value from openRel()
    return targetRelId;
  }


  /*** Inserting projected records into the target relation ***/

  // Take care to reset the searchIndex before calling the project function
  // using RelCacheTable::resetSearchIndex()
  RelCacheTable::resetSearchIndex(srcRelId);

  Attribute record[numAttrs];



  while (BlockAccess::project(srcRelId, record) == SUCCESS) {
      // the variable `record` will contain the next record

      Attribute proj_record[tar_nAttrs];

      //iterate through 0 to tar_attrs-1:
      //    proj_record[attr_iter] = record[attr_offset[attr_iter]]
      for(int i=0;i<tar_nAttrs;i++)
        proj_record[i]=record[attr_offset[i]];
      
      ret = BlockAccess::insert(targetRelId, proj_record);

      if (ret!=SUCCESS) {
          // close the targetrel by calling Schema::closeRel()
          Schema::closeRel(targetRel);
          // delete targetrel by calling Schema::deleteRel()
          Schema::deleteRel(targetRel);
          return ret;
      }
  }

  // Close the targetRel by calling Schema::closeRel()
  Schema::closeRel(targetRel);
  return SUCCESS;
}

int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) {

  // get the srcRelation1's rel-id using OpenRelTable::getRelId() method
  int srcRelId1=OpenRelTable::getRelId(srcRelation1);
  
  // get the srcRelation2's rel-id using OpenRelTable::getRelId() method
  int srcRelId2=OpenRelTable::getRelId(srcRelation2);

  // if either of the two source relations is not open
  //     return E_RELNOTOPEN
  if(srcRelId1==E_RELNOTOPEN || srcRelId2==E_RELNOTOPEN)
    return E_RELNOTOPEN;

  AttrCatEntry attrCatEntry1, attrCatEntry2;
  // get the attribute catalog entries for the following from the attribute cache
  // (using AttrCacheTable::getAttrCatEntry())
  // - attrCatEntry1 = attribute1 of srcRelation1
  // - attrCatEntry2 = attribute2 of srcRelation2
  int ret1=AttrCacheTable::getAttrCatEntry(srcRelId1, attribute1, &attrCatEntry1);
  int ret2=AttrCacheTable::getAttrCatEntry(srcRelId2, attribute2, &attrCatEntry2);

  // if attribute1 is not present in srcRelation1 or attribute2 is not
  // present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
  //     return E_ATTRNOTEXIST.
  if(ret1==E_ATTRNOTEXIST || ret2==E_ATTRNOTEXIST)
    return E_ATTRNOTEXIST;

  // if attribute1 and attribute2 are of different types return E_ATTRTYPEMISMATCH
  if(attrCatEntry1.attrType!=attrCatEntry2.attrType)
    return E_ATTRTYPEMISMATCH;

  // get the relation catalog entries for the relations from the relation cache
  // (use RelCacheTable::getRelCatEntry() function)
  RelCatEntry relCatEntry1, relCatEntry2;
  RelCacheTable::getRelCatEntry(srcRelId1, &relCatEntry1);
  RelCacheTable::getRelCatEntry(srcRelId2, &relCatEntry2);
  int numOfAttributes1 = relCatEntry1.numAttrs/* number of attributes in srcRelation1 */;
  int numOfAttributes2 = relCatEntry2.numAttrs/* number of attributes in srcRelation2 */;
  
  // iterate through all the attributes in both the source relations and check if
  // there are any other pair of attributes other than join attributes
  // (i.e. attribute1 and attribute2) with duplicate names in srcRelation1 and
  // srcRelation2 (use AttrCacheTable::getAttrCatEntry())
  // If yes, return E_DUPLICATEATTR
  int ret;
  unordered_set<string> s;
  for (int i = 0; i < numOfAttributes1; i++) {
      AttrCatEntry tempEntry;
      ret=AttrCacheTable::getAttrCatEntry(srcRelId1, i, &tempEntry);
      if(ret!=SUCCESS)
        return ret;
      s.insert(tempEntry.attrName);
  }

  for (int i = 0; i < numOfAttributes2; i++) {
    AttrCatEntry tempEntry;
    ret=AttrCacheTable::getAttrCatEntry(srcRelId2, i, &tempEntry);
    if(ret!=SUCCESS)
      return ret;
    if (s.count(tempEntry.attrName)) {
        if (!(strcmp(tempEntry.attrName, attribute1) == 0 &&
              strcmp(tempEntry.attrName, attribute2) == 0))
            return E_DUPLICATEATTR;
    }
  }

  // if rel2 does not have an index on attr2
  // create it using BPlusTree:bPlusCreate()
  // if call fails, return the appropriate error code
  // (if your implementation is correct, the only error code that will
  // be returned here is E_DISKFULL)
  if(attrCatEntry2.rootBlock==-1){
    ret=BPlusTree::bPlusCreate(srcRelId2, attribute2);
    if(ret!=SUCCESS)
      return ret;
  }

  int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
  // Note: The target relation has number of attributes one less than
  // nAttrs1+nAttrs2 since join attribute is only replicated once

  // declare the following arrays to store the details of the target relation
  char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
  int targetRelAttrTypes[numOfAttributesInTarget];

  // iterate through all the attributes in both the source relations and
  // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
  // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
  for(int i=0;i<numOfAttributes1;i++){
    AttrCatEntry tempEntry;
    AttrCacheTable::getAttrCatEntry(srcRelId1, i, &tempEntry);
    strcpy(targetRelAttrNames[i], tempEntry.attrName);
    targetRelAttrTypes[i]=tempEntry.attrType;
  }
  int idx=numOfAttributes1;
  for(int i=0;i<numOfAttributes2;i++){
    AttrCatEntry tempEntry;
    AttrCacheTable::getAttrCatEntry(srcRelId2, i, &tempEntry);
    if(strcmp(tempEntry.attrName, attribute2)==0) continue;
    strcpy(targetRelAttrNames[idx], tempEntry.attrName);
    targetRelAttrTypes[idx++]=tempEntry.attrType;
  }

  // create the target relation using the Schema::createRel() function
  ret=Schema::createRel(targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes);

  // if createRel() returns an error, return that error
  if(ret!=SUCCESS)
    return ret;

  // cout<<targetRelation<<" created succesfully"<<endl;

  // Open the targetRelation using OpenRelTable::openRel()
  int targetRelId=OpenRelTable::openRel(targetRelation);

  // if openRel() fails (No free entries left in the Open Relation Table)
  if(targetRelId<0 || targetRelId>=MAX_OPEN){
    // delete target relation by calling Schema::deleteRel()
    Schema::deleteRel(targetRelation);
    // return the error code
    return targetRelId;
  }

  Attribute record1[numOfAttributes1];
  Attribute record2[numOfAttributes2];
  Attribute targetRecord[numOfAttributesInTarget];

  RelCacheTable::resetSearchIndex(srcRelId1);
  // this loop is to get every record of the srcRelation1 one by one
  while (BlockAccess::project(srcRelId1, record1) == SUCCESS) {

      // reset the search index of `srcRelation2` in the relation cache
      // using RelCacheTable::resetSearchIndex()
      RelCacheTable::resetSearchIndex(srcRelId2);

      // reset the search index of `attribute2` in the attribute cache
      // using AttrCacheTable::resetSearchIndex()
      AttrCacheTable::resetSearchIndex(srcRelId2, attribute2);

      // this loop is to get every record of the srcRelation2 which satisfies
      //the following condition:
      // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
      for(int i=0;i<numOfAttributes1;i++){
        targetRecord[i]=record1[i];
      }
      int cnt=0;
      while (BlockAccess::search(srcRelId2, record2, attribute2, record1[attrCatEntry1.offset], EQ, cnt) == SUCCESS ) {

          // copy srcRelation1's and srcRelation2's attribute values(except
          // for attribute2 in rel2) from record1 and record2 to targetRecord
          idx=numOfAttributes1;
          for(int i=0;i<numOfAttributes2;i++){
            if(i==attrCatEntry2.offset) continue;
            targetRecord[idx++]=record2[i];
          }

          // insert the current record into the target relation by calling
          // BlockAccess::insert()
          ret=BlockAccess::insert(targetRelId, targetRecord);

          if(ret!=SUCCESS/* insert fails (insert should fail only due to DISK being FULL) */) {

              // close the target relation by calling OpenRelTable::closeRel()
              OpenRelTable::closeRel(targetRelId);
              // delete targetRelation (by calling Schema::deleteRel())
              Schema::deleteRel(targetRelation);
              return E_DISKFULL;
          }
      }
  }

  // close the target relation by calling OpenRelTable::closeRel()
  OpenRelTable::closeRel(targetRelId);
  return SUCCESS;
}