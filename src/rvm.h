#ifndef _LRVM_H_
#define _LRVM_H_

#include <stdint.h>

#include "rvm_type.h"

#if 0
extern typedef struct seg seg_t;
extern typedef struct rvm rvm_t;
extern typedef struct trans trans_t;
#endif 

/* API definition */

/* initialize the library with the specified directory 
 * as a backing store, return related rvm identification
 */
extern rvm_t rvm_init(const char * directory);


extern void rvm_terminate(rvm_t rvm);

/* destroy segment completely, erasing its backing store.
 * The segment should not be mapped
 */
extern void rvm_destroy(rvm_t rvm, const char *segname);

/* create a recoverable virtual memory and read the data 
 * from its backing store if there is. 
 * a segment can be only mapped once, more than once is 
 * an error. In our design segname is also the backing 
 * store file name, return the address of virtual memory
 */
extern void *rvm_map (rvm_t rvm, const char *segname, int size);

/* unmap a segment from memory, segbase is the returned address */
extern void rvm_unmap(rvm_t rvm, void *segbase);

/* begin a transaction that will modify the 
 * segments listed in segbases 
 */
extern trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases);

extern void rvm_about_to_modify(trans_t id, void * segbase, int offset, int size);

/* commit all changes */
extern void rvm_commit_trans(trans_t tid);

extern void rvm_abort_trans(trans_t tid);

/* play through any commited or aborted items in the log files */
extern void rvm_truncate_log(rvm_t rvm);

/* if enabled, print infomation about what is doing */
extern void rvm_verbose(int enable_flag);

#endif
