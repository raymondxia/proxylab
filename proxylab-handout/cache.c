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

/* Defining Global variables */
unsigned int timecounter = 0;


web_object* checkCache(cache_LL* cache, char* path) 
{
    dbg_printf("\nCACHE >> Checking Cache for %s\n", path);

    web_object* cursor = cache->head;
    //increment the global counter for the time every time we search the cache
    timecounter++;

    while(cursor != NULL)
    {
        dbg_printf("CACHE >> Compare to %s\n", cursor->path);

        if(!strcmp(cursor->path, path)) {
            //the object at cursor has just been used! 
            //change its timestamp to reflect the current time
            cursor->timestamp = timecounter;
            dbg_printf("CACHE >> Found in cache!\n");
            return cursor;
        }

        cursor = cursor->next;
    }

    dbg_printf("CACHE >> Not found in cache.\n");
    //We return NULL if we did not find the object in the cache
    return NULL;
}

void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize)
{
    dbg_printf("\nCACHE >> Adding to cache: %s\n", path);


    dbg_printf("CACHE >> Allocating %lu bytes for new web_object.\n", sizeof(web_object));
    //toAdd will hold all the information regarding the new web object
    //that is to be added to the cache linked list
    web_object* toAdd = Calloc(1, sizeof(web_object));


    dbg_printf("CACHE >> Creating cache object.\n");


    toAdd->data = Calloc(1, MAX_OBJECT_SIZE);
    dbg_printf("CACHE >> Attempting adding data of size %d\n", addSize);
    dbg_printf(" to field of size %lu\n", sizeof(toAdd->data));
    //We use memcpy because we have to treat data as a byte array, not a string
    memcpy(toAdd->data, data, addSize);
    dbg_printf("CACHE >> Copied data.\n");
    //update the time stamp of the new object to reflect the current time
    toAdd->timestamp = timecounter;
    dbg_printf("CACHE >> Updated timestamp.\n");

    toAdd->path = Calloc(1, MAXLINE);
    strcpy(toAdd->path, path);
    dbg_printf("CACHE >> Copied path.\n");
    //update the size of the new object
    toAdd->size = addSize;
    dbg_printf("CACHE >> Updated size.\n");
    //Increment the cache size
    cache->size += addSize;
    dbg_printf("CACHE >> Incremented cache size.\n");

    //Adding the object to the head of the linked list representing the cache
    toAdd->next = cache->head;
    dbg_printf("CACHE >> Object's next = cache's head.\n");

    cache->head = toAdd;
    dbg_printf("CACHE >> Cache list points here.\n");
    timecounter++;

    //If the addition of this object has caused the cache to exceed the
    //max size, we evict objects until the cache is of a proper size
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

    while(cursor != NULL)
    {
        if(cursor->timestamp < leastRecentTime)
            leastRecentTime = cursor->timestamp;
        cursor = cursor->next;
    }

    //reset the cursor
    cursor = cache->head;

    /* We now have two cases:
    * 1. The object to be evicted is at the head of the list
    * 2. The object to be evicted is not at the head
    */

    web_object *temp;
    /* Case 1: Evict the object at the head */
    if( cache->head->timestamp == leastRecentTime)
    {
        temp = cache->head;
        cache->head = cache->head->next;
        //since i've allocated memory for these fields, I need to free them
        free(temp->data);
        free(temp->path);
        free(temp);
    }

    /*
     * Case 2: The object is not at the head.
     * This loop goes back and deletes that old cache object
     * that was found in the first while loop.
     */
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
