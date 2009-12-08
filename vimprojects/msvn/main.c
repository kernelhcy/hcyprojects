#include <stdlib.h>
#include <stdio.h>
#include "libemail/libemail.h"

int main(int argc, char *argv[])
{
	log_info("Test mail.");
	
	email_entry *mail = NULL;
	mail = email_entry_init();

	email_init(mail);
	email_send(mail);
	email_close(mail);

	email_entry_free(mail);

	return 0;
}
