/* multi.c - test that basic persistency works for multiple segments */


#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define SEGNAME0  "testseg1"

#define OFFSET0  10
#define OFFSET1  100

#define STRING0 "hello, world"
#define STRING1 "black agagadrof!"


void proc1() 
{
     rvm_t rvm;
     char* segs[2];
     trans_t trans;
     
     rvm = rvm_init("rvm_segments");

     rvm_destroy(rvm, SEGNAME0);

     segs[0] = (char*) rvm_map(rvm, SEGNAME0, 1000);
     //remap a segment, it will report error
	 segs[1] = (char*) rvm_map(rvm, SEGNAME0, 1000);


     trans = rvm_begin_trans(rvm, 2, (void **)segs);

	 if (segs[0]) {
		rvm_about_to_modify(trans, segs[0], OFFSET0, 100);
		strcpy(segs[0]+OFFSET0, STRING0);
	 }
	 if (segs[1]) {
		rvm_about_to_modify(trans, segs[1], OFFSET1, 100);
		strcpy(segs[1]+OFFSET1, STRING1);
	 }

     rvm_commit_trans(trans);
	
	 rvm_unmap(rvm, segs[0]);
	 rvm_unmap(rvm, segs[1]);
}


void proc2() 
{
     rvm_t rvm;
     char *segs[1];

     rvm = rvm_init("rvm_segments");
     segs[0] = (char*) rvm_map(rvm, SEGNAME0, 1000);

     if(strcmp(segs[0] + OFFSET0, STRING0)) {
	  printf("ERROR in segment (%s)\n",
		 segs[0]+OFFSET0);
	  exit(2);
     }
     if(strcmp(segs[0] + OFFSET1, STRING1) == 0) {
	  printf("ERROR in segment (%s)\n",
		 segs[1]+OFFSET1);
	  exit(2);
     }

     printf("OK\n");
	 rvm_unmap(rvm, segs[0]);
}


int main(int argc, char **argv) 
{
#if 1
     int pid;

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc1();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     proc2();
#else
	 proc1();
	 proc2();
#endif
     return 0;
}
