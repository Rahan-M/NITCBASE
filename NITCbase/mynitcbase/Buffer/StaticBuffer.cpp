// the declarations for this class can be found at "StaticBuffer.h"
#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer(){
    // initialize all blocks as free
    for(int i=0;i<BUFFER_CAPACITY;i++)
        metainfo[i].free=true;
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer(){}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum<0 || blockNum>DISK_BLOCKS)
        return E_OUTOFBOUND;

    int allocatedBuffer;

    for(int i=0;i<BUFFER_CAPACITY;i++)
        if(metainfo[i].free){
            allocatedBuffer=i;
            break;
        }
    
    metainfo[allocatedBuffer].free=false;
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