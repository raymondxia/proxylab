/* Defining macros for the proxy's maximum cache size
   and the maximum size of web objects to be cached */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "cache.h"
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>

/* The cache will be represented as a linked list of web objects
   The eviction policy will be LRU and each object will hold a
   timestamp indicating when it was last used */

typedef struct web_object {
  char *data;
  unsigned int timestamp;
  struct web_object* next;
}web_object;

typedef struct cache_LL {
  web_object* head;
  unsigned int cache_size;
}cache_LL;

void addToCache(cache_LL cache, web_object obj);


