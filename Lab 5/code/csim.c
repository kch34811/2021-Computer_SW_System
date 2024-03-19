/* 20200437 김채현 */
/* kch3481@postech.ac.kr */

#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct{
    bool valid;
    unsigned long long tag;
    int LRU;
} cache_line;

typedef struct{
    cache_line *lines;
} cache_set;

typedef struct{
    cache_set *sets;
    int s;
    int E;
    int b;
} cache;

bool verbose = false;

void cache_check(cache *cache, int *hits, int *misses, int *evictions, unsigned long long address, int count);
void set_index_and_tag (unsigned long long address, int s, int b, int *set_index, unsigned long long *tag);
bool cache_hit (cache_set *set, unsigned long long tag, int E, int count);
int LRU_index (cache_set *set, int E);

int main(int argc, char *argv[])
{
    int hits  = 0;
    int misses = 0;
    int evictions = 0;
    cache cache = {};

    char *tracefile_name;
    FILE *tracefile = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "s:E:b:t:hv")) != -1){
        switch(opt){
             case 'h' :
                printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
                return 0;
             case 'v' :
                verbose = true;
                break;
            case 's':
                cache.s = atoi(optarg);
                break;
            case 'E' :
                cache.E = atoi(optarg);
                break;
            case 'b' :
                cache.b = atoi(optarg);
                break;
            case 't' :
                tracefile_name = malloc (strlen(optarg) + 1);
                strcpy(tracefile_name, optarg);
                break;
            default :
                return 1;
        }    
    }
    
    tracefile = fopen(tracefile_name, "r");
    if (tracefile == NULL) return 1;

    //initialize cache
    cache.sets = malloc(sizeof(cache_set) * (2 << cache.s));
    for (int i = 0; i < (1 << cache.s); i++){
        cache.sets[i].lines = malloc (sizeof(cache_line) * cache.E);
        for (int j = 0; j < cache.E; j++){
            cache.sets[i].lines[j].valid = 0;
            cache.sets[i].lines[j].tag = 0;
            cache.sets[i].lines[j].LRU = -1;
        }
    }
   
    //
    char operation;
    unsigned long long address;
    int size, count = 0;

    while(fscanf(tracefile, " %c %llx,%d", &operation, &address, &size) != EOF) {
        switch (operation) {
            case 'L' :
                if (verbose) printf("%c %llx,%d ", operation, address, size);
                cache_check(&cache, &hits, &misses, &evictions, address, count);
                if (verbose) printf("\n");
                break;
            case 'M' :
                if (verbose) printf("%c %llx,%d ", operation, address, size);
                cache_check(&cache, &hits, &misses, &evictions, address, count);
                cache_check(&cache, &hits, &misses, &evictions, address, count);
                if (verbose) printf("\n");
                break;
            case 'S':
                if (verbose) printf("%c %llx,%d ", operation, address, size);
                cache_check(&cache, &hits, &misses, &evictions, address, count);
                if (verbose) printf("\n");
                break;
        }
        count++;
    }

    printSummary(hits, misses, evictions);
   
    //free the cache
    for (int i = 0; i < (1 << cache.s); i++) {
        free(cache.sets[i].lines);
    }
    free(cache.sets);
    fclose(tracefile);
    return 0;
}


void cache_check(cache *cache, int *hits, int *misses, int *evictions, unsigned long long address, int count) {
    int set_index;
    unsigned long long tag;
    set_index_and_tag (address, cache->s, cache->b, &set_index, &tag);
    cache_set *set = &cache->sets[set_index];
   
    //hit
    if (cache_hit(set, tag, cache->E, count)){
        if (verbose) printf("hit ");
        *hits +=1;
        return;
    }
   
    //miss
    if (verbose) printf("miss ");
    *misses +=1;

    //eviction
    int empty_line_index = -1;
    for (int i = 0; i < cache->E; i++){
        if (set->lines[i].valid == 0)
           empty_line_index = i;
    }
    if (empty_line_index == -1){
        if (verbose) printf("eviction ");
        *evictions +=1;
        int LRU_line_index = LRU_index (set, cache->E);
        set->lines[LRU_line_index].valid = 1;
        set->lines[LRU_line_index].tag = tag;
        set->lines[LRU_line_index].LRU = count;
    }

    else{
        set->lines[empty_line_index].valid = 1;
        set->lines[empty_line_index].tag = tag;
        set->lines[empty_line_index].LRU = count;
    }
}

void set_index_and_tag (unsigned long long address, int s, int b, int *set_index, unsigned long long *tag) {
    *set_index = ((address >> b) % (1 << s));
    *tag = address >> (s + b);
}

bool cache_hit (cache_set *set, unsigned long long tag, int E, int count) {
    for (int i = 0; i < E; i++){
        if (set->lines[i].valid == 1 && set->lines[i].tag == tag){
            set->lines[i].LRU = count;
            return true;
        }
    }
    return false;
}

int LRU_index (cache_set *set, int E) {
    int LRU_line_index = 0;
    int LRU = set->lines[0].LRU;
    for (int i = 1; i < E; i++){
        if (set->lines[i].LRU < LRU){
            LRU = set->lines[i].LRU;
            LRU_line_index = i;
        }
    }
    return LRU_line_index;
}