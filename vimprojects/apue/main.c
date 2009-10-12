#include "apue.h"

int log_to_stderr = 1;

int main(int argc, char *argv[])
{

	printf("Hello UPAE! PID: %d\n", getpid());

	runit(argc, argv);
	
	return 0;
}
