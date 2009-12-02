#include <stdlib.h>
#include <stdio.h>
#include "ftp.h"

int main(int argc, char *argv[])
{
	netbuf *ctrl;
	if (ftp_connect("202.117.21.117", &ctrl) < 0)
	{
		log_err("Connect error");
		return 1;
	}

	if (ftp_login("hcy", "hcy1988hcy!", ctrl) < 0)
	{
		log_err("Login error.");
		return 1;
	}
	
	if (ftp_ls(NULL, ".", ctrl) < 0)
	{
		log_err("List error.");
		return -1;
	}

	ftp_cdup(ctrl);
	ftp_ls(NULL, ".", ctrl);
	ftp_quit(ctrl);

	return 0;
}
