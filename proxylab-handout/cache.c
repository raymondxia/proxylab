/* Defining macros for the proxy's maximum cache size
   and the maximum size of web objects to be cached */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "cache.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>

/* The cache will be represented as a linked list of web objects
   The eviction policy will be LRU and each object will hold a
   timestamp indicating when it was last used */

typedef struct web_object{
  char *data;
  unsigned int timestamp;
  unsigned int size;
  char* path;
  struct web_object* next;
} web_object;

typedef struct cache_LL{
  web_object* head;
  unsigned int size;
}cache_LL;

/* Defining Global variables */
unsigned int timecounter = 0;

web_object* checkCache(cache_LL* cache, char* path);
void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize);
void evictAnObject(cache_LL* cache);

web_object* checkCache(cache_LL* cache, char* path) {
	web_object* cursor = cache->head;
	timecounter++;
	while(cursor != NULL)
	{
		if(strcmp(cursor->path, path)) {
			cursor->timestamp = timecounter;
			return cursor;
		}
			
		cursor = cursor->next;
	}
	return NULL;
}

void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize)
{
	web_object* toAdd = calloc(1, sizeof(web_object));
	strcpy(toAdd->data, data);
	toAdd->timestamp = timecounter;
	strcpy(toAdd->path, path);
	//if(additionalSize > MAX_OBJECT_SIZE)
	//	return;
	toAdd->size = addSize;
	cache->size += addSize;
	//Adding the object to the head of the linked list representing the cache
	toAdd->next = cache->head;
	cache->head = toAdd;
	timecounter++;

	while(cache->size > MAX_CACHE_SIZE)
	{
		evictAnObject(cache);
	}
}

void evictAnObject (cache_LL* cache)
{
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

	while(cursor != NULL)
	{
		if(cursor->timestamp == leastRecentTime)
		{
			if(cursor->next->next != NULL) {
				cache->size -= cursor->size;
				cursor->next = cursor->next->next;
			}
			else
				cursor->next = NULL;
		}
		cursor = cursor->next; 
	}
}

