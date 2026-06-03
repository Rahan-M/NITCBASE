#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // StaticBuffer buffer;
  // OpenRelTable cache;

  // Disk functions expects 2 arguments, 1 buffer of size 2048 bytes and block number

  unsigned char buffer[2048];
  // We use unsigned char as it is of size 1 bytes,
  // This accurately simulates one block

  Disk::readBlock(buffer, 0);
  // 7000 is a random unused block number

  for(int i=0;i<10;i++){
    std::cout<<int(buffer[i])<<'\n';
  }
  // return FrontendInterface::handleFrontend(argc, argv);
  return 0;
}