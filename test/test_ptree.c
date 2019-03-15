#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include </home/ubuntu1604/os-team4/include/linux/prinfo.h> // change this before use

#define SYS_ptree 398

typedef enum {false, true} bool;

int main(int argc, char *argv[]) {

	int result;

	struct prinfo *buf;
	int *nr;
	
	struct prinfo p;

	bool *is_printed;
	
	int i;

	if (argc < 2) {
		printf("nr needed");
		return -1;
	}
	
	nr = (int*)malloc(sizeof(int));

	if(nr != NULL)
		*nr = atoi(argv[1]);	
	else
		return 1;
	
	buf = (struct prinfo *)malloc(sizeof(struct prinfo)*(*nr));
	
	is_printed = (bool *)calloc(*(nr), sizeof(bool));

	if(!buf || !is_printed)
		return 1;		
	
	errno = 0;

	result = syscall(SYS_ptree, buf, nr);

	printf("%d", result);

	if(result != 0) {

		printf("%d", errno);

		if(errno == EINVAL)
			perror("Invalid argument");
		else if(errno == EFAULT)
 			perror("Bad address");

		return -1;
	}
	
	for(i = 0 ; i < *nr ; i++) {
		
		if(is_printed[i])
			continue;

		p = buf[i];	
		
		printf("%s,%d,%lld,%d,%d,%d,%lld\n", p.comm, p.pid, p.state, p.parent_pid, p.first_child_pid, p.next_sibling_pid, p.uid);

		is_printed[i] = true;
	}

	free(nr);
	free(buf);
	free(is_printed);
}
