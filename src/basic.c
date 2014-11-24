/* basic.c - test that basic persistency works */

#include "rvm.h"
#include <stdio.h>
#include <string.h>

#define TEST_STRING "hello, world"
#define OFFSET2 1000


/* proc1 writes some data, commits it, then exits */
void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[0];
     
     rvm = rvm_init("rvm_segments");
	 rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);

#if 0
     
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     
     rvm_about_to_modify(trans, segs[0], 0, 100);
     sprintf(segs[0], TEST_STRING);
     
     rvm_about_to_modify(trans, segs[0], OFFSET2, 100);
     sprintf(segs[0]+OFFSET2, TEST_STRING);
     
     rvm_commit_trans(trans);

     abort();
	 rvm_terminate(rvm);
#endif
}


int main(int argc, char **argv)
{
     proc1();

     return 0;
}
