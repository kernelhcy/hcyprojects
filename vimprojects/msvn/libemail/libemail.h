#ifndef _MSVN_LIB_EMAIL_H
#define _MSVN_LIB_EMAIL_H

#include "buffer.h"
#include "headers.h"


/**
 * 定义邮件的各种信息。
 */
struct email_entry_
{
	buffer_array 	*to_addrs; 			//收信人地址
	buffer_array 	*also_to_addrs; 	//抄送地址
	buffer_array 	*s_to_addrsl; 		//秘密发送地址

	buffer 		*from;					//发信人地址
	buffer 		*show_from; 			//收信人所看到的发信地址
	buffer 		*from_name; 			//发信人名字
 
	buffer 		*subject; 				//主题
	buffer 		*content; 				//内容

	//发信人登录的邮箱认证
	//发信人所登录的邮箱地址为上面变量from
	buffer 		*mail_addr; 			//邮箱地址，与from相同
	buffer 		*username;  			//发信人用户名
	buffer 		*password; 				//密码

	buffer_array 	*attachments; 		//附件地址

	int 	html_text; 					//邮件的内容使用html格式还是普通文本
	int 	auth;

};
typedef struct email_entry_ email_entry;

/**
 * 对email_entry的操作函数
 */

email_entry* email_entry_init();
void  email_entry_free(email_entry *mail);


/*
 ************************
 * 以下是对邮件的各种操作
 ***********************
 */
//初始化和关闭邮件。
int email_init(email_entry *mail);
int email_close(email_entry *mail);

//收发邮件
int email_send(email_entry *mail);
int email_recv(email_entry *mail);

#endif
