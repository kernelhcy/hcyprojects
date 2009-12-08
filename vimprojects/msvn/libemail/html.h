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

#endif
