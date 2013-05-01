/* Defining macros for the proxy's maximum cache size
and the maximum size of web objects to be cached */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "cache.h"
#include "csapp.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>


#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* The cache will be represented as a linked list of web objects
The eviction policy will be LRU and each object will hold a
timestamp indicating when it was last used */
/*
typedef struct web_object{
char* data;
unsigned int timestamp;
unsigned int size;
char* path;
struct web_object* next;
} web_object;

typedef struct cache_LL{
web_object* head;
unsigned int size;
}cache_LL;
*/

/* Defining Global variables */
unsigned int timecounter = 0;

//web_object* checkCache(cache_LL* cache, char* path);
//void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize);
//void evictAnObject(cache_LL* cache);

web_object* checkCache(cache_LL* cache, char* path) {

  dbg_printf("\nCACHE >> Checking Cache for %s\n", path);

web_object* cursor = cache->head;
timecounter++;
while(cursor != NULL)
{
dbg_printf("CACHE >> Compare to %s\n", cursor->path);

if(!strcmp(cursor->path, path)) {
cursor->timestamp = timecounter;
dbg_printf("CACHE >> Found in cache!\n");
return cursor;
}

cursor = cursor->next;
}

dbg_printf("CACHE >> Not found in cache.\n");
return NULL;
}

void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize)
{
        dbg_printf("\nCACHE >> Adding to cache: %s\n", path);


dbg_printf("CACHE >> Allocating %lu bytes for new web_object.\n", sizeof(web_object));
web_object* toAdd = Calloc(1, sizeof(web_object));


        dbg_printf("CACHE >> Creating cache object.\n");


/******* FIX! ******/
//have to treat data as byte array, not as string
//strcpy(toAdd->data, data);

//try allocating space for these strings!
        toAdd->data = Calloc(1, MAX_OBJECT_SIZE);
dbg_printf("CACHE >> Attempting adding data of size %d\n", addSize);
dbg_printf(" to field of size %lu\n", sizeof(toAdd->data));
        memcpy(toAdd->data, data, addSize);
dbg_printf("CACHE >> Copied data.\n");

toAdd->timestamp = timecounter;
dbg_printf("CACHE >> Updated timestamp.\n");

toAdd->path = Calloc(1, MAXLINE);
strcpy(toAdd->path, path);
dbg_printf("CACHE >> Copied path.\n");

toAdd->size = addSize;
dbg_printf("CACHE >> Updated size.\n");

cache->size += addSize;
dbg_printf("CACHE >> Incremented cache size.\n");

//Adding the object to the head of the linked list representing the cache
toAdd->next = cache->head;
dbg_printf("CACHE >> Object's next = cache's head.\n");

cache->head = toAdd;
dbg_printf("CACHE >> Cache list points here.\n");
timecounter++;

while(cache->size > MAX_CACHE_SIZE)
{
evictAnObject(cache);
}

        dbg_printf("CACHE >> Done adding.\n");
}

void evictAnObject (cache_LL* cache)
{
        dbg_printf("CACHE >> Evicting from cache.\n");

unsigned int leastRecentTime = cache->head->timestamp;
web_object* cursor = cache->head;
//The following while loop finds the exact timestamp of the least
//recently used web object by iterating through the cache

dbg_printf("AH!\n");

while(cursor != NULL)
{
if(cursor->timestamp < leastRecentTime)
leastRecentTime = cursor->timestamp;
cursor = cursor->next;
}

//reset the cursor
cursor = cache->head;


web_object *temp;

        if( cache->head->timestamp == leastRecentTime)
        {
temp = cache->head;
cache->head = cache->head->next;
free(temp->data);
free(temp->path);
free(temp);
        }

        // this loop goes back and deletes that old cache object
        while(cursor->next != NULL)
{
if(cursor->next->timestamp == leastRecentTime)
{
cache->size -= cursor->next->size;
cursor->next = cursor->next->next;
temp = cursor->next;
//since i've allocated memory for these fields, need to free them
free(temp->data);
free(temp->path);
free(temp);

return;
}

cursor = cursor->next;
}

}
