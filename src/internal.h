#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdio.h>
#include "rvm_type.h"

extern void set(const char *filename, int value);
extern int status(rvm_t rvm, const char *segname);
extern int seg_free(map_t maps[], int size, const char *segname);
extern int next_entry(map_t map[], int size);
extern int get_segid(map_t map[], int seg_num, const void *segbase);


#endif
