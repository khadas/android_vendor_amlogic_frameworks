#ifndef ANDROID_GETINMEMORY_H
#define ANDROID_GETINMEMORY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utils/Log.h>

namespace android {

struct MemoryStruct {
  char *memory;
  size_t size;
};


size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);


//char* getFileByCurl(char* url);
char* getFileByCurl(char* url);


}

#endif // ANDROID_GETINMEMORY_H

