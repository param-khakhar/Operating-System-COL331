/* exit2.c

   Simple program to test whether running a user program works.
 	
   Just invokes a system call that shuts down the OS. */

#include <syscall.h>
#include <stdio.h>

int
main (void)
{
  int id = exec("exitn");
  printf("forked %d\n",id);
  if(id > 0) wait(id);
  exit (1234);
  /* not reached */
}
