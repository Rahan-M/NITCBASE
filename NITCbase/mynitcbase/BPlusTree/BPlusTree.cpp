#include "BPlusTree.h"
#include<iostream>
#include <cstring>
using namespace std;

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // declare searchIndex which will be used to store search index for attrName.
    /* get the search index corresponding to attribute with name attrName
       using AttrCacheTable::getSearchIndex(). */
    // cout<<"Called"<<endl;
    IndexId searchIndex;
    int ret=AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    /* load the attribute cache entry into attrCatEntry using
     AttrCacheTable::getAttrCatEntry(). */
    ret=AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // declare variables block and index which will be used during search
    int block, index;

    if (searchIndex.block == -1 && searchIndex.index==-1) {
        // (search is done for the first time)
        // start the search from the first entry of root.
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block==-1) {
            // no index on attrName
            return RecId{-1, -1};
        }

    } else {
        /*a valid searchIndex points to an entry in the leaf index of the attribute's
        B+ Tree which had previously satisfied the op for the given attrVal.*/

        block = searchIndex.block;
        index = searchIndex.index + 1;  // search is resumed from the next index.

        // load block into leaf using IndLeaf::IndLeaf().
        IndLeaf leaf(block);

        // declare leafHead which will be used to hold the header of leaf.
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        // load header into leafHead using BlockBuffer::getHeader().

        if (index >= leafHead.numEntries) {
            /* (all the entries in the block has been searched; search from the
            beginning of the next leaf index block. */

            // update block to rblock of current block and index to 0.
            block=leafHead.rblock;
            index=0;
            if (block == -1) {
                // (end of linked list reached - the search is done.)
                return RecId{-1, -1};
            }
        }
    }

    /******  Traverse through all the internal nodes according to value
             of attrVal and the operator op                             ******/

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */

    while(StaticBuffer::getStaticBlockType(block)==IND_INTERNAL) {  

        // load the block into internalBlk using IndInternal::IndInternal().
        IndInternal internalBlk(block);

        HeadInfo intHead;
        // load the header of internalBlk into intHead using BlockBuffer::getHeader()
        internalBlk.getHeader(&intHead);
        // declare intEntry which will be used to store an entry of internalBlk.
        InternalEntry intEntry;

        if ( op==NE || op==LT || op==LE) { /* op is one of NE, LT, LE */
            /*
            - NE: need to search the entire linked list of leaf indices of the B+ Tree,
            starting from the leftmost leaf index. Thus, always move to the left.

            - LT and LE: the attribute values are arranged in ascending order in the
            leaf indices of the B+ Tree. Values that satisfy these conditions, if
            any exist, will always be found in the left-most leaf index. Thus,
            always move to the left.
            */

            // load entry in the first slot of the block into intEntry
            // using IndInternal::getEntry().
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        } else {
            /*
            - EQ, GT and GE: move to the left child of the first entry that is
            greater than (or equal to) attrVal
            (we are trying to find the first entry that satisfies the condition.
            since the values are in ascending order we move to the left child which
            might contain more entries that satisfy the condition)
            */
            int idx=0;
            while (idx<intHead.numEntries){
                internalBlk.getEntry(&intEntry, idx);
                if(op==GT){
                    if(compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType)>0)
                        break;
                }else if(op==EQ || op==GE){ // EQ OR GE
                    if(compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType)>=0)
                        break;
                }
                idx++;
            }
            
            /*
             traverse through all entries of internalBlk and find an entry that
             satisfies the condition.
             if op == EQ or GE, then intEntry.attrVal >= attrVal
             if op == GT, then intEntry.attrVal > attrVal
             Hint: the helper function compareAttrs() can be used for comparing
            */

            if (idx<intHead.numEntries) {
                // move to the left child of that entry
                block =  intEntry.lChild;

            } else {
                // move to the right child of the last entry of the block
                // i.e numEntries - 1 th entry of the block
                block =  intEntry.rChild;// right child of last entry
            }
        }
    }

    // NOTE: `block` now has the block number of a leaf index block.

    /******  Identify the first leaf index entry from the current position
                that satisfies our condition (moving right)             ******/

    while (block != -1) {
        // load the block into leafBlk using IndLeaf::IndLeaf().
        IndLeaf leafBlk(block);
        HeadInfo leafHead;

        // load the header to leafHead using BlockBuffer::getHeader().
        leafBlk.getHeader(&leafHead);

        // declare leafEntry which will be used to store an entry from leafBlk
        Index leafEntry;

        while (index<leafHead.numEntries /*index < numEntries in leafBlk*/) {

            // load entry corresponding to block and index into leafEntry
            // using IndLeaf::getEntry().
            leafBlk.getEntry(&leafEntry, index);

            int cmpVal =compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);
            /* comparison between leafEntry's attribute value and input attrVal using compareAttrs()*/
            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)
            ) {
                // (entry satisfying the condition found)

                // set search index to {block, index}
                searchIndex.block=block;
                searchIndex.index=index;
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                // return the recId {leafEntry.block, leafEntry.slot}.
                return RecId{leafEntry.block, leafEntry.slot};

            } else if ((op == EQ || op == LE || op == LT) && cmpVal > 0) {
                /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */

                // return RecId {-1, -1};
                return RecId{-1, -1};
            }

            // search next index.
            ++index;
        }

        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisying that op. */
        if (op != NE) {
            break;
        }

        // block = next block in the linked list, i.e., the rblock in leafHead.
        // update index to 0.
        block=leafHead.rblock;
        index=0;
    }

    // no entry satisying the op was found; return the recId {-1,-1}
    return {-1, -1};
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]) {

    // if relId is either RELCAT_RELID or ATTRCAT_RELID:
    //     return E_NOTPERMITTED;
    if(relId==RELCAT_RELID || relId==ATTRCAT_RELID)
        return E_NOTPERMITTED;

    // get the attribute catalog entry of attribute `attrName`
    // using AttrCacheTable::getAttrCatEntry()
    AttrCatEntry attrCatEntry;
    int ret=AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // if getAttrCatEntry fails
    //     return the error code from getAttrCatEntry
    if(ret!=SUCCESS)
        return ret;

    if (attrCatEntry.rootBlock!=-1) {
        return SUCCESS;
    }

    /******Creating a new B+ Tree ******/

    // get a free leaf block using constructor 1 to allocate a new block
    IndLeaf rootBlockBuf; // invokes IndLeaf()

    // (if the block could not be allocated, the appropriate error code
    //  will be stored in the blockNum member field of the object)

    // declare rootBlock to store the blockNumber of the new leaf block
    int rootBlock = rootBlockBuf.getBlockNum();

    
    // if there is no more disk space for creating an index
    if (rootBlock == E_DISKFULL) {
        return E_DISKFULL;
    }
    
    // Update the rootBlock in attribute cache BEFORE inserting records
    attrCatEntry.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

    RelCatEntry relCatEntry;

    // load the relation catalog entry into relCatEntry
    // using RelCacheTable::getRelCatEntry().
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;/* first record block of the relation */

    /***** Traverse all the blocks in the relation and insert them one
           by one into the B+ Tree *****/
    while (block != -1) {

        // declare a RecBuffer object for `block` (using appropriate constructor)
        RecBuffer recBuffer(block);
        unsigned char slotMap[relCatEntry.numSlotsPerBlk];

        // load the slot map into slotMap using RecBuffer::getSlotMap().
        recBuffer.getSlotMap(slotMap);
        // for every occupied slot of the block
        for(int slot=0;slot<relCatEntry.numSlotsPerBlk;slot++){
            if(slotMap[slot]==SLOT_UNOCCUPIED) continue;

            Attribute record[relCatEntry.numAttrs];
            // load the record corresponding to the slot into `record`
            // using RecBuffer::getRecord().
            recBuffer.getRecord(record, slot);

            // declare recId and store the rec-id of this record in it
            RecId recId{block, slot};

            // insert the attribute value corresponding to attrName from the record
            // into the B+ tree using bPlusInsert.
            // (note that bPlusInsert will destroy any existing bplus tree if
            // insert fails i.e when disk is full)
            int retVal = bPlusInsert(relId, attrName, record[attrCatEntry.offset], recId);

            if (retVal == E_DISKFULL) {
                // (unable to get enough blocks to build the B+ Tree.)
                return E_DISKFULL;
            }
        }

        // get the header of the block using BlockBuffer::getHeader()
        HeadInfo headInfo;
        recBuffer.getHeader(&headInfo);

        // set block = rblock of current block (from the header)
        block=headInfo.rblock;
    }
    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum) {
    if (rootBlockNum<0 || rootBlockNum>=DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    int type =StaticBuffer::getStaticBlockType(rootBlockNum); /* type of block (using StaticBuffer::getStaticBlockType())*/

    if (type == IND_LEAF) {
        // declare an instance of IndLeaf for rootBlockNum using appropriate
        // constructor
        IndLeaf indLeaf(rootBlockNum);
        // release the block using BlockBuffer::releaseBlock().
        indLeaf.releaseBlock();
        return SUCCESS;

    } else if (type == IND_INTERNAL) {
        // declare an instance of IndInternal for rootBlockNum using appropriate
        // constructor
        IndInternal indInternal(rootBlockNum);
        // load the header of the block using BlockBuffer::getHeader().
        HeadInfo headInfo;
        indInternal.getHeader(&headInfo);

        /*iterate through all the entries of the internalBlk and destroy the lChild
        of the first entry and rChild of all entries using BPlusTree::bPlusDestroy().
        (the rchild of an entry is the same as the lchild of the next entry.
        take care not to delete overlapping children more than once ) */
        
        InternalEntry internalEntry;
        indInternal.getEntry(&internalEntry, 0);

        bPlusDestroy(internalEntry.lChild);
        
        int n=headInfo.numEntries;
        for(int i=0;i<n;i++){
            indInternal.getEntry(&internalEntry, i);
            bPlusDestroy(internalEntry.rChild);
        }
        
        // release the block using BlockBuffer::releaseBlock().
        indInternal.releaseBlock();
        return SUCCESS;

    } else {
        // (block is not an index block.)
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry attrCatEntry;
    int ret=AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    
    // if getAttrCatEntry() failed
    //     return the error code
    if(ret!=SUCCESS)
        return ret;

    int blockNum = attrCatEntry.rootBlock;
    /* rootBlock of B+ Tree (from attrCatEntry) */
    
    if (blockNum==-1) {
        return E_NOINDEX;
    }

    // find the leaf block to which insertion is to be done using the
    // findLeafToInsert() function

    int leafBlkNum = findLeafToInsert(blockNum, attrVal, attrCatEntry.attrType);

    // insert the attrVal and recId to the leaf block at blockNum using the
    // insertIntoLeaf() function.
    // declare a struct Index with attrVal = attrVal, block = recId.block and
    // slot = recId.slot to pass as argument to the function.
    Index index;
    index.attrVal=attrVal;
    index.block=recId.block;
    index.slot=recId.slot;
    ret= insertIntoLeaf(relId, attrName, leafBlkNum, index);
    // NOTE: the insertIntoLeaf() function will propagate the insertion to the
    //       required internal nodes by calling the required helper functions
    //       like insertIntoInternal() or createNewRoot()

    if (ret==E_DISKFULL) {
        // destroy the existing B+ tree by passing the rootBlock to bPlusDestroy().
        bPlusDestroy(blockNum);
        // update the rootBlock of attribute catalog cache entry to -1 using
        attrCatEntry.rootBlock=-1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType) {
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum)!=IND_LEAF){  // use StaticBuffer::getStaticBlockType()
        // declare an IndInternal object for block using appropriate constructor
        IndInternal indInternal(blockNum);

        // get header of the block using BlockBuffer::getHeader()
        HeadInfo headInfo;
        indInternal.getHeader(&headInfo);

        /* 
            iterate through all the entries, to find the first entry whose
            attribute value >= value to be inserted.
            NOTE: the helper function compareAttrs() declared in BlockBuffer.h
            can be used to compare two Attribute values. 
        */
        int n=headInfo.numEntries, idx=-1;
        InternalEntry entry;
        for(int i=0;i<n;i++){
            indInternal.getEntry(&entry, i);
            if(compareAttrs(attrVal, entry.attrVal, attrType)<=0){
                idx=i;
                break;
            }
        }

        if (idx==-1/*no such entry is found*/) {
            // set blockNum = rChild of (nEntries-1)'th entry of the block
            // (i.e. rightmost child of the block)
            InternalEntry lastEntry;
            indInternal.getEntry(&lastEntry, headInfo.numEntries - 1);
            blockNum = lastEntry.rChild;
        } else {
            // set blockNum = lChild of the entry that was found
            blockNum=entry.lChild;
        }
    }

    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // declare an IndLeaf instance for the block using appropriate constructor
    IndLeaf indLeaf(blockNum);
    HeadInfo blockHeader;
    // store the header of the leaf index block into blockHeader
    // using BlockBuffer::getHeader()
    indLeaf.getHeader(&blockHeader);

    // the following variable will be used to store a list of index entries with
    // existing indices + the new index to insert
    Index indices[blockHeader.numEntries + 1];

    /*
        Iterate through all the entries in the block and copy them to the array indices.
        Also insert `indexEntry` at appropriate position in th[e indices array maintaining
        the ascending order.
        - use IndLeaf::getEntry() to get the entry
        - use compareAttrs() declared in BlockBuffer.h to compare two Attribute structs
    */
    int n=blockHeader.numEntries, idx=-1;
    for(int i=0;i<n;i++){
        Index tempEntry;
        indLeaf.getEntry(&tempEntry, i);
        if(compareAttrs(tempEntry.attrVal, indexEntry.attrVal, attrCatEntry.attrType)<=0){
            indices[i]=tempEntry;
        }else{
            indices[i]=indexEntry;
            idx=i;
            for(int j=i+1;j<=n;j++){
                Index tempEntry;
                indLeaf.getEntry(&tempEntry, j-1);
                indices[j]=tempEntry;
            }
            break;
        }
    }
    if(idx==-1){
        idx=n;
        indices[n]=indexEntry;
    }

    if(blockHeader.numEntries != MAX_KEYS_LEAF){
        // (leaf block has not reached max limit)

        // increment blockHeader.numEntries and update the header of block
        blockHeader.numEntries++;
        // using BlockBuffer::setHeader().
        indLeaf.setHeader(&blockHeader);

        // iterate through all the entries of the array `indices` and populate the
        // entries of block with them using IndLeaf::setEntry().

        for(int i=0;i<=n;i++)
            indLeaf.setEntry(indices+i, i);

        return SUCCESS;
    }

    // If we reached here, the `indices` array has more than entries than can fit
    // in a single leaf index block. Therefore, we will need to split the entries
    // in `indices` between two leaf blocks. We do this using the splitLeaf() function.
    // This function will return the blockNum of the newly allocated block or
    // E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = splitLeaf(blockNum, indices);

    // if splitLeaf() returned E_DISKFULL
    //     return E_DISKFULL
    if(newRightBlk==E_DISKFULL)
        return E_DISKFULL;

    int ret;
    if (blockHeader.pblock!=-1) {  // check pblock in header
        // insert the middle value from `indices` into the parent block using the
        // insertIntoInternal() function. (i.e the last value of the left block) 
        
        // the middle value will be at index 31 (given by constant MIDDLE_INDEX_LEAF)
        
        // create a struct InternalEntry with attrVal = indices[MIDDLE_INDEX_LEAF].attrVal,
        InternalEntry middleEntry;
        middleEntry.attrVal=indices[MIDDLE_INDEX_LEAF].attrVal;
        
        // lChild = currentBlock, rChild = newRightBlk and pass it as argument to
        // the insertIntoInternalFunction as follows
        middleEntry.lChild=blockNum;
        middleEntry.rChild=newRightBlk;
        
        // insertIntoInternal(relId, attrName, parent of current block, new internal entry)
        ret= insertIntoInternal(relId, attrName, blockHeader.pblock, middleEntry);    
    }
    else{
        // the current block was the root block and is now split. a new internal index
        // block needs to be allocated and made the root of the tree.
        // To do this, call the createNewRoot() function with the following arguments

        ret= createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal, blockNum, newRightBlk);
    }

    // if either of the above calls returned an error (E_DISKFULL), then return that
    // else return SUCCESS
    if(ret==E_DISKFULL) return ret;
    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) {
    // declare rightBlk, an instance of IndLeaf using constructor 1 to obtain new
    // leaf index block that will be used as the right block in the splitting
    IndLeaf rLeaf;

    // declare leftBlk, an instance of IndLeaf using constructor 2 to read from
    // the existing leaf block
    IndLeaf lLeaf(leafBlockNum);

    int rightBlkNum = rLeaf.getBlockNum()/* block num of right blk */;
    int leftBlkNum = lLeaf.getBlockNum()/* block num of left blk */;

    if (rightBlkNum==E_DISKFULL) {
        //(failed to obtain a new leaf index block because the disk is full)
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using BlockBuffer::getHeader()
    lLeaf.getHeader(&leftBlkHeader);
    rLeaf.getHeader(&rightBlkHeader);

    // set rightBlkHeader with the following values
    rightBlkHeader.blockType=IND_LEAF;
    // - number of entries = (MAX_KEYS_LEAF+1)/2 = 32,
    rightBlkHeader.numEntries=(MAX_KEYS_LEAF+1)/2;
    // - pblock = pblock of leftBlk
    rightBlkHeader.pblock=leftBlkHeader.pblock;
    // - lblock = leftBlkNum
    rightBlkHeader.lblock=leftBlkNum;
    // - rblock = rblock of leftBlk
    rightBlkHeader.rblock=leftBlkHeader.rblock;
    // and update the header of rightBlk using BlockBuffer::setHeader()
    rLeaf.setHeader(&rightBlkHeader);
    
    // set leftBlkHeader with the following values
    // - number of entries = (MAX_KEYS_LEAF+1)/2 = 32
    leftBlkHeader.numEntries=(MAX_KEYS_LEAF+1)/2;
    // - rblock = rightBlkNum
    leftBlkHeader.rblock=rightBlkNum;
    // and update the header of leftBlk using BlockBuffer::setHeader() */
    lLeaf.setHeader(&leftBlkHeader);

    // set the first 32 entries of leftBlk = the first 32 entries of indices array
    // and set the first 32 entries of newRightBlk = the next 32 entries of
    // indices array using IndLeaf::setEntry().
    for(int i=0;i<=MIDDLE_INDEX_LEAF;i++){
        lLeaf.setEntry(indices+i, i);
        rLeaf.setEntry(indices+i+MIDDLE_INDEX_LEAF+1, i);
    }

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // declare intBlk, an instance of IndInternal using constructor 2 for the block
    // corresponding to intBlockNum
    IndInternal intBlk(intBlockNum);
    HeadInfo blockHeader;

    // load blockHeader with header of intBlk using BlockBuffer::getHeader().
    intBlk.getHeader(&blockHeader);
    
    // declare internalEntries to store all existing entries + the new entry
    InternalEntry internalEntries[blockHeader.numEntries + 1];

    /*
    Iterate through all the entries in the block and copy them to the array
    `internalEntries`. Insert `indexEntry` at appropriate position in the
    array maintaining the ascending order.
        - use IndInternal::getEntry() to get the entry
        - use compareAttrs() to compare two structs of type Attribute

    Update the lChild of the internalEntry immediately following the newly added
    entry to the rChild of the newly added entry.
    */

    int n=blockHeader.numEntries, idx=-1;
    for(int i=0;i<n;i++){
        InternalEntry tempEntry;
        intBlk.getEntry(&tempEntry, i);
        if(compareAttrs(tempEntry.attrVal, intEntry.attrVal, attrCatEntry.attrType)<=0){
            internalEntries[i]=tempEntry;
        }else{
            idx=i;
            internalEntries[i]=intEntry;
            for(int j=i+1;j<=n;j++){
                InternalEntry tempEntry;
                intBlk.getEntry(&tempEntry, j-1);
                internalEntries[j]=tempEntry;
            }
            break;
        }
    }

    if(idx==-1){
        idx=n;
        internalEntries[n]=intEntry;
    }

    if(idx>0)
        internalEntries[idx-1].rChild=intEntry.lChild;
    
    if(idx<n)
        internalEntries[idx+1].lChild=intEntry.rChild;

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL) {
        // (internal index block has not reached max limit)

        // increment blockheader.numEntries and update the header of intBlk
        // using BlockBuffer::setHeader().
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);

        // iterate through all entries in internalEntries array and populate the
        // entries of intBlk with them using IndInternal::setEntry().
        for(int i=0;i<=n;i++){
            intBlk.setEntry(internalEntries+i, i);
        }

        return SUCCESS;
    }

    // If we reached here, the `internalEntries` array has more than entries than
    // can fit in a single internal index block. Therefore, we will need to split
    // the entries in `internalEntries` between two internal index blocks. We do
    // this using the splitInternal() function.
    // This function will return the blockNum of the newly allocated block or
    // E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk==E_DISKFULL) {

        // Using bPlusDestroy(), destroy the right subtree, rooted at intEntry.rChild.
        // This corresponds to the tree built up till now that has not yet been
        // connected to the existing B+ Tree
        bPlusDestroy(intEntry.rChild);
        return E_DISKFULL;
    }

    int ret;
    if (blockHeader.pblock!=-1/* the current block was not the root */) {  // (check pblock in header)
        // insert the middle value from `internalEntries` into the parent block
        // using the insertIntoInternal() function (recursively).

        // the middle value will be at index 50 (given by constant MIDDLE_INDEX_INTERNAL)

        // create a struct InternalEntry with lChild = current block, rChild = newRightBlk
        InternalEntry parentEntry;
        parentEntry.lChild=intBlockNum; 
        parentEntry.rChild=newRightBlk;

        // and attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal
        parentEntry.attrVal=internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;
        // and pass it as argument to the insertIntoInternalFunction as follows

        ret=insertIntoInternal(relId, attrName, blockHeader.pblock, parentEntry);

    } else {
        // the current block was the root block and is now split. a new internal index
        // block needs to be allocated and made the root of the tree.
        // To do this, call the createNewRoot() function with the following arguments

        ret=createNewRoot(relId, attrName, internalEntries[MIDDLE_INDEX_INTERNAL].attrVal, intBlockNum, newRightBlk);
    }

    // if either of the above calls returned an error (E_DISKFULL), then return that
    if(ret==E_DISKFULL) return ret;
    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) {
    // declare rightBlk, an instance of IndInternal using constructor 1 to obtain new
    // internal index block that will be used as the right block in the splitting
    IndInternal rightBlk;

    // declare leftBlk, an instance of IndInternal using constructor 2 to read from
    // the existing internal index block
    IndInternal leftBlk(intBlockNum);

    int rightBlkNum = rightBlk.getBlockNum()/* block num of right blk */;
    int leftBlkNum = leftBlk.getBlockNum()/* block num of left blk */;

    if (rightBlkNum==E_DISKFULL/* newly allocated block has blockNum E_DISKFULL */) {
        //(failed to obtain a new internal index block because the disk is full)
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using BlockBuffer::getHeader()
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    // set rightBlkHeader with the following values
    // - number of entries = (MAX_KEYS_INTERNAL)/2 = 50
    rightBlkHeader.numEntries=MAX_KEYS_INTERNAL/2;
    // - pblock = pblock of leftBlk
    rightBlkHeader.pblock=leftBlkHeader.pblock;
    // and update the header of rightBlk using BlockBuffer::setHeader()
    rightBlk.setHeader(&rightBlkHeader);

    // set leftBlkHeader with the following values
    // - number of entries = (MAX_KEYS_INTERNAL)/2 = 50
    leftBlkHeader.numEntries=MAX_KEYS_INTERNAL/2;
    leftBlkHeader.rblock=rightBlkNum;
    // and update the header using BlockBuffer::setHeader()
    leftBlk.setHeader(&leftBlkHeader);

    
    // - set the first 50 entries of leftBlk = index 0 to 49 of internalEntries
    //   array
    for(int i=0;i<MIDDLE_INDEX_INTERNAL;i++){
        leftBlk.setEntry(&internalEntries[i], i);
        rightBlk.setEntry(&internalEntries[i+MIDDLE_INDEX_INTERNAL+1], i);
    }
    // - set the first 50 entries of newRightBlk = entries from index 51 to 100
    //   of internalEntries array using IndInternal::setEntry().
    //   (index 50 will be moving to the parent internal index block)
    

    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild)/* block type of a child of any entry of the internalEntries array */;
    //            (use StaticBuffer::getStaticBlockType())

/*---- SET PBLOCK OF CHILDREN OF RIGHT BLOCK TO POINT TO RIGHT BLOCK----*/

    BlockBuffer childBuffer(internalEntries[MIDDLE_INDEX_INTERNAL+1].lChild);
    HeadInfo childHeader;
    childBuffer.getHeader(&childHeader);
    childHeader.pblock=rightBlkNum;
    childBuffer.setHeader(&childHeader);

    for (int i=0;i<MIDDLE_INDEX_INTERNAL;i++) {
        /* each child block of the new right block */
        // declare an instance of BlockBuffer to access the child block using
        // constructor 2
        BlockBuffer childBuffer(internalEntries[MIDDLE_INDEX_INTERNAL+i+1].rChild);
        HeadInfo childHeader;
        childBuffer.getHeader(&childHeader);
        // update pblock of the block to rightBlkNum using BlockBuffer::getHeader()
        childHeader.pblock=rightBlkNum;
        // and BlockBuffer::setHeader().
        childBuffer.setHeader(&childHeader);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // declare newRootBlk, an instance of IndInternal using appropriate constructor
    IndInternal newRootBlk;
    // to allocate a new internal index block on the disk

    int newRootBlkNum = newRootBlk.getBlockNum()/* block number of newRootBlk */;

    if (newRootBlkNum == E_DISKFULL) {
        // (failed to obtain an empty internal index block because the disk is full)

        // Using bPlusDestroy(), destroy the right subtree, rooted at rChild.
        bPlusDestroy(rChild);
        // This corresponds to the tree built up till now that has not yet been
        // connected to the existing B+ Tree

        return E_DISKFULL;
    }

    // update the header of the new block with numEntries = 1 using
    // BlockBuffer::getHeader() and BlockBuffer::setHeader()
    HeadInfo rootHeader;
    newRootBlk.getHeader(&rootHeader);
    rootHeader.numEntries=1;
    rootHeader.blockType=IND_INTERNAL;
    newRootBlk.setHeader(&rootHeader);
    
    // create a struct InternalEntry with lChild, attrVal and rChild from the
    InternalEntry entry;
    entry.lChild=lChild;
    entry.rChild=rChild;
    entry.attrVal=attrVal;
    // arguments and set it as the first entry in newRootBlk using IndInternal::setEntry()
    newRootBlk.setEntry(&entry, 0);

    // declare BlockBuffer instances for the `lChild` and `rChild` blocks using
    BlockBuffer leftChild(lChild);
    BlockBuffer rightChild(rChild);
    // appropriate constructor and update the pblock of those blocks to `newRootBlkNum`
    // using BlockBuffer::getHeader() and BlockBuffer::setHeader()
    HeadInfo leftHeader, rightHeader;
    leftChild.getHeader(&leftHeader); rightChild.getHeader(&rightHeader);
    leftHeader.pblock=newRootBlkNum; rightHeader.pblock=newRootBlkNum;
    leftChild.setHeader(&leftHeader); rightChild.setHeader(&rightHeader);
    // update rootBlock = newRootBlkNum for the entry corresponding to `attrName`
    attrCatEntry.rootBlock=newRootBlkNum;
    // in the attribute cache using AttrCacheTable::setAttrCatEntry().
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);
    return SUCCESS;
}