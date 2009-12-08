#include <stdlib.h>
#include <stdio.h>
#include "ftplib.h"

int main(int argc, char *argv[])
{
	netbuf *ctrl;
	FtpConnect("202.117.21.117", &ctrl);
	FtpDir(NULL, ".", ctrl);
	FtpClose(ctrl);
	return 0;
}
