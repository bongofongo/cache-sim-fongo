#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct line{
    unsigned char valid;
    long unsigned int tag;
    unsigned int maketime;
};
typedef struct line line;

int hit, miss, evict;
unsigned int clock;


line* make_cold_dcache(int E, int s, int b) {
    line* cres = (line*)malloc(s*sizeof(line));
    for (int i = 0; i < s; i++) {
        cres[i].valid = 0;
        cres[i].tag = 12837461293857;
        cres[i].maketime = clock;
    }
    return cres;
}

line** make_cold_scache(int E, int s, int b) {
    line** cres = (line**)malloc(s*sizeof(line*));
    for (int i = 0; i < s; i++) {
        cres[i] = (line*)malloc(E*sizeof(line));
    }
    for (int i = 0; i < s; i++) {
        for (int j = 0; j < E; j++) {
            cres[i][j].valid = 0;
            cres[i][j].tag = 129384710293857;
            cres[i][j].maketime = clock;
        }
    }
    return cres;
}

long unsigned int find_set_index(long unsigned int address, int s, int b) {
    long unsigned int mask = pow(2, s+b)-1;
    return ((address & mask) >> b);
}

long unsigned int find_tag(long unsigned int address, int s, int b) {
    long unsigned int mask = ~((1UL << 63) >> (b+s-1));
    return mask & (address >> (b+s));
}
void load_direct_data(line* cache, long unsigned int address, int setno, int s, int b) {
    long unsigned int index = find_set_index(address, s, b);
    long unsigned int tag = find_tag(address, s, b);
    int i = index % setno;
    if (cache[i].valid == 0) {
        cache[i].valid = 1;
        cache[i].tag = tag;
        miss++;
    } else {
        if (cache[i].tag == tag) {
            hit++;
        } else {
            miss++, evict++;
            cache[i].tag = tag;
        }
    }
    cache[i].maketime = clock;
}

void load_ass_data(line** cache, long unsigned int address, int setno, int s, int b, int E) {
    long unsigned int index = find_set_index(address, s, b);
    long unsigned int tag = find_tag(address, s, b);
    int i = index % setno;
    int j, success = 0;

    // Looking for hits
    for (j = 0; j < E; j++) {
        if (cache[i][j].tag == tag) {
            hit++;
            success = 1;
            cache[i][j].maketime = clock;
            break;
        }
    }

    // Looking for empty lines
    if (success != 1) {
        for (j = 0; j < E; j++) {
            if (cache[i][j].valid == 0) {
                miss++;
                cache[i][j].valid = 1;
                cache[i][j].tag = tag;
                success = 1;
                cache[i][j].maketime = clock;
                break;
            }
        }   
    }
    
    // If there is no empty line
    if (success != 1) {
        int lru = 1000000, col;
        for (j = 0;j<E;j++) {
            if (lru > cache[i][j].maketime) {
                lru = cache[i][j].maketime;
                col = j;
            }
        }
        evict++, miss++;
        cache[i][col].tag = tag;
        cache[i][col].maketime = clock;
    }
    
}

int simulate(FILE* tp, int s, int E, int b) {
    // Direct Associativity
    int setno = pow(2, s);
    char file_line[70];
    char instruction;
    long unsigned int address; 
    unsigned int size;
    if (E == 1) {
        // Cache Development and Printing
        line* cache = make_cold_dcache(E, setno, b);
        printf("Cold Cache:\n");
        for (int i = 0; i < setno; i++) {
            printf("(%d, %lx, %d)\n", cache[i].valid, cache[i].tag, cache[i].maketime);
        }
        printf("\n");

        printf("File Reading:\n");
        while (fgets(file_line, sizeof(file_line), tp)) {
            sscanf(file_line, " %c %lx,%d", &instruction, &address, &size);
            printf("%c %lx,%d: ", instruction, address, size);
            if (instruction == 'M') {
                load_direct_data(cache, address, setno, s, b);
                load_direct_data(cache, address, setno, s, b);
            } else if (instruction == 'L' || instruction == 'S') {
                load_direct_data(cache, address, setno, s, b);
            }
            clock++;
            printf("hit: %d, miss %d, evictions: %d\n", hit, miss, evict);
        }
        printf("\nFilled Cache:\n");
        for (int i=0;i<setno;i++) {
            printf("(%d, %lx, %d)\n", cache[i].valid, cache[i].tag, cache[i].maketime);
        }
    }

    if (E > 1) {
        // Cache Development and Printing
        line** cache = make_cold_scache(E, setno, b);
        printf("Cold Cache: \n");
        for (int i = 0; i < setno; i++) {
            for (int j = 0; j < E; j++) {
                printf("(%d, %lx, %d) ", cache[i][j].valid, cache[i][j].tag, cache[i][j].maketime);
            }
            printf("\n");
        }
        printf("\n");

        printf("File Reading:\n");
        while (fgets(file_line, sizeof(file_line), tp)) {
            sscanf(file_line, " %c %lx,%d", &instruction, &address, &size);
            printf("%c %lx,%d: ", instruction, address, size);
            if (instruction == 'M') {
                load_ass_data(cache, address, setno, s, b, E);
                load_ass_data(cache, address, setno, s, b, E);
            } else if (instruction == 'L' || instruction == 'S') {
                load_ass_data(cache, address, setno, s, b, E);
            }
            clock++;
            printf("hit: %d, miss %d, evictions: %d\n", hit, miss, evict);
        }

    }

    return 0;
}

int main(int argc, char **argv)
{
    FILE *t;
    int E = 1, b = 1, s = 1;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-E"))
            E = atoi(argv[i+1]);
        else if (!strcmp(argv[i], "-s"))
            s = atoi(argv[i+1]);
        else if (!strcmp(argv[i], "-b"))
            b = atoi(argv[i+1]);
        else if (!strcmp(argv[i], "-t"))
            t = fopen(argv[i+1], "r");     
    }
    if (t == NULL) {
        fprintf(stderr, "File not found.");
        exit(1);
    }

    hit = 0, miss = 0, evict = 0, clock = 0;
    simulate(t, s, E, b);

    printSummary(hit, miss, evict);
    return 0;
}






