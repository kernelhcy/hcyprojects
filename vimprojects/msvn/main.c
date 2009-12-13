#include <stdlib.h>
#include <stdio.h>
#include "libemail/libemail.h"
#include "libemail/html.h"
#include "libemail/buffer.h"

int main(int argc, char *argv[])
{
	log_info("Test mail.");
	
	email_entry *mail = NULL;
	mail = email_entry_init();

	email_init(mail);
	email_send(mail);
	email_close(mail);

	email_entry_free(mail);


	html_entry *html = html_init();

	begin_head(html);
	i_title(html, "Test Title");
	end_head(html);

	begin_body(html);
	begin_p(html);
	i_br(html);
	i_a(html, "Google", "http://www.google.com");
	i_font(html, "Font Test", "黑体", 25, "red");
	end_p(html);
	end_body(html);

	html_close(html);

	printf("%s\n",html -> entry -> ptr);
	return 0;
}
