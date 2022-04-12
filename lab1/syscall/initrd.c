#include<sys/syscall.h>
#include<unistd.h>
#include<stdio.h>
int main()
{
	int rc;
	char *buf;
	rc = syscall(548, buf, 20);
	
	if(rc!=-1)
	{
		printf("%s\n",buf);
		printf("%d\n",rc);
	}
	else
	{
		printf("%d\n",rc);
	}
	while(1) {}
}

