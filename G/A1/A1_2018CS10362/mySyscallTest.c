#include<stdio.h>
#include<linux/kernel.h>
#include<sys/syscall.h>
#include<unistd.h>

int main()
{
	long int mySyscallTest = syscall(548);
	printf("System call sys_mySyscall returned %ld\n",mySyscallTest);
	return 0;
}
