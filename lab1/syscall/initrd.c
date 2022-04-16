#include<sys/syscall.h>
#include<unistd.h>
#include<stdio.h>
int main()
{
	char buf[25], buf1[25];
	long rc = syscall(548, buf, 20);
	printf("buf: %s rc: %ld\n",buf,rc);
	
	long rc1 = syscall(548, buf1, 2);
	printf("rc1 :%ld\n",rc1);
	while(1) {}
}

