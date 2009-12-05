#include <stdlib.h>
#include <stdio.h>
#include "ftp.h"

int main(int argc, char *argv[])
{
	netbuf *ctrl;
	if (ftp_connect("hcy:hcy1988hcy!@202.117.21.117/home/hcy/ftp/linux", &ctrl) < 0)
	{
		log_err("Connect error");
		return 1;
	}

	//if (ftp_login("hcy", "hcy1988hcy!", ctrl) < 0)
	//{
	//	log_err("Login error.");
	//	return 1;
	//}
	
	if (ftp_ls(NULL, NULL, ctrl) < 0)
	{
		log_err("List error.");
		return -1;
	}
	//ftp_get("gmp-4.3.1.tar.gz", "gmp-4.3.1.tar.gz", FTPLIB_IMAGE, ctrl );

	ftp_put("Makefile", "Makefile", FTPLIB_IMAGE, ctrl );

	ftp_quit(ctrl);

	return 0;
}
