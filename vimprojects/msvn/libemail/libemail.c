#include "libemail.h"
#include "log.h"

email_entry* email_entry_init()
{
	log_info("email entry init.");

	email_entry *mail =  NULL;
	mail = (email_entry*)malloc(sizeof(&mail));
	
	if (NULL == mail)
	{
		log_err("Malloc email entry error. %s %d", __FILE__, __LINE__);
		exit(1);
	}

	return mail;
}

void email_entry_free(email_entry * mail)
{
	log_info("free email entry.");
	if (NULL == mail)
	{
		return;
	}

	free(mail -> from);

	free(mail);
	return;
}

int email_init(email_entry *mail)
{
	log_info("email init.");
	return 0;
}

int email_send(email_entry *mail)
{
	log_info("send mail...");
	return 0;
}

int email_close(email_entry *mail)
{
	log_info("close email.");
	return 0;

}

