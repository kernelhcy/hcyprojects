#ifndef _MSVN_HTML_H
#define _MSVN_HTML_H

/**
 * 本文件中的函数创建一个html文件
 */

#include "headers.h"
#include "buffer.h"

/**
 * 用于存放html文件。
 */
typedef struct html_entry_
{
	buffer 	*entry;
}html_entry;

html_entry* html_init();
int html_close(html_entry *html);

int begin_head(html_entry *);
int end_head(html_entry *);

int i_title(html_entry *, const char *);

int begin_body(html_entry *);
int end_body(html_entry *);

int begin_p(html_entry *);
int end_p(html_entry*);

int i_br(html_entry*);
int i_a(html_entry*, const char *text, const char *url);
int i_font(html_entry*, const char *text, const char *font, int size, const char *color);



#endif
