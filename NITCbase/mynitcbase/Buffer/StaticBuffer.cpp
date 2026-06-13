// the declarations for this class can be found at "StaticBuffer.h"
#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer(){
    // initialize all blocks as free
    for(int i=0, bMapSlot=0;i<4;i++){
        unsigned char tempBuff[BLOCK_SIZE];
        Disk::readBlock(tempBuff, i);
        for(int slot=0;slot<BLOCK_SIZE;slot++, bMapSlot++)
            blockAllocMap[bMapSlot]=tempBuff[slot];
    }
    for(int i=0;i<BUFFER_CAPACITY;i++){
        metainfo[i].free=true;
        metainfo[i].dirty=false;
        metainfo[i].timeStamp=-1;
        metainfo[i].blockNum=-1;
    }
}

StaticBuffer::~StaticBuffer(){
    for(int i=0, bMapSlot=0;i<4;i++){
        unsigned char tempBuff[BLOCK_SIZE];
        for(int slot=0;slot<BLOCK_SIZE;slot++, bMapSlot++)
            tempBuff[slot]=blockAllocMap[bMapSlot];
        Disk::writeBlock(tempBuff, i);
    }

    // write back dirty blocks
    for(int i=0;i<BUFFER_CAPACITY;i++)
        if(!metainfo[i].free && metainfo[i].dirty)
            Disk::writeBlock(blocks[i], metainfo[i].blockNum);

}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum<0 || blockNum>DISK_BLOCKS)
        return E_OUTOFBOUND;

    for(int i=0;i<BUFFER_CAPACITY;i++)
        metainfo[i].timeStamp++;

    int allocatedBuffer=-1;

    for(int i=0;i<BUFFER_CAPACITY;i++)
        if(metainfo[i].free){
            allocatedBuffer=i;
            break;
        }
    
    if(allocatedBuffer==-1){
        allocatedBuffer=0;
        for(int i=0;i<BUFFER_CAPACITY;i++)
            if(metainfo[i].timeStamp>metainfo[allocatedBuffer].timeStamp)
                allocatedBuffer=i;
        if(metainfo[allocatedBuffer].dirty)
            Disk::writeBlock(blocks[allocatedBuffer], metainfo[allocatedBuffer].blockNum);
        
    }

    metainfo[allocatedBuffer].free=false;
    metainfo[allocatedBuffer].dirty=false;
    metainfo[allocatedBuffer].timeStamp=0;
    metainfo[allocatedBuffer].blockNum=blockNum;
    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum){
    if(blockNum<0 || blockNum>DISK_BLOCKS)
        return E_OUTOFBOUND;

    for(int i=0;i<BUFFER_CAPACITY;i++)
        if(metainfo[i].blockNum==blockNum)
            return i;
    
    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
    int bufferNum=getBufferNum(blockNum);

    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    if(bufferNum==E_BLOCKNOTINBUFFER)
    return E_BLOCKNOTINBUFFER;
    
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
    if(bufferNum==E_OUTOFBOUND)
        return E_OUTOFBOUND;
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo
    metainfo[bufferNum].dirty=true;
    return SUCCESS;
}