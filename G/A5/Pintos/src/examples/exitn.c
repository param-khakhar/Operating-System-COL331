/* exitn.c

   Simple program to test whether running a user program works.
 	
   Just invokes a system call that shuts down the OS. */

#include <syscall.h>

int
main (void)
{
   exit(42);
   exit(-2); 
   exit(1234);
  /* not reached */
}
